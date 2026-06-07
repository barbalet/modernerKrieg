# modernerKrieg Technical Plan

`modernerKrieg` is the portable C engine for the MOSUL public demo. The Mac
and Windows applications should be native platform wrappers over stable C
interfaces; all rules, state, movement, combat, civilian behavior, replay,
validation, and AI-vs-AI simulation stay in C.

The first fully fledged demo target is the 2003 Market / Commercial Streets
battle slice: a 500 m x 500 m urban security fight with U.S. patrol forces,
irregular armed threats, protected civilians, multistorey buildings, roof
access, shops, alleys, courtyards, rubble, search/cache points, and
civilian-risk consequences.

## Current Audit

### Gameplay Area Availability

The repository now has a multi-level JSON-backed gameplay area for the 2003
demo:

- `assets/mosul/manifests/market_commercial_streets_2003_building_levels.json`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/level_01_ground.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/level_02_roofs_and_second_floor.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/level_03_upper_floor.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/level_04_roof_access.png`

The JSON describes:

- schema version 1
- map id `market_commercial_streets_2003`
- 500 m x 500 m world size
- 7000 x 7000 px source/runtime level scale at 14 px/m
- four vertical levels from ground through roof access
- 25 authored feature rectangles for walls, doors, windows, breach holes,
  stairs, roof edges, and movement/LOS blocking
- 8 authored building regions with storey counts and roof-level references

The C asset layer already parses and validates the building-level JSON, checks
runtime PNG references, exposes feature lookup helpers, and validates scenario
references through `asset.building_level_manifest`.

This is an effective starting point. It is not yet a complete gameplay
topology. The parsed manifest currently lives in the asset/scenario validation
path; it is not yet owned by `engine/core` as pathable, searchable, shootable,
AI-usable state.

### Map Availability

The repository also has an effective visual map foundation for the 2003 demo:

- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/01_ground_level.png`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/02_level_2_alpha.png`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/03_level_3_alpha.png`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/04_roof_access_alpha.png`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/assets/map_data/market_commercial_streets_demo_7000/05_multistorey_mask.png`
- `assets/mosul/source/maps/market_commercial_streets_demo_2003/imgs/market_commercial_streets_demo_7000/preview_1400.png`
- `assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png`

The art is good enough as a first visual battle map: roads, market stalls,
vehicles, alleys, courtyards, roof masses, stair-like features, and dense cover
are visible.

What still needs to be made gameplay-grade:

- core-owned gameplay-area state loaded from the JSON manifest
- coordinate transforms between world meters, tactical board space, level PNG
  pixels, topology nodes, and frontend draw commands
- collision and navigation products derived from or linked to the gameplay area
- room/interior graph for shops, courtyards, doorways, windows, and internal
  movement
- vertical connectivity graph for level 2, level 3, rooftops, stairs, ladders,
  and roof access
- per-building state for enterable, locked, breached, searched, occupied,
  roof-controlled, or civilian-shelter status
- scenario and AI logic that use gameplay-area ids rather than coarse terrain
  zones

Conclusion: the new gameplay area is the correct foundation. The next work is
not to replace it, but to wire it through the core until pathing, LOS, civilian
AI, urban combat, replay, and AI-vs-AI battles all depend on it.

### Sprite And Asset Availability

The runtime sprite set contains 1,064 rendered PNGs:

- 640 infantry sprites: 16 combatant roles x 5 body states x 8 facings
- 168 civilian sprites: 7 civilian archetypes x 3 body states x 8 facings
- 64 weapon sprites: 8 weapon types x 8 facings
- 192 vehicle sprites: 8 vehicle types x 3 damage states x 8 facings

The C asset layer validates map, sprite, marker, runtime render, and
building-level manifests. Native frontends should consume validated asset and
render data from the C engine, not rediscover assets independently.

### Current Engine Capability

Already present:

- deterministic C core with game state, fixed-step simulation, snapshots,
  scenario loading, scoring, AAR text, replay/event tooling, and headless tools
- units, soldiers, weapons, wounds, suppression, morale states, hidden contacts,
  suspected/false contacts, objectives, civilians, civilian risk, and basic AI
  order emission
- renderer-independent board projection and tactical overlays
- headless AI-vs-AI tools and CTest balance smoke coverage
- CMake presets for default, headless, and strict warning-as-error builds
- Xcode command-line AI battle project
- C asset parsing for the JSON building-level gameplay area

