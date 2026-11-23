# The harness hardware is CERN-OHL-S-2.0, and the firmware stays Apache-2.0

Date: 2026-11-24
Status: Accepted

## Context

[`two-channel-harness-built-in-house`](./2026-08-09-two-channel-harness-built-in-house.md)
committed the schematic and the capture firmware to the open side of the front
store, and kept the calibration procedure and the Layer 4 test vectors held. It
named the side and never named the terms.

As of this entry the repository carries no license file of any kind, and no
source file carries a notice. Apache-2.0 for the firmware has been the working
assumption, inherited from the software side of the business, but it is an
assumption and not a committed fact: nothing in the history states it. Under
default copyright the published design is all rights reserved. A partner who
fabricates the board to reproduce a published number has no grant permitting it,
which inverts the reason the harness was built in house at all.

The timing is now rather than later for two reasons. The `obol-harness` KiCad
project has existed since 2026-09-22 and is under active design, and the git SOP
tags the commit carrying the fabrication outputs `harness-v<N>-fab`. A license is
read by whoever receives a design, and the first recipient is the fabricator.
Settling the terms after boards exist means settling them with copies already
conveyed under no terms.

Two of the four pressures that justified building the harness are terms-shaped,
and they set what the license has to carry. **Certification adoption:** a rig any
partner can fabricate is a rig certification can scale on, and a rig nobody may
legally fabricate is not one. **Reference status:** data whose reproduction
requires someone else's gate makes this project a reference for the numbers while
somebody else holds the gate.

Apache-2.0 everywhere was the path of least thought, and hardware is where it
fails. Apache-2.0 grants copyright and patent rights over a work of authorship.
KiCad files are works of authorship; a populated board is not, and the license
says nothing about making a physical object from the documents it covers. Using
it for the hardware would leave the most valuable artifact governed by terms that
do not address what anyone actually does with it.

There is a sharper asymmetry underneath. The autoranging analog front end is
deferred, gated on Stage 2 data, and whoever builds it first defines what the
instrument is. Under permissive terms that successor can be closed, and this
project's own reference instrument then acquires a closed better version. That is
the exact dependency the build-in-house entry refused to accept from a commercial
vendor. Refusing a vendor's closed instrument and then licensing the replacement
so that the next closed instrument is legal is half a decision.

## Alternatives considered

- **Apache-2.0 for everything, hardware included.** Rejected: a software license
  applied to a board. It covers the design documents, grants nothing about the
  physical object, and leaves every improvement closable.
- **CERN-OHL-P-2.0, permissive.** Rejected: fixes the subject matter, keeps the
  posture. The autoranging successor can still close.
- **CERN-OHL-W-2.0, weakly reciprocal.** The serious contender. Changes to the
  harness files return, and the harness may sit inside a larger design without
  licensing that design. Rejected because nothing wants the harness inside a
  larger product. It is a bench instrument, so the flexibility W buys is not
  flexibility this artifact needs, while the one leak it permits, a closed
  derivative front end conveyed as part of a larger instrument, is the single
  failure mode this decision exists to close.
- **TAPR OHL, or Solderpad.** Rejected: TAPR is older, less maintained, and its
  reciprocity weaker and less tested. Solderpad is an Apache wrapper and inherits
  the posture rejected above. CERN-OHL-2.0 is the family with current
  maintenance, three deliberately separated variants, and SPDX identifiers.
- **Dual license: CERN-OHL-S plus a commercial exception sold with the Layer 4
  package.** Deferred rather than rejected. It needs a copyright position and a
  contributor agreement that do not exist today, and it is reachable only while
  this project holds the copyright in the hardware entire. This entry keeps it
  open; see Consequences.

## Decision

Two licenses, split by what a file is rather than by where it sits.

**CERN-OHL-S-2.0 governs the hardware design.** That is `harness/hardware/`: the
KiCad project, the custom rules file, the worksheet, the generated BOM, and the
fabrication outputs under `harness/hardware/fab/`. SPDX identifier
`CERN-OHL-S-2.0`. Text at [`../../LICENSES/CERN-OHL-S-2.0.txt`](../../LICENSES/CERN-OHL-S-2.0.txt).

**Apache-2.0 governs everything executable.** That is `harness/firmware/`, both
capture and DUT, plus `harness/scripts/`, `harness/tests/`, `harness/doctor.py`,
`harness/hardware/check.py`, and the host tooling. SPDX identifier `Apache-2.0`.
Text at [`../../LICENSES/Apache-2.0.txt`](../../LICENSES/Apache-2.0.txt). This
writes the firmware's existing posture down for the first time; it is not a
change to it.

