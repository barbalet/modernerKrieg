# Scenario Format

MOSUL scenario data uses plain UTF-8 key/value text with one `key=value` pair per line. Blank lines and `#` comments are ignored.

The first supported format version is:

```text
format=modernerKrieg.scenario.v1
```

## Paths

Scenario files live in `game/mosul/scenarios/`. The default demo scenario is:

```text
game/mosul/scenarios/market_commercial_streets_2003.mkscenario
```

Asset references inside a scenario must be repo-relative paths under `assets/mosul/`. The loader validates referenced map and sprite manifests before the scenario reaches the core. The optional `asset.building_level_manifest` reference validates the JSON floor stack, runtime PNG paths, wall/opening records, building regions, and map dimensions. The optional `asset.topology_manifest` reference validates the tactical topology graph tied to that building-level manifest; Market / Commercial Streets scenarios require both JSON references.

## Sections

The current loader supports:

- scenario metadata: `name`, `seed`, and optional `briefing`
- score metadata: optional `score.success_threshold`, `score.partial_threshold`, and score weight fields
- after-action copy: optional `after_action.success`, `after_action.partial`, and `after_action.failure`
- asset references: `asset.map_manifest`, `asset.sprite_manifest`, optional `asset.building_level_manifest`, and optional `asset.topology_manifest`
- map metadata and tile grid
- `tile_range.*` records for compact tile overrides
- `terrain.*` records for line-of-sight and cover zones
- `controller.*`, `faction.*`, and `force.*` records
- `spawn_zone.*`, `unit_template.*`, `civilian_archetype.*`, and `civilian_group.*` population records
- `objective.*` records
- `weapon.*` records
- `civilian.*` records with optional population, sprite, level, topology, intent, destination, and speed references
- `unit.*` records with optional `hidden`, `revealed`, `concealment`, population, level, and topology fields
- nested `unit.N.soldier.M.*` records

References use zero-based indexes within their section. For example, `force.0.faction_index=0` references `faction.0`, while `unit.1.soldier.0.weapon_index=1` references `weapon.1`.

## Validation

The loader rejects:

- missing or unsupported `format`
- missing referenced map or sprite manifests
- missing required Market / Commercial Streets building-level or topology manifests
- unsafe asset paths
- unknown enum values
- invalid controller, faction, force, weapon, or soldier references
- invalid hidden-contact fields, such as negative concealment
- invalid population template ids, civilian group ids, spawn zone ids, sprite ids, or topology node ids
- population starts outside their spawn zone, outside the authored topology node, or assigned to the wrong side
- invalid score thresholds, such as a success threshold below the partial threshold
- invalid score weights, such as negative objective, risk, casualty, or time weights
- out-of-bounds map tiles, terrain, civilians, objectives, and units
- invalid topology ids, levels, building regions, portals, vertical links, one-way portals, unreachable enterable nodes, or semantic zones
- scenarios that the portable C core refuses to load

CTest covers the default 2003 data file, fixture parity, gameplay-area topology handoff, objective labels, briefing/after-action text, score thresholds and weights, hidden-contact fields, interaction terrain zones, missing asset references, missing Market topology, invalid force references, invalid threshold ordering, impossible objective bounds, the compact AI-only control smoke scenario, and a contested civilian-risk smoke scenario.

Population CTests also cover bad civilian sprite ids, bad population topology ids, unit templates whose default spawn zone belongs to the wrong side, and compact scenario variants for empty-map, interior contact, civilian panic, rooftop threat, cache search, evacuation, and blocked-path coverage.

## Scenario Population

The 2003 demo scenario now carries population metadata as first-class C data. These records are optional for tiny smoke fixtures, but the main Mosul scenario should use them for all civilians, hidden threat groups, and player-force starts.

Spawn zones define pathable placement areas:

```text
spawn_zone.1.id=market_stalls_crowd
spawn_zone.1.name=Market Stalls Crowd
spawn_zone.1.kind=crowd
spawn_zone.1.side=civilian
spawn_zone.1.level_id=level_01_ground
spawn_zone.1.topology_node_id=market_stalls_ground
spawn_zone.1.bounds=176,164,86,48
spawn_zone.1.capacity=8
spawn_zone.1.active=true
```

