# Engine Architecture

`modernerKrieg` keeps the simulation core portable and headless. The SDL3 app, Mosul content, renderer helpers, autoplay tools, and future AI policies sit above that core.

## Layer Rules

- `engine/core` owns deterministic game state, rules, snapshots, orders, random numbers, and the fixed-step loop.
- `engine/render` projects core snapshots into renderer-independent board views.
- `engine/platform/sdl3` owns windowing, input, and SDL rendering.
- `engine/tools/autoplay` owns command-line headless runs for smoke tests, replay generation, and future AI-vs-AI batches.
- `engine/ai` is reserved for controller policies that consume snapshots and emit the same orders as a human player.
- `game/mosul` owns Mosul-specific fixtures, data, scenario construction, and later campaign/demo rules.
- `tests` owns headless test executables and reusable test helpers.

`engine/core` must not include SDL headers, depend on Mosul content, open windows, load art assets, or perform platform-specific I/O.

CTest includes a boundary smoke check that scans `engine/core/include` and `engine/core/src` for SDL includes. That check is intentionally simple, but it makes accidental platform leakage visible during ordinary test runs.

## Fixed-Step Flow

The SDL app and headless runner both advance simulation through `mk_game_run_fixed_steps`. This keeps test runs, interactive play, watched AI-only games, and future replay playback on the same simulation path.

```text
scenario data -> mk_game_load_scenario
controller/input -> order structs
mk_game_run_fixed_steps -> mk_game_step
snapshot/event output -> renderer, tests, replay, or headless transcript
```

The first runner only advances the Mosul fixture. Later cycles should let it choose scenarios, controller assignments, seeds, tick limits, output logs, and stop conditions.
