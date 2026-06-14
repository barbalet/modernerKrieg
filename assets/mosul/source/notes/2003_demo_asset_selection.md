# 2003 Demo Asset Selection

This source asset import is limited to the current playable demo target:

- Era: 2003.
- Scenario: Market / Commercial Streets.
- Combat area: 500 m x 500 m.
- Overview map scale: 7,000 px x 7,000 px, about 14 px per meter.
- Full combat scale target: 35,000 px x 35,000 px, about 70 px per meter.

Included source groups:

- `line_art/`: Mosul context, combatant, weapon, vehicle, tactics, and U.S.-aligned reference plates relevant to the 2003 demo art direction.
- `sprite_sheets/`: 128 px tactical sheets for combatants, movement stances, U.S.-aligned troops, vehicles, and weapons.
- `sprite_sheets/source_angles/infantry_128/`: three approved source angles for all 16 demo infantry roles across standing, crouch, prone, wounded, and dead states.
- `sprite_sheets/source_angles/civilians_128/`: three approved source angles for seven civilian archetypes across standing, wounded, and dead states.
- `sprite_sheets/source_angles/weapons_128/`: three approved source angles for 128 px weapon sprites.
- `sprite_sheets/source_angles/vehicles_1024/`: three approved source angles for 1024 px vehicle sprites across intact, damaged, and destroyed states.
- `maps/market_commercial_streets_demo_2003/`: the 2003 Market / Commercial Streets line-art minimap, multistorey line-art overview, previews, unmodified layer manifest, and renderer/source map layers.

Excluded source groups:

- 2016-2017 battle-axis art.
- 2017 West Mosul Old City section art.
- Old City 500 m later-scenario overviews.
- 64 px renderings.
- `.DS_Store` and other local operating-system metadata.

Runtime-facing rendered flips are intentionally excluded from this source directory and stored under `assets/shared/runtime/sprites/rendered/`.
Runtime-facing map level PNGs are generated or copied from the approved source layers into `assets/mosul/runtime/maps/market_commercial_streets_2003/levels/`. Gameplay floor, wall, door, window, breach, stair, roof-edge, and storey-region metadata is stored in `assets/mosul/manifests/market_commercial_streets_2003_building_levels.json`.
