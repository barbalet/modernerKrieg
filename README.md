# modernerKrieg

`modernerKrieg` is a portable tactical wargame engine for the MOSUL public demo.

The project starts fresh as an SDL3 + CMake codebase. `derZweiteWeltkrieg` remains a design reference for deterministic tactical rules and engine layering, but this repository is not a submodule consumer or direct dependency.

## Current Shape

- `engine/core/` contains the pure C simulation core.
- `engine/render/` contains renderer-independent board-view, pan/zoom, and marker projection code.
- `engine/platform/sdl3/` contains the optional SDL3 app shell.
- `game/mosul/` contains Mosul-specific scenario/content code built above the reusable core.
- `tests/core/` contains headless C tests.
- `assets/mosul/source/` contains raw Mosul source art and references for the 2003 Market / Commercial Streets demo.
- `PLAN.md` describes the full engine and Mosul demo direction.

## Build

Configure and build the headless core/tests:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
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
- 128 px top-down combatant, stance, vehicle, and weapon sheets
- source-angle weapon sprites
- reference plates for combatants, weapons, vehicles, and urban tactics

The current code scenario remains a small placeholder while the data and scenario layer is redirected to the 2003 demo.

## Interaction

The core now supports deterministic board interaction without depending on SDL:

- pick a unit at a map coordinate
- select or clear a selected unit
- issue hold/rally/other orders
- issue a move order and advance toward the target on each simulation tick
- trace line of sight between map positions or units
- report blocking terrain and target cover
- resolve deterministic unit fire, ammo spend, hits, wounds, casualties, and suppression
- track ready, suppressed, pinned, and broken unit states from live strength, training, and suppression
- slow or halt movement under suppression and apply fire penalties from combat stress
- fit the tactical map to a screen rectangle
- pan and zoom a board view
- project terrain, objectives, units, and soldier offsets into screen space

When the SDL3 app target is available, left-click selects a unit, right-click gives the selected unit a move order, arrow keys pan the board, and plus/minus zoom the board.

## Design Intent

The game is commanded at unit scale, while the engine tracks meaningful soldier-level detail inside each unit: role, weapon, ammo, wounds, suppression, offset, exposure, and casualty state. That gives the Mosul demo room for modern urban systems such as civilians, drones, IEDs, breach actions, rooftops, hidden defenders, and asymmetric morale without turning the interface into individual-soldier micromanagement.
