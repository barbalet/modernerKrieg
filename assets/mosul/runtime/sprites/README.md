# Runtime Sprites

Generated sprite extracts, packed sheets, and runtime sprite metadata belong here.

Current imported runtime set:

- `rendered/infantry_128/`: 640 PNGs, covering 16 demo infantry roles x 5 body states x 8 runtime facings.
- `rendered/civilians_128/`: 168 PNGs, covering 7 civilian archetypes x 3 body states x 8 runtime facings.
- `rendered/weapons_128/`: 64 PNGs, covering 8 weapon types x 8 runtime facings.
- `rendered/vehicles_1024/`: 192 PNGs, covering 8 vehicle types x 3 damage states x 8 runtime facings.
- `rendered/traffic_vehicles_1024/`: 24 PNGs, covering 3 dynamic traffic vehicle types x 8 runtime facings.
- `manifest.json`: source-angle and runtime-facing rules from the private render pipeline, with paths rewritten for this repository.
- `rendered/render_manifest.json`: detailed list of the 1,088 rendered PNGs.

Human, civilian, and weapon sprites are `128 x 128`. Vehicle sprites are `1024 x 1024`.
