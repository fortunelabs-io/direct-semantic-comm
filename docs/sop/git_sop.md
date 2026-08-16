# Git SOP

*Standard operating procedure for the repository. Scope is one developer. Rules
that exist to coordinate people are absent; every rule below exists because this
repository is the evidence chain for a measurement claim.*

**The question this SOP answers.** *Which commit produced this number, and what
was the state of the specification when it was produced?* A rule that does not
help answer that question is not in this document.

---

## What the repository is

`main` is the state of record. A claim is true of the project when it is true of
`main`, and the history is what allows a figure in `FINDINGS.md` to be traced
back to the capture script, the pinned interpreter, and the criterion that was
in force when the capture ran.

Four kinds of thing live here, and they do not overlap:

| Where | What | Changes when |
|---|---|---|
| `contracts/` | declarations that gate everything below them | a declaration is answered or reopened |
| `todos/` | gates: claim, command, criterion, prediction | the specification changes |
| `docs/adr/` | decisions and why they were taken | a decision is taken or superseded |
| `harness/results/` | what the gates measured | a gate runs |

Status lives in none of them. Status lives in the issue tracker, per
[`issue_sop.md`](./issue_sop.md). A number written in two places is a number that
will eventually disagree with itself.

---

## Branches

`main` is always buildable and always the state of record.

**Work directly on `main`** when the change is documentation, a specification
edit, or repository furniture. A branch for a typo is ceremony, and ceremony
practised alone decays into ceremony ignored.

**Branch** when the work can leave the repository in a state where a claim
inside it is unverifiable, or when it will span more than one session. Firmware,
capture scripts, tests, and hardware always branch.

**Naming: `<type>/<slug>`.** A gate branch carries the mise task name verbatim,
because that name is already the gate's identity in three files:

```
gate/jitter          work on the gate invoked by `mise run jitter`
hw/harness-v1-layout KiCad, review, fabrication outputs
fix/wrap-extension
chore/…  doc/…  adr/…  spike/…
```

`spike/` is throwaway by declaration. A spike branch is never merged; what
survives it is rewritten on a branch of another type. This is the only way to
explore without the exploration entering the record as if it were considered.

**Merge with `--no-ff`.** The branch boundary is the record of what was done as
one piece of work. A fast-forward erases it, and a merge commit costs nothing.

**Delete the branch after merging**, locally and on `origin`. A merged branch
left standing is indistinguishable at a glance from work in progress.

`git fetch --prune` after a merge, so a branch deleted on `origin` stops
appearing in `git branch -a` as though it were still live. The remote-tracking
ref outlives the branch otherwise, which defeats the rule above.

---

## Commits

Conventional Commits, already in use here. Subjects are imperative — *correct*,
not *correcting* — because git's own generated subjects are imperative and a
mixed log reads as two authors. Five commits is the cheapest this switch will
ever be.

| Type | For |
|---|---|
| `gate` | lands a `results/*.json` and the test that reads it |
| `feat` | new capability in capture firmware, DUT firmware, or host tooling |
| `fix` | corrects behaviour that was wrong |
| `spec` | edits `contracts/` or `todos/` |
| `adr` | adds or supersedes an entry in `docs/adr/` |
| `hw` | KiCad, BOM, fabrication outputs |
| `docs` | prose that is not a specification |
| `chore` | tooling, pins, repository furniture |

`gate` is a separate type from `feat` because it is the only commit type that
adds evidence rather than capability, and it is the type the whole history will
be filtered on later.

### Three atomicity rules

**A specification change and the result it grades never share a commit.** A
criterion edited alongside the number it judges is a criterion that could have
been edited to fit the number, and nothing in the diff distinguishes the two
cases. When a criterion turns out to be wrong, the `spec` commit lands first, on
its own, carrying its reason.

**The capture script is committed before the run, never after.** A script
committed alongside the capture it produced cannot be shown to be the script
that ran. The result JSON and the test that reads it may share a commit with
each other; the script that produced the JSON must already be in history.