Important gaps:

- the building-level JSON is validated as an asset, but not yet loaded into
  durable core gameplay state
- civilian records are passive risk/stress entities, not full movement agents
- no civilian AI route planning, panic, compliance, crowd behavior, shelter
  seeking, evacuation, or instructions model
- unit movement is still direct target movement, not topology/path movement
- LOS and cover still rely on coarse terrain instead of JSON-authored walls,
  windows, doors, floors, roofs, and elevations
- breach/search/cache/roof systems are mostly affordances, not full rules
- SwiftUI/native frontend boundary is not yet a stable C ABI
- the demo scenario still needs a larger civilian population and richer opfor
  presence to make the new asset set meaningful

## Product Goal

After 100 development cycles, the repository should contain a fully fledged C
demo engine for the MOSUL 2003 battle:

- one deterministic playable scenario loaded entirely from data
- multi-level JSON gameplay area loaded into core simulation state
- collision, navigation, interior, roof, LOS, and semantic-zone data connected
  to that gameplay area
- unit-scale commands with soldier-level consequences
- moving civilian population with deterministic AI
- opposing-force AI, player-side tactical AI for AI-only tests, and civilian AI
  running together
- combat, suppression, concealment, breach/search, rooftop, and civilian-risk
  rules that work on the map topology
- stable C API for native Mac and Windows wrappers
- headless, CTest, and Xcode AI-only battle checks that catch stalls, bad
  balance, broken assets, invalid maps, and broken replays

## Architecture Rules

- `engine/core` owns deterministic rules, state, orders, movement, LOS, combat,
  civilians, scoring, replay, and validation.
- `engine/assets` owns manifest parsing and validation for map, sprite, marker,
  render, building-level, collision, navigation, and topology products.
- `engine/render` owns renderer-independent projection, sprite/marker
  selection, level draw ordering, overlays, and frontend draw-command
  preparation.
- `engine/ai` owns player, opfor, and civilian policies that emit ordinary C
  orders or civilian intent updates.
- `game/mosul` owns scenario data, Mosul-specific templates, map topology data,
  and demo tuning.
- Native Mac and Windows wrappers own windowing, input, image upload/cache,
  panels, platform packaging, and presentation only.
- No gameplay rule should exist only in Swift, Objective-C, C++, C#, or a
  platform frontend.

## Gameplay Area Integration Direction

The JSON building-level manifest should become the authoritative source for the
demo area's vertical art stack and first pass of physical affordances. The core
should use it through a gameplay-area layer rather than reaching back into raw
asset parsing during every simulation query.

Required integration:

- add a core gameplay-area type with immutable loaded level, feature, region,
  coordinate-transform, and semantic metadata
- translate pixel rectangles into world-space blockers, openings, portals, and
  elevation records
- add explicit topology records for rooms, shop interiors, alleys, courtyards,
  roofs, stairs, ladders, exits, shelters, caches, objectives, and danger zones
- keep authored ids stable so scenarios, replays, AI debug, and native
  frontends can refer to the same area/node/feature ids
- derive or attach tactical collision and navigation products from this area
- make LOS, cover, movement, civilian routes, breach/search, rooftop access,
  objective control, replay, and AI-vs-AI use gameplay-area queries
- preserve deterministic behavior: no platform image processing or frontend-only
  geometry decisions during simulation

Current JSON schema version 1 is enough for level art, first blockers/openings,
stair markers, and building regions. The plan assumes schema version 2 or a
companion topology JSON will be added for rooms, portals, semantic zones, and
dynamic gameplay state templates.

## Civilian Movement Model

Civilian AI should be deterministic, cheap, and legible.

State per civilian:

- archetype, group id, protected/non-protected flag
- current state: sheltering, fleeing, frozen, following instructions, wounded,
  dead
- current level/area/node, map position, facing, speed, path, destination
- stress, risk, confidence in nearby friendly force, compliance, panic threshold
- last threat source, last safe node, assigned shelter/exit, and movement
  cooldown

Inputs:

- nearby gunfire, explosions/future hooks, armed movement, visible casualties,
  fire lanes, objectives, blocked paths
- player instructions, safe corridors, breach/search events, building
  entry/exit, roof/stair access
- civilian group cohesion, family separation, wounded civilians, and crowd
  density

Actions:

