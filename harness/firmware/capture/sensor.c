/*
 * sensor.c - see sensor.h.
 *
 * Alternate function numbers here are per pin, not per peripheral. Table
 * 9 of DocID026289 Rev 4 heads AF04 "I2C1/I2C2/I2C3" and AF09
 * "I2C2/I2C3", so I2C3 appears in both columns and which one applies
 * depends on the pin. On this part every port B I2C2/I2C3 SDA sits at
 * AF09 (PB3, PB4, PB8, PB9) while the clocks sit at AF04. Reading the
 * column header alone and applying AF4 to all four pins is the error this
 * comment exists to stop being made twice: AF04 on PB4 is not a different
 * function, it is blank, so I2C3 would have initialised cleanly, reported
 * no error, and never reached the pad.
 */
#include "sensor.h"
#include "register_map.h"
#include "timing_budget.h"

/* Table 9, DocID026289 Rev 4. */
#define AF_I2C1_SCL_PB6   4UL
#define AF_I2C1_SDA_PB7   4UL
#define AF_I2C3_SCL_PA8   4UL
#define AF_I2C3_SDA_PB4   9UL

static void gpio_bus_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;

    /* I2C1: PB6 (SCL), PB7 (SDA) - alternate function, open-drain,
     * pull-up, AF4. */
    GPIOB->MODER   &= ~((3UL << (6 * 2)) | (3UL << (7 * 2)));
    GPIOB->MODER   |=  ((2UL << (6 * 2)) | (2UL << (7 * 2))); /* AF mode */
    GPIOB->OTYPER  |=  (1UL << 6) | (1UL << 7);                /* open-drain */
    GPIOB->PUPDR   &= ~((3UL << (6 * 2)) | (3UL << (7 * 2)));
    GPIOB->PUPDR   |=  ((1UL << (6 * 2)) | (1UL << (7 * 2))); /* pull-up */
    GPIOB->AFR[0]  &= ~((0xFUL << (6 * 4)) | (0xFUL << (7 * 4)));
    GPIOB->AFR[0]  |=  ((AF_I2C1_SCL_PB6 << (6 * 4))
                      | (AF_I2C1_SDA_PB7 << (7 * 4)));

    /* I2C3: PA8 (SCL, AF4), PB4 (SDA, AF9) - same treatment, split across
     * ports so I2C1 and I2C3 never share a physical pin group. The two
     * alternate function numbers differ; see the file header. */
    GPIOA->MODER   &= ~(3UL << (8 * 2));
    GPIOA->MODER   |=  (2UL << (8 * 2));
    GPIOA->OTYPER  |=  (1UL << 8);
    GPIOA->PUPDR   &= ~(3UL << (8 * 2));
    GPIOA->PUPDR   |=  (1UL << (8 * 2));
    GPIOA->AFR[1]  &= ~(0xFUL << ((8 - 8) * 4));
    GPIOA->AFR[1]  |=  (AF_I2C3_SCL_PA8 << ((8 - 8) * 4));

    GPIOB->MODER   &= ~(3UL << (4 * 2));
    GPIOB->MODER   |=  (2UL << (4 * 2));
    GPIOB->OTYPER  |=  (1UL << 4);
    GPIOB->PUPDR   &= ~(3UL << (4 * 2));
    GPIOB->PUPDR   |=  (1UL << (4 * 2));
    GPIOB->AFR[0]  &= ~(0xFUL << (4 * 4));
    GPIOB->AFR[0]  |=  (AF_I2C3_SDA_PB4 << (4 * 4));
}

static void i2c_peripheral_init(I2C_TypeDef *i2c)
{
    i2c->CR1 &= ~I2C_CR1_PE; /* disable before configuring */
    /* CR2 FREQ must state the actual APB1 rate, so it is derived from the
     * clock plan rather than written as a literal. A hardcoded 50 here
     * would have survived the move from 100 MHz to 96 MHz SYSCLK and left
     * every I2C timing off by four percent. */
    i2c->CR2   = I2C_PCLK1_MHZ;
    i2c->CCR   = I2C_CCR_VALUE;  /* from timing_budget.h, statically checked */
    i2c->TRISE = I2C_TRISE_VALUE;
    i2c->CR1 |= I2C_CR1_PE;
}

void sensor_bus_init(void)
{
    gpio_bus_init();
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN | RCC_APB1ENR_I2C3EN;
    i2c_peripheral_init(I2C1);
    i2c_peripheral_init(I2C3);
}
