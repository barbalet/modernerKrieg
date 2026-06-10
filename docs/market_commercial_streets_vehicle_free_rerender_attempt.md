# Market / Commercial Streets Vehicle-Free Rerender Attempt

Last updated: 2026-06-09. This records the first cycle 3 rerender attempt for
the 2003 Market / Commercial Streets demo.

## Goal

Regenerate the Market / Commercial Streets ground map in the approved MOSUL
black-and-white contemporary graphite line-art style, with all baked cars,
buses, motorcycles, trucks, vans, and wreck-like vehicle silhouettes removed
from the base map art.

Cycle 3 is not complete yet because the generated candidate is not a safe
drop-in replacement for the existing 7,000 px gameplay-aligned map stack.

## Method

Tool path used:

- built-in Codex image generation tool
- existing map loaded as the visual edit/reference target
- no Pillow cleanup
- no local blur/fill/inpaint patching
- no stick art or schematic replacement art

Generated candidates:

- `assets/mosul/runtime/maps/market_commercial_streets_2003/candidates/vehicle_free_candidate_first_pass_1254.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/candidates/vehicle_free_candidate_1254.png`

The second candidate is the better review image. It removes the baked traffic
clutter and keeps the same general dense top-down market line-art style.

## Result

The candidate output is useful as visual direction, but it is not accepted as
the live map layer for these reasons:

- output size is 1,254 x 1,254, not the required 7,000 x 7,000 source/runtime
  overview scale
- generated building footprints, wall edges, awning positions, and street
  widths are close but not pixel-aligned with the existing building-level JSON,
  topology graph, LOS rectangles, movement blockers, and runtime upper-floor
  overlays
- replacing `level_01_ground.png` with this candidate would desynchronize the
  renderer from the C gameplay data

The live gameplay assets therefore remain unchanged:

- `assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/level_01_ground.png`
- source 7,000 px map PNGs under `assets/mosul/source/maps/...`

## Required To Complete Cycle 3

A valid cycle 3 completion still requires a true vehicle-free 7,000 px rerender
that preserves the current gameplay geometry. Acceptable paths are:

- rerun the original approved map renderer/source workflow with the audited
  vehicle clusters removed before rendering
- use a true image-editing/inpainting workflow that preserves the exact source
  pixels outside the audited vehicle bounds and regenerates only those regions
  in matching graphite line art
- regenerate the building-level JSON, topology graph, LOS blockers, movement
  blockers, and upper-floor overlays to match a redesigned full-map rerender

The preferred path is the first one: rerender from the original map source with
vehicles omitted. The third path is much larger and should not be treated as a
simple art cleanup.

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