- stay sheltered, freeze, flee, move to shelter, move to exit, follow
  instructions, disperse from fire lane, avoid armed units, assist or cluster
  near family, or become casualty/incapacitated

Implementation approach:

- build civilian navigation from the gameplay-area topology
- update civilian intent at a lower cadence than tactical unit ticks
- use deterministic weighted choices from the scenario seed
- prefer safe graph routes over straight-line motion
- expose civilian intent, path, stress, risk, and area/node ids in snapshots and
  replay events
- add CTests for every state transition and every path/risk rule

## Development Cycle Ledger

- Ledger baseline date: 2026-06-07.
- Previous planned budget: 80 cycles.
- Added budget from gameplay-area integration rebaseline: 20 cycles.
- Planned cycle budget: 100 cycles.
- Planned cycle shape: 10 milestones x 10 cycles each.
- Completed cycles in this ledger: 91.
- Current cycle: 91.
- Remaining planned cycles: 9.
- Next cycle batch: cycles 92-100, Demo Hardening, Performance, And Release Readiness.

Prior implementation work is captured in the current audit and is not counted
against this rebaselined ledger. When a development batch is completed,
increment `Completed cycles in this ledger`, advance `Current cycle`, reduce
`Remaining planned cycles`, update `Next cycle batch`, and add a log row.

| Date | Cycles Completed | Current Cycle | Remaining Cycles | Focus | Notes |
| --- | ---: | ---: | ---: | --- | --- |
| 2026-06-07 | 0 | 0 | 100 | Rebaseline | Updated from 80 to 100 cycles after the multi-level JSON gameplay area landed on main. |
| 2026-06-07 | 10 | 10 | 90 | Gameplay Area Adoption And Validation | Added core gameplay-area state, Mosul JSON-to-core handoff, world/pixel and blocker queries, replay/debug exposure, and validation tests. |
| 2026-06-07 | 20 | 20 | 80 | Topology Authoring Model | Added C-validated topology JSON, core topology state and queries, Mosul scenario handoff, debug/replay exposure, and topology validation tests. |
| 2026-06-07 | 30 | 30 | 70 | Collision, Navigation, LOS, And Cover Products | Added derived tactical point queries, sampled gameplay-area LOS, navigation and cover lookups, game LOS integration, headless tactical-product exposure, and Mosul/core tests. |
| 2026-06-07 | 40 | 40 | 60 | Topology-Aware Movement | Added deterministic topology route planning, compact per-unit route following, vertical level transitions, route failure reporting, board-view route waypoint overlays, AI withdraw route requests, replay route fields, and heap-backed Mosul scenario validation. |
| 2026-06-07 | 41 | 41 | 59 | CI Automation Guardrail | Added a per-commit GitHub Actions C-engine workflow, CI runner script, timestamped failure logs, cross-platform default/strict tests, AI-only smoke checks, and sanitizer coverage on macOS/Linux. |
| 2026-06-07 | 46 | 46 | 54 | Scenario Population Data Foundation | Added C-level spawn zones, unit templates, civilian archetypes/groups, richer Market 2003 civilians and hidden opfor, topology-linked population validation, replay exposure, docs, and tests. |
| 2026-06-07 | 61 | 61 | 39 | Scenario Variants, Civilian AI, And Search Hook | Added seven compact Market 2003 scenario variants, C-level civilian intent/destination/path state, deterministic civilian AI movement, civilian instruction API, replay route exposure, search/cache reveal hooks, docs, and smoke tests. |
| 2026-06-07 | 76 | 76 | 24 | Urban Interactions And AI Guardrails | Completed cycles 62-70 with persistent search state, breachable portal state, interaction scoring, rooftop/elevation fire modifiers, replay/contact support, and core tests; completed cycles 71-76 with player AI search/breach behavior, broader AI-battle stall signatures, and cache-search AI battle CTest coverage. |
| 2026-06-07 | 91 | 91 | 9 | Tactical AI, Demo API, And Hardening Start | Completed cycles 77-80 with objective-aware AI, hidden topology defender overwatch, updated balance expectations, and AI tests; completed cycles 81-90 with the portable `modernerKriegDemo` C session API, draw-command export, screen picking, selected move helpers, lifecycle/performance counters, and C demo session tests; completed cycle 91 by adding demo counters and the expanded default/strict CTest matrix entry point. |

## Cycle Delta From Previous Plan

