# Market / Commercial Streets Vehicle-Free Rerender

Last updated: 2026-06-12. This records the cycle 3 vehicle-free rerender for
the 2003 Market / Commercial Streets demo.

## Goal

Regenerate the Market / Commercial Streets ground map in the approved MOSUL
black-and-white contemporary graphite line-art style, with all baked cars,
buses, motorcycles, trucks, vans, and wreck-like vehicle silhouettes removed
from the base map art.

## Method

Tool path used:

- built-in Codex image generation tool
- existing map loaded as the visual edit/reference target
- no manual Pillow cleanup
- no local blur/fill/inpaint patching over vehicle marks
- no stick art or schematic replacement art

Generated candidates:

- `assets/mosul/runtime/maps/market_commercial_streets_2003/candidates/vehicle_free_candidate_first_pass_1254.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/candidates/vehicle_free_candidate_1254.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/candidates/vehicle_free_rerender_try1_7000/`

The second `1254 x 1254` candidate is the better first-pass review image. It
removes the baked traffic clutter and keeps the same general dense top-down
market line-art style.

On 2026-06-11, the original local source workflow was recovered from session
history and rerun with a vehicle-free source prompt. That workflow uses a
generated square source image, then a deterministic Pillow post-process to
produce exact `7000 x 7000` ground/composite/multistorey PNGs and `1400 x 1400`
previews. The output is saved under
`vehicle_free_rerender_try1_7000/`.

## Result

The `vehicle_free_rerender_try1_7000/` output was promoted to the live source
and runtime map stack after the gameplay data was reauthored to match the new
geometry. It was not a drop-in pixel replacement for the old map, so the
promotion also updated the building-level manifest, topology graph, line of
sight samples, movement/blocking samples, scenario placements, and tests.

Updated live runtime assets:

- `assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/level_01_ground.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/level_02_roofs_and_second_floor.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/level_03_upper_floor.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/level_04_roof_access.png`

Updated live source assets:

- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/imgs/market_commercial_streets_demo_7000/`

The default scenario now declares eight traffic vehicles: six moving road
vehicles and two courtyard parked vehicles. The main roads no longer use static
cover-up vehicle records. The 26-record static-road layout remains available as
`market_commercial_streets_nonblocking_static_roads_2003.mkscenario` for
comparison and regression coverage.

Cycle 3 is complete through the larger reauthoring path: the base map art has
no baked static traffic vehicle ink, and the upper-story overlays, building
levels, topology, LOS checks, movement blockers, scenario data, and C tests now
match the promoted vehicle-free map.

## Prompt Summary

The accepted direction prompt asked for the existing map to remain the edit
target, preserving the same geometry, camera, square crop, building footprints,
wall positions, roof outlines, central north-south street, east-west market
street, alley widths, stalls, awnings, courtyards, stairs, and rooftop details,
while removing all baked vehicles and filling those areas with continuous
street surface, cracks, curb grit, market debris, hatching, dust, and shadows.

Avoided terms included cars, buses, motorcycles, trucks, vans, parked vehicle
outlines, wreck-like vehicle silhouettes, vehicle-shaped white cutouts, labels,
text, UI, symbols, stick art, schematic placeholders, and cartoon style.
