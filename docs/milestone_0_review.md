# Milestone 0 Review

Milestone 0 establishes the portable skeleton for `modernerKrieg`: CMake, pure C core, optional SDL3 app, headless runner, tests, asset placeholders, and documentation for the clean-break engine boundary.

## Completed Scope

- Clean-break attribution placeholder: `docs/third_party.md`.
- SDL3 dependency policy: optional `find_package(SDL3 CONFIG)` app target.
- Normalized repository layout for engine, game, docs, tests, assets, AI, and autoplay tools.
- Top-level CMake project with warning options and stable output directories.
- Pure C core static library.
- Core public smoke API and fixed-step runner.
- Headless test executables and headless Mosul runner.
- Reusable test helper header with transcript support.
- Optional SDL3 platform target and explicit SDL input state plumbing.
- Shared logging/result-name helpers for app, tools, and tests.
- Public core math/value helpers for vectors, integer coordinates, rectangles, distances, and clamps.
- Placeholder Mosul asset folders and README stubs.
- CMake presets for default, headless, and strict builds.
- CI-friendly build matrix notes.
- CTest smoke check proving `engine/core` does not include SDL headers.
- Architecture notes documenting core/platform/content boundaries.

## Local Verification

Run on macOS with AppleClang. SDL3 was not found locally, and the optional SDL target skipped cleanly.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Result: 5/5 tests passed.

```sh
cmake --preset strict
cmake --build --preset strict
ctest --preset strict
./build/strict/bin/mk_headless_run --steps 3
```

Result: strict warning-as-error build passed, 5/5 tests passed, and the headless Mosul runner completed three deterministic ticks.

```sh
cmake --preset headless
cmake --build --preset headless
ctest --preset headless
./build/headless/bin/mk_headless_run --steps 3 --quiet
```

Result: headless preset build passed, 5/5 tests passed, and the quiet headless runner completed without output.

## Known Gaps

- SDL3 app compilation was not verified locally because SDL3 is not installed or not discoverable by CMake.
- Windows and Linux smoke tests are documented but not yet run.
- AI-vs-AI controller policies are not implemented yet; only the headless fixed-step path exists.
- Scenario data is still constructed in C fixtures and should move toward validated data files in Milestone 1.

## Milestone 1 Entry Checklist

Start Milestone 1 with the core state model:

- stable IDs, limits, ownership rules, and result/error codes
- side, faction, force, command identity, and controller-slot structs
- terrain, tile, zone, elevation, and movement-cost data structures
- map dimensions, tile storage, bounds checks, and terrain lookup helpers
- weapon, ammunition, reload, range-band, and fire-mode definitions