The previous plan assumed the map still needed gameplay-grade level data. The
repo now has that first gameplay-area asset, so the plan changes shape:

- cycles 1-10 now adopt, validate, and expose the JSON gameplay area to the
  core instead of inventing the level stack from scratch
- cycles 11-20 add the missing topology authoring layer that schema version 1
  does not yet provide
- cycles 81-90 are now reserved for full C demo integration and native wrapper
  contract work
- cycles 91-100 carry final hardening, performance, AI-vs-AI balance, replay,
  and release readiness

Net change: add 20 cycles, for 100 total. No further cycles are required in the
current estimate. If every individual building interior must be hand-authored
as room-by-room geometry rather than tactical areas, budget an additional 10 to
20 content/topology cycles later.

## Planned Cycle Batches

### Cycles 1-10: Gameplay Area Adoption And Validation

Status: completed on 2026-06-07.

Goal: make the new multi-level JSON gameplay area a first-class C input for the
demo.

Deliverables:

- Add a core-facing gameplay-area structure or adapter loaded from the validated
  building-level manifest.
- Store level ids, elevations, PNG paths, feature ids, region ids, and
  world/pixel scale in scenario-loaded state.
- Add coordinate transforms for world meters to level pixels and level pixels to
  world meters.
- Add area query helpers for blockers, openings, stairs, roof access, building
  membership, and feature lookup by world position.
- Validate that `asset.building_level_manifest` is required for the main 2003
  scenario and optional only for tiny smoke fixtures.
- Add tests for missing JSON, missing level PNG, bad dimensions, wrong map id,
  invalid level id, invalid feature rectangle, and bad region roof reference.
- Add tests for pixel/world conversions at map corners, feature centers, doors,
  windows, stairs, and out-of-bounds positions.
- Expose gameplay-area metadata in debug snapshots and headless transcripts.
- Update docs so this JSON is described as the current gameplay-area foundation,
  not merely an art manifest.
- Keep default, strict, asset, scenario, and AI-battle smoke checks green.

Exit criteria:

- The 2003 scenario loads gameplay-area data into core-reachable state.
- C tests prove blockers/openings can be queried in world coordinates.
- Headless output can name the gameplay-area manifest and current level stack.

### Cycles 11-20: Topology Authoring Model

Status: completed on 2026-06-07.

Goal: fill the gap between rectangle features and a real urban tactical graph.

Deliverables:

- Add schema version 2 or a companion topology JSON for rooms, shops, alleys,
  courtyards, roofs, stairs, ladders, exits, shelters, caches, objectives, and
  danger zones.
- Define stable ids for topology nodes, portals, vertical connectors, semantic
  zones, and dynamic area states.
- Link topology nodes to level ids, building regions, world bounds, feature
  blockers, and render/debug labels.
- Model portals: door, window, breach hole, archway, stair, ladder, roof edge,
  street crossing, and rubble passage.
- Model dynamic portal states: open, closed, locked, blocked, breached, searched,
  compromised, and unsafe.
- Model semantic zones: civilian shelter, evacuation exit, market crowd, cache,
  overwatch roof, search objective, restricted fire lane, and danger area.
- Add validation for duplicate ids, missing levels, unreachable nodes, one-way
  portal errors, orphan buildings, invalid roofs, and impossible vertical links.
- Add an authored first topology pass for the Market / Commercial Streets demo.
- Add debug dump tooling to print topology coverage and unreachable graph
  fragments.
- Update `docs/scenario_format.md` and `docs/asset_pipeline.md`.

Exit criteria:

- The demo has a C-loadable topology graph tied to the JSON gameplay area.
- Tests reject broken or incomplete topology before simulation starts.
- Every major demo building region has an enterable or deliberately blocked
  tactical interpretation.

### Cycles 21-30: Collision, Navigation, LOS, And Cover Products

Status: completed on 2026-06-07.

Goal: turn the gameplay area and topology into fast tactical queries.

Deliverables:

- Generate or hand-author tactical collision products from features, portals,
  building regions, and topology nodes.
- Generate or hand-author navigation products for units and civilians, including
  different costs for street, alley, interior, rubble, roof, stair, and crowd.
- Add movement blockers for walls, windows, roof edges, vehicles, rubble, and
  closed/blocked portals.
- Add LOS occluders for walls, closed doors, interior walls, parapets,
  buildings, vehicles, and future smoke hooks.
