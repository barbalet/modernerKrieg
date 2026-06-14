# Market / Commercial Streets Baked Vehicle Audit

Last updated: 2026-06-09. This audit satisfies dynamic vehicle cleanup cycle 2
for the 2003 Market / Commercial Streets demo.

## Scope

Inspected source art:

- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/01_ground_level.png`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/imgs/market_commercial_streets_demo_7000/preview_1400.png`

Checked layer metadata:

- `02_level_2_alpha.png`, `03_level_3_alpha.png`, and
  `04_roof_access_alpha.png` are transparent upper-level overlays. No traffic
  vehicle cleanup should be authored into these upper layers unless the source
  rerender introduces a rooftop or upper-floor vehicle by mistake.

Coordinate convention:

- bounds are approximate source-map pixels in the 7,000 x 7,000 overview
- origin is top-left, with x increasing right and y increasing downward
- the source represents 500 m x 500 m, so 14 px is approximately 1 m

This is a visual audit for rerender direction. It is not a pixel-editing mask.
Cycle 3 must regenerate the approved line-art map without baked vehicles rather
than patching, blurring, filling, or painting over the current PNGs.

## Classification Rules

- `dynamic_traffic`: road-lane cars, buses, motorcycles, or vans that can become
  runtime `traffic_vehicle.*` scenario records.
- `removable_background_traffic`: incidental parked or ambiguous traffic marks
  that should disappear from the base map and do not need scenario records.
- `abandoned_cover`: parked or obstructing vehicles that may return later as
  explicit terrain/cover objects, not baked line art.
- `destroyed_terrain`: wreck-like or damaged vehicle marks that may return later
  as explicit rubble/wreck terrain, not baked line art.

## Cluster Inventory

| Audit ID | Approx Source Bounds | Count | Classification | Rerender Requirement |
| --- | --- | ---: | --- | --- |
| BV-A01 | x 0-750, y 1650-3150 | 5 | abandoned_cover | Remove the west-edge parked vans/cars from the base street art. Reintroduce only as explicit cover or static wreck records. |
| BV-A02 | x 750-1900, y 2400-3300 | 6 | removable_background_traffic | Remove the west market-street parked traffic embedded along the awnings and curb. |
| BV-A03 | x 1900-3050, y 2400-3500 | 7 | dynamic_traffic | Promote the diagonal cars and small vehicles at the western approach to scenario traffic candidates. |
| BV-A04 | x 2850-3950, y 0-1350 | 8 | dynamic_traffic | Remove the north-avenue cars/vans from base art and replace with runtime traffic placements if needed. |
| BV-A05 | x 2850-4000, y 1350-2650 | 9 | dynamic_traffic | Remove the mid-north avenue traffic line; preserve road texture and curb detail only. |
| BV-A06 | x 3950-5300, y 0-2100 | 6 | removable_background_traffic | Remove small parked or courtyard-adjacent traffic-like marks from the eastern north blocks. |
| BV-A07 | x 5450-6900, y 0-2300 | 7 | abandoned_cover | Remove top-right parked cars/vans from the map art. Any retained obstacle must be authored as explicit cover/terrain. |
| BV-A08 | x 3000-4200, y 2700-3600 | 12 | dynamic_traffic | Remove all central-intersection cars, motorcycles, carts, and traffic clutter from base art; runtime traffic and civilian crowd props should own this space. |
| BV-A09 | x 3000-4200, y 3200-4050 | 5 | destroyed_terrain | Remove the wreck-like central marks from base art. If tactically wanted, replace with explicit wreck/rubble terrain records. |
| BV-A10 | x 4200-5600, y 2450-3400 | 6 | removable_background_traffic | Remove east market-street parked vehicles and small traffic marks from the base layer. |
| BV-A11 | x 5600-7000, y 2450-3900 | 8 | abandoned_cover | Remove right-edge buses/vans and parked cars. Reintroduce only as explicit static cover or dynamic vehicles. |
| BV-A12 | x 2750-4100, y 3700-5200 | 8 | dynamic_traffic | Remove southbound street cars/vans/bus-like marks and replace with runtime traffic placements where needed. |
| BV-A13 | x 2750-4100, y 5200-7000 | 9 | dynamic_traffic | Remove lower avenue buses/cars from base art and move required traffic into scenario data. |
| BV-A14 | x 0-900, y 4300-7000 | 4 | removable_background_traffic | Remove southwest edge vehicle-like marks unless explicitly reauthored as terrain. |
| BV-A15 | x 5600-7000, y 3900-7000 | 7 | removable_background_traffic | Remove southeast side-lane and courtyard vehicle-like marks from base art. |

Estimated total baked vehicle or vehicle-like marks: 107. The count is
intentionally conservative: ambiguous carts, motorcycles, compact cars, parked
vans, buses, and wreck-like vehicle silhouettes are included so cycle 3 can
rerender the base map cleanly instead of leaving small traffic remnants behind.

## Required Cycle 3 Output

Cycle 3 should produce vehicle-free replacements for:

- source `01_ground_level.png`
- source `00_minimap_composite.png`
- source `preview_1400.png`
- source `06_multistorey_lineart_overview.png`, if it inherits visible street
  vehicles from the ground art
- runtime `overview.png`
- runtime `levels/level_01_ground.png`

The rerender should preserve street cracks, curb tone, awning shadows, market
stalls, wall outlines, roof detail, rubble, and pedestrian-scale street clutter.
It should not preserve cars, buses, motorcycles, truck silhouettes, traffic
lane vehicles, parked-vehicle outlines, or vehicle-shaped white cutouts.
