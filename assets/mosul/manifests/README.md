# Mosul Manifests

This folder contains validated metadata for source art and runtime products.

- `market_commercial_streets_2003.mapmanifest` describes the first public demo map layers.
- `market_commercial_streets_2003_building_levels.json` describes the runtime floor PNG stack, LOS blockers, movement blockers, doors, windows, breach holes, stairs, roof edges, and building storey regions for the 2003 demo.
- `market_commercial_streets_2003_topology.json` describes the first tactical topology graph over that floor stack: nodes, portals, vertical connectors, blocked areas, and semantic zones.
- `mosul_2003_sprites.spritemanifest` describes the first tactical sprite sources and runtime IDs.
- `mosul_2003_markers.markermanifest` describes tactical overlay marker colors, shapes, and scale hints.

Most compact manifests use deliberately simple `key=value` text so the C test suite can validate them without external dependencies. The building-level and topology manifests are JSON because they have repeated floor, feature, region, node, portal, and semantic-zone records; they are still parsed by the local C asset layer rather than a third-party JSON dependency.
