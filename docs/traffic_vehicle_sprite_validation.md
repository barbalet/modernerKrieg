# Traffic Vehicle Sprite Validation

Last updated: 2026-06-10. This document closes dynamic vehicle cleanup cycle 4
and is now wired into cycle 8 CTest verification.

## Scope

Validated runtime traffic vehicle sprites under:

```text
assets/shared/runtime/sprites/rendered/traffic_vehicles_1024/
```

Expected set:

- `traffic_civilian_car`
- `traffic_city_bus`
- `traffic_motorcycle`

Each item must provide these eight runtime facings:

- `north`
- `south`
- `east`
- `west`
- `north_east`
- `north_west`
- `south_east`
- `south_west`

Total expected runtime PNGs: 24.

## Validation Command

Run from the repository root:

```bash
python3 scripts/validate_traffic_vehicle_sprites.py
```

The script reads PNG chunks directly with the Python standard library. It does
not use Pillow and does not perform any rendering or asset mutation.

When CMake finds a Python 3 interpreter, the same check is also available as:

```bash
ctest --test-dir build --output-on-failure -R mk_traffic_vehicle_sprite_validation
```

## Checks

The validator enforces:

- every expected car, bus, and motorcycle facing exists
- every runtime PNG is present in `render_manifest.json`
- compact sprite manifest runtime ids exist for:
  - `traffic_civilian_car_intact_north`
  - `traffic_city_bus_intact_north`
  - `traffic_motorcycle_intact_north`
- every PNG is `1024 x 1024`
- every PNG is 8-bit RGBA
- all four corners are fully transparent
- the complete outer border is fully transparent
- each sprite has non-transparent vehicle pixels

Current validation output:

```text
traffic vehicle sprite validation passed
items=3 facings=8 sprites=24 size=1024x1024 color=RGBA outer_border_alpha=0
```

## Notes

The current traffic vehicle sprites use a binary alpha matte: vehicle pixels are
opaque and the surrounding air is fully transparent. No semitransparent soft
edge pixels were detected during manual inspection. That is acceptable for the
current renderer requirement because the air around cars, buses, and
motorcycles no longer renders as a white rectangle.

Future art polish may add antialiased semitransparent edge pixels, but that is
not required to close cycle 4. The important requirement for this cycle is that
runtime traffic vehicles are independent sprites with transparent borders and
manifest-backed IDs, not baked map ink.
