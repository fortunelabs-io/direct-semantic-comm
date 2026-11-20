/*
 * sensor.c - see sensor.h.
 *
 * AF numbers are per pin (Table 9, DocID026289 Rev 4). Port B SDA pins
 * (PB3, PB4, PB8, PB9) use AF09, clock pins use AF04. AF04 on PB4 is blank:
 * I2C3 would initialise without error and never reach the pad.
 */

#include "sensor.h"
#include "register_map.h"
#include "timing_budget.h"

/* AF numbers come from sensor.h. */
#define AF_I2C1_SCL_PB6   ((uint32_t) I2C1_SCL_AF)
#define AF_I2C1_SDA_PB7   ((uint32_t) I2C1_SDA_AF)
#define AF_I2C3_SCL_PA8   ((uint32_t) I2C3_SCL_AF)
#define AF_I2C3_SDA_PB4   ((uint32_t) I2C3_SDA_AF)

static void gpio_bus_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;

    /* I2C1: PB6 (SCL), PB7 (SDA) */
    GPIOB->MODER   &= ~((3UL << (6 * 2)) | (3UL << (7 * 2)));
    GPIOB->MODER   |=  ((2UL << (6 * 2)) | (2UL << (7 * 2)));
    GPIOB->OTYPER  |=  (1UL << 6) | (1UL << 7);
    GPIOB->PUPDR   &= ~((3UL << (6 * 2)) | (3UL << (7 * 2)));
    GPIOB->PUPDR   |=  ((1UL << (6 * 2)) | (1UL << (7 * 2)));
    GPIOB->AFR[0]  &= ~((0xFUL << (6 * 4)) | (0xFUL << (7 * 4)));
    GPIOB->AFR[0]  |=  ((AF_I2C1_SCL_PB6 << (6 * 4))
                      | (AF_I2C1_SDA_PB7 << (7 * 4)));

    /* I2C3: PA8 (SCL), PB4 (SDA) */
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
    i2c->CR1 &= ~I2C_CR1_PE;
    /* CR2 FREQ is the APB1 rate. */
    i2c->CR2   = I2C_PCLK1_MHZ;
    i2c->CCR   = I2C_CCR_VALUE;
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
