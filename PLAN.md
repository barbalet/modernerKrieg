# modernerKrieg Technical Plan

`modernerKrieg` is the portable C engine for the MOSUL 2003 public demo. The app layer should be a native SwiftUI wrapper over stable C interfaces; all rules, scenario state, movement, combat, civilian behavior, replay, and deterministic validation stay in C.

The first fully fledged demo target is the 2003 Market / Commercial Streets battle slice: a 500 m x 500 m urban security fight with U.S. patrol forces, irregular armed threats, protected civilians, multistorey buildings, roof access, shops, alleys, courtyards, rubble, search/cache points, and civilian-risk consequences.

## Current Audit

### Map Availability

The repository has an effective visual map foundation for the 2003 demo:

- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/01_ground_level.png`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/02_level_2_alpha.png`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/03_level_3_alpha.png`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/04_roof_access_alpha.png`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/05_multistorey_mask.png`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/imgs/market_commercial_streets_demo_7000/preview_1400.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png`

The map manifest describes a 500 m x 500 m world at 14 px/m overview scale. The source map layers are 7000 x 7000 pixels. The art is good enough as a first visual battle map: roads, market stalls, vehicles, alleys, courtyards, roof masses, stair-like features, and dense cover are visible.

What is missing is gameplay-grade map data:

- no generated collision mask exists at `assets/mosul/maps/market_commercial_streets_2003_collision.mask`
- no generated navigation grid exists at `assets/mosul/maps/market_commercial_streets_2003_navigation.grid`
- no room/interior graph exists for shop interiors, courtyards, doorways, windows, or internal movement
- no vertical connectivity graph exists for level 2, level 3, rooftops, stairs, or roof access
- no per-building metadata exists for enterable, locked, breached, searched, occupied, or civilian shelter state
- current scenario terrain is still coarse: 10 x 10 tiles and six hand-authored terrain zones

Conclusion: the visual map is effective for the demo, but the engine still needs an interior/navigation layer before building interiors can matter tactically.

### Sprite And Asset Availability

The runtime sprite set now contains 1,064 rendered PNGs:

- 640 infantry sprites: 16 combatant roles x 5 body states x 8 facings
- 168 civilian sprites: 7 civilian archetypes x 3 body states x 8 facings
- 64 weapon sprites: 8 weapon types x 8 facings
- 192 vehicle sprites: 8 vehicle types x 3 damage states x 8 facings

The C asset layer validates map, sprite, marker, and runtime render manifests. The SwiftUI layer should consume validated render-manifest entries from the C engine, not rediscover assets independently.

### Current Engine Capability

Already present:

- deterministic C core with game state, fixed-step simulation, snapshots, scenario loading, scoring, AAR text, and replay/event tooling
- units, soldiers, weapons, wounds, suppression, morale states, hidden contacts, suspected/false contacts, objectives, civilians, civilian risk, and basic AI order emission
- renderer-independent board projection and tactical overlays
- headless AI-vs-AI tools and CTest balance smoke coverage
- CMake presets for default, headless, and strict warning-as-error builds
- Xcode command-line AI battle project

Important gaps:

- civilian records are passive risk/stress entities, not full movement agents
- no civilian AI route planning, panic, compliance, crowd behavior, shelter seeking, evacuation, or instructions model
- unit movement is direct target movement, not nav-graph/path movement
- line of sight and cover use coarse terrain, not interior walls, windows, doors, floors, or roofs
- breach/search/cache/roof systems are mostly affordances, not full rules
- SwiftUI wrapper boundary is not yet a stable C ABI
- the demo scenario has only one civilian in the main 2003 file, despite the new civilian art set

## Product Goal

After 80 development cycles, the repository should contain a fully fledged demo engine for the MOSUL 2003 battle:

- one deterministic playable scenario loaded entirely from data
- visual map, multistorey map metadata, collision, navigation, and interior/roof topology
- unit-scale commands with soldier-level consequences
- moving civilian population with deterministic AI
- opposing-force AI, player-side tactical AI for AI-only tests, and civilian AI running together
- combat, suppression, concealment, breach/search, rooftop, and civilian-risk rules that work on the map topology
- stable C API for a SwiftUI wrapper
- headless and Xcode AI-only battle checks that catch stalls, bad balance, broken assets, and invalid maps

## Architecture Rules

- `engine/core` owns deterministic rules, state, orders, movement, LOS, combat, civilians, scoring, replay, and validation.
- `engine/assets` owns manifest parsing and validation for map, sprite, marker, render, collision, navigation, and topology products.
- `engine/render` owns renderer-independent projection, sprite/marker selection, and frontend draw-command preparation.
- `engine/ai` owns player/opfor/civilian policies that emit ordinary C orders or civilian intent updates.
- `game/mosul` owns scenario data, Mosul-specific templates, map topology data, and demo tuning.
- SwiftUI owns windowing, input, presentation, image upload/cache, panels, and platform packaging only.
- No gameplay rule should exist only in Swift.

