/*
 * timebase.h - the capture engine's clock tree and microsecond counter.
 *
 * Owns everything the timestamp depends on: SYSCLK/PLL bring-up, TIM2 as
 * a free-running 32-bit microsecond tick, and the DWT cycle counter. Edge
 * capture (conversion-ready interrupt handling) lands here too once Tier
 * 1 starts, per the repository map in todos/stage0_todo.md; it is not
 * written yet, and Tier 0 only needs the tick running and readable.
 */
#ifndef CAPTURE_ENGINE_TIMEBASE_H
#define CAPTURE_ENGINE_TIMEBASE_H

#include <stdint.h>

/* Brings SYSCLK to 96 MHz via PLL from HSI, sets flash wait states,
 * starts TIM2 as a free-running 1 microsecond tick, and enables the
 * DWT cycle counter. Must run before sensor_bus_init(), which depends on
 * APB1 having reached its final rate. */
void timebase_init(void);

/* Current value of the free-running microsecond counter (TIM2->CNT). */
uint32_t timebase_now_us(void);

/* Busy-wait for the given number of microseconds, wrap-safe. */
void timebase_delay_us(uint32_t us);

#endif /* CAPTURE_ENGINE_TIMEBASE_H */
