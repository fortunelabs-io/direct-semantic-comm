/*
 * main.c - just for firmware check right now.
 * TODO: Tier 1: add INA226 read and USB CDC output.
 */
#include "register_map.h"
#include "timebase.h"
#include "sensor.h"

/* The LED is a board property, set at build time:
 *   cmake -DLED_PORT_ID=2 -DLED_PIN=13 -DLED_ACTIVE_LOW=1 ...
 * LED_PORT_ID is 0/1/2 for GPIOA/B/C, because #if cannot evaluate the
 * pointer casts behind the GPIOx macros.
 *
 * Default is PC13 active-low, common on STM32F411CEU6 breakouts.
 * If no blink appears, check this first. 
 * */
#ifndef LED_PORT_ID
#define LED_PORT_ID     2
#endif
#ifndef LED_PIN
#define LED_PIN         13U
#endif
#ifndef LED_ACTIVE_LOW
#define LED_ACTIVE_LOW  1
#endif

#if   LED_PORT_ID == 0
#define LED_PORT        GPIOA
#define LED_PORT_EN     RCC_AHB1ENR_GPIOAEN
#elif LED_PORT_ID == 1
#define LED_PORT        GPIOB
#define LED_PORT_EN     RCC_AHB1ENR_GPIOBEN
#elif LED_PORT_ID == 2
#define LED_PORT        GPIOC
#define LED_PORT_EN     RCC_AHB1ENR_GPIOCEN
#else
#error "LED_PORT_ID must be 0 (GPIOA), 1 (GPIOB) or 2 (GPIOC)"
#endif

#define LED_BLINK_HALF_PERIOD_US 500000U

static void led_init(void)
{
    RCC->AHB1ENR |= LED_PORT_EN;

    LED_PORT->MODER &= ~(3UL << (LED_PIN * 2));
    LED_PORT->MODER |=  (1UL << (LED_PIN * 2));
}

static void led_set(int on)
{
#if LED_ACTIVE_LOW
    on = !on;
#endif
    LED_PORT->BSRR = on ? (1UL << LED_PIN) : (1UL << (LED_PIN + 16));
}

int main(void)
{
    timebase_init();
    sensor_bus_init();
    led_init();

    for (;;) {
        led_set(1);
        timebase_delay_us(LED_BLINK_HALF_PERIOD_US);
        led_set(0);
        timebase_delay_us(LED_BLINK_HALF_PERIOD_US);
    }
}