## Civilian Movement Model

Civilian AI should be deterministic, cheap, and legible.

State per civilian:

- archetype, group id, protected/non-protected flag
- current state: sheltering, fleeing, frozen, following instructions, wounded, dead
- current floor/area/node, map position, facing, speed, path, destination
- stress, risk, confidence in nearby friendly force, compliance, panic threshold
- last threat source, last safe node, assigned shelter/exit, and movement cooldown

Inputs:

- nearby gunfire, explosions, armed movement, visible casualties, fire lanes, objectives, blocked paths
- player instructions, safe corridors, breach/search events, building entry/exit, roof/stair access
- civilian group cohesion, family separation, wounded civilians, and crowd density

Actions:

- stay sheltered, freeze, flee, move to shelter, move to exit, follow instructions, disperse from fire lane, avoid armed units, assist/cluster near family, or become casualty/incapacitated

Implementation approach:

- build a civilian navigation graph from the map topology
- update civilian intent at a lower cadence than unit ticks to keep behavior stable
- use deterministic weighted choices from the scenario seed
- prefer safe graph routes over straight-line motion
- expose civilian intent, path, stress, and risk in snapshots and replay events
- add CTests for every state transition and every path/risk rule

## Map And Building Interior Model

The current art supports a multistorey demo, but not yet a real interior simulation. The engine should add generated or authored map products:

- collision mask: blocked/open movement at tactical resolution
- navigation grid: movement costs and blocked cells
- topology graph: rooms, shops, alleys, courtyards, rooftops, stairs, doors, windows, breach points, exits
- floor graph: ground, level 2, level 3, roof with vertical connectors
- LOS occluders: exterior walls, interior walls, parapets, vehicles, rubble, smoke/future effects
- semantic zones: market stalls, shopfronts, civilian shelter areas, caches, rooftops, danger areas, objectives

The first implementation can be partly authored data over the visual map. Do not wait for perfect computer-vision extraction. The demo needs reliable tactical topology more than clever extraction.

## Development Cycle Ledger

- Ledger baseline date: 2026-06-07.
- Planned cycle budget: 80 cycles.
- Planned cycle shape: 8 milestones x 10 cycles each.
- Completed cycles in this ledger: 0.
- Current cycle: 0.
- Remaining planned cycles: 80.
- Next cycle batch: cycles 1-10, Map Data Foundation.

When a development batch is completed, increment `Completed cycles in this ledger`, advance `Current cycle`, reduce `Remaining planned cycles`, update `Next cycle batch`, and add a log row.

| Date | Cycles Completed | Current Cycle | Remaining Cycles | Focus | Notes |
| --- | ---: | ---: | ---: | --- | --- |
| 2026-06-07 | 0 | 0 | 80 | Baseline | New 80-cycle technical plan created after the larger asset-base update. |

## Planned Cycle Batches

### Cycles 1-10: Map Data Foundation

Goal: turn the visual 2003 map into validated map data that the engine can reason about.

Deliverables:

- Add map-product schema for collision masks, navigation grids, floor layers, room ids, portals, and vertical connectors.
- Extend `mk_asset_map_manifest_t` or add dedicated map-product manifests without bloating the old simple map loader.
- Add validation for manifest paths, dimensions, world scale, layer ids, and required map products.
- Create first authored `market_commercial_streets_2003` topology data over the existing map art.
- Generate or hand-author initial collision and navigation products under `assets/mosul/maps/`.
- Represent streets, stalls, alleys, courtyards, shop interiors, rooftops, blocked routes, stairs, and breach points.
- Add tests that fail when collision/navigation/topology files are missing or dimensionally inconsistent.
- Keep map products source-safe: generated/runtime products stay outside `assets/mosul/source/`.
- Update docs with map product provenance and rebuild rules.
- Verify default, strict, Xcode AI battle, and direct map-validation tests.

Exit criteria:

- The engine can load a map topology graph for the 2003 demo.
- The map manifest no longer points at nonexistent collision/navigation products.
- Tests prove the topology covers roads, buildings, interiors, roofs, and exits.

### Cycles 11-20: Navigation, Movement, And LOS

Goal: replace straight-line movement assumptions with topology-aware tactical movement.

Deliverables:

