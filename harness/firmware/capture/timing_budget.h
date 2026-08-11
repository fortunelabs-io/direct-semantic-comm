/*
 * timing_budget.h - the capture engine's clock plan, checked at compile
 * time against datasheet limits.
 *
 * Every constant here is one of two kinds, and they are kept apart on
 * purpose:
 *
 *   LIMIT_*  transcribed from DocID026289 Rev 4 (STM32F411xC/xE), with
 *            the table cited beside it. These are properties of the
 *            silicon. Changing one is a claim about the part.
 *   Chosen   this firmware's clock plan. Changing one is a design choice.
 *
 * The static assertions below compare the second group against the first.
 * That is the only arrangement in which they can fail. An assertion whose
 * two sides are both chosen values proves nothing except that the
 * preprocessor can multiply: the first revision of this file asserted
 * 1 * 2 <= 2 and passed under a host x86 compiler with no part in the
 * room. Every value on the left of an assertion below is derived from the
 * clock plan; every value on the right came off a datasheet page.
 *
 * Tier 0 is "nothing powered", so none of this is a measurement. It is
 * the arithmetic that Tier 1 will measure against, failing early and at
 * build time if the plan and the part ever disagree.
 */
#ifndef CAPTURE_ENGINE_TIMING_BUDGET_H
#define CAPTURE_ENGINE_TIMING_BUDGET_H

/* ---- Limits: DocID026289 Rev 4 ---------------------------------------
 *
 * Table 41 "Main PLL characteristics", page 87, unless noted otherwise. */
#define LIMIT_PLL_IN_MIN_HZ    950000UL      /* f_PLL_IN  min  0.95 MHz */
#define LIMIT_PLL_IN_MAX_HZ    2100000UL     /* f_PLL_IN  max  2.10 MHz */
#define LIMIT_VCO_OUT_MIN_HZ   100000000UL   /* f_VCO_OUT min   100 MHz */
#define LIMIT_VCO_OUT_MAX_HZ   432000000UL   /* f_VCO_OUT max   432 MHz */
#define LIMIT_PLL_OUT_MIN_HZ   24000000UL    /* f_PLL_OUT min    24 MHz */
#define LIMIT_PLL_OUT_MAX_HZ   100000000UL   /* f_PLL_OUT max   100 MHz */

/* f_PLL48_OUT, Table 41. USB OTG FS is clocked from the PLL Q divider and
 * this is the only rate at which it is in specification. The Tier 0 claim
 * in todos/stage0_todo.md names "USB device" as one of the four
 * requirements the selected part must satisfy, and the Tier 1 `stream`
 * gate is stated against USB CDC, so this is load-bearing and not
 * decorative. */
#define REQUIRED_USB_CLK_HZ    48000000UL

/* Section 2.2, page 13: "The maximum frequency of the two AHB buses is
 * 100 MHz ... The maximum allowed frequency of the low-speed APB domain
 * is 50 MHz." */
#define LIMIT_HCLK_MAX_HZ      100000000UL
#define LIMIT_PCLK1_MAX_HZ     50000000UL

/* Register widths, RM0383. */
#define LIMIT_TIM_PSC_MAX      0xFFFFUL
#define LIMIT_I2C_CCR_MIN      4UL          /* CCR < 4 is not permitted */
#define LIMIT_I2C_CCR_MAX      0xFFFUL      /* 12-bit field */
#define LIMIT_I2C_FREQ_MIN_MHZ 2UL          /* CR2 FREQ, standard mode */
#define LIMIT_I2C_FREQ_MAX_MHZ 50UL

/* ---- Chosen: the clock plan -------------------------------------------
 *
 * HSI 16 MHz -> PLL -> 96 MHz SYSCLK, with the PLL Q divider landing on
 * exactly 48 MHz for USB.
 *
 * 96 MHz rather than the part's 100 MHz maximum, and the reason is USB.
 * SYSCLK = VCO_OUT / PLLP with PLLP in {2,4,6,8}, so a 100 MHz SYSCLK
 * forces VCO_OUT to 200 or 400 MHz (600 and 800 exceed the 432 MHz
 * ceiling). Neither 200/48 nor 400/48 is an integer, so no PLLQ yields a
 * USB clock in specification: on this part 100 MHz SYSCLK and USB are
 * mutually exclusive. A 192 MHz VCO gives up four megahertz and buys back
 * the peripheral the gate's own claim requires. */
#define HSI_HZ           16000000UL
#define PLLM             16UL
#define PLLN             192UL
#define PLLP             2UL
#define PLLQ             4UL

#define PLL_IN_HZ        (HSI_HZ / PLLM)              /* 1 MHz  */
#define VCO_OUT_HZ       (PLL_IN_HZ * PLLN)           /* 192 MHz */
#define SYSCLK_HZ        (VCO_OUT_HZ / PLLP)          /* 96 MHz  */
#define USB_CLK_HZ       (VCO_OUT_HZ / PLLQ)          /* 48 MHz  */

#define AHB_PRESCALER    1UL
#define APB1_PRESCALER   2UL
#define HCLK_HZ          (SYSCLK_HZ / AHB_PRESCALER)  /* 96 MHz */
#define PCLK1_HZ         (HCLK_HZ / APB1_PRESCALER)   /* 48 MHz */

_Static_assert(PLL_IN_HZ >= LIMIT_PLL_IN_MIN_HZ
            && PLL_IN_HZ <= LIMIT_PLL_IN_MAX_HZ,
    "PLL input (HSI/PLLM) is outside the datasheet's f_PLL_IN range");

