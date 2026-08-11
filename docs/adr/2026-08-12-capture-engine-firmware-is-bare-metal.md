# The capture engine firmware is bare-metal register-level, not CubeMX HAL

**Date:** 2026-08-12 **Status:** Accepted
**Amends:** `2026-08-09-capture-engine-is-stm32-part-still-open.md` (revises the
toolchain clause only; the family decision, STM32, and the part decision,
STM32F411CEU6 in `2026-08-12-capture-engine-part-is-stm32f411ceu6.md`, stand
unchanged)

## Context

The prior entry decided "STM32, CubeMX plus CMake" as one clause, on
sourcing and skill grounds. Those grounds support the family choice; they
were never independently tested against the firmware's own requirement.

That same entry already identifies the deciding factor for the jitter
gate: *"it will be met or missed by where the I2C read sits relative to
the conversion-ready interrupt."* Scaffolding Tier 0 gate 1's toolchain
task surfaces the conflict directly — HAL abstracts exactly that
boundary.

## Alternatives considered

- **HAL via CubeMX-generated init**, as the prior entry names. Meets the
  letter of that decision. Rejected: the ISR-to-read path is read through
  generic driver code (`HAL_I2C_xxx` and its callback structure) rather
  than written and audited as this project's own code, at the one place
  the prior entry already flagged as decisive.
- **Hybrid** — CubeMX-style HAL for non-time-critical init (clock tree,
  GPIO mode, peripheral enable), bare-metal register access for the
  ISR-to-read path itself. Considered as a middle ground. Rejected: a
  reviewer now has to track, per function, whether it's the trusted
  hand-written path or the vendor-generated one. A codebase uniformly
  readable end to end is easier to defend than one with an internal
  boundary drawn in the middle of the part that matters most.
- **Bare-metal, register-level throughout.** Chosen.

## Decision

The capture engine firmware is written bare-metal, register-level, with
no CubeMX-generated HAL anywhere in it. This revises the toolchain clause
of the prior entry. The family (STM32) and part (STM32F411CEU6) decisions
are unaffected.

The harness is meant to be fully controlled. Its authority rests on every
register write in the jitter-critical path being traceable to a line of
code this project wrote and can defend, not to vendor-generated code
whose internal timing behavior is trusted rather than read. This is the
same standard `2026-08-09-two-channel-harness-built-in-house.md` already
applied to the commercial instrument itself: a dependency on someone
else's black box at the exact point that matters is the single point of
failure that entry was written to avoid. Here it just moves from hardware
to firmware.

## What this decision does not claim

Does not claim HAL is unreliable in general, or that CubeMX-generated
init is unsound for other, non-timing-critical firmware elsewhere in this
project. This is scoped to the capture engine specifically, because it is
the one piece of software in this project whose defects are not visible
in the data it produces.

## Consequences

- arm-none-eabi-gcc stays the compiler; CubeMX is dropped from this
  firmware's build entirely, so nothing here needs a HAL version pinned
  in `.mise.toml`.
- Clock tree, GPIO alternate-function mapping, and I2C timing register
  values are derived by hand from DocID026289 Rev 4 and documented inline
  in the source, since there is no generated init to fall back on.
- No regeneration path if the pin map changes later. Accepted cost, given
  what this firmware is measured against.
- Tier 0 gate 1's toolchain task can now be scaffolded directly on top of
  this decision.

## What would reopen this

Register-level init proving to be a larger, demonstrated source of
defects than HAL abstraction would have been, for example repeated
clock-tree bugs across boards during Stage 2. Reopened on a shown failure
rate, not in advance of one.
