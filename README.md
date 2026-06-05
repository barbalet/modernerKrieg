# modernerKrieg

`modernerKrieg` is a portable tactical wargame engine for a Mosul demo.

The project starts fresh as an SDL3 + CMake codebase. `derZweiteWeltkrieg` remains a design reference for deterministic tactical rules and engine layering, but this repository is not a submodule consumer or direct dependency.

## Current Shape

- `engine/core/` contains the pure C simulation core.
- `engine/platform/sdl3/` contains the optional SDL3 app shell.
- `tests/core/` contains headless C tests.
- `assets/mosul/source/` is reserved for raw Mosul source art and references.
- `PLAN.md` describes the full engine and Mosul demo direction.

## Build

Configure and build the headless core/tests:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The SDL3 app target is enabled automatically when CMake can find SDL3. If SDL3 is installed in a custom prefix, pass its CMake package location:

```sh
cmake -S . -B build -DSDL3_DIR=/path/to/sdl3/lib/cmake/SDL3
cmake --build build
```

If SDL3 is not available, CMake still builds the portable core and tests.

## Design Intent

The game is commanded at unit scale, while the engine tracks meaningful soldier-level detail inside each unit: role, weapon, ammo, wounds, suppression, offset, exposure, and casualty state. That gives the Mosul demo room for modern urban systems such as civilians, drones, IEDs, breach actions, rooftops, hidden defenders, and asymmetric morale without turning the interface into individual-soldier micromanagement.
