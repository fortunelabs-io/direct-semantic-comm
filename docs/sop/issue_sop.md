# Issue SOP

*How GitHub Issues are used on this project. Scope is one developer, so an issue
is never an assignment. It is the only place a prediction can be recorded before
the run that tests it, and the only place a gate's status is allowed to live.*

---

## The model: one gate, one issue

`todos/stage0_todo.md` states its own constraint:

> This file is the specification and stays that way. Status is not repeated
> here. A number appearing in both files is a number that will disagree with
> itself.

That constraint is what the tracker exists to satisfy. Four places, no overlap:

| Question | Answered by |
|---|---|
| What must be true, and how is it tested? | `todos/`, `contracts/` |
| Has it been tested yet, and what was predicted? | the issue |
| What did it measure? | `harness/results/*.json`, summarised in `FINDINGS.md` |
| Why was it built this way? | `docs/adr/` |

An issue never restates a criterion. It links to the specification and holds
what the specification cannot: when it was run, what was predicted beforehand,
and which commit closed it.

---

## Identity

**A gate's handle is its mise task name.** `jitter`, `negctl`, `gain`. That name
already appears in `.mise.toml`, in `todos/stage0_todo.md`, and in
`harness/README.md`; inventing a letter scheme would give one gate a fourth
name, and a fourth name is a fourth thing to keep in agreement.

**Title format:** `S<stage> <handle> — <the claim, in the specification's own words>`

```
S0 jitter — the timestamp is taken at the edge and not after the read
S0 negctl — commanding a change moves the trace, and the markers bracket what they claim
```

Prefixing with the stage keeps the search box useful once Stage 1 opens.

> **Discrepancy to reconcile before opening these.**
> `contracts/stage_minus1_contract.md` cites "Stage 0 item C3" for the ESP-NOW
> version check. Under the tier ordering in `todos/stage0_todo.md`, the third
> item of Tier 2 is the radio-disabled image, not the version check. Fix the
> citation to name the handle (`link`) rather than a position, so it cannot
> drift again when a gate is inserted.

---

## The Stage 0 issue inventory

Eighteen issues, opened tier by tier. Handles match `mise run <handle>`.

| Tier | Handle | Type | Note |
|---|---|---|---|
| 0 | `toolchain` | gate | |
| 0 | `choices` | bench | no command; proved by `phase` two tiers down |
| 1 | `rate` | gate | |
| 1 | `jitter` | gate | |
| 1 | `dropped` | gate | |
| 1 | `stream` | gate | |
| 1 | `wrap` | gate | |
| 2 | `phase` | gate | |
| 2 | `link` | gate | |
| 2 | `link-noradio` | gate | same run as `link`, separate assertion |
| 3 | `fabricate` | bench | layout, review, order, assemble |
| 3 | `incoming` | gate | before any board is powered |
| 3 | `noop` | gate | |
| 3 | `negctl` | gate | the only gate that detects a physically wrong setup |
| 3 | `gain` | gate | |
| 3 | `timing` | gate | |
| 3 | `bandwidth` | gate | |
| 3 | `supply` | gate | |

`choices` is the one bench action with no gate beneath it in its own tier. Its
proof arrives at `phase`, and recording that dependency in the issue is what
stops a Gray sequence written carelessly in Tier 0 from being discovered in
Tier 2 without anyone connecting the two.

---

## Labels

Small on purpose. A tracker with thirty labels is a tracker whose labels are not
read.

| Label | Meaning |
|---|---|
| `stage:-1` … `stage:3` | which stage the item belongs to |
| `tier:0` … `tier:3` | tier within the stage, matching the specification's headings |
| `type:gate` | has a claim, a command, and a criterion that can fail |
| `type:bench` | physical work with no command of its own |
| `type:anomaly` | something the specification did not predict |
| `type:decision` | an open argument that will produce an ADR |
| `type:chore` | tooling and repository work |
| `blocked` | an open gate sits above this one |
| `prediction-missing` | opened after its run; the result is worth less |
| `invalidated` | closed once, and something below it changed |

`prediction-missing` is a label rather than a note because it must be visible in
the list view. The specification is explicit that a prediction added after the
run is worth nothing, and the tracker should say so without being opened.

Create them:

