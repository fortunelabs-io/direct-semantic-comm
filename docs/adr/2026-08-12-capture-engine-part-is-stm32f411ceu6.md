# The capture engine part is STM32F411CEU6

**Date:** 2026-08-12 **Status:** Accepted
**Supersedes:** `2026-08-09-capture-engine-is-stm32-part-still-open.md` (closes Tier 0 gate 1's open part number)

## Context

The prior entry fixed the capture engine family to STM32 on sourcing and
skill grounds, and left the part number open. Two parts were checked
against their datasheets, not a selector table:

- **STM32F401CEU6**, UFQFPN48 (LCSC C161736)
- **STM32F411CEU6**, UFQFPN48 (LCSC C60420)

Both are read from ST datasheet DS10086 Rev 5 (F401xD/xE) and DocID026289
Rev 4 (F411xC/xE) respectively.

## What both parts satisfy

Same package, same pin mapping, checked from the bootloader alternate
function table in each datasheet:

- **Two I2C masters on separate pins.** I2C1 (PB6/PB7), I2C2 (PB10/PB3),
  I2C3 (PA8/PB4) — three independent instances, each on a non-overlapping
  pin pair. Any two of three clear the requirement.
- **10 GPIO with edge interrupt.** Up to 81 I/O with interrupt capability
  on both parts, clearing the requirement with large margin.
- **Microsecond timer.** Two 32-bit timers (TIM2, TIM5) on both parts.
  F401 runs them to 84 MHz, F411 to 100 MHz. Both resolve a 1 microsecond
  tick with margin.
- **USB device.** USB 2.0 full-speed device/host/OTG controller with
  on-chip PHY on both parts.

Neither datasheet check finds a capability difference that matters to
this gate. This is the same conclusion the family-level ADR already
reached: not a capability decision.

## Decision

STM32F411CEU6 (LCSC C60420).

STM32F401CEU6 (LCSC C161736) satisfies the gate on paper, but stock at
LCSC was 25 units at time of check. Stage 2 is a long campaign needing
ten boards populated plus spares; 25 units is the same single point of
failure the family-level ADR was written to remove, just moved one level
down from family to part. STM32F411CEU6 is pin-compatible, same package,
same I2C and timer pin mapping, and carries ample stock at the same
distributor. Sourcing depth decides between two parts that are otherwise
equivalent for this gate.

## What this decision does not claim

It does not claim STM32F411CEU6 measures better or that STM32F401CEU6
would have failed the gate. Both clear all four requirements from the
datasheet. The choice rests entirely on stock depth at time of check,
which is a sourcing fact and not a technical one. Recorded here for
traceability, not as a technical tie-breaker dressed up after the fact.

## Consequences

Tier 0 gate 1 (`toolchain`, issue #1) can now carry a prediction against
a named part and become runnable. This ADR fixes the part; it does not
itself close the gate. The gate still closes only when a blink builds
and flashes on this part and the two I2C peripherals are confirmed from
this datasheet in the firmware, not assumed from this record.

## What would reopen this

LCSC stock for STM32F411CEU6 drops to a level that cannot supply ten
boards plus spares before Stage 2 begins. If that happens, the search
resumes from the STM32 range, not a silent substitution.
