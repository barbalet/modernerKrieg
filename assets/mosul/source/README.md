# Mosul Source Assets

This directory is for original Mosul art and reference assets before they are cropped, sliced, recolored, packed into atlases, or otherwise transformed for the engine.

Use this folder for:

- source line-art plates from the Mosul research/art brief
- original top-down sprite sheets at 128 px scale
- approved source-angle sprites for infantry, civilians, weapons, and vehicles
- original map sketches, section maps, and battle-axis plates
- raw vehicle, weapon, combatant, terrain, and civilian reference images
- notes that document where an asset came from and what it is allowed to be used for

Suggested subfolders:

```text
line_art/
sprite_sheets/
  source_angles/
    infantry_128/
    civilians_128/
    weapons_128/
    vehicles_1024/
maps/
references/
notes/
```

Keep source assets unmodified. Engine-ready files should live outside this folder, for example:

- `assets/mosul/sprites/` for sliced or cleaned sprite PNGs
- `assets/mosul/atlases/` for atlas metadata and packed sheets
- `assets/mosul/maps/` for playable map data
- `assets/mosul/runtime/maps/` for runtime floor PNGs generated or copied from approved source layers

Every committed source asset should have clear provenance and usage rights. Do not add downloaded, copyrighted, private, or restricted material unless it is allowed to be stored in this repository.

The current 2003 demo source-angle sprite set uses `north`, `north_east`, and `east` as the approved authored angles. Infantry sprites use `standing`, `crouch`, `prone`, `wounded`, and `dead` states. Civilian sprites use `standing`, `wounded`, and `dead` states for seven non-combatant archetypes. Vehicle sprites use `intact`, `damaged`, and `destroyed` states. Runtime-facing flips and generated products live outside this folder under `assets/mosul/runtime/sprites/`.
