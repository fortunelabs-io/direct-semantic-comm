<!--
Self-merged. The value of a PR alone is not review; it is a stable diff an issue
can cite. Open one when the branch touches firmware or hardware, or when the
diff is too large to read in one sitting. Otherwise merge with --no-ff and skip
this. See docs/sop/git_sop.md.
-->

## What this changes

## Issue

Closes #

## Checks

- [ ] No specification change shares a commit with a result it grades
- [ ] Any capture script was committed **before** the run it produced
- [ ] No energy figure appears in a commit message (binding until `negctl` closes)
- [ ] `mise run doctor` clean, if anything here was measured
