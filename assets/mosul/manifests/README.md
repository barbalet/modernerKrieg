# Mosul Manifests

This folder contains validated metadata for source art and runtime products.

- `market_commercial_streets_2003.mapmanifest` describes the first public demo map layers.
- `mosul_2003_sprites.spritemanifest` describes the first tactical sprite sources and runtime IDs.
- `mosul_2003_markers.markermanifest` describes tactical overlay marker colors, shapes, and scale hints.

The current format is deliberately simple `key=value` text so the C test suite can validate manifests without a JSON dependency. It can be replaced later if the scenario format settles on JSON or another structured format.
