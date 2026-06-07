# Autoplay Tools

This directory contains command-line tools for headless game runs.

The first tool, `mk_headless_run`, loads a MOSUL scenario and advances it for a fixed number of deterministic ticks.

Useful options:

- `--scenario PATH`: load a specific `.mkscenario` file.
- `--project-root PATH`: resolve repo-relative asset references from a specific root.
- `--steps N` or `--max-ticks N`: run a fixed number of ticks.
- `--seed N`: override the scenario seed for repeatable variants.
- `--quiet`: suppress console transcript output.
- `--transcript PATH`: write the run transcript to a file.
- `--replay PATH`: write a versioned replay/event file with unit, objective, score, contact, and end records.
- `--ai-only`: let tactical AI controllers issue orders for both non-civilian sides before each tick.
- `--aar`: append a deterministic after-action summary after the result line.
- `--briefing`: print or record the scenario briefing before the first tick.
- `--debug-log`: print or record replay-oriented debug lines after each tick.
- `--expect-objective SIDE`: fail the run unless at least one objective is controlled by `player`, `opfor`, `civilian`, or `neutral`.
- `--expect-min-score N`: fail the run unless the final deterministic score is at least `N`.

The first replay format is line-oriented text headed by `mk_replay version=1`. It is intended for deterministic validation first, then future playback tooling.

Future cycles should extend this into richer AI-vs-AI autoplay with controller assignment, replay playback/validation, score-threshold checks, and batch balance summaries.
