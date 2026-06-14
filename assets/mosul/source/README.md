# Mosul Source Assets

This directory is for original Mosul-specific art and reference assets before they are cropped, recolored, copied into runtime map layers, or otherwise transformed for the engine.

Use this folder for:

- source line-art plates from the Mosul research/art brief
- original map sketches, section maps, and battle-axis plates
- raw Mosul-specific vehicle, weapon, combatant, terrain, and civilian reference images
- notes that document where an asset came from and what it is allowed to be used for

Shared tactical sprite source sheets live under `assets/shared/source/sprite_sheets/` so Mosul and Fallujah can reuse the same top-down unit, civilian, weapon, and vehicle vocabulary.

Suggested subfolders:

```text
line_art/
maps/
references/
notes/
```

Keep source assets unmodified. Engine-ready files should live outside this folder, for example:

- `assets/shared/runtime/sprites/` for shared tactical sprite PNGs and render manifests
- `assets/mosul/atlases/` for Mosul-specific atlas metadata and packed sheets if needed
- `assets/mosul/maps/` for playable Mosul map data
- `assets/mosul/runtime/maps/` for runtime floor PNGs generated or copied from approved source layers

Every committed source asset should have clear provenance and usage rights. Do not add downloaded, copyrighted, private, or restricted material unless it is allowed to be stored in this repository.

The current shared source-angle sprite set uses `north`, `north_east`, and `east` as the approved authored angles. Infantry sprites use `standing`, `crouch`, `prone`, `wounded`, and `dead` states. Civilian sprites use `standing`, `wounded`, and `dead` states for seven non-combatant archetypes. Combat/support vehicle sprites use `intact`, `damaged`, and `destroyed` states. Dynamic traffic vehicle sprites use an `intact` state and live under `traffic_vehicles_1024/`. Runtime-facing flips and generated products live under `assets/shared/runtime/sprites/`.
