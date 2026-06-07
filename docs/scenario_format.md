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
- `objective.*` records
- `weapon.*` records
- `civilian.*` records
- `unit.*` records with optional `hidden`, `revealed`, and `concealment` fields
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
- invalid score thresholds, such as a success threshold below the partial threshold
- invalid score weights, such as negative objective, risk, casualty, or time weights
- out-of-bounds map tiles, terrain, civilians, objectives, and units
- invalid topology ids, levels, building regions, portals, vertical links, one-way portals, unreachable enterable nodes, or semantic zones
- scenarios that the portable C core refuses to load

CTest covers the default 2003 data file, fixture parity, gameplay-area topology handoff, objective labels, briefing/after-action text, score thresholds and weights, hidden-contact fields, interaction terrain zones, missing asset references, missing Market topology, invalid force references, invalid threshold ordering, impossible objective bounds, the compact AI-only control smoke scenario, and a contested civilian-risk smoke scenario.

## Gameplay Area And Topology

The Market / Commercial Streets scenario uses two JSON manifests in addition to the compact map and sprite manifests:

```text
asset.building_level_manifest=assets/mosul/manifests/market_commercial_streets_2003_building_levels.json
asset.topology_manifest=assets/mosul/manifests/market_commercial_streets_2003_topology.json
```

The building-level manifest owns levels, wall/opening rectangles, and building regions. The topology manifest owns tactical nodes, portals between nodes, vertical connectors, portal state, and semantic zones such as civilian shelters, evacuation exits, caches, overwatch roofs, search objectives, restricted fire lanes, and danger areas. Both are loaded into the C core gameplay-area state before scenario validation finishes.

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
objective.0.position=330,238
objective.0.radius_m=28
objective.0.value=5
```
