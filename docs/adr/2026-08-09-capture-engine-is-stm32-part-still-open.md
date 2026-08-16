# The capture engine is an STM32, and the part number stays open

**Date:** 2026-08-09
**Status:** Superseded in part. The family decision (STM32) stands and is still
the record for it. Both clauses that made this entry's title true are gone:

- **The part number is closed.** STM32F411CEU6, by
  [`2026-08-12-capture-engine-part-is-stm32f411ceu6.md`](./2026-08-12-capture-engine-part-is-stm32f411ceu6.md),
  which declares `Supersedes:` against this entry. Tier 0 gate 1 has since
  closed.
- **The toolchain clause is revised.** No CubeMX and no vendor HAL anywhere in
  the capture firmware, by
  [`2026-08-12-capture-engine-firmware-is-bare-metal.md`](./2026-08-12-capture-engine-firmware-is-bare-metal.md),
  which declares `Amends:` against it.

Read below for the family argument and for what it deliberately did not claim.
Everything it says about the part number staying open, about CubeMX, and about
generated initialisation being committed, is no longer true of this project.

## Context

Tier 0 gate 1 of `todos/stage0_todo.md` requires one part whose datasheet shows
two independent I2C masters on separate pins, ten GPIO with edge interrupt, a
microsecond timer, and USB device. `.mise.toml` named two families in reach:
RP2040 with pico-sdk, and STM32 with arm-none-eabi-gcc plus CubeMX.

**Both families meet all four requirements on paper.** RP2040 carries two I2C
controllers, a 64-bit microsecond timer, USB device, and edge interrupt on every
GPIO. This is therefore not a capability decision, and writing it up as one
would be false in a way that a later reader could not detect.

## Alternatives considered

- **RP2040, pico-sdk.** Meets the four requirements. Rejected on the two grounds
  below, neither of which is about the silicon.
- **STM32, CubeMX plus CMake.** Chosen.
- **Defer the family until a part search runs across both.** Rejected: the
  search space is the problem, not the answer. Two families of several hundred
  parts each is a search that does not terminate, and the two constraints below
  cut it before capability does.

## Decision

STM32. **The part number is not selected and Tier 0 gate 1 stays open.**

Two reasons, both about the supply chain and the developer rather than the part:

**Sourcing and availability.** The same pressure that
[two-channel-harness-built-in-house](./2026-08-09-two-channel-harness-built-in-house.md)
names for the commercial instrument applies to the part on the board. Ten boards
are spares only if all ten can be populated, and a part with an intercontinental
supply path reintroduces the single point of failure that entry was written to
remove. Stage 2 is a long campaign and a part shortage mid-campaign costs the
campaign.

**Established skill.** That same entry defers work pairing a high cost of
failure with a skill not yet established. The capture firmware is the highest
cost-of-failure software in this project — it is the timebase every later number
is read against, and a defect in it is not visible in the data it produces.
Learning a second SDK inside that firmware would be exactly the combination the
earlier entry declined.

## What this decision does not claim

It does not claim STM32 measures better.

In particular it **does not rest on hardware timer input capture**, which was
available as an argument and is deliberately not made. The jitter gate is stated
against a standard deviation under 2 microseconds, and it will be met or missed
by where the I2C read sits relative to the conversion-ready interrupt. That is a
firmware structure question with the same answer on either family. Recorded here
so a later reader does not assume the stronger argument was made and quietly
inherit it.

## Consequences

The capture toolchain pin in `.mise.toml` stays OPEN. arm-none-eabi-gcc and
CubeMX are named; versions cannot be pinned until the part is.

**This record does not close Tier 0 gate 1.** That gate closes when a part is
named, a blink builds and flashes on it, and the two I2C peripherals are read
from the datasheet rather than from a product page. The gate's requirement is
about a part, and this entry has chosen a family.

Part narrowing inherits two constraints that eliminate more of the STM32 range
than the requirement list suggests. Two I2C masters **on separate pins** is the
binding one: parts that list two I2C instances often mux them onto overlapping
pin groups, which is a datasheet pinout question rather than a peripheral count.
USB device removes much of the F0 and G0 range. Both are checked against the
datasheet, not a selector table.

Concentrates the toolchain on ST's tooling. CubeMX generates, and generated code
that is committed is code this project owns and must read. The generated
initialisation is committed; it is not treated as vendor code that escapes
review.

Watch for: the part search stalling on optimisation. The requirement is a part
that meets four criteria and can be bought ten times over, not the best part in
the family. A search that has run longer than the afternoon Stage -1 cost is a
search that has changed shape.

## What would reopen this

No STM32 is found that meets all four requirements with acceptable local
availability, in which case RP2040 is reconsidered on capability grounds it
already satisfies.

The named part requires a paid toolchain or a proprietary programmer, which
would contradict the open-fabrication argument that
[two-channel-harness-built-in-house](./2026-08-09-two-channel-harness-built-in-house.md)
rests on: a rig any partner can fabricate is a rig certification can scale on,
and a rig needing a licensed compiler is not one.