Unit templates give reusable scenario roles and default starts:

```text
unit_template.3.id=rooftop_watcher
unit_template.3.name=Rooftop Watcher
unit_template.3.role=overwatch
unit_template.3.side=opfor
unit_template.3.training=regular
unit_template.3.default_spawn_zone_id=hotel_roof_threat
unit_template.3.expected_soldiers=1
```

Civilian archetypes and groups bind runtime sprites, stress/risk defaults, and topology-aware population clusters:

```text
civilian_archetype.0.id=vendor_adult
civilian_archetype.0.sprite_id=civilian_adult_128_n
civilian_archetype.0.baseline_stress=1
civilian_archetype.0.baseline_risk=0
civilian_archetype.0.compliance=60
civilian_archetype.0.protected_noncombatant=true

civilian_group.0.id=market_vendors
civilian_group.0.archetype_id=vendor_adult
civilian_group.0.spawn_zone_id=market_stalls_crowd
civilian_group.0.level_id=level_01_ground
civilian_group.0.topology_node_id=market_stalls_ground
civilian_group.0.expected_count=2
```

Units and civilians may then reference those records:

```text
civilian.1.archetype_id=vendor_adult
civilian.1.group_id=market_vendors
civilian.1.spawn_zone_id=market_stalls_crowd
civilian.1.level_id=level_01_ground
civilian.1.topology_node_id=market_stalls_ground
civilian.1.intent=evacuate
civilian.1.destination=45,170
civilian.1.speed_m_per_tick=2.25

unit.3.template_id=rooftop_watcher
unit.3.spawn_zone_id=hotel_roof_threat
unit.3.level_id=level_04_roof_access
unit.3.topology_node_id=hotel_roof_access
```

The loader validates that referenced sprite runtime ids exist in the sprite manifest, referenced topology nodes exist on the declared level, positions fit the referenced spawn zone and topology node, and template spawn zones are neutral or owned by the template side.

Civilian intent is optional. Supported values are `none`, `shelter`, `flee`, `evacuate`, `follow_instructions`, `freeze`, and `assist_group`. A civilian with `destination` starts with `has_destination` set in core state; if the topology can route between the current node and destination level, the route is assigned during scenario load. If no route is available, the core keeps a deterministic straight-line fallback and records the route failure reason.

Traffic vehicles are separate from the base map art and can carry units. Cars
and buses use `boarding_mode=inside`; motorcycles use `boarding_mode=on`. Moving
demo records define a start position, destination, speed, facing, seat capacity,
active state, and movement blocking state so the C core can treat them as
scenario entities rather than map decoration. Parked/static records omit
`destination`; they render and block movement at their start position until a
runtime command or script assigns a destination.

```text
traffic_vehicle.0.id=north_market_bus
traffic_vehicle.0.name=North Market Bus
traffic_vehicle.0.kind=bus
traffic_vehicle.0.sprite_id=traffic_city_bus_intact_north
traffic_vehicle.0.level_id=level_01_ground
traffic_vehicle.0.position=246,382
traffic_vehicle.0.destination=246,230
traffic_vehicle.0.destination_level_id=level_01_ground
traffic_vehicle.0.speed_m_per_tick=5.5
traffic_vehicle.0.facing_degrees=270
traffic_vehicle.0.seat_capacity=24
traffic_vehicle.0.occupied_seats=0
traffic_vehicle.0.boarding_mode=inside
traffic_vehicle.0.active=true
traffic_vehicle.0.blocks_movement=true
```

## Gameplay Area And Topology

The Market / Commercial Streets scenario uses two JSON manifests in addition to the compact map and sprite manifests:

```text
asset.building_level_manifest=assets/mosul/manifests/market_commercial_streets_2003_building_levels.json
asset.topology_manifest=assets/mosul/manifests/market_commercial_streets_2003_topology.json
```

The building-level manifest owns levels, wall/opening rectangles, and building regions. The topology manifest owns tactical nodes, portals between nodes, vertical connectors, portal state, and semantic zones such as civilian shelters, evacuation exits, caches, overwatch roofs, search objectives, restricted fire lanes, and danger areas. Both are loaded into the C core gameplay-area state before scenario validation finishes.

