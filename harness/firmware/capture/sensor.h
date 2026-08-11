/*
 * sensor.h - GPIO and I2C bring-up for the two INA226 buses.
 *
 * Two independent masters, one per channel, so a read stall on one bus
 * cannot block the other: I2C1 on PB6 (SCL, AF4) / PB7 (SDA, AF4), I2C3
 * on PA8 (SCL, AF4) / PB4 (SDA, AF9), per Table 9 of DocID026289 Rev 4.
 * All four are bonded out on UFQFPN48 (pins 42, 43, 29, 40).
 *
 * PB4 is NJTRST at reset and belongs to the SWJ-DP group. Configuring it
 * as I2C3_SDA releases the JTAG reset line, which costs nothing here
 * because the ST-Link attaches over SWD on PA13/PA14, but it does mean
 * JTAG is unavailable once this runs.
 *
 * INA226 register config and read (shunt-only continuous, 140
 * microseconds, averaging one, alert latch transparent, register pointer
 * retained, shunt voltage register only) is Tier 1 work and is not
 * written yet. Tier 0 only needs both peripherals to initialise cleanly,
 * which is the "two I2C peripherals recorded from the datasheet" half of
 * the gate's pass criterion in todos/stage0_todo.md.
 */
#ifndef CAPTURE_ENGINE_SENSOR_H
#define CAPTURE_ENGINE_SENSOR_H

/* ---- The datasheet record ---------------------------------------------
 *
 * Table 9 and Figure 10 of DocID026289 Rev 4. These are the values
 * sensor.c programs into AFR and nothing else is programmed there, so the
 * record the Tier 0 gate publishes and the values the firmware uses
 * cannot drift apart: they are the same constants.
 *
 * The two alternate function numbers differ and that is not a
 * transcription error. Table 9 heads AF04 "I2C1/I2C2/I2C3" and AF09
 * "I2C2/I2C3", so I2C3 appears in both columns and which one applies is a
 * per-pin fact. AF04 on PB4 is blank. */
#define I2C1_SCL_PIN   "PB6"
#define I2C1_SCL_AF    4U
#define I2C1_SCL_PKG   42U
#define I2C1_SDA_PIN   "PB7"
#define I2C1_SDA_AF    4U
#define I2C1_SDA_PKG   43U

#define I2C3_SCL_PIN   "PA8"
#define I2C3_SCL_AF    4U
#define I2C3_SCL_PKG   29U
#define I2C3_SDA_PIN   "PB4"
#define I2C3_SDA_AF    9U
#define I2C3_SDA_PKG   40U

#define CAPTURE_PART      "STM32F411CEU6"
#define CAPTURE_PACKAGE   "UFQFPN48"
#define CAPTURE_DATASHEET "DocID026289 Rev 4"

/* Configures PB6/PB7 (I2C1) and PA8/PB4 (I2C3) as open-drain,
 * pull-up, AF4, then brings up both I2C peripherals in standard mode
 * at 100 kHz using the CCR/TRISE values computed in timing_budget.h.
 * Must run after timebase_init() sets APB1 to its final rate. */
void sensor_bus_init(void);

#endif /* CAPTURE_ENGINE_SENSOR_H */
