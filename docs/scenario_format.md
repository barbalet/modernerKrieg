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

Asset references inside a scenario must be repo-relative paths under `assets/mosul/`. The loader validates referenced map and sprite manifests before the scenario reaches the core.

## Sections

The current loader supports:

- scenario metadata: `name`, `seed`
- asset references: `asset.map_manifest`, `asset.sprite_manifest`
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
- unsafe asset paths
- unknown enum values
- invalid controller, faction, force, weapon, or soldier references
- invalid hidden-contact fields, such as negative concealment
- out-of-bounds map tiles, terrain, civilians, objectives, and units
- scenarios that the portable C core refuses to load

CTest covers the default 2003 data file, fixture parity, hidden-contact fields, missing asset references, invalid force references, and impossible objective bounds.