The core derives tactical products from those two manifests at query time:
movement blocking, sampled LOS blocking, navigation cost, cover value, portal
state, topology node context, and semantic-zone context. Scenario files should
therefore reference stable gameplay-area ids instead of duplicating collision or
cover rules in scenario-specific keys.

Movement orders can now request deterministic topology routes through the C
core. When both the current unit position and destination resolve to enterable
topology nodes, units follow compact route waypoints through portals and update
their level when a vertical connector is reached. Existing straight-line orders
remain a fallback for smoke fixtures or targets outside the authored topology.

Civilian AI also consumes the topology. The fixed-step loop updates civilian
risk first, then civilian intent and movement. Civilians can seek authored
`civilian_shelter` zones, evacuate toward `evacuation_exit` zones, follow a
C-level instruction destination, or freeze when low compliance and danger make
movement unreliable. Replay civilian events expose intent, destination, route
state, route failure reason, stress, and risk so CI artifacts can explain
movement problems.

## Interaction Terrain

Terrain zones can carry first-pass interaction affordances before final interaction art and rules are complete. Frontends project these zones through the marker manifest:

- `breach_point`: breach/search affordance at a door, shopfront, gate, or wall segment.
- `rooftop`: rooftop or stair access affordance.
- `suspected_ied`: possible cache/search hazard affordance.

Example:

```text
terrain.3.name=Shopfront Breach Point
terrain.3.kind=breach_point
terrain.3.bounds=286,214,18,24
terrain.3.cover=2
terrain.3.movement_cost=3
terrain.3.blocks_line_of_sight=false
```

The C core exposes persistent search and breach hooks for semantic zones,
terrain zones, and topology portals. Searchable semantic zones and terrain keep
`searched`, `searched_tick`, and `last_search_outcome` runtime state so
AI-vs-AI runs, scoring, and replay validation can see what happened after the
caller returns.

Supported search outcomes are clear/no threat, cache found, hidden threat
revealed, protected civilian found, booby-trap/future hazard hook, and
intelligence clue. `cache` semantic zones and `suspected_ied` terrain resolve
to cache-found when no hidden threat is revealed first; `danger_area` semantic
zones resolve to the booby-trap hook; `search_objective` zones and rubble or
breach terrain can resolve to intelligence. Search actions emit deterministic
`search` contact records for headless replay review.

Topology portals can be breached at runtime. Closed or locked doors, gates, and
shutters can transition to `breached`, which makes the portal routeable by the
topology pathfinder. Blocked, unsafe, window, and roof-edge portals remain
unsafe for breach. Breach actions emit deterministic `breach` contact records.

The score model now includes `interaction_points` for searched terrain,
searched semantic zones, and breached portals. Replay score events include the
same `interaction` field so CI artifacts expose whether an AI-only battle is
actually performing urban interaction work.

## Briefing And Outcome Text

Scenario text fields are intentionally short and single-line so they can be carried through the portable C structs, headless transcripts, and lightweight app shells without a document parser.

```text
briefing=Secure the market junction, identify armed threats, and keep protected civilians out of the line of fire.
score.success_threshold=450
score.partial_threshold=150
score.objective_weight=100
score.civilian_risk_weight=10
score.player_casualty_weight=50
score.civilian_casualty_weight=100
score.time_weight=1
after_action.success=The patrol secured the junction while preserving civilian safety and force cohesion.
after_action.partial=The patrol made progress, but civilian risk, casualties, or delay kept the result limited.
after_action.failure=The patrol did not secure the market junction within acceptable risk.
```

If thresholds or weights are omitted, the loader applies the core defaults. If after-action text is omitted, the deterministic score summary is still available.

## Objectives

Objectives require a formal `name` and may include a short `label` for HUDs, replay files, and overlays.

```text
objective.0.name=Secure Market Junction
objective.0.label=Market Junction
objective.0.kind=control
objective.0.position=250,285
objective.0.radius_m=28
objective.0.value=5
```
