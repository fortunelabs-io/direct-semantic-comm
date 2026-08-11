/*
 * timebase.c - see timebase.h.
 */
#include "timebase.h"
#include "register_map.h"
#include "timing_budget.h"

static void clock_init(void)
{
    /* Wait states before the clock goes up, never after. RM0383 flash
     * latency table at 2.7-3.6V: 3 WS covers 90 MHz < HCLK <= 100 MHz,
     * which is where this plan's 96 MHz lands. Prefetch and the
     * instruction/data caches on. */
    FLASH->ACR = FLASH_ACR_LATENCY_3WS | FLASH_ACR_PRFTEN
               | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { /* wait for HSI */ }

    /* PLL from the 16 MHz HSI. Dividers come from timing_budget.h, where
     * they are asserted against the datasheet's PLL limits at build time:
     *
     *   VCO_in  = HSI / PLLM = 16 / 16     = 1 MHz
     *   VCO_out = VCO_in * PLLN = 1 * 192  = 192 MHz
     *   SYSCLK  = VCO_out / PLLP = 192 / 2 = 96 MHz
     *   USB     = VCO_out / PLLQ = 192 / 4 = 48 MHz exactly
     *
     * Read-modify-write, not a wholesale assignment. Writing the whole
     * register would zero PLLQ, which is not a legal divider and would
     * leave USB OTG FS without a clock; the Tier 0 claim names USB device
     * as one of the part's four required peripherals.
     *
     * PLLP is encoded (P/2)-1, so a /2 divider is the value 0. */
    RCC->PLLCFGR = (RCC->PLLCFGR
                    & ~(RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk
                      | RCC_PLLCFGR_PLLP_Msk | RCC_PLLCFGR_PLLQ_Msk
                      | RCC_PLLCFGR_PLLSRC_Msk))
                 | RCC_PLLCFGR_PLLSRC_HSI
                 | (PLLM            << RCC_PLLCFGR_PLLM_Pos)
                 | (PLLN            << RCC_PLLCFGR_PLLN_Pos)
                 | (((PLLP / 2) - 1) << RCC_PLLCFGR_PLLP_Pos)
                 | (PLLQ            << RCC_PLLCFGR_PLLQ_Pos);

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { /* wait for PLL lock */ }

    /* Bus prescalers before the switch, so no bus is ever briefly
     * overclocked as SYSCLK steps from 16 MHz to 96 MHz. APB1 is capped
     * at 50 MHz on this part and /2 puts it at 48 MHz. */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk))
              | RCC_CFGR_HPRE_DIV1
              | RCC_CFGR_PPRE1_DIV2;

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) {
        /* wait for the switch to take */
    }
}

static void tim2_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = TIM2_PSC_VALUE; /* from timing_budget.h, statically checked */
    TIM2->ARR = 0xFFFFFFFFUL;   /* free-running 32-bit microsecond tick */
    TIM2->CR1 |= TIM_CR1_CEN;
}

static void dwt_init(void)
{
    *DEMCR |= DEMCR_TRCENA;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA;
    DWT->CYCCNT = 0;
}

void timebase_init(void)
{
    clock_init();
    tim2_init();
    dwt_init();
}

uint32_t timebase_now_us(void)
{
    return TIM2->CNT;
}

void timebase_delay_us(uint32_t us)
{
    uint32_t start = timebase_now_us();
    while ((uint32_t)(timebase_now_us() - start) < us) { /* spin, wrap-safe by unsigned subtraction */ }
}
