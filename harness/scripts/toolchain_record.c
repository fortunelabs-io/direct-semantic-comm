/*
 * toolchain_record.c - emits the Tier 0 gate's record from the firmware's
 * own constants.
 *
 * This program is the gate. Not because of what it prints, but because of
 * what it takes to compile it: including timing_budget.h evaluates every
 * static assertion in that header, each of which compares a value derived
 * from the clock plan against a limit transcribed from DocID026289 Rev 4.
 * A clock plan that violates a datasheet limit does not build, and the
 * gate fails at the compiler rather than at a hand-written number.
 *
 * It is built with the *host* compiler on purpose. timing_budget.h is
 * integer arithmetic on datasheet constants with nothing target-specific
 * in it, so the host evaluates exactly the assertions the cross compiler
 * would, and the Tier 0 criterion stays satisfiable with nothing powered.
 * Whether arm-none-eabi-gcc can produce an image the part will execute is
 * a different claim, and it is the Tier 1 `blink` gate.
 *
 * The values printed come from the same headers the firmware compiles
 * against, so the published record cannot claim an alternate function the
 * firmware does not program into AFR.
 */
#include <stdio.h>

#include "timing_budget.h"
#include "sensor.h"

static void emit_line(const char *role, const char *pin,
                      unsigned af, unsigned package_pin, int last)
{
    printf("      \"%s\": { \"pin\": \"%s\", \"af\": %u, \"package_pin\": %u }%s\n",
           role, pin, af, package_pin, last ? "" : ",");
}

int main(void)
{
    printf("{\n");
    printf("  \"part\": \"%s\",\n", CAPTURE_PART);
    printf("  \"package\": \"%s\",\n", CAPTURE_PACKAGE);
    printf("  \"part_datasheet\": \"%s\",\n", CAPTURE_DATASHEET);

    printf("  \"clock_plan_hz\": {\n");
    printf("    \"pll_in\": %lu,\n",  PLL_IN_HZ);
    printf("    \"vco_out\": %lu,\n", VCO_OUT_HZ);
    printf("    \"sysclk\": %lu,\n",  SYSCLK_HZ);
    printf("    \"hclk\": %lu,\n",    HCLK_HZ);
    printf("    \"pclk1\": %lu,\n",   PCLK1_HZ);
    printf("    \"usb\": %lu,\n",     USB_CLK_HZ);
    printf("    \"tim2_kernel\": %lu,\n", TIM2_KERNEL_CLK_HZ);
    printf("    \"tim2_tick\": %lu\n",    TIM2_ACTUAL_TICK_HZ);
    printf("  },\n");

    printf("  \"datasheet_limits_hz\": {\n");
    printf("    \"pll_in_min\": %lu,\n",  LIMIT_PLL_IN_MIN_HZ);
    printf("    \"pll_in_max\": %lu,\n",  LIMIT_PLL_IN_MAX_HZ);
    printf("    \"vco_out_min\": %lu,\n", LIMIT_VCO_OUT_MIN_HZ);
    printf("    \"vco_out_max\": %lu,\n", LIMIT_VCO_OUT_MAX_HZ);
    printf("    \"hclk_max\": %lu,\n",    LIMIT_HCLK_MAX_HZ);
    printf("    \"pclk1_max\": %lu,\n",   LIMIT_PCLK1_MAX_HZ);
    printf("    \"usb_required\": %lu\n", REQUIRED_USB_CLK_HZ);
    printf("  },\n");

    printf("  \"timer_tick_us\": %lu,\n", TIMER_TICK_US);
    printf("  \"jitter_budget_us\": %lu,\n", JITTER_BUDGET_US);

    printf("  \"i2c_registers\": {\n");
    printf("    \"tim2_psc\": %lu,\n",  TIM2_PSC_VALUE);
    printf("    \"i2c_ccr\": %lu,\n",   I2C_CCR_VALUE);
    printf("    \"i2c_trise\": %lu,\n", I2C_TRISE_VALUE);
    printf("    \"i2c_cr2_freq_mhz\": %lu\n", I2C_PCLK1_MHZ);
    printf("  },\n");

    printf("  \"i2c_peripherals\": [\n");
    printf("    {\n      \"instance\": \"I2C1\",\n");
    emit_line("scl", I2C1_SCL_PIN, I2C1_SCL_AF, I2C1_SCL_PKG, 0);
    emit_line("sda", I2C1_SDA_PIN, I2C1_SDA_AF, I2C1_SDA_PKG, 1);
    printf("    },\n");
    printf("    {\n      \"instance\": \"I2C3\",\n");
    emit_line("scl", I2C3_SCL_PIN, I2C3_SCL_AF, I2C3_SCL_PKG, 0);
    emit_line("sda", I2C3_SDA_PIN, I2C3_SDA_AF, I2C3_SDA_PKG, 1);
    printf("    }\n");
    printf("  ]\n");

    printf("}\n");
    return 0;
}