**One gate per commit.** Two results in one commit cannot be reverted
separately, and the first thing wanted from a gate that later looks wrong is to
remove it without removing its neighbour.

### The body of a `gate` commit

Names what cannot be recovered from the diff:

```
gate: close jitter, sd 1.4 us over ten minutes

mise run jitter
results/stage0_jitter.json
mise run doctor: clean
Prediction held: read is out of the interrupt path, no 73 us mode.

Closes #7
```

`mise run doctor` is named because a capture taken under a leaked environment is
well-formed and wrong, and the diff cannot show which interpreter ran.

### Standing rules

Inherited from `todos/stage0_todo.md` and binding on commits:

- **No energy figure appears in a commit message before the negative control
  gate is closed.** Until then the harness is a board that produces numbers, and
  a number in a commit message is quoted long before it is qualified.
- **A `results/*.json` is never edited by hand.** A hand-edited result is not a
  result. If one is wrong, delete it and re-run; the commit that deletes it says
  why.
- **Measured shunt values replace nominal from the gain gate onward**, in code
  and in prose. A commit that reintroduces a nominal value is a regression even
  though nothing fails.

### History

Amend and rebase freely while a commit is unpushed. Once pushed, never — even
alone. The reason is not collaboration; it is that a pushed commit is the only
copy that survives this machine, and the SHA may already be cited from an issue.

---

## Tags

Annotated tags, two kinds, both marking a point that the physical world can be
compared against:

- **`stage<N>-closed`** — every gate issue for the stage is closed and the
  stage's findings document exists. This is the commit at which the harness
  stops being a board that produces numbers.
- **`harness-v<N>-fab`** — on the commit carrying the exact Gerbers sent for
  fabrication. Ten boards will exist matching one commit, and when board seven
  behaves differently from board two, this tag is what says they should not
  have.

---

## What is tracked

`harness/.gitignore` governs everything under `harness/` and states its own
rule: ignore by explicit extension, never by blanket wildcard under `results/`.
Read it before adding a pattern.

The root `.gitignore` covers only what is not specific to the harness — OS and
editor droppings. Anything harness-shaped belongs in the harness file, next to
the reasoning for its neighbours.

Tracked deliberately, against the instinct to ignore them: `.mise.toml` (it is
the pin), `sdkconfig.defaults` (the generated `sdkconfig` is not), fabrication
outputs, and every `results/*.json`.

---

## Architecture decision records

**Write one** when a decision would be expensive to reverse, or when a future
reader would otherwise attribute the outcome to accident rather than to a
choice. Rejected alternatives are the payload; a record that lists only what was
chosen has recorded nothing.

**Filename:** `docs/adr/YYYY-MM-DD-<kebab-slug>.md`, the date being the date of
the decision, not of the writing.

**Status:** `Proposed`, `Accepted`, or `Superseded by <file>`.

**Never edit the Decision section of an Accepted record.** Supersede it with a
new file that names what survives from the old one and what changes — as
[`two-channel-harness-built-in-house`](../adr/2026-08-09-two-channel-harness-built-in-house.md)
already does. An edited decision destroys the evidence that the project once
believed otherwise, which is the part worth keeping.

**The ADR is written from the argument, not from memory.** The argument is held
in a `type:decision` issue and the record is committed while it is still open.
See [`issue_sop.md`](./issue_sop.md).

---

## Pull requests

Optional, and self-merged. The value of a PR alone is not review; it is a stable
URL carrying a diff that an issue can cite.

Open one when the branch touches firmware or hardware, or when the diff is too
large to read in one sitting. Otherwise merge the branch directly with
`--no-ff`.

---

## Push cadence

**Push at the end of every session.** The failure mode for one developer is a
dead disk, not a merge conflict, and an uncommitted capture is a capture that
must be re-run on hardware.

Before pushing: `git status` shows nothing unexpected, and no `results/` file
appears that a gate commit did not intend.
