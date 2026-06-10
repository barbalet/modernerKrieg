# Asset Pipeline

`modernerKrieg` keeps source art, runtime assets, and metadata separate.

## Folders

- `assets/mosul/source/`: unmodified source art, source maps, references, and provenance notes.
- `assets/mosul/manifests/`: validated metadata that describes how source art maps to game/runtime concepts.
- `assets/mosul/runtime/`: generated runtime images, tiles, atlases, and other rebuildable products.
- `assets/mosul/maps/`: playable map products and tile/navigation metadata.
- `assets/mosul/atlases/`: packed atlas sheets and atlas metadata.
- `assets/mosul/sprites/`: engine-ready extracted sprites when sheets are sliced.

## Map Manifests

Map manifests describe authored map art before it becomes gameplay terrain. They record:

- map ID and display name
- world size in meters
- pixels per meter
- origin convention
- source layers, z order, alpha behavior, and intended runtime output path
- optional collision, navigation, and pathfinding output paths

The first map target is the 2003 Market / Commercial Streets demo, using the imported 7000 px map source layers.

## Building Level Manifests

The 2003 demo now has a C-validated building-level JSON manifest:

- `assets/mosul/manifests/market_commercial_streets_2003_building_levels.json`

It links the runtime floor PNG stack to gameplay geometry:

- four 7,000 px line-art PNGs under `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/`
- per-level PNG path, alpha mode, elevation, and default LOS/movement behavior
- explicit feature rectangles for walls, doors, windows, breach holes, stairs, and roof edges
- explicit `blocks_los`, `blocks_movement`, `allows_los`, and `allows_movement` booleans
- building regions with storey counts and roof-level IDs

One-storey buildings should still appear on level 2 as roofs. They should have no authored level 3 or level 4 content unless a roof access or higher structure is explicitly present. Doors and breach holes should override wall blockers for both line of sight and movement. Windows should allow line of sight but normally still block movement.

## Topology Manifests

Topology manifests turn validated level rectangles into a tactical graph. The
2003 demo uses:

- `assets/mosul/manifests/market_commercial_streets_2003_topology.json`

It records:

- stable node ids for streets, alleys, shops, courtyards, roofs, stairwells,
  caches, shelters, and deliberately blocked buildings
- portal ids and states for doors, windows, breach holes, archways, stairs,
  ladders, roof edges, street crossings, and rubble passages
- semantic zones for civilian shelters, evacuation exits, market crowds,
  caches, overwatch roofs, search objectives, restricted fire lanes, and danger
  areas
- level, region, feature, bounds, movement-cost, vertical-link, and debug-label
  metadata for C validation and replay/debug output

Topology manifests are loaded only after their matching building-level manifest.
Validation rejects duplicate ids, missing level/region/feature references,
orphan building regions, one-way portal mistakes, impossible vertical links,
invalid semantic zones, and unreachable enterable graph fragments.

## Derived Tactical Products

The first collision, navigation, line-of-sight, and cover products are derived
inside the portable C core from the validated building-level and topology
manifests. There is no separate generated tactical-product file yet.

Current derived products include:

- movement blockers from walls, windows, roof edges, blocked buildings, and
  closed/locked/blocked/unsafe portals
- LOS blockers from wall-like features, with doors, windows, and breach holes
  overriding blockers where the manifest allows sight
- navigation cost hints from topology node kind, portal movement cost, rubble,
  crowd, danger, and restricted-fire semantic zones
- cover hints from walls, windows, doors, breach holes, roof edges, interiors,
  shelters, caches, and rooftop zones
- sampled gameplay-area LOS traces with feature ids for the first blocking
  feature
- deterministic topology routes through enterable nodes and routeable portals,
  including vertical transitions through stairs or ladders

These derived products are exposed through C queries so native frontends,
headless runs, AI, and tests see the same tactical interpretation.

## Sprite Manifests

Sprite manifests describe source sheets, source-angle sprites, generated runtime sprites, and runtime sprite IDs. They record:

- sheet path and tile size
- pivot point
- scale in meters
- side, role, state, facing, and runtime ID for each frame
- fallback marker to use when a sprite is missing

The compact C-validated sprite manifest is:

- `assets/mosul/manifests/mosul_2003_sprites.spritemanifest`

The full imported render-pipeline manifests are loaded and validated by the C asset layer:

- `assets/mosul/runtime/sprites/manifest.json`
- `assets/mosul/runtime/sprites/rendered/render_manifest.json`

The current runtime sprite set contains 1,088 PNGs, and tests assert that every render-manifest path exists:

- 640 infantry sprites: 16 demo roles x 5 states x 8 facings.
- 168 civilian sprites: 7 civilian archetypes x 3 states x 8 facings.
- 64 weapon sprites: 8 weapon types x 8 facings.
- 216 vehicle sprites: 8 combat/support vehicle types x 3 damage states x 8 facings, plus 3 intact dynamic traffic vehicle types x 8 facings.

## Population Asset References

Scenario population records use sprite runtime IDs from the compact sprite
manifest. Civilian archetypes should reference the first-frame runtime id for
their current placeholder state, such as `civilian_adult_128_n`; the full
render manifest remains available to a renderer that needs every facing and
state. The Mosul scenario loader validates civilian archetype sprite ids against
the sprite manifest before the C core accepts the scenario.

Spawn zones, civilian groups, and unit templates reference gameplay-area
topology ids rather than image paths. This keeps art provenance in the manifest
layer and keeps scenario files focused on gameplay placement, side ownership,
and AI population intent.

