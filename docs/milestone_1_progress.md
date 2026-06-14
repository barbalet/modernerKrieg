# Milestone 1 Progress

This note tracks the first half of Milestone 1: core state expansion before data-file parsing.

## Completed Cycles

### 1.01 Stable IDs, Limits, Ownership, and Result Codes

- Added fixed capacities for controllers, forces, civilians, and map tiles.
- Kept ID ownership local to each container: add functions assign stable 1-based IDs.
- Extended validation so bad references fail during scenario load.

### 1.02 Side, Faction, Force, Command Identity, and Controller Slots

- Added controller slots with human, scripted AI, tactical AI, and observer modes.
- Added force records tied to side, faction, controller, and command identity.
- Added command identity records with names and callsigns for forces and units.

### 1.03 Terrain, Tile, Zone, Elevation, and Movement Cost Data

- Kept existing terrain zones for broad area cover/LOS.
- Added map tiles with coordinate, terrain kind, elevation, cover, movement cost, LOS blocking, and movement blocking.

### 1.04 Map Dimensions, Tile Storage, Bounds Checks, and Terrain Lookup

- Added tile grid configuration, tile lookup, and tile replacement helpers.
- Added validation for tile coordinates, tile count, and movement costs.

### 1.05 Weapon, Ammunition, Reload, Range Band, and Fire Mode Definitions

- Extended weapon profiles with fire mode, ammo kind, magazine capacity, reload ticks, and cooldown ticks.

### 1.06 Soldier Roles, Stance, Facing, Equipment, Health, Wound, and Casualty State

- Extended soldiers with max health, ammo capacity, stance, wound state, stress, exposure, equipment flags, sensor flags, reload/cooldown timers, and movement availability.

### 1.07 Unit Records

- Extended units with force/controller IDs, command identity, order source, morale, fatigue, command disruption, communications state, and cover posture.

### 1.08 Minimal Civilian Records

- Added `mk_civilian_t` records independent of combat units.
- Kept the existing civilian unit in the Mosul fixture for current rendering/tests while the civilian system matures.

### 1.09 Deterministic Random Seed Handling

- Existing deterministic RNG remains unchanged and covered by tests.

### 1.10 Scenario Container State

- Scenario, game, and snapshot state now include controllers, forces, civilians, objectives, map, units, and time.
- The Mosul fixture now assigns both combat sides to tactical AI controller slots and civilians to an observer slot.

## Verification

```sh
cmake --build build
ctest --test-dir build --output-on-failure
cmake --build --preset strict
ctest --preset strict
./build/strict/bin/mk_headless_run --steps 3 --quiet
```

Result: default and strict builds passed, 5/5 tests passed in both paths, and the strict headless runner completed successfully.

## Next Cycles

Milestone 1 should continue with:

- 1.11 movement intent and order-source metadata
- 1.12 serialization-friendly snapshot compare/copy helpers
- 1.13 first scenario/content data format choice
- 1.14 parser for core content and controller assignments
- 1.15 validation errors for invalid parsed data
