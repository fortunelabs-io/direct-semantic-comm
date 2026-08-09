# Tests

Pure. Read JSON from `../results/`, compute, exit. **Silent on success, exit 0.**

**A test may not prompt.** A test that prompts is a test whose result depends on who ran it. Prompting belongs in `../scripts/`, and that split is the reason the two directories exist.

**Empty, deliberately.** An empty test file is silent and exits 0 — indistinguishable from a gate that passed. Scaffolding this directory with stubs would report a closed ladder on a harness that does not exist. One file per gate, written when its capture script has something to read.
