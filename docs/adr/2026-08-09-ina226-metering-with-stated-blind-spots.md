# Metering uses an INA226 with two stated blind spots, and the choice is revisited at Stage 3

**Date:** 2026-08-09
**Status:** Superseded by [2026-08-09-two-channel-harness-built-in-house.md](./2026-08-09-two-channel-harness-built-in-house.md)

## Context

The precedent methodology this project follows was run on a Power Profiler Kit
II: 100 kSa/s, automatic ranging from sub-microamp to an amp, and eight digital
inputs on the same instrument, so the current trace and the phase markers share
one timebase by construction. Espressif's own measurement guide recommends that
class of instrument, and explains why: the span from sleep current to active
current defeats meters that cannot range quickly.

An INA226 is roughly two orders of magnitude cheaper and is what this project
already planned to use. Reading its datasheet showed the default configuration
could not have resolved the transmit phase at all, and that no single shunt value
spans sleep to transmit on a fixed-range sixteen-bit part.

Without a decision, the project either overbuys an instrument before knowing
whether the direction survives Stage 1, or discovers at Stage 2 that its harness
was configured to average the measurement away.

## Alternatives considered

- Power Profiler Kit II, one unit, metering one node per run while both nodes'
  phase markers feed its digital inputs for a shared timebase. Solves ranging,
  bandwidth and alignment together and matches the precedent. Held as the
  fallback, not rejected on merit.
- Two INA226s plus a separate logic capture. Adds a timebase alignment problem
  that the Conversion Ready alert only partially solves.
- Two shunt values across two run sets, merged analytically. Rejected: adds a
  run-to-run equivalence assumption to buy a range the model does not use.

## Decision

Core runs use an INA226 per node, configured as follows, all of it forced by the
datasheet rather than chosen: shunt-only continuous mode at 140 microseconds with
averaging set to one; rail voltage measured once per run and treated as constant,
which is what buys shunt-only mode; high-speed I2C, since a register read at
400 kHz is comparable to the conversion time and would drop conversions; the
Alert pin configured as Conversion Ready and routed to the same capture as the
phase markers, since the device timestamps nothing and I2C reads are not
deterministic; and a Kelvin four-wire connection to the shunt.

Two limits are stated in the document rather than discovered later.

Phase boundary location carries an intrinsic uncertainty of one conversion time,
because the delta-sigma output is a window average and a conversion straddling a
boundary blends both phases. Against a transmit phase of order one millisecond
this is on the order of ten percent. No quantity the project reports depends on
locating a boundary more precisely, per the identification decision.

Deep sleep is not measured. A shunt sized to keep transmit peaks on scale places
sleep current at or below one least significant bit. Sleep is identical across
conditions and cancels in every comparison this document makes, and every term
in the cost model is an awake-window quantity.

The choice is revisited at Stage 3, when the encoder's own timing is known and
the ablation error budget has been measured against a real curve.

## Consequences

Commits Stage -1 to a shunt-sizing calculation and Stage 0 to a bandwidth check
alongside its accuracy checks, including a dropped-conversion count and a
rail-stability check.

Commits the driver to exposing conversion time, averaging mode, high-speed I2C
and the Conversion Ready alert. An off-the-shelf INA226 driver at default
configuration satisfies none of these and cannot be adopted unmodified.

Rules out any absolute battery-lifetime claim from this harness, and the document
makes none.

Watch for: an encoder whose forward pass turns out shorter than a few
conversion times. The repeat-and-slope method covers it, but if the encode term
ends up dominated by measurement error rather than by physics, that is the
signal to spend on the fallback instrument rather than to argue with the data.
