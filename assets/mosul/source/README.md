# Mosul Source Assets

This directory is for original Mosul art and reference assets before they are cropped, sliced, recolored, packed into atlases, or otherwise transformed for the engine.

Use this folder for:

- source line-art plates from the Mosul research/art brief
- original top-down sprite sheets at 64 px and 128 px scale
- original map sketches, section maps, and battle-axis plates
- raw vehicle, weapon, combatant, terrain, and civilian reference images
- notes that document where an asset came from and what it is allowed to be used for

Suggested subfolders:

```text
line_art/
sprite_sheets/
maps/
references/
notes/
```

Keep source assets unmodified. Engine-ready files should live outside this folder, for example:

- `assets/mosul/sprites/` for sliced or cleaned sprite PNGs
- `assets/mosul/atlases/` for atlas metadata and packed sheets
- `assets/mosul/maps/` for playable map data

Every committed source asset should have clear provenance and usage rights. Do not add downloaded, copyrighted, private, or restricted material unless it is allowed to be stored in this repository.