- Add C pathfinding over grid/topology data with deterministic A* or equivalent.
- Add per-area movement costs, blocked cells, dynamic blocked states, and route validation.
- Add unit movement along paths with route progress, stuck detection, and cancellation.
- Add civilian-compatible path requests with safe-route weighting.
- Add floor-aware movement and vertical transitions through stairs/roof access points.
- Add LOS over map occluders, doors/windows, rooftops, elevation, parapets, and interior walls.
- Add cover evaluation from room/wall/window/vehicle/rubble topology.
- Update board-view overlays to project routes, floor selection, visible areas, and blocked path reasons.
- Add replay events for path assignment, path failure, and vertical movement.
- Add deterministic tests for pathfinding, vertical transitions, LOS, and cover.

Exit criteria:

- Units can move through streets, alleys, interiors, and rooftops by path.
- LOS and cover match authored building topology.
- Headless smoke tests detect stalled movement.

### Cycles 21-30: Scenario Data Expansion

Goal: make the 2003 battle scenario data-rich enough for a real demo.

Deliverables:

- Expand `.mkscenario` or introduce companion data files for forces, civilian groups, spawn zones, AI plans, and map topology references.
- Add reusable templates for soldiers, squads, weapons, civilian archetypes, vehicles, and hidden threat groups.
- Replace single-civilian placeholder with a small market population using the 168 civilian runtime sprites.
- Add civilian groups: vendors, shoppers, families, bystanders, wounded/incapacitated civilians.
- Add opfor groups: hidden cell, rooftop watcher, armed looter, machine-gunner, RPG threat, cache guard.
- Add player force templates for patrol, support weapons, breacher, medic, marksman, and vehicle crew.
- Add validation for template ids, asset ids, spawn bounds, pathable starts, faction references, and impossible loadouts.
- Add scenario variants for smoke tests: empty map, interior contact, civilian panic, rooftop threat, cache search, evacuation.
- Add deterministic fixture parity where useful, then reduce hard-coded Mosul fixture reliance.
- Update docs for scenario format v2.

Exit criteria:

- Main 2003 scenario loads a credible population and opposing force from data.
- Scenario validation rejects unpathable starts, missing sprite ids, invalid civilian groups, and impossible force templates.

### Cycles 31-40: Civilian Movement And Civilian AI

Goal: turn civilians into deterministic moving agents whose behavior affects tactics and scoring.

Deliverables:

- Extend `mk_civilian_t` with profile, group id, floor/topology node, destination, path, speed, compliance, panic, and intent fields.
- Add civilian intent update step separate from tactical unit orders.
- Implement shelter, freeze, flee, follow-instructions, disperse, assist-group, wounded, and dead states.
- Add safe-zone, exit-zone, shelter-zone, and danger-zone data to map/scenario files.
- Compute threat maps from gunfire, visible armed units, explosions/future events, casualties, and blocked routes.
- Add civilian pathfinding that avoids fire lanes, armed units, hidden threat zones when known, and blocked interiors.
- Add group cohesion behavior so civilians do not scatter randomly unless panic breaks cohesion.
- Add player instructions as orders/events that civilians may follow based on stress and trust.
- Add replay/debug transcript lines for civilian state changes and route choices.
- Add CTests for every civilian state transition, path choice, risk change, and scoring effect.

Exit criteria:

- AI-only battle runs include moving civilians.
- Civilian movement can create meaningful tactical constraints without random nondeterminism.
- Civilian harm/risk is explainable in replay/AAR output.

### Cycles 41-50: Urban Combat, Breach, Search, And Rooftops

Goal: make the modern urban systems mechanically real.

Deliverables:

- Implement breach/search/cache actions on topology nodes and semantic zones.
- Add door/window/portal states: closed, open, breached, blocked, searched.
- Add hidden threat reveal rules for interiors, rooftops, caches, and line-of-sight proximity.
- Add rooftop/elevation combat modifiers, exposure, vertical LOS, and roof access restrictions.
- Add suppression and casualty effects that interact with rooms, cover, windows, and civilian proximity.
- Add non-lethal restraint/hold-fire pressure when civilians cross fire lanes.
- Add search outcomes: no threat, cache found, enemy revealed, booby trap/future hook, civilian found.
- Add opfor displacement and ambush behavior using rooms, roofs, and caches.
- Add after-action scoring for breach/search success, civilian protection, cache handling, force preservation, and time.
- Add tests for every interaction, including invalid actions and replay determinism.

Exit criteria:

- The demo can produce contact inside buildings, on rooftops, and around civilians.
- Search/breach choices affect objectives and AAR scoring.

### Cycles 51-60: Tactical AI And Autoplay Balance

Goal: make AI-only runs meaningful enough to expose engine problems early.

Deliverables:

