# The host transport is native USB CDC, and the bare-metal no-HAL rule is scoped to the measurement path

**Date:** 2026-09-13 **Status:** Accepted
**Argument of record:** D1 in [`../hardware-harness-v1/hardware_hld.md`](../hardware-harness-v1/hardware_hld.md) §2; the exit named in [`../dsc_hld.md`](../dsc_hld.md) R1 / O2
**Amends:** [`capture-engine-firmware-is-bare-metal`](./2026-08-12-capture-engine-firmware-is-bare-metal.md) (narrows "no HAL anywhere in the capture engine firmware" to "no HAL in the *measurement path*"). That record's status line now points forward here; its body is not edited.
**Verification (2026-09-13):** No datasheet fact gates this policy decision. USB device pins are confirmed bonded on UFQFPN48 (DocID026289 Rev 7, Table 8: PA11/PA12 = pins 32/33 USB_FS_DM/DP, PA9 = pin 30 OTG_FS_VBUS, PA10 = pin 31 USB_FS_ID); the exact 48 MHz USB clock is provided by the HSE tree of the paired clock ADR.

## Context

[`../dsc_hld.md`](../dsc_hld.md) §4.3 names the capture-to-host wire protocol and
its transport "the largest hole," and R1 sharpens it: the bare-metal ADR forbids
vendor HAL anywhere in the capture engine firmware, and a hand-written USB device
stack is "substantially more firmware than everything currently in
`firmware/capture/` combined." R1 offered two exits that do not weaken that ADR's
actual argument: take the 2 Mbaud UART path the timing budget priced, or amend the
no-HAL scope to the measurement path and admit a stack for transport.

The hardware HLD closed **D1 on native USB CDC** and, in doing so, **foreclosed the
UART exit**: the board has no UART bridge, and the host connector lands on the
STM32's own USB pins ([`harness_spec.md`](../hardware-harness-v1/harness_spec.md)
§1). One exit remains, and this ADR takes it deliberately rather than by omission.

The bare-metal ADR's argument, read precisely, is about one thing: "every register
write in the jitter-critical path being traceable to a line of code this project
wrote." Its own scope note already says it is "scoped to the capture engine
specifically, because it is the one piece of software whose defects are not visible
in the data," and it declines to claim HAL is unsound for non-timing-critical
firmware. USB transport is not on the jitter-critical path: it is the lowest
priority task in the ISR-to-read-to-stream ordering
([`../dsc_hld.md`](../dsc_hld.md) §6.2), below the CNVR EXTI and the I2C read, and
it never touches TIM2 or the timestamp.

## Alternatives considered

- **Hand-write a minimal USB CDC device stack, no-HAL literally everywhere.** For:
  strict purity; zero third-party code in the firmware; full control end to end,
  which is the project's stated ethos. Against: it is the single largest firmware
  effort in the capture engine ([`../dsc_hld.md`](../dsc_hld.md) §4.3), spent on
  code with no bearing on the timebase, so the purity buys nothing the bare-metal
  ADR's argument actually values; a transport defect is still just a lost or
  malformed record, which the stream framing must detect regardless. Rejected on
  cost against benefit.
- **Amend the no-HAL scope to the measurement path; admit an audited USB device
  stack for transport only.** Chosen. The measurement path (clock tree, EXTI
  timestamp, I2C read, pointer discipline, TIM2) stays bare-metal register-level
  and this-project's-own, unchanged. A well-understood USB device stack (for
  example TinyUSB, or ST's low-level USB device library used as a library, not
  CubeMX-generated init) carries CDC, confined to the transport layer.
- **Revert to a UART bridge.** Foreclosed by D1; out of scope, noted for
  completeness.

## Decision

**Native USB CDC is the committed host transport.** The bare-metal ADR's "no
CubeMX-generated HAL anywhere in it" is narrowed to **"no HAL in the measurement
path"**: the jitter-critical path stays exactly as that ADR requires, and a
third-party or vendor USB device stack is admitted for the CDC transport under
these constraints, which bind whichever stack is later chosen:

1. The USB stack runs at the **lowest interrupt priority**, below the CNVR EXTI and
   the I2C events ([`../dsc_hld.md`](../dsc_hld.md) §6.2). It may be starved; the
   timestamp may not be delayed.
2. The USB stack **does not read or write TIM2**, does not take timestamps, and
   does not touch the ISR-to-read queue. It consumes finished records only.
3. The stack is used as a **library with hand-written, audited glue**, not as
   CubeMX-generated init. Clock, GPIO, and USB-peripheral enable for the USB block
   are hand-written from the datasheet like the rest. The 48 MHz USB clock is exact
   off the HSE tree of the paired ADR
   ([`capture-engine-clock-is-hse-8mhz-crystal`](./2026-09-13-capture-engine-clock-is-hse-8mhz-crystal.md)).
4. The specific stack is a **follow-on selection**; the constraints above are the
   binding part of this decision.

The wire-protocol **record format** ([`../dsc_hld.md`](../dsc_hld.md) §4.3: the
48-bit-timestamp recommendation and the type field) is a separate open item and is
**not decided here**. This ADR decides the transport and its relationship to the
bare-metal rule, not the bytes on the wire.

## Consequences

- The bare-metal ADR
  ([`capture-engine-firmware-is-bare-metal`](./2026-08-12-capture-engine-firmware-is-bare-metal.md))
  is **amended, not superseded**: its measurement-path decision stands verbatim. On
  acceptance of this ADR, that record's status line is updated to point forward
  here, per [`../sop/git_sop.md`](../sop/git_sop.md); its body is not edited.
- `register_map.h` gains the USB peripheral and its RCC enable; `startup.s` gains
  the USB interrupt vector ([`../dsc_hld.md`](../dsc_hld.md) §6.4 already flags the
  four-entry vector table as needing extension before any capture interrupt is
  wired). These measurement-path-adjacent parts are hand-written; the stack sits
  above them.
- [`../dsc_hld.md`](../dsc_hld.md) R1 and O2 are **partly discharged**: the
  transport is decided and its cost is bounded by using a stack. O2's remaining
  half, a record format that satisfies the `wrap` gate, stays open.
- Native USB requires the in-spec 48 MHz clock, which is why this ADR is paired with
  the HSE-crystal ADR of the same date; neither stands fully without the other.

## What would reopen this

- A demonstrated failure rate in the admitted stack that a hand-written one would
  not have had (the mirror of the bare-metal ADR's own reopen condition), which
  pushes the transport back to hand-written, or to UART on a respin.
- The record format (O2) forcing a transport property the CDC path cannot meet,
  though at 114 kB/s against USB full-speed that is not expected.
