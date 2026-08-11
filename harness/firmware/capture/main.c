/*
 * main.c - entry point for the Tier 0 "a blink builds and flashes" check.
 *
 * Not one of the three files the repository map in todos/stage0_todo.md
 * names for firmware/capture/ (timebase.c, sensor.c, stream.c). Those
 * three are the capture engine's actual components; this file is the
 * smallest possible glue that brings them up and proves it on an LED,
 * which is what the gate asks for at Tier 0. Flagged here rather than
 * folded silently into one of the three, since the map did not call for a
 * fourth file.
 */
#include "register_map.h"
#include "timebase.h"
#include "sensor.h"

/* The LED is a property of the board, not of the part, and the gate's
 * criterion is that a blink *flashes* - which means somebody has to see
 * it. Set these three to match the board in hand, at build time:
 *
 *   cmake -DLED_PORT_ID=2 -DLED_PIN=13 -DLED_ACTIVE_LOW=1 ...
 *
 * LED_PORT_ID is 0 for GPIOA, 1 for GPIOB, 2 for GPIOC. It is a small
 * integer rather than the GPIOx macro because those expand to pointer
 * casts, which #if cannot evaluate.
 *
 * The default is PC13 active-low, which is the arrangement on the common
 * STM32F411CEU6 breakout boards. It is a default and not a datasheet
 * fact: PC13 is bonded out on UFQFPN48 (pin 2), but whether an LED hangs
 * off it is the board's business. If the blink does not appear, this is
 * the first thing to check and not evidence about the part.
 */
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
    LED_PORT->MODER |=  (1UL << (LED_PIN * 2)); /* push-pull output */
}

static void led_set(int on)
{
#if LED_ACTIVE_LOW
    on = !on;
#endif
    /* BSRR: bits 15:0 set, bits 31:16 reset. */
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
