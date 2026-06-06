# modernerKrieg

`modernerKrieg` is a portable tactical wargame engine for the MOSUL public demo.

The project starts fresh as an SDL3 + CMake codebase. `derZweiteWeltkrieg` remains a design reference for deterministic tactical rules and engine layering, but this repository is not a submodule consumer or direct dependency.

## Current Shape

- `engine/core/` contains the pure C simulation core.
- `engine/render/` contains renderer-independent board-view, pan/zoom, and marker projection code.
- `engine/assets/` contains manifest parsing and validation for map, sprite, and marker metadata.
- `engine/ai/` contains controller policies that emit normal core orders.
- `engine/platform/sdl3/` contains the optional SDL3 app shell.
- `engine/tools/autoplay/` contains the deterministic headless run tool.
- `game/mosul/` contains Mosul-specific scenario/content code built above the reusable core.
- `tests/autoplay/` contains fixed-step/headless runner tests.
- `tests/core/` contains headless C tests.
- `tests/render/` contains renderer-independent board-view tests.
- `docs/` contains architecture, build matrix, scenario-format, attribution, and milestone progress notes.
- `assets/mosul/source/` contains raw Mosul source art and references for the 2003 Market / Commercial Streets demo.
- `assets/mosul/runtime/` contains generated or copied runtime products, including the current map overview.
- `assets/mosul/maps/`, `assets/mosul/atlases/`, and `assets/mosul/sprites/` are placeholders for generated/runtime-ready assets.
- `PLAN.md` describes the full engine and Mosul demo direction.

## Build

Configure and build the headless core/tests:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Or use presets:

```sh
cmake --preset headless
cmake --build --preset headless
ctest --preset headless
```

For a stricter warning-as-error check:

```sh
cmake --preset strict
cmake --build --preset strict
ctest --preset strict
```

Run the current headless smoke scenario:

```sh
./build/bin/mk_headless_run --steps 10
```

With the headless preset build directory:

```sh
./build/headless/bin/mk_headless_run --steps 10
```

Run an explicit scenario file, override the deterministic seed, and write a transcript:

```sh
./build/bin/mk_headless_run \
  --scenario game/mosul/scenarios/market_commercial_streets_2003.mkscenario \
  --max-ticks 10 \
  --seed 12345 \
  --transcript build/mosul_transcript.txt
```

Run both tactical sides under deterministic AI:

```sh
./build/bin/mk_headless_run --ai-only --max-ticks 10
```

Write an after-action summary into a deterministic AI-only transcript:

```sh
./build/bin/mk_headless_run \
  --ai-only \
  --max-ticks 10 \
  --briefing \
  --debug-log \
  --aar \
  --quiet \
  --transcript build/mosul_ai_aar.txt
```

Add simple balance expectations to fail a headless run when the expected state is not reached:

```sh
./build/bin/mk_headless_run \
  --ai-only \
  --max-ticks 3 \
  --expect-objective opfor \
  --expect-min-score -10 \
  --quiet
```

The SDL3 app target is provisional. If SDL3 proves workable for the game feel and deployment path, it will be used. If not, the contingency is a new SwiftUI GUI over the same tactical model. The SDL3 target is enabled automatically when CMake can find SDL3. If SDL3 is installed in a custom prefix, pass its CMake package location:

```sh
cmake -S . -B build -DSDL3_DIR=/path/to/sdl3/lib/cmake/SDL3
cmake --build build
```

If SDL3 is not available, CMake still builds the portable core and tests.

## Current Demo Direction

The public MOSUL demo target is the 2003 Market / Commercial Streets scenario: a 500 m x 500 m central-city security fight after Mosul's fall during Operation Iraqi Freedom. The imported source assets currently include:

- line-art map previews and multi-storey source layers
- Market / Commercial Streets ground, upper-floor, roof-access, multistorey-mask, preview, and layer-manifest files
- 128 px top-down combatant, stance, vehicle, and weapon sheets
- source-angle weapon sprites
- reference plates for combatants, weapons, vehicles, and urban tactics

The current demo scenario now loads from validated data at `game/mosul/scenarios/market_commercial_streets_2003.mkscenario`. A C fixture remains available for tests so the data-backed path can be compared against the original scenario shape.

The first scenario and asset manifests are in place:

- `game/mosul/scenarios/market_commercial_streets_2003.mkscenario`
- `assets/mosul/manifests/market_commercial_streets_2003.mapmanifest`
- `assets/mosul/manifests/mosul_2003_sprites.spritemanifest`
- `assets/mosul/manifests/mosul_2003_markers.markermanifest`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png`

CTest validates those files and rejects broken references.

## Interaction

The core now supports deterministic board interaction without depending on SDL:

- pick a unit at a map coordinate
- select or clear a selected unit
- issue hold/rally/other orders
- issue withdraw orders through AI when units are pinned or heavily suppressed
- issue a move order and advance toward the target on each simulation tick
- run the same fixed-step loop from the SDL app or a headless command-line runner
- load scenario state with controllers, forces, civilians, map tiles, terrain zones, objectives, and units
- trace line of sight between map positions or units
- report blocking terrain and target cover
- resolve deterministic unit fire, ammo spend, hits, wounds, casualties, and suppression
- retain contact reports for fire, reveal, and civilian-risk events
- reveal hidden contacts through proximity and line-of-sight checks
- retain suspected-danger and false-contact reports with confidence and terrain metadata
- track civilian risk and stress from dangerous fire lanes and close armed movement
- update objective-control state and score outcomes from objective control, civilian risk, casualties, time, and scenario thresholds
- carry scenario briefing and after-action text through data loading, snapshots, and headless output
- produce deterministic debug/replay transcript lines for headless runs and future replay tooling
- track ready, suppressed, pinned, and broken unit states from live strength, training, and suppression
- slow or halt movement under suppression and apply fire penalties from combat stress
- represent richer soldier state, including stance, wound state, ammo capacity, stress, exposure, equipment flags, sensor flags, and reload/cooldown timers
- fit the tactical map to a screen rectangle
- pan and zoom a board view
- project terrain, objectives, units, and soldier offsets into screen space
- project tactical overlays for selection, movement, fire, suppression, hidden contacts, suspected/false contacts, casualties, objectives, objective control, and civilian risk
- validate map, sprite, and marker manifests without SDL
- load the 2003 scenario from data with parser-side and core-side validation
- run a selected scenario through the headless tool with seed, tick-count, quiet, and transcript controls
- run player and opposing tactical controllers as deterministic AI-only headless runs with move, suppress, hold, civilian-risk restraint, and withdraw choices
- hand map, sprite, and marker manifests to the SDL app, which can load PNGs when SDL3_image is available and fall back otherwise

The next implementation push is replay and command facing: versioned replay event files, AI behavior around suspected contacts, investigate/search commands, and score/objective presentation in the SDL shell.

When the SDL3 app target is available, left-click selects a unit, right-click gives the selected unit a move order, arrow keys pan the board, and plus/minus zoom the board.

## Design Intent

The game is commanded at unit scale, while the engine tracks meaningful soldier-level detail inside each unit: role, weapon, ammo, wounds, suppression, offset, exposure, and casualty state. That gives the Mosul demo room for modern urban systems such as civilians, drones, IEDs, breach actions, rooftops, hidden defenders, and asymmetric morale without turning the interface into individual-soldier micromanagement.
