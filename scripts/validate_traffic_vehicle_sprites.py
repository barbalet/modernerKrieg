#!/usr/bin/env python3
"""Validate MOSUL dynamic traffic vehicle runtime sprites.

The check intentionally uses only the Python standard library. It reads PNG
headers and alpha rows directly so validation does not depend on Pillow or any
renderer-side image stack.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
EXPECTED_ITEMS = ("traffic_civilian_car", "traffic_city_bus", "traffic_motorcycle")
EXPECTED_FACINGS = (
    "north",
    "south",
    "east",
    "west",
    "north_east",
    "north_west",
    "south_east",
    "south_west",
)


@dataclass(frozen=True)
class PngStats:
    width: int
    height: int
    color_type: int
    corner_alpha: tuple[int, int, int, int]
    border_alpha_max: int
    nontransparent_pixels: int
    semitransparent_pixels: int
    bbox: tuple[int, int, int, int]


def _paeth(left: int, up: int, up_left: int) -> int:
    estimate = left + up - up_left
    dist_left = abs(estimate - left)
    dist_up = abs(estimate - up)
    dist_up_left = abs(estimate - up_left)
    if dist_left <= dist_up and dist_left <= dist_up_left:
        return left
    if dist_up <= dist_up_left:
        return up
    return up_left


def _read_png_rgba(path: Path) -> tuple[int, int, list[bytearray]]:
    data = path.read_bytes()
    if data[:8] != PNG_SIGNATURE:
        raise ValueError(f"{path}: not a PNG file")

    pos = 8
    width = 0
    height = 0
    color_type = -1
    idat = bytearray()
    while pos < len(data):
        if pos + 8 > len(data):
            raise ValueError(f"{path}: truncated PNG chunk header")
        length = struct.unpack(">I", data[pos : pos + 4])[0]
        pos += 4
        chunk_type = data[pos : pos + 4]
        pos += 4
        chunk = data[pos : pos + length]
        pos += length
        pos += 4

        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type, compression, png_filter, interlace = struct.unpack(
                ">IIBBBBB",
                chunk,
            )
            if bit_depth != 8 or color_type != 6 or compression != 0 or png_filter != 0 or interlace != 0:
                raise ValueError(
                    f"{path}: expected 8-bit non-interlaced RGBA PNG, "
                    f"got bit_depth={bit_depth} color_type={color_type} interlace={interlace}"
                )
        elif chunk_type == b"IDAT":
            idat.extend(chunk)
        elif chunk_type == b"IEND":
            break

    if width <= 0 or height <= 0 or color_type != 6:
        raise ValueError(f"{path}: missing or invalid IHDR")

    bytes_per_pixel = 4
    row_size = width * bytes_per_pixel
    decoded = zlib.decompress(bytes(idat))
    rows: list[bytearray] = []
    previous = bytearray(row_size)
    offset = 0
    for _y in range(height):
        filter_type = decoded[offset]
        offset += 1
        row = bytearray(decoded[offset : offset + row_size])
        offset += row_size

        for index in range(row_size):
            left = row[index - bytes_per_pixel] if index >= bytes_per_pixel else 0
            up = previous[index]
            up_left = previous[index - bytes_per_pixel] if index >= bytes_per_pixel else 0
            if filter_type == 1:
                row[index] = (row[index] + left) & 0xFF
            elif filter_type == 2:
                row[index] = (row[index] + up) & 0xFF
            elif filter_type == 3:
                row[index] = (row[index] + ((left + up) // 2)) & 0xFF
            elif filter_type == 4:
                row[index] = (row[index] + _paeth(left, up, up_left)) & 0xFF
            elif filter_type != 0:
                raise ValueError(f"{path}: unsupported PNG filter {filter_type}")

        rows.append(row)
        previous = row

    return width, height, rows


def inspect_png(path: Path) -> PngStats:
    width, height, rows = _read_png_rgba(path)
    border_alpha_max = 0
    nontransparent_pixels = 0
    semitransparent_pixels = 0
    min_x = width
    min_y = height
    max_x = -1
    max_y = -1

    for y, row in enumerate(rows):
        for x in range(width):
            alpha = row[(x * 4) + 3]
            if y == 0 or y == height - 1 or x == 0 or x == width - 1:
                border_alpha_max = max(border_alpha_max, alpha)
            if alpha > 0:
                nontransparent_pixels += 1
                if alpha < 255:
                    semitransparent_pixels += 1
                min_x = min(min_x, x)
                min_y = min(min_y, y)
                max_x = max(max_x, x)
                max_y = max(max_y, y)

    return PngStats(
        width=width,
        height=height,
        color_type=6,
        corner_alpha=(
            rows[0][3],
            rows[0][((width - 1) * 4) + 3],
            rows[height - 1][3],
            rows[height - 1][((width - 1) * 4) + 3],
        ),
        border_alpha_max=border_alpha_max,
        nontransparent_pixels=nontransparent_pixels,
        semitransparent_pixels=semitransparent_pixels,
        bbox=(min_x, min_y, max_x, max_y),
    )


def validate(root: Path) -> list[str]:
    errors: list[str] = []
    render_root = root / "assets/shared/runtime/sprites/rendered"
    traffic_root = render_root / "traffic_vehicles_1024/civilian"
    manifest_path = render_root / "render_manifest.json"
    compact_manifest_path = root / "assets/shared/manifests/shared_tactical_sprites.spritemanifest"

    render_manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    render_entries = render_manifest.get("rendered", [])
    manifest_paths = {entry.get("path", "") for entry in render_entries}

    compact_text = compact_manifest_path.read_text(encoding="utf-8")
    for item in EXPECTED_ITEMS:
        compact_id = f"{item}_intact_north"
        if f"runtime_id={compact_id}" not in compact_text:
            errors.append(f"compact sprite manifest missing runtime id {compact_id}")

    validated = 0
    for item in EXPECTED_ITEMS:
        for facing in EXPECTED_FACINGS:
            relative_path = (
                f"assets/shared/runtime/sprites/rendered/traffic_vehicles_1024/"
                f"civilian/{item}/intact/{facing}.png"
            )
            path = root / relative_path
            if not path.exists():
                errors.append(f"missing runtime traffic vehicle sprite {relative_path}")
                continue
            if relative_path not in manifest_paths:
                errors.append(f"render manifest missing {relative_path}")
            try:
                stats = inspect_png(path)
            except ValueError as exc:
                errors.append(str(exc))
                continue

            if stats.width != 1024 or stats.height != 1024:
                errors.append(f"{relative_path}: expected 1024 x 1024, got {stats.width} x {stats.height}")
            if stats.corner_alpha != (0, 0, 0, 0):
                errors.append(f"{relative_path}: expected transparent corners, got {stats.corner_alpha}")
            if stats.border_alpha_max != 0:
                errors.append(f"{relative_path}: expected transparent outer border, got alpha {stats.border_alpha_max}")
            if stats.nontransparent_pixels <= 0:
                errors.append(f"{relative_path}: no visible vehicle pixels")
            validated += 1

    if validated != len(EXPECTED_ITEMS) * len(EXPECTED_FACINGS):
        errors.append(f"expected 24 traffic vehicle sprites, validated {validated}")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate MOSUL traffic vehicle runtime sprites")
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    errors = validate(root)
    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    print("traffic vehicle sprite validation passed")
    print("items=3 facings=8 sprites=24 size=1024x1024 color=RGBA outer_border_alpha=0")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