- Add cover records for walls, corners, windows, stalls, vehicles, rubble,
  interior depth, parapets, and elevation.
- Add elevation-aware LOS across level 1 through level 4.
- Add fast query caches for blocker lookup, LOS sampling, cover lookup, and
  level membership.
- Add tests for movement blocking, LOS blocking, window LOS without movement,
  door/opening behavior, stairs, roofs, and parapets.
- Add render/debug overlays for collision, navigation costs, LOS rays, visible
  cells, cover values, and current level.
- Add asset audit checks for missing or stale generated products.

Exit criteria:

- Core code can answer movement, LOS, and cover queries from gameplay-area data.
- Tests prove walls, doors, windows, breaches, stairs, and roofs behave
  differently.
- Query performance is predictable enough for AI-vs-AI runs.

### Cycles 31-40: Topology-Aware Movement

Status: completed on 2026-06-07.

Goal: replace straight-line movement assumptions with deterministic tactical
pathing.

Deliverables:

- Add deterministic pathfinding over navigation/topology data.
- Add per-agent path requests for units, soldiers-as-state, civilians, and AI
  previews.
- Add path following with progress, facing, speed, cancellation, reroute, and
  stuck detection.
- Add vertical movement through stairs, ladders, roof access, and interior
  connectors.
- Add route validation for unpathable starts, blocked destinations, locked
  portals, and dynamic hazards.
- Add movement effects from suppression, wounds, stress, stance, burden, crowd
  density, and civilian proximity.
- Add replay events for path assignment, path step, vertical movement, path
  failure, reroute, and stuck timeout.
- Update AI order emission to request paths instead of direct target motion.
- Update board-view overlays to project current route, planned route, blocked
  reason, and level transition.
- Add deterministic tests for route choice, reroute, vertical movement, blocked
  movement, and stall detection.

Exit criteria:

- Units can move through streets, alleys, interiors, and rooftops by path.
- Headless smoke tests detect stalled movement.
- AI-only battle movement no longer depends on direct-line movement.

### Cycles 41-50: Scenario Population And Data Expansion

Status: completed on 2026-06-07. Cycle 41 completed for CI automation at user
request. Cycles 42-46 completed for population schema, richer Market 2003 data,
replay exposure, and validation. Cycles 47-50 completed with compact scenario
variants and data/content convention close-out.

Goal: make the 2003 battle scenario data-rich enough for a real demo.

Deliverables:

- Expand `.mkscenario` or companion data for force templates, civilian groups,
  spawn zones, AI plans, and topology references.
- Add reusable templates for soldiers, squads, weapons, civilian archetypes,
  vehicles, hidden threat groups, and equipment profiles.
- Replace the single-civilian placeholder with a small market population using
  the civilian runtime sprites.
- Add civilian groups: vendors, shoppers, families, bystanders, wounded or
  incapacitated civilians, and sheltering civilians.
- Add opfor groups: hidden cell, rooftop watcher, armed looter, machine-gunner,
  RPG threat, cache guard, and mobile scout.
- Add player force templates for patrol, support weapons, breacher, medic,
  marksman, vehicle crew, and command element.
- Add pathable spawn validation against gameplay-area topology.
- Add validation for template ids, asset ids, faction references, controller
  references, impossible loadouts, and topology node references.
- Add scenario variants for smoke tests: empty map, interior contact, civilian
  panic, rooftop threat, cache search, evacuation, and blocked path.
- Update docs for scenario format v2 and Mosul content conventions.

Exit criteria:

- Main 2003 scenario loads a credible population and opposing force from data.
- Scenario validation rejects unpathable starts, missing sprite ids, invalid
  civilian groups, bad topology ids, and impossible force templates.

### Cycles 51-60: Civilian Movement And Civilian AI

Status: completed on 2026-06-07.

Goal: turn civilians into deterministic moving agents whose behavior affects
tactics and scoring.

Deliverables:

- Extend civilian state with profile, group id, level/topology node,
  destination, path, speed, compliance, panic, and intent fields.
- Add a civilian intent update step separate from tactical unit orders.
- Implement shelter, freeze, flee, follow-instructions, disperse, assist-group,
  wounded, and dead states.
- Compute threat maps from gunfire, visible armed units, casualties, hostile
  proximity, blocked routes, and future explosion/smoke hooks.
- Add civilian pathfinding that avoids fire lanes, armed units, known threat
  zones, exposed roads, blocked interiors, and overcrowded exits.