_Static_assert(VCO_OUT_HZ >= LIMIT_VCO_OUT_MIN_HZ
            && VCO_OUT_HZ <= LIMIT_VCO_OUT_MAX_HZ,
    "PLL VCO output is outside the datasheet's f_VCO_OUT range");

_Static_assert(SYSCLK_HZ >= LIMIT_PLL_OUT_MIN_HZ
            && SYSCLK_HZ <= LIMIT_PLL_OUT_MAX_HZ,
    "SYSCLK is outside the datasheet's f_PLL_OUT range");

_Static_assert(HCLK_HZ <= LIMIT_HCLK_MAX_HZ,
    "HCLK exceeds the 100 MHz AHB maximum");

_Static_assert(PCLK1_HZ <= LIMIT_PCLK1_MAX_HZ,
    "PCLK1 exceeds the 50 MHz APB1 maximum");

/* The assertion that would have caught the 100 MHz plan. */
_Static_assert(VCO_OUT_HZ % PLLQ == 0 && USB_CLK_HZ == REQUIRED_USB_CLK_HZ,
    "PLL Q divider does not produce exactly 48 MHz; USB OTG FS would be "
    "out of specification and the Tier 0 claim names USB device");

/* ---- TIM2: the microsecond timebase -----------------------------------
 *
 * When the APB1 prescaler is not 1 the timer kernel clock is twice PCLK1,
 * so TIM2 runs at 96 MHz from a 48 MHz peripheral bus. Table 4 footnote 1
 * confirms APB1 timers reach a 100 MHz TIMxCLK on this part. This assumes
 * TIMPRE = 0 in RCC_DCKCFGR, which is the reset value and is not written
 * by this firmware. */
#define TIM2_KERNEL_CLK_HZ  (PCLK1_HZ * 2UL)          /* 96 MHz */
#define TIM2_TARGET_TICK_HZ 1000000UL                 /* 1 us per tick */
#define TIM2_PSC_VALUE      ((TIM2_KERNEL_CLK_HZ / TIM2_TARGET_TICK_HZ) - 1UL)
#define TIM2_ACTUAL_TICK_HZ (TIM2_KERNEL_CLK_HZ / (TIM2_PSC_VALUE + 1UL))

_Static_assert(TIM2_KERNEL_CLK_HZ % TIM2_TARGET_TICK_HZ == 0,
    "TIM2 kernel clock does not divide evenly to a 1 MHz tick; the "
    "prediction that TIM2 resolves one microsecond exactly does not hold");

_Static_assert(TIM2_PSC_VALUE <= LIMIT_TIM_PSC_MAX,
    "TIM2 prescaler value exceeds the 16-bit PSC register width");

/* ---- I2C: two masters, standard mode ----------------------------------
 *
 * CCR = PCLK1 / (2 * f_SCL) for standard mode.
 * TRISE = (PCLK1_MHz * max_rise_ns / 1000) + 1, max rise 1000 ns per the
 * I2C-bus specification for standard mode. */
#define I2C_TARGET_SCL_HZ    100000UL
#define I2C_CCR_VALUE        (PCLK1_HZ / (2UL * I2C_TARGET_SCL_HZ))
#define I2C_MAX_RISE_NS      1000UL
#define I2C_PCLK1_MHZ        (PCLK1_HZ / 1000000UL)
#define I2C_TRISE_VALUE      ((I2C_PCLK1_MHZ * I2C_MAX_RISE_NS) / 1000UL + 1UL)

_Static_assert(I2C_CCR_VALUE >= LIMIT_I2C_CCR_MIN
            && I2C_CCR_VALUE <= LIMIT_I2C_CCR_MAX,
    "I2C CCR value is outside the 12-bit register's permitted range");

_Static_assert(I2C_PCLK1_MHZ >= LIMIT_I2C_FREQ_MIN_MHZ
            && I2C_PCLK1_MHZ <= LIMIT_I2C_FREQ_MAX_MHZ,
    "I2C CR2 FREQ field is outside the permitted 2..50 MHz range");

_Static_assert(PCLK1_HZ % 1000000UL == 0,
    "PCLK1 is not a whole number of megahertz, so the CR2 FREQ field "
    "cannot describe it exactly");

/* ---- Margin against the Tier 1 jitter budget --------------------------
 *
 * The Tier 1 gate ("the timestamp is taken at the edge and not after the
 * read") requires an inter-timestamp standard deviation under 2
 * microseconds. Tier 0 measures nothing; what is checked here is that the
 * timebase itself is not the bottleneck, at a 1:2 sampling margin.
 *
 * TIMER_TICK_US is derived from the clock plan rather than written down,
 * so a future change to PLLN, the APB1 prescaler or the prescaler value
 * that coarsens the tick fails this assertion instead of silently eating
 * the margin the Tier 1 gate is stated against. */
#define JITTER_BUDGET_US     2UL
#define TIMER_TICK_US        (1000000UL / TIM2_ACTUAL_TICK_HZ)

_Static_assert(TIM2_ACTUAL_TICK_HZ == TIM2_TARGET_TICK_HZ,
    "TIM2 prescaler does not land on the intended 1 MHz tick");

_Static_assert(TIMER_TICK_US * 2UL <= JITTER_BUDGET_US,
    "TIM2's tick resolution does not give at least 2x margin against the "
    "Tier 1 jitter budget");

#endif /* CAPTURE_ENGINE_TIMING_BUDGET_H */