- Expand player-side tactical AI for movement, cover use, investigate/search, breach, restraint, casualty response, and objective control.
- Expand opfor AI for defend, ambush, displace, hide, threaten civilians, avoid suicidal exposure, and exploit interiors/roofs.
- Add civilian AI into the same fixed-step autoplay loop.
- Add batch AI battle scenarios with deterministic seeds and expected outcome envelopes.
- Add stall detection for units and civilians separately.
- Add coverage for path failures, endless civilian panic loops, no-contact battles, over-lethal fire, and score collapse.
- Add replay diff tooling for AI-only regressions.
- Add performance counters for pathfinding, LOS, civilian updates, and replay output.
- Tune scoring weights and scenario starts against multi-seed runs.
- Keep Xcode command-line project aligned with the CMake AI battle runner.

Exit criteria:

- AI-only runs exercise player, opfor, and civilian behavior together.
- Batch tests catch stalls, impossible paths, broken civilian behavior, and bad scoring.

### Cycles 61-70: SwiftUI Wrapper Contract

Goal: give the SwiftUI app a stable, minimal C boundary without moving gameplay into Swift.

Deliverables:

- Define public C API headers for creating/destroying a game session, loading scenario data, stepping fixed ticks, issuing orders, querying snapshots, and reading render commands.
- Add C ABI-safe structs for snapshot summaries, units, soldiers, civilians, contacts, objectives, terrain overlays, routes, sprites, and AAR text.
- Add stable ids for map assets, sprite render entries, markers, rooms, topology nodes, civilians, units, and contacts.
- Add draw-command export from `engine/render` so SwiftUI can render map layers, sprites, markers, overlays, routes, text labels, and selection state.
- Add input mapping helpers for map picking, selected unit/civilian/contact/objective ids, and command preview.
- Add SwiftUI sample wrapper or Xcode target that links the C libraries and opens the 2003 demo without owning rules.
- Add smoke tests for C ABI calls from C and, if feasible, a Swift build smoke.
- Add memory ownership rules and session lifecycle tests.
- Add replay playback API for SwiftUI debugging and AAR review.
- Document frontend responsibilities and forbidden gameplay responsibilities.

Exit criteria:

- SwiftUI can load the demo, step it, issue orders, query snapshots, and draw from C-owned state.
- The frontend boundary is stable enough for UI work to proceed independently.

### Cycles 71-80: Demo Hardening, Performance, And Release Readiness

Goal: finish the engine side of a playable public demo.

Deliverables:

- Profile and optimize map topology lookup, pathfinding, LOS, civilian updates, AI, and render-command generation.
- Add scenario start/end conditions for public demo flow.
- Add save/replay/load support sufficient for bug reports and AAR reproduction.
- Add deterministic replay validation for civilian movement and topology events.
- Add asset and scenario audit tools for missing files, bad paths, unreferenced runtime sprites, unreachable topology nodes, and invalid exits.
- Add full demo test matrix: default, strict, headless, Xcode AI battle, scenario validation, map topology validation, asset validation, replay validation, and AI batch.
- Add debug output for SwiftUI: selected entity dump, path debug, LOS debug, civilian intent debug, and scoring breakdown.
- Tune the 2003 demo for a readable first playable experience.
- Update README, docs, scenario format, build matrix, and asset pipeline docs.
- Freeze the engine demo contract and mark the next plan for UI polish and content expansion.

Exit criteria:

- The C engine can run the complete MOSUL 2003 demo deterministically in headless and SwiftUI-wrapped modes.
- Map interiors, civilians, combatants, AI, scoring, replay, and AAR all interact coherently.
- The demo is stable enough for repeated AI-only battles and manual playtesting.

## Testing Policy

Every cycle batch must keep these checks green:

- `cmake --preset default`
- `cmake --build --preset default`
- `ctest --preset default --output-on-failure`
- `cmake --preset strict`
- `cmake --build --preset strict`
- `ctest --preset strict --output-on-failure`
- direct `mk_ai_battle` seed batch for the main scenario
- Xcode command-line AI battle build when Xcode project files change

Add focused tests before or with each system:

- map product validation tests
- topology/pathfinding tests
- LOS/cover/elevation tests
- civilian AI state-transition tests
- civilian path/risk/scoring tests
- breach/search/roof tests
- replay determinism tests
- SwiftUI C ABI smoke tests

## Definition Of Done

The 80-cycle plan is done when:

- the 2003 demo uses validated visual map layers plus gameplay-grade collision, navigation, topology, and interior/roof data
- civilians move, react, flee, freeze, shelter, follow instructions, create risk, and affect scoring through deterministic C logic
- combatants use interiors, rooftops, cover, breach/search actions, suppression, hidden contacts, and objective logic
- AI-only runs include both tactical sides and civilians
- SwiftUI can wrap the C engine without owning gameplay rules
- all public demo assets referenced by the scenario are validated by C tests
- replay/AAR output explains the outcome well enough to debug bad battles
