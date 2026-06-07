# Engine Architecture

`modernerKrieg` keeps the simulation core portable and headless. Native platform frontends, the C demo session API, Mosul content, renderer helpers, autoplay tools, and AI policies sit above that core.

## Layer Rules

- `engine/core` owns deterministic game state, rules, snapshots, orders, random numbers, and the fixed-step loop.
- `engine/render` projects core snapshots into renderer-independent board views.
- `engine/demo` owns the native-wrapper C contract: session lifecycle, Mosul scenario loading, AI-only stepping, summaries, draw-command export, screen picking, and simple order helpers.
- Native platform frontends own windowing, input, image loading, and presentation.
- `engine/tools/autoplay` owns command-line headless runs for smoke tests, replay generation, and future AI-vs-AI batches.
- `engine/ai` owns controller policies that consume snapshots and emit the same orders as a human player.
- `game/mosul` owns Mosul-specific fixtures, data, scenario construction, and later campaign/demo rules.
- `tests` owns headless test executables and reusable test helpers.

`engine/core` must not include frontend headers, depend on Mosul content, open windows, load art assets, or perform platform-specific I/O.

CTest includes a boundary smoke check that scans `engine/core/include` and `engine/core/src` for platform frontend includes. That check is intentionally simple, but it makes accidental platform leakage visible during ordinary test runs.

## Fixed-Step Flow

Native frontends and headless runners should advance simulation through `mk_game_run_fixed_steps`. This keeps test runs, interactive play, watched AI-only games, and future replay playback on the same simulation path.

```text
scenario data -> mk_game_load_scenario
controller/input -> order structs
mk_game_run_fixed_steps -> mk_game_step
snapshot/event output -> renderer, tests, replay, or headless transcript
```

The first runner only advances the Mosul fixture. Later cycles should let it choose scenarios, controller assignments, seeds, tick limits, output logs, and stop conditions.

## Native Wrapper Contract

Native Mac and Windows frontends should link `modernerKriegDemo` first. That
layer deliberately exposes C-owned state without importing SwiftUI, Win32,
SDL, Metal, Direct2D, or other presentation frameworks:

- create and destroy an opaque `mk_demo_session_t`
- load the default Mosul 2003 scenario or a specific `.mkscenario` path
- step fixed ticks with optional AI-only order emission
- query `mk_demo_summary_t`, `mk_game_snapshot_t`, and performance counters
- fit the board to a screen rectangle
- collect stable draw commands for levels, units, civilians, soldiers,
  objectives, contacts, and tactical overlays
- map screen coordinates back to picks for units, civilians, contacts,
  objectives, terrain, topology nodes, portals, and semantic zones
- issue simple orders or selected-unit move orders from screen coordinates

Frontend code remains responsible for windows, input devices, image loading,
GPU drawing, menus, save panels, platform packaging, and platform accessibility.
It must not duplicate gameplay rules, hidden-contact logic, civilian AI,
objective scoring, route state, or combat resolution.
