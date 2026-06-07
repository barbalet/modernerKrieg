# Autoplay Tools

This directory contains command-line tools for headless game runs.

The first tool, `mk_headless_run`, loads a MOSUL scenario and advances it for a fixed number of deterministic ticks.

The `mk_ai_battle` tool runs repeated MOSUL AI-vs-AI battles with both tactical sides controlled by AI. It is available from CMake and from the checked-in `modernerKriegAIBattles.xcodeproj` command-line project. It can sweep deterministic seed ranges and fail a batch when settlement, stall, or score expectations are not met. The stall watchdog hashes units, civilians, contact reports, searched terrain/zones, and portal breach state so interaction progress counts as real battle progress.

The `mk_replay_validate` tool validates the first line-oriented `.mkreplay` event format, can assert final replay result/outcome fields, and can print a compact tick-by-tick playback summary from the validated event stream. Score events include objective, contested, interaction, and civilian-risk fields.

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
- `--expect-outcome OUTCOME`: fail unless the final outcome is `success`, `partial`, `failure`, or `in_progress`.
- `--expect-contested N`: fail unless the final score reports exactly `N` contested objectives.
- `--expect-min-civilian-risk N`: fail unless final civilian risk is at least `N`.
- `--expect-min-score N`: fail the run unless the final deterministic score is at least `N`.

The first replay format is line-oriented text headed by `mk_replay version=1`. Validate one with:

```sh
mk_replay_validate --expect-result MK_OK --expect-outcome failure build/mosul_ai.mkreplay
```

Play back a selected tick range with:

```sh
mk_replay_validate --playback --from-tick 2 --to-tick 10 build/mosul_ai.mkreplay
```

`mk_ai_battle` useful options:

- `--battles N`: run `N` sequential AI battles.
- `--forever`: keep starting new battles until interrupted.
- `--ticks N` or `--max-ticks N`: cap each battle.
- `--summary-every N`: print progress every `N` ticks.
- `--watchdog N`: report a stall when battle state does not change for `N` ticks.
- `--seed N`: set the first battle seed.
- `--seed-step N`: advance explicit seeds as `seed + (battle_index - 1) * seed_step`.
- `--fail-on-stall`: return a failure code when the watchdog trips.
- `--expect-settled N`: fail unless at least `N` battles reach success or partial outcome.
- `--expect-max-stalled N`: fail unless no more than `N` battles trip the watchdog.
- `--expect-min-worst-score N`: fail unless the lowest final score across the batch is at least `N`.
- `--keep-running-after-outcome`: continue ticking after success or partial outcome for soak testing.
- `--verbose`: include per-unit order and position lines.

Example seed-sweep balance check:

```sh
mk_ai_battle --battles 5 --ticks 160 --seed 84985359904819 --seed-step 101 --quiet --fail-on-stall --expect-settled 5 --expect-max-stalled 0 --expect-min-worst-score 440
```
