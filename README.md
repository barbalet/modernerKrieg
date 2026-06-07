# modernerKrieg

`modernerKrieg` is a portable tactical wargame engine for the MOSUL public demo.

The project starts fresh as a portable C + CMake engine with native Mac and Windows frontends planned above it. `derZweiteWeltkrieg` remains a design reference for deterministic tactical rules and engine layering, but this repository is not a submodule consumer or direct dependency.

## Current Shape

- `engine/core/` contains the pure C simulation core.
- `engine/render/` contains renderer-independent board-view, pan/zoom, and marker projection code.
- `engine/assets/` contains manifest parsing and validation for map, sprite, and marker metadata.
- `engine/ai/` contains controller policies that emit normal core orders.
- `engine/tools/autoplay/` contains deterministic headless run, replay validation/playback, and AI-vs-AI battle tools.
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

Run repeated AI-vs-AI battles with progress/watchdog output:

```sh
./build/bin/mk_ai_battle --battles 5 --ticks 160 --summary-every 10 --watchdog 40
```

Run a quiet AI-vs-AI seed sweep that fails on stalls or weak batch results:

```sh
./build/bin/mk_ai_battle \
  --battles 5 \
  --ticks 160 \
  --seed 84985359904819 \
  --seed-step 101 \
  --quiet \
  --fail-on-stall \
  --expect-settled 5 \
  --expect-max-stalled 0 \
  --expect-min-worst-score 440
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

Write a versioned replay/event file for deterministic validation:

```sh
./build/bin/mk_headless_run \
  --ai-only \
  --max-ticks 10 \
  --quiet \
  --replay build/mosul_ai.mkreplay
```

Validate a replay/event file:

```sh
./build/bin/mk_replay_validate --expect-result MK_OK build/mosul_ai.mkreplay
```

Play back a concise replay timeline for a tick range:

```sh
./build/bin/mk_replay_validate --playback --from-tick 2 --to-tick 10 build/mosul_ai.mkreplay
```

Add simple balance expectations to fail a headless run when the expected state is not reached:

```sh
./build/bin/mk_headless_run \
  --ai-only \
  --max-ticks 3 \
  --expect-objective opfor \
  --expect-outcome failure \
  --expect-min-score -10 \
  --quiet
```

The previous SDL experiment has been removed. The launchable interfaces should now be platform-native shells over the same C libraries: a Mac frontend first, then a Windows frontend, with the command-line tools remaining the deterministic test and debugging surface.

## Xcode AI Battles

The checked-in Xcode command-line project is:

```sh
modernerKriegAIBattles.xcodeproj
```

Open it in Xcode and run the shared `mk_ai_battle` scheme to build the portable C core, asset parser, AI, renderer helper, Mosul scenario loader, and AI battle runner. The scheme runs the five-seed AI-vs-AI balance check by default, prints regular battle summaries plus watchdog stall reports, and exits nonzero when settlement, stall, or worst-score expectations are not met.
Battles stop once they reach a success or partial outcome; pass `--keep-running-after-outcome` for longer soak runs that should keep checking for late tactical stalls.

You can also build it from Terminal:

```sh
xcodebuild -project modernerKriegAIBattles.xcodeproj -scheme mk_ai_battle -configuration Debug build
```

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

The core now supports deterministic board interaction without depending on a platform frontend:

- pick a unit at a map coordinate
- select or clear a selected unit
- issue hold/rally/other orders
- issue investigate orders toward unresolved suspected or false contacts
- issue withdraw orders through AI when units are pinned or heavily suppressed
- issue a move order and advance toward the target on each simulation tick
- run the same fixed-step loop from native frontends or a headless command-line runner
- load scenario state with controllers, forces, civilians, map tiles, terrain zones, objectives, and units
- trace line of sight between map positions or units
- report blocking terrain and target cover
- resolve deterministic unit fire, ammo spend, hits, wounds, casualties, and suppression
- retain contact reports for fire, reveal, and civilian-risk events
- reveal hidden contacts through proximity and line-of-sight checks
- retain suspected-danger and false-contact reports with confidence and terrain metadata
- pick unresolved suspected/false contact reports for player-facing investigate commands
- track civilian risk and stress from dangerous fire lanes and close armed movement
- update objective-control state and score outcomes from objective control, civilian risk, casualties, time, scenario thresholds, and scenario score weights
- carry scenario briefing and after-action text through data loading, snapshots, and headless output
- produce deterministic debug transcript lines plus versioned replay/event files that can be validated and played back by `mk_replay_validate`
- run AI-only seed sweeps with settlement, stall, and score expectations for balance checks
- track ready, suppressed, pinned, and broken unit states from live strength, training, and suppression
- slow or halt movement under suppression and apply fire penalties from combat stress
- represent richer soldier state, including stance, wound state, ammo capacity, stress, exposure, equipment flags, sensor flags, and reload/cooldown timers
- fit the tactical map to a screen rectangle
- pan and zoom a board view
- project terrain, objectives, units, and soldier offsets into screen space
- project tactical overlays for selection, movement, fire, suppression, hidden contacts, suspected/false contacts, casualties, objectives, objective control, civilian risk, and visible unit order status
- validate map, sprite, and marker manifests from portable C
- load the 2003 scenario from data with parser-side and core-side validation
- run a selected scenario through the headless tool with seed, tick-count, quiet, and transcript controls
- run player and opposing tactical controllers as deterministic AI-only headless runs with move, investigate, overwatch, suppress, hold, civilian-risk restraint, and withdraw choices
- hand map, sprite, and marker manifests to platform frontends for PNG-backed map, unit, marker, HUD, briefing, status, and AAR presentation

The first 100-cycle plan is complete. Next work should focus on final art replacement, deeper search/breach/rooftop rules, a native Mac frontend, a native Windows frontend, and platform packaging validation.

## Design Intent

The game is commanded at unit scale, while the engine tracks meaningful soldier-level detail inside each unit: role, weapon, ammo, wounds, suppression, offset, exposure, and casualty state. That gives the Mosul demo room for modern urban systems such as civilians, drones, IEDs, breach actions, rooftops, hidden defenders, and asymmetric morale without turning the interface into individual-soldier micromanagement.