**The hardware version is pinned to 2**, not "version 2 or later". Under §8.3 a
variant named without a version lets a licensee treat the design as released
under any CERN-OHL version carrying that letter, which is a term this project
would not have read. Moving to a later version is a later decision taken against
its own text.

Both texts are committed verbatim as published, Apache-2.0 from
`apache.org/licenses/LICENSE-2.0.txt` and CERN-OHL-S-2.0 from the CERN OHWR
project wiki, and are never edited. CERN-OHL-S states in its own preamble that it
may be used in unmodified form only.

**The boundary between the two is declared, not inferred.** CERN-OHL-S §1.6
counts loading as Making, so absent a declaration the capture firmware can be read
as part of the Complete Source that §4 requires a product's recipient to receive,
and on the §3.3(d) reading the whole would then have to be licensed under
CERN-OHL-S. Apache-2.0 is a Compatible Licence by §1.2(c), so the firmware
qualifies as an Available Component under §1.7(a), and §3.3(d) excludes Available
Components from the obligation to license the modified whole. The capture
firmware and the host tooling are therefore declared Available Components, and
they remain Apache-2.0 inside a CERN-OHL-S product. The declaration runs one way
only: Apache-2.0 code may be taken into a CERN-OHL-S design, and nothing in
CERN-OHL-S reaches back onto it.

**Catalog parts are Available Components by §1.7(b)(i).** The INA226AIDGSR and
the STM32F411CEU6 are purchasable parts with public datasheets, so the license
never reaches inside them. This is the same boundary the build-in-house entry
drew when it scoped the project to the timebase and not the front end, restated
in the license's own vocabulary instead of only in prose.

**The Source Location is the repository**, `github.com/fortunelabs-io/direct-semantic-comm`,
named in the root `LICENSE` because §1.9 defines a Source Location as somewhere
the licensor reasonably believes will stay reachable for three years. Boards
carry that URL and the SPDX identifier in the silkscreen and the schematic title
block. The §4 mechanism that would compel every downstream product to display the
Source Location is deliberately not invoked: an obligation this project cannot
observe being met is ceremony, and the reciprocity in §3 is what actually
protects the design.

**What is held is unchanged.** The calibration procedure and the Layer 4 test
vectors are not published and are not covered by either license. A license states
terms for what is released and says nothing about what is not.

## Consequences

Commits the project to the strong reading. Anyone who conveys a board based on
this design, or a modified design, must provide the complete design source or the
location of it, under CERN-OHL-S. Including the covered source in a larger design
makes the larger design modified Covered Source by §3.2. That is the intent and
it is also the price: a commercial partner who wants the harness inside a closed
product cannot have it on these terms. The answer to that partner is the deferred
dual license, decided on its own evidence, not a quiet swap to the W variant.

Covers the autoranging front end before it is designed. Whoever builds it, this
project or a partner, publishes it.

Makes the fabrication outputs Covered Source. The `harness-v<N>-fab` tag now also
marks the commit a recipient is entitled to be pointed at, so the repository
moving hosts stops being a convenience question and becomes a three-year
obligation under §1.9.

Keeps the dual license reachable only while the copyright stays whole. Accepting
an outside contribution to `harness/hardware/` under CERN-OHL-S alone would put
that file beyond this project's power to relicense. So if the commercial
exception is to remain a real option, a contributor position precedes the first
accepted hardware patch rather than following it.

No `DECISION_LOG.md`. The build-in-house entry states that this harness is
instance one and earns generalization after it has metered three things, so it is
not yet a reusable asset and does not carry a log. If it ever becomes one, the
log starts then and this entry is its first row.

**Not done, named so it is not mistaken for done:** per-file SPDX headers, the
notice in the KiCad title block and the worksheet, and a license line in
`harness/README.md` and `harness/hardware/README.md`. Until the headers exist the
split is asserted in one place and is invisible in the files themselves, which is
the same drift the fab parity rule exists to catch one layer down. The copyright
holder is written as FortuneLabs, matching the board name in the hardware HLD; if
a legal entity string is required it replaces that one everywhere in a single
`chore` commit.

Watch for: a claim that the held calibration procedure is part of the Complete
Source a recipient must receive under §4, on the §1.8 reading that it is
necessary interfacing information. The position taken here is that the board is
Made and runs as an instrument without it, and that the procedure qualifies a
number rather than configuring the hardware. It is never published, so it is
never Covered Source. If that position is challenged the answer is a new ADR, not
an edit to this one.

Watch for: variant drift. S, W and P are three licenses, not three settings of
one. A file or a README that says "CERN-OHL-2.0" with no variant letter states
nothing, and the S is the whole decision.