- Add group cohesion behavior so civilians do not scatter randomly unless panic
  breaks cohesion.
- Add player instructions as C orders/events that civilians may follow based on
  stress, distance, trust, and current danger.
- Add shelter and evacuation scoring tied to topology ids.
- Add replay/debug transcript lines for civilian state changes, route choices,
  stress changes, and risk causes.
- Add CTests for every civilian state transition, path choice, risk change, and
  scoring effect.

Exit criteria:

- AI-only battle runs include moving civilians.
- Civilian movement can create meaningful tactical constraints without
  nondeterministic behavior.
- Civilian harm/risk is explainable in replay and AAR output.

### Cycles 61-70: Urban Combat, Breach, Search, And Rooftops

Status: completed on 2026-06-07. Cycle 61 added first-pass C-level
semantic-zone and terrain search hooks, cache-found outcomes, hidden threat
reveals, replay contact records, and tests. Cycles 62-70 completed persistent
search state, portal breach results, breached routeability, interaction scoring,
booby-trap/intelligence outcomes, rooftop/elevation fire modifiers, replay
contact names, and core search/breach tests.

Goal: make the modern urban systems mechanically real.

Deliverables:

- Implement breach/search/cache actions on topology nodes and semantic zones.
- Add door/window/portal states: closed, open, locked, breached, blocked,
  searched, compromised, and unsafe.
- Add hidden threat reveal rules for interiors, rooftops, caches, proximity, and
  line-of-sight.
- Add rooftop/elevation combat modifiers, exposure, vertical LOS, and roof
  access restrictions.
- Add suppression and casualty effects that interact with rooms, cover, windows,
  civilians, and elevation.
- Add non-lethal restraint and hold-fire pressure when civilians cross fire
  lanes.
- Add search outcomes: no threat, cache found, enemy revealed, booby trap/future
  hook, civilian found, or intelligence clue.
- Add opfor displacement and ambush behavior using rooms, roofs, caches, and
  retreat routes.
- Add after-action scoring for breach/search success, civilian protection,
  cache handling, force preservation, objective control, and time.
- Add tests for every interaction, including invalid actions and replay
  determinism.

Exit criteria:

- The demo can produce contact inside buildings, on rooftops, and around
  civilians.
- Search/breach choices affect objectives and AAR scoring.

### Cycles 71-80: Tactical AI And AI-Vs-AI Balance

Status: completed on 2026-06-07. Cycles 71-76 completed player-side AI
searching nearby false-contact terrain, searching nearby semantic cache/search
zones, breaching nearby closed/locked portals, expanded AI battle progress
signatures for civilians/search/portal state, and CTest coverage for cache
search AI battles. Cycles 77-80 completed objective-aware player/opfor movement,
hidden topology defender overwatch, updated AI balance expectations, and focused
AI tests.

Goal: make AI-only runs meaningful enough to expose engine problems early.

Deliverables:

- Expand player-side tactical AI for movement, cover use, investigate/search,
  breach, restraint, casualty response, civilian instruction, and objective
  control.
- Expand opfor AI for defend, ambush, displace, hide, threaten civilians, avoid
  suicidal exposure, and exploit interiors/roofs.
- Add civilian AI into the same fixed-step autoplay loop.
- Add AI planners that reason over topology nodes, routes, known contacts,
  suspected contacts, civilian risk, cover, and objectives.
- Add batch AI battle scenarios with deterministic seeds and expected outcome
  envelopes.
- Add separate stall detection for units, civilians, path requests, combat, and
  scenario outcome.
- Add coverage for path failures, endless civilian panic loops, no-contact
  battles, over-lethal fire, score collapse, and AI oscillation.
- Add replay diff tooling for AI-only regressions.
- Add performance counters for pathfinding, LOS, civilian updates, AI decisions,
  and replay output.
- Keep the Xcode command-line project aligned with the CMake AI battle runner.

Exit criteria:

- AI-only runs exercise player, opfor, and civilian behavior together.
- Batch tests catch stalls, impossible paths, broken civilian behavior, bad
  scoring, and broken topology.

### Cycles 81-90: C Demo Integration And Native Wrapper Contract

Status: completed on 2026-06-07 with the portable `modernerKriegDemo` C session
API, opaque session lifecycle, default/specific scenario loading, AI-only fixed
stepping, summary/snapshot queries, board fitting, draw-command export, screen
picking, selection, selected-unit move helpers, performance counters, CMake
library wiring, native-wrapper docs, and platform-free demo session tests.

