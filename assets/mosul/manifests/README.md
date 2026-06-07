# Mosul Manifests

This folder contains validated metadata for source art and runtime products.

- `market_commercial_streets_2003.mapmanifest` describes the first public demo map layers.
- `market_commercial_streets_2003_building_levels.json` describes the runtime floor PNG stack, LOS blockers, movement blockers, doors, windows, breach holes, stairs, roof edges, and building storey regions for the 2003 demo.
- `mosul_2003_sprites.spritemanifest` describes the first tactical sprite sources and runtime IDs.
- `mosul_2003_markers.markermanifest` describes tactical overlay marker colors, shapes, and scale hints.

Most compact manifests use deliberately simple `key=value` text so the C test suite can validate them without external dependencies. The building-level manifest is JSON because it has repeated floor, feature, and storey-region records; it is still parsed by the local C asset layer rather than a third-party JSON dependency.
