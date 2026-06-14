# Traffic Vehicle Cycle 8 Verification

Cycle 8 is the verification and polish pass for dynamic Mosul traffic vehicles.
It turns the software-verifiable checks into repeatable CTest coverage and
verifies them against the promoted vehicle-free map.

## Automated Checks

The CTest suite now includes:

- `mk_traffic_vehicle_sprite_validation`
- `mk_traffic_vehicle_runtime_smoke`

`mk_traffic_vehicle_sprite_validation` runs
`scripts/validate_traffic_vehicle_sprites.py`. It uses the Python standard
library to inspect PNG chunks directly and verifies that all 24 traffic vehicle
runtime sprites exist, are `1024 x 1024` RGBA PNGs, have transparent corners,
have a transparent outer border, contain visible vehicle pixels, and are listed
in `render_manifest.json`.

`mk_traffic_vehicle_runtime_smoke` runs
`scripts/check_traffic_vehicle_runtime_smoke.py`. It creates a deterministic
headless replay from the default 2003 Market / Commercial Streets scenario and
checks that:

- the gameplay area reports the expected `7000 x 7000` tactical map metadata
- the replay reports eight traffic vehicle records
- every tick contains a complete record for each vehicle
- every vehicle record uses a known traffic sprite ID
- every vehicle exposes valid kind, boarding mode, seat, occupancy, active, and
  movement-blocker fields
- at least three traffic vehicles move by at least one meter during the smoke
  pass
- at least two traffic vehicles begin without destinations, proving parked
  courtyard vehicles are represented as static runtime blockers rather than
  ignored map artwork

This gives the renderer and gameplay layer a repeatable in-game proof that
traffic vehicles are dynamic runtime entities rather than static map marks.

## Visual Map Status

The approved runtime Market / Commercial Streets map PNGs are now the cycle 3
vehicle-free rerender. The live source/runtime level stack, building-level
manifest, topology manifest, scenario placements, LOS samples, movement samples,
and smoke expectations were reauthored together so the C gameplay data matches
the new map geometry.

## Local Verification

Run from the repository root:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "mk_traffic_vehicle|mk_asset_manifest|mk_demo_session|mk_headless_run_replay_output"
```

For the full local suite:

```sh
ctest --test-dir build --output-on-failure
```