Goal: expose the complete C demo cleanly to native Mac and Windows frontends.

Deliverables:

- Define public C API headers for creating/destroying a session, loading
  scenarios, stepping fixed ticks, issuing orders, querying snapshots, and
  reading render commands.
- Add C ABI-safe structs for snapshot summaries, units, soldiers, civilians,
  contacts, objectives, topology nodes, routes, sprites, overlays, and AAR text.
- Add stable ids for map assets, level ids, sprite render entries, markers,
  topology nodes, civilians, units, contacts, and replay events.
- Add draw-command export from `engine/render` for map overview, level stack,
  sprites, markers, overlays, routes, LOS, civilian debug, and selection state.
- Add input mapping helpers for map picking, selected unit/civilian/contact/
  objective/topology ids, and command preview.
- Add C-level frontend session tests that simulate native UI usage without
  depending on SwiftUI or Windows UI code.
- Add replay playback API for frontend debugging and AAR review.
- Add memory ownership rules and session lifecycle tests.
- Add a minimal Mac wrapper smoke target only if it remains a wrapper over C
  state and does not move gameplay rules out of the engine.
- Document frontend responsibilities and forbidden gameplay responsibilities.

Exit criteria:

- Native frontends can load the demo, step it, issue orders, query snapshots,
  and draw from C-owned state.
- The C engine remains runnable and testable without any platform frontend.

### Cycles 91-100: Demo Hardening, Performance, And Release Readiness

Status: in progress. Cycle 91 completed on 2026-06-07 with demo session
performance counters, CTest coverage for the native-wrapper contract, updated
default replay/balance expectations after objective-aware AI, and documentation
for frontend responsibilities.

Goal: finish the C engine side of a playable public demo.

Deliverables:

- Profile and optimize gameplay-area lookup, topology pathfinding, LOS,
  civilian updates, AI, replay, and render-command generation.
- Add scenario start/end conditions for public demo flow.
- Add save/replay/load support sufficient for bug reports and AAR reproduction.
- Add deterministic replay validation for civilian movement, topology events,
  breach/search, AI orders, and scoring.
- Add asset and scenario audit tools for missing files, bad paths, unreferenced
  runtime sprites, unreachable topology nodes, invalid exits, and stale level
  products.
- Add full demo test matrix: default, strict, headless, Xcode AI battle,
  scenario validation, gameplay-area validation, topology validation, asset
  validation, replay validation, and AI battle batch.
- Add debug output for native frontends: selected entity dump, path debug, LOS
  debug, civilian intent debug, topology node debug, and scoring breakdown.
- Tune the 2003 demo for a readable first playable experience.
- Update README, docs, scenario format, build matrix, asset pipeline, and
  milestone progress notes.
- Freeze the engine demo contract and mark the next plan for native UI polish
  and content expansion.

Exit criteria:

- The C engine can run the complete MOSUL 2003 demo deterministically in
  headless and native-wrapper modes.
- Map interiors, civilians, combatants, AI, scoring, replay, and AAR all
  interact coherently.
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

- gameplay-area JSON validation tests
- world/pixel coordinate transform tests
- topology/pathfinding tests
- LOS/cover/elevation tests
- civilian AI state-transition tests
- civilian path/risk/scoring tests
- breach/search/roof tests
- replay determinism tests
- C ABI/session smoke tests

AI-only battle testing is mandatory. The demo is not considered C-playable until
both tactical sides and civilians can run through deterministic AI-vs-AI battles
using the same gameplay-area data as manual play.

## Definition Of Done

The 100-cycle plan is done when:

- the 2003 demo loads the multi-level JSON gameplay area into core C simulation
  state
- movement, LOS, cover, interiors, roofs, breach/search, civilian behavior, AI,
  and scoring use gameplay-area/topology queries
- civilians move, react, flee, freeze, shelter, follow instructions, create
  risk, and affect scoring through deterministic C logic
- combatants use interiors, rooftops, cover, breach/search actions,
  suppression, hidden contacts, and objective logic
- AI-only runs include both tactical sides and civilians
- native Mac and Windows wrappers can use the C engine without owning gameplay
  rules
- all public demo assets referenced by the scenario are validated by C tests
- replay/AAR output explains the outcome well enough to debug bad battles
