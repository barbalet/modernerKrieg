# Autoplay Tools

This directory contains command-line tools for headless game runs.

The first tool, `mk_headless_run`, loads a MOSUL scenario and advances it for a fixed number of deterministic ticks.

The `mk_ai_battle` tool runs repeated MOSUL AI-vs-AI battles with both tactical sides controlled by AI. It is available from CMake and from the checked-in `modernerKriegAIBattles.xcodeproj` command-line project.

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

`mk_ai_battle` useful options:

- `--battles N`: run `N` sequential AI battles.
- `--forever`: keep starting new battles until interrupted.
- `--ticks N` or `--max-ticks N`: cap each battle.
- `--summary-every N`: print progress every `N` ticks.
- `--watchdog N`: report a stall when battle state does not change for `N` ticks.
- `--fail-on-stall`: return a failure code when the watchdog trips.
- `--verbose`: include per-unit order and position lines.