```sh
R=fortunelabs-io/direct-semantic-comm
gh label create "stage:-1" -R $R -c "5319e7" -d "Stage -1: configuration contract"
gh label create "stage:0"  -R $R -c "5319e7" -d "Stage 0: the harness is qualified"
gh label create "tier:0"   -R $R -c "c5def5" -d "nothing powered"
gh label create "tier:1"   -R $R -c "c5def5" -d "capture engine alone, on breakouts"
gh label create "tier:2"   -R $R -c "c5def5" -d "device under test, still on breakouts"
gh label create "tier:3"   -R $R -c "c5def5" -d "the fabricated harness"
gh label create "type:gate"     -R $R -c "0e8a16" -d "claim, command, criterion that can fail"
gh label create "type:bench"    -R $R -c "bfd4f2" -d "physical work, no command of its own"
gh label create "type:anomaly"  -R $R -c "d93f0b" -d "not predicted by the specification"
gh label create "type:decision" -R $R -c "fbca04" -d "open argument, produces an ADR"
gh label create "type:chore"    -R $R -c "ededed" -d "tooling and repository work"
gh label create "blocked"            -R $R -c "b60205" -d "an open gate sits above this one"
gh label create "prediction-missing" -R $R -c "e99695" -d "opened after its run; worth less"
gh label create "invalidated"        -R $R -c "d93f0b" -d "closed once, something below it changed"
```

The nine GitHub defaults (`bug`, `enhancement`, `good first issue`, …) describe a
project that takes contributions. Delete them.

---

## Lifecycle

### 1. Opened before the run

A gate issue is opened before its command is run, and the prediction is written
into it at that moment. This is the issue's whole reason to exist. An issue
opened after the run has its prediction field left empty and carries
`prediction-missing`; the prediction is not backdated, because a backdated
prediction is indistinguishable from a correct one and devalues every other
prediction in the tracker by association.

### 2. One tier at a time

Open the issues for a tier when the tier above it closes. Twenty open gates
cannot show which one is next, and the ordering in the specification exists
because of the work a late failure invalidates behind it.

Where a dependency runs across tiers — `choices` proved by `phase` — the two
issues reference each other at the time the first is opened.

### 3. Blocked

A gate whose predecessor is open carries `blocked`. Remove the label when the
predecessor closes. Nothing below an open gate gets run.

### 4. Closed by evidence

A gate issue closes when three things are true, all named in the issue:

1. The results file exists in `harness/results/` and is committed.
2. `mise run <handle>`'s test exited 0 with no output.
3. `mise run doctor` was clean for the run that produced the file.

Close it with `Closes #N` in the `gate` commit, not by clicking the button. The
link then lives in git history, which survives the repository moving hosts.

A bench issue closes by naming the gate issue that proved it. It never closes on
its own authority — a solder joint is proved by Tier 3 passing, not by
inspecting the joint.

### 5. Invalidation reopens; it does not duplicate

When a closed gate's premise changes — a firmware fix, a board revision, a
criterion corrected — **reopen the original issue**, label it `invalidated`, and
write what changed. Do not open a fresh one. A gate that passed, stopped
passing, and passed again is the most informative object in the tracker, and
splitting it across three issue numbers throws that away.

### 6. Anomalies

Anything the specification did not predict gets a `type:anomaly` issue,
referencing the gate issue it appeared under. It is closed by one of: a `spec`
commit that folds it into the specification, an ADR that decides it, or a
written statement that it is out of scope with the reason. An anomaly is never
closed silently — the whole discipline here is separating measured from
borrowed, and an unexplained observation is neither.

### 7. Decisions become ADRs

A `type:decision` issue is where the argument happens and the alternatives are
written down while they are still live. It closes when the ADR commit lands,
with the ADR path in the closing comment. Writing the record while the issue is
open is what keeps the rejected alternatives real; written afterwards they are
reconstructed, and reconstructed alternatives are always the ones that were easy
to reject.

---

## Milestones

One milestone per stage, named `Stage 0`, `Stage 1`, and so on. The milestone
closes when its findings document exists and every issue in it is closed. The
progress bar is then a real statement about the project, which is unusual for a
progress bar and worth not spoiling with issues that do not gate anything.

---

## What does not get an issue

Anything that will be done within the hour. Anything already fully stated in the
specification and not yet reached — that is what the specification is for.
Anything whose only content is "remember to". A tracker holding today's shell
commands is a tracker nobody opens, and once it is not opened, the gate issues
in it stop being a record of anything.
