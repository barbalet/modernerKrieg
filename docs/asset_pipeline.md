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

The current runtime sprite set contains 896 PNGs, and tests assert that every render-manifest path exists:

- 640 infantry sprites: 16 demo roles x 5 states x 8 facings.
- 64 weapon sprites: 8 weapon types x 8 facings.
- 192 vehicle sprites: 8 vehicle types x 3 damage states x 8 facings.

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
- invalid marker colors or impossible marker dimensions
- paths that point outside the repository asset tree

## Runtime Generation

Generated assets should be reproducible. Do not edit generated files by hand. The initial app can load a single map overview image; later work can add tiled map products and packed sprite atlases when the image size or sprite count requires it.

Current runtime product:

- `assets/mosul/runtime/maps/market_commercial_streets_2003/overview.png`: copied from the source `preview_1400.png` as the first runtime map overview.
- `assets/mosul/runtime/sprites/rendered/`: copied from the MOSUL render pipeline as the first complete runtime-facing sprite set.