## Marker Manifests

Marker manifests describe tactical overlay presentation without requiring final marker artwork. They record:

- marker ID and gameplay kind
- fallback shape
- color and alpha
- world-space radius hint
- screen-space line width hint

The first marker manifest covers selection, movement, fire, overwatch, suppression, casualty, objective, hidden contact, breach/search, rooftop/stair access, and civilian-risk markers.

## Validation

Every committed manifest should be validated by CTest. Validation should reject:

- missing required fields
- non-positive world or tile dimensions
- missing source files
- invalid layer/frame counts
- duplicate or empty IDs
- topology graph gaps, one-way portal mistakes, impossible vertical links, and
  orphan building regions
- invalid marker colors or impossible marker dimensions
- paths that point outside the repository asset tree

## Runtime Generation

Generated assets should be reproducible. Do not edit generated files by hand. The initial app can load a single map overview image; later work can add tiled map products and packed sprite atlases when the image size or sprite count requires it.

Current runtime product:

- `assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png`: copied from the source `preview_1400.png` as the first runtime map overview.
- `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/`: copied from the source 7,000 px ground, roof/second-floor, upper-floor, and roof-access line-art layers.

Traffic vehicles are now represented as dynamic runtime sprites rather than scenario data baked into the map. The current approved 7,000 px Market / Commercial Streets map layers still contain visible static traffic vehicle ink and should be rerendered from approved line-art map source without baked cars, buses, or motorcycles once that source renderer is available. Do not patch those map layers with simplified icon art, stick art, or blur/fill cleanup.
- `assets/mosul/runtime/sprites/rendered/`: copied from the MOSUL render pipeline as the first complete runtime-facing sprite set.

## Dynamic Vehicle And Map Cleanup Cycles

Current active cycle: cycle 8, verification and polish. Last completed cycle:
cycle 7, renderer and interaction pass. Blocked cycle: cycle 3,
vehicle-free map rerender, blocked on an exact 7,000 px geometry-preserving map
renderer. Last updated: 2026-06-10.

When work advances, update this tracker in the same commit as the relevant
code, art, or asset changes. Keep exactly one cycle marked `active` until the
dynamic vehicle work is complete.

The baked-vehicle cleanup and moving-vehicle integration should remain split
into reviewable cycles. This work crosses source art, generated runtime assets,
scenario records, C core simulation, renderer integration, and QA, so each
cycle should leave the repository in a testable state.

Cycle 2 audit artifact:

- `docs/market_commercial_streets_vehicle_bake_audit.md`

Cycle 3 current artifacts:

- `docs/market_commercial_streets_vehicle_free_rerender_attempt.md`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/candidates/vehicle_free_candidate_1254.png`

Cycle 4 validation artifacts:

- `docs/traffic_vehicle_sprite_validation.md`
- `scripts/validate_traffic_vehicle_sprites.py`

Cycle 5 integration artifacts:

- `docs/traffic_vehicle_scenario_integration.md`
- `tests/mosul/test_mosul_scenario_data.c`

Cycle 6 runtime artifacts:

- `docs/traffic_vehicle_runtime_behavior.md`
- `engine/core/src/mk_core.c`
- `engine/render/include/mk_board_view.h`
- `engine/render/src/mk_board_view.c`
- `engine/tools/autoplay/mk_headless_run.c`
- `engine/tools/autoplay/mk_replay_validate.c`
- `tests/core/test_core.c`

Cycle 7 renderer and interaction artifacts:

- `docs/traffic_vehicle_renderer_interaction.md`
- `engine/demo/include/mk_demo.h`
- `engine/demo/src/mk_demo.c`
- `tests/demo/test_demo_session.c`

| Cycle | Status | Name | Exit Criteria |
| --- | --- | --- | --- |
| 1 | completed | Submodule hygiene | `mosul` and `modernerKrieg` are fast-forwarded to the tips of `main`, and the nested gitlink updates are committed upward so every repository records the same baseline. |
| 2 | completed | Asset audit | Every baked car, bus, motorcycle, and vehicle-like mark in the Market / Commercial Streets map layers is identified and classified as removable background traffic, dynamic traffic, abandoned cover, or destroyed terrain. |
| 3 | blocked | Vehicle-free map rerender | The approved 7,000 px line-art map overview and level PNGs are regenerated without baked traffic vehicles, with no Pillow cleanup, blur/fill inpainting, stick art, or simplified replacement marks. |
| 4 | completed | Dynamic vehicle asset validation | Cars, buses, and motorcycles exist as generated `1024 x 1024` RGBA runtime sprites with alpha edges, matching the established line-art style and render manifest IDs. |
| 5 | completed | Scenario and data integration | `traffic_vehicle.*` records define positions, destinations, speed, facing, seat capacity, boarding mode, active state, and movement blocking. |
| 6 | completed | Runtime behavior | Path following, routing failures, occupant position updates, entering/exiting cars and buses, mounting/dismounting motorcycles, collision, blocking, save/snapshot, replay, and deterministic AI behavior are verified. |
| 7 | completed | Renderer and interaction pass | Traffic vehicles draw only from runtime RGBA sprites, expose picking/selection and boarding controls, and keep source angle art out of the live renderer path. |
| 8 | active | Verification and polish | CTests, asset manifest validation, alpha-edge validation, visual map before/after checks, and an in-game smoke pass show a vehicle-free base map with moving dynamic vehicles. |
