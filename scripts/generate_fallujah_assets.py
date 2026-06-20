#!/usr/bin/env python3
"""Generate Fallujah 2004 source-map, runtime map, and scenario assets."""

from __future__ import annotations

import json
import math
import struct
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ASSET_ROOT = ROOT / "assets" / "fallujah"
SCENARIO_ROOT = ROOT / "game" / "fallujah" / "scenarios"
MAP_ID = "southern_industrial_foothold_2004"
WORLD_M = 1600
PIXELS = 2400
PREVIEW_PIXELS = 1400
PPM = PIXELS / WORLD_M
SOURCE_MAP_ROOT = ASSET_ROOT / "source" / "maps" / MAP_ID
SOURCE_MAP_DATA = SOURCE_MAP_ROOT / "assets" / "map_data" / f"{MAP_ID}_2400"
SOURCE_IMG_DIR = SOURCE_MAP_ROOT / "imgs" / f"{MAP_ID}_2400"
SOURCE_REFERENCE_DIR = SOURCE_MAP_ROOT / "references"
VECTOR_PATH = SOURCE_MAP_ROOT / "vector" / "fallujah_sinaa_industrial_osm_20260614.json"
DERIVED_TEXTURE_PATH = SOURCE_REFERENCE_DIR / "derived_satellite_graphite_texture.png"

BUILDING_REGIONS = [
    ("industrial_warehouse_west", 120, 930, 310, 210, 2),
    ("repair_yards_east", 1030, 1030, 340, 250, 1),
    ("district_office", 970, 610, 240, 210, 3),
    ("residential_row_north", 520, 250, 390, 260, 2),
    ("mosque_school_edge", 1130, 310, 250, 220, 2),
    ("clinic_compound", 250, 620, 220, 170, 1),
    ("market_shops", 580, 790, 320, 180, 2),
    ("workshop_roof_watch", 1350, 760, 170, 210, 3),
]

# Reference-derived urban blocks around the real Sina'a / Industrial District street grid.
# They are presentation art, not enterable gameplay buildings; gameplay blockers live in BUILDING_REGIONS.
URBAN_FABRIC_ZONES = [
    ("north_residential_grid", 350, 115, 980, 445, 34, 31, 0),
    ("central_market_grid", 360, 560, 700, 360, 42, 34, 5),
    ("southwest_industrial_yards", 85, 895, 640, 455, 64, 48, 11),
    ("southeast_industrial_yards", 930, 840, 575, 520, 72, 54, 17),
    ("southern_residential_fringe", 380, 1250, 650, 280, 48, 38, 23),
]


def px(meters: float) -> int:
    return int(round(meters * PPM))


def meters(pixel: int) -> float:
    return pixel / PPM


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def write_png(path: Path, width: int, height: int, pixels: bytearray | bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    rows = bytearray()
    stride = width * 4
    for y in range(height):
        rows.append(0)
        start = y * stride
        rows.extend(pixels[start:start + stride])

    payload = zlib.compress(bytes(rows), level=7)
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", ihdr) + png_chunk(b"IDAT", payload) + png_chunk(b"IEND", b""))


def read_png_filter0_rgba(path: Path) -> tuple[int, int, bytearray] | None:
    if not path.exists():
        return None
    data = path.read_bytes()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        return None
    offset = 8
    width = height = 0
    color_type = None
    compressed = bytearray()
    while offset + 8 <= len(data):
        length = struct.unpack(">I", data[offset:offset + 4])[0]
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        offset += 12 + length
        if kind == b"IHDR":
            width, height, bit_depth, color_type, compression, png_filter, interlace = struct.unpack(">IIBBBBB", payload)
            if bit_depth != 8 or color_type != 6 or compression != 0 or png_filter != 0 or interlace != 0:
                return None
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            break
    if width <= 0 or height <= 0 or color_type != 6:
        return None
    raw = zlib.decompress(bytes(compressed))
    stride = width * 4
    pixels = bytearray(width * height * 4)
    src = 0
    for y in range(height):
        filter_type = raw[src]
        src += 1
        if filter_type != 0:
            return None
        start = y * stride
        pixels[start:start + stride] = raw[src:src + stride]
        src += stride
    return width, height, pixels


def canvas(color: tuple[int, int, int, int]) -> bytearray:
    return bytearray(color * (PIXELS * PIXELS))


def blend_pixel(pixels: bytearray, x: int, y: int, color: tuple[int, int, int, int]) -> None:
    if x < 0 or y < 0 or x >= PIXELS or y >= PIXELS:
        return

    offset = (y * PIXELS + x) * 4
    sr, sg, sb, sa = color
    if sa >= 255:
        pixels[offset:offset + 4] = bytes(color)
        return

    alpha = sa / 255.0
    inv = 1.0 - alpha
    pixels[offset] = int(sr * alpha + pixels[offset] * inv)
    pixels[offset + 1] = int(sg * alpha + pixels[offset + 1] * inv)
    pixels[offset + 2] = int(sb * alpha + pixels[offset + 2] * inv)
    pixels[offset + 3] = min(255, int(sa + pixels[offset + 3] * inv))


def rect(
    pixels: bytearray,
    x: int,
    y: int,
    width: int,
    height: int,
    color: tuple[int, int, int, int],
) -> None:
    x0 = max(0, x)
    y0 = max(0, y)
    x1 = min(PIXELS, x + width)
    y1 = min(PIXELS, y + height)
    if x1 <= x0 or y1 <= y0:
        return

    if color[3] == 255:
        row = bytes(color) * (x1 - x0)
        for yy in range(y0, y1):
            start = (yy * PIXELS + x0) * 4
            pixels[start:start + len(row)] = row
        return

    for yy in range(y0, y1):
        for xx in range(x0, x1):
            blend_pixel(pixels, xx, yy, color)


def dot(
    pixels: bytearray,
    cx: int,
    cy: int,
    radius: int,
    color: tuple[int, int, int, int],
) -> None:
    radius = max(1, radius)
    r2 = radius * radius
    for yy in range(cy - radius, cy + radius + 1):
        if yy < 0 or yy >= PIXELS:
            continue
        for xx in range(cx - radius, cx + radius + 1):
            if xx < 0 or xx >= PIXELS:
                continue
            dx = xx - cx
            dy = yy - cy
            d2 = dx * dx + dy * dy
            if d2 > r2:
                continue
            falloff = 1.0 - d2 / (r2 + 1)
            alpha = max(10, int(color[3] * (0.35 + 0.65 * falloff)))
            blend_pixel(pixels, xx, yy, (color[0], color[1], color[2], alpha))


def graphite_line(
    pixels: bytearray,
    x0: int,
    y0: int,
    x1: int,
    y1: int,
    color: tuple[int, int, int, int],
    width: int = 3,
    jitter: float = 1.0,
    passes: int = 2,
) -> None:
    dx = x1 - x0
    dy = y1 - y0
    length = max(1.0, (dx * dx + dy * dy) ** 0.5)
    nx = -dy / length
    ny = dx / length
    steps = max(1, int(length / 4.0))
    radius = max(1, min(14, width // 2))
    for p in range(passes):
        phase = (p + 1) * 1.618
        for step in range(steps + 1):
            t = step / steps
            wobble = math.sin(t * 19.13 + phase) * jitter + math.sin(t * 41.7 + phase * 0.37) * jitter * 0.42
            pressure = 0.72 + 0.20 * math.sin(t * 9.7 + phase)
            xx = int(round(x0 + dx * t + nx * wobble))
            yy = int(round(y0 + dy * t + ny * wobble))
            dot(pixels, xx, yy, radius, (color[0], color[1], color[2], int(color[3] * pressure)))


def rect_outline(
    pixels: bytearray,
    x: int,
    y: int,
    width: int,
    height: int,
    color: tuple[int, int, int, int],
    line: int = 3,
) -> None:
    graphite_line(pixels, x, y, x + width, y, color, width=line, jitter=1.1, passes=2)
    graphite_line(pixels, x, y + height, x + width, y + height, color, width=line, jitter=1.1, passes=2)
    graphite_line(pixels, x, y, x, y + height, color, width=line, jitter=1.1, passes=2)
    graphite_line(pixels, x + width, y, x + width, y + height, color, width=line, jitter=1.1, passes=2)


def world_rect(x: float, y: float, width: float, height: float) -> tuple[int, int, int, int]:
    return px(x), px(y), px(width), px(height)


def load_vector_map() -> dict[str, object]:
    return json.loads(VECTOR_PATH.read_text(encoding="utf-8"))


def points_px(feature: dict[str, object]) -> list[tuple[int, int]]:
    points = feature.get("points_m", [])
    return [(px(float(point[0])), px(float(point[1]))) for point in points]


def hash01(seed: int, a: int, b: int) -> float:
    value = (seed * 1103515245 + a * 73856093 + b * 19349663 + 0x9E3779B9) & 0xFFFFFFFF
    value ^= value >> 13
    value = (value * 1274126177) & 0xFFFFFFFF
    return (value & 0xFFFFFF) / float(0x1000000)


def graphite_canvas() -> bytearray:
    pixels = canvas((27, 28, 29, 255))
    stride = PIXELS * 4
    for y in range(0, PIXELS, 2):
        row = y * stride
        for x in range(0, PIXELS, 2):
            grain = ((x * 37 + y * 17 + (x // 11) * 13 + (y // 7) * 19) % 25) - 12
            offset = row + x * 4
            base = max(17, min(43, 29 + grain // 3))
            pixels[offset] = base
            pixels[offset + 1] = base + 1
            pixels[offset + 2] = base + 2
            if x + 1 < PIXELS:
                pixels[offset + 4] = max(18, min(44, base + 2))
                pixels[offset + 5] = max(19, min(45, base + 3))
                pixels[offset + 6] = max(20, min(46, base + 4))
    return pixels


def overlay_texture(pixels: bytearray) -> None:
    loaded = read_png_filter0_rgba(DERIVED_TEXTURE_PATH)
    if loaded is None:
        return
    width, height, texture = loaded
    if width != PIXELS or height != PIXELS:
        return
    for offset in range(0, len(pixels), 4):
        alpha = texture[offset + 3]
        if alpha == 0:
            continue
        blend_pixel(
            pixels,
            (offset // 4) % PIXELS,
            (offset // 4) // PIXELS,
            (texture[offset], texture[offset + 1], texture[offset + 2], min(92, alpha)),
        )


def fill_polygon(pixels: bytearray, points: list[tuple[int, int]], color: tuple[int, int, int, int]) -> None:
    if len(points) < 3:
        return
    ys = [point[1] for point in points]
    y0 = max(0, min(ys))
    y1 = min(PIXELS - 1, max(ys))
    for y in range(y0, y1 + 1):
        intersections: list[int] = []
        for index, (x_a, y_a) in enumerate(points):
            x_b, y_b = points[(index + 1) % len(points)]
            if y_a == y_b:
                continue
            if (y >= min(y_a, y_b)) and (y < max(y_a, y_b)):
                t = (y - y_a) / (y_b - y_a)
                intersections.append(int(round(x_a + (x_b - x_a) * t)))
        intersections.sort()
        for x_start, x_end in zip(intersections[0::2], intersections[1::2]):
            rect(pixels, x_start, y, x_end - x_start + 1, 1, color)


def draw_landuse(pixels: bytearray, vector: dict[str, object]) -> None:
    for feature in vector.get("features", []):
        if feature.get("kind") != "landuse":
            continue
        klass = feature.get("class")
        pts = points_px(feature)
        if klass == "industrial":
            fill_polygon(pixels, pts, (70, 68, 60, 58))
            edge = (157, 151, 130, 82)
        else:
            fill_polygon(pixels, pts, (54, 55, 51, 36))
            edge = (119, 117, 105, 60)
        for a, b in zip(pts, pts[1:] + pts[:1]):
            graphite_line(pixels, a[0], a[1], b[0], b[1], edge, width=3, jitter=1.4, passes=1)


def road_width_px(klass: str) -> int:
    return {
        "motorway": 22,
        "motorway_link": 18,
        "trunk": 19,
        "trunk_link": 15,
        "primary": 17,
        "primary_link": 14,
        "secondary": 14,
        "secondary_link": 12,
        "tertiary": 11,
        "tertiary_link": 9,
        "residential": 7,
        "unclassified": 7,
        "service": 5,
        "track": 4,
        "path": 3,
        "footway": 3,
    }.get(klass, 6)


def draw_feature_polyline(
    pixels: bytearray,
    points: list[tuple[int, int]],
    color: tuple[int, int, int, int],
    width: int,
    jitter: float,
    passes: int,
) -> None:
    for a, b in zip(points, points[1:]):
        graphite_line(pixels, a[0], a[1], b[0], b[1], color, width=width, jitter=jitter, passes=passes)


def draw_transport_network(pixels: bytearray, vector: dict[str, object]) -> None:
    road_features = [feature for feature in vector.get("features", []) if feature.get("kind") == "highway"]
    rail_features = [feature for feature in vector.get("features", []) if feature.get("kind") == "railway"]
    water_features = [feature for feature in vector.get("features", []) if feature.get("kind") == "waterway"]

    for feature in water_features:
        pts = points_px(feature)
        draw_feature_polyline(pixels, pts, (80, 93, 95, 138), width=24, jitter=1.4, passes=2)
        draw_feature_polyline(pixels, pts, (151, 166, 160, 58), width=12, jitter=1.0, passes=1)

    for feature in road_features:
        klass = str(feature.get("class", ""))
        pts = points_px(feature)
        width = road_width_px(klass)
        draw_feature_polyline(pixels, pts, (22, 23, 23, 220), width=width + 6, jitter=1.3, passes=2)

    for feature in road_features:
        klass = str(feature.get("class", ""))
        pts = points_px(feature)
        width = road_width_px(klass)
        edge_alpha = 150 if width >= 12 else 92
        draw_feature_polyline(pixels, pts, (132, 128, 112, edge_alpha), width=max(2, width // 3), jitter=1.1, passes=1)
        if width >= 12:
            draw_feature_polyline(pixels, pts, (196, 188, 154, 84), width=2, jitter=0.7, passes=1)

    for feature in rail_features:
        pts = points_px(feature)
        draw_feature_polyline(pixels, pts, (39, 40, 41, 230), width=9, jitter=0.7, passes=2)
        draw_feature_polyline(pixels, pts, (168, 162, 134, 140), width=2, jitter=0.5, passes=1)
        for a, b in zip(pts, pts[1:]):
            dx = b[0] - a[0]
            dy = b[1] - a[1]
            length = max(1.0, (dx * dx + dy * dy) ** 0.5)
            nx = -dy / length
            ny = dx / length
            for step in range(0, int(length), 28):
                t = step / length
                cx = a[0] + dx * t
                cy = a[1] + dy * t
                graphite_line(
                    pixels,
                    int(cx - nx * 7),
                    int(cy - ny * 7),
                    int(cx + nx * 7),
                    int(cy + ny * 7),
                    (176, 170, 144, 108),
                    width=2,
                    jitter=0.4,
                    passes=1,
                )


def draw_door_gap(pixels: bytearray, rx: int, ry: int, rw: int, rh: int, door_width: int = 44) -> None:
    door_x = rx + rw // 2 - door_width // 2
    rect(pixels, door_x, ry + rh - 9, door_width, 12, (43, 44, 43, 235))
    graphite_line(pixels, door_x, ry + rh - 12, door_x + door_width, ry + rh - 12, (218, 214, 191, 150), width=2, jitter=0.8, passes=1)


def draw_windows(pixels: bytearray, rx: int, ry: int, rw: int, rh: int) -> None:
    marks = [
        (rx + rw // 3 - 14, ry + 2, 28, 5),
        (rx + (2 * rw) // 3 - 14, ry + 2, 28, 5),
        (rx + rw - 7, ry + rh // 2 - 18, 5, 36),
    ]
    for x, y, w, h in marks:
        rect(pixels, x, y, w, h, (42, 43, 43, 190))
        rect_outline(pixels, x, y, w, h, (214, 209, 186, 118), line=1)


def graphite_fill_rect(
    pixels: bytearray,
    x: int,
    y: int,
    width: int,
    height: int,
    color: tuple[int, int, int, int],
    hatch: bool = False,
) -> None:
    rect(pixels, x, y, width, height, color)
    if hatch:
        spacing = max(18, min(52, (width + height) // 17))
        start = x - height
        end = x + width
        for sx in range(start, end, spacing):
            graphite_line(
                pixels,
                sx,
                y + height - 3,
                sx + height,
                y + 3,
                (95, 93, 84, 62),
                width=2,
                jitter=1.0,
                passes=1,
            )


def draw_building(
    pixels: bytearray,
    x: float,
    y: float,
    width: float,
    height: float,
    fill: tuple[int, int, int, int] = (58, 57, 54, 238),
    edge: tuple[int, int, int, int] = (215, 210, 186, 210),
) -> None:
    rx, ry, rw, rh = world_rect(x, y, width, height)
    graphite_fill_rect(pixels, rx, ry, rw, rh, fill, hatch=True)
    rect_outline(pixels, rx, ry, rw, rh, edge, line=4)
    inset = max(10, min(rw, rh) // 18)
    rect_outline(pixels, rx + inset, ry + inset, rw - inset * 2, rh - inset * 2, (118, 115, 104, 115), line=2)
    graphite_line(pixels, rx + inset, ry + rh - inset, rx + rw - inset, ry + inset, (96, 94, 87, 70), width=2, jitter=1.1, passes=1)
    draw_door_gap(pixels, rx, ry, rw, rh)
    draw_windows(pixels, rx, ry, rw, rh)


def draw_urban_fabric_zone(
    pixels: bytearray,
    zone: tuple[str, float, float, float, float, int, int, int],
) -> None:
    _, x, y, width, height, cell_w, cell_h, seed = zone
    x0, y0, w, h = world_rect(x, y, width, height)
    step_x = px(cell_w)
    step_y = px(cell_h)
    for yy in range(y0, y0 + h, step_y):
        for xx in range(x0, x0 + w, step_x):
            local_x = (xx - x0) // max(1, step_x)
            local_y = (yy - y0) // max(1, step_y)
            skip = hash01(seed, local_x, local_y)
            if skip < 0.16:
                continue
            jitter_x = int((hash01(seed + 3, local_x, local_y) - 0.5) * step_x * 0.32)
            jitter_y = int((hash01(seed + 7, local_x, local_y) - 0.5) * step_y * 0.32)
            bw = max(px(18), int(step_x * (0.52 + hash01(seed + 11, local_x, local_y) * 0.36)))
            bh = max(px(14), int(step_y * (0.50 + hash01(seed + 13, local_x, local_y) * 0.34)))
            bx = xx + jitter_x
            by = yy + jitter_y
            if bx < 0 or by < 0 or bx + bw > PIXELS or by + bh > PIXELS:
                continue
            alpha = 54 + int(hash01(seed + 17, local_x, local_y) * 50)
            rect_outline(pixels, bx, by, bw, bh, (138, 133, 116, alpha), line=2)
            if hash01(seed + 19, local_x, local_y) > 0.55:
                inset = max(4, min(bw, bh) // 5)
                rect_outline(pixels, bx + inset, by + inset, bw - inset * 2, bh - inset * 2, (108, 105, 95, alpha // 2), line=1)
            if hash01(seed + 23, local_x, local_y) > 0.68:
                graphite_line(pixels, bx, by + bh // 2, bx + bw, by + bh // 2, (95, 92, 82, alpha // 2), width=1, jitter=0.7, passes=1)


def draw_osm_building_footprints(pixels: bytearray, vector: dict[str, object]) -> None:
    for feature in vector.get("features", []):
        if feature.get("kind") != "building":
            continue
        pts = points_px(feature)
        fill_polygon(pixels, pts, (67, 66, 61, 150))
        for a, b in zip(pts, pts[1:]):
            graphite_line(pixels, a[0], a[1], b[0], b[1], (204, 198, 174, 138), width=3, jitter=1.0, passes=1)


def draw_grid_reference(pixels: bytearray) -> None:
    for m in range(0, WORLD_M + 1, 200):
        pos = px(m)
        graphite_line(pixels, pos, 0, pos, PIXELS, (82, 80, 72, 26), width=1, jitter=1.5, passes=1)
        graphite_line(pixels, 0, pos, PIXELS, pos, (82, 80, 72, 26), width=1, jitter=1.5, passes=1)


def draw_ground() -> bytearray:
    vector = load_vector_map()
    pixels = graphite_canvas()
    overlay_texture(pixels)
    draw_landuse(pixels, vector)
    for zone in URBAN_FABRIC_ZONES:
        draw_urban_fabric_zone(pixels, zone)
    draw_osm_building_footprints(pixels, vector)
    draw_transport_network(pixels, vector)
    for building in BUILDING_REGIONS:
        draw_building(pixels, *building[1:5])
    draw_grid_reference(pixels)
    return pixels


def draw_overlay(regions: list[tuple[str, float, float, float, float, int]], minimum_storeys: int) -> bytearray:
    pixels = canvas((0, 0, 0, 0))
    fill = (203, 199, 175, 34)
    edge = (235, 229, 203, 172)
    stair = (122, 174, 194, 168)

    for _, x, y, width, height, storeys in regions:
        if storeys < minimum_storeys:
            continue
        rx, ry, rw, rh = world_rect(x, y, width, height)
        graphite_fill_rect(pixels, rx, ry, rw, rh, fill, hatch=True)
        rect_outline(pixels, rx, ry, rw, rh, edge, line=4)
        inset = max(10, min(rw, rh) // 16)
        rect_outline(pixels, rx + inset, ry + inset, rw - inset * 2, rh - inset * 2, (226, 222, 196, 82), line=2)
        rect(pixels, rx + rw - 30, ry + 18, 18, 28, stair)
        graphite_line(pixels, rx + rw - 30, ry + 18, rx + rw - 12, ry + 46, (222, 238, 244, 116), width=2, jitter=0.9, passes=1)

    return pixels


def draw_multistorey_mask(regions: list[tuple[str, float, float, float, float, int]]) -> bytearray:
    pixels = canvas((0, 0, 0, 255))
    for _, x, y, width, height, storeys in regions:
        if storeys < 2:
            continue
        rx, ry, rw, rh = world_rect(x, y, width, height)
        rect(pixels, rx, ry, rw, rh, (255, 255, 255, 255))
    return pixels


def compose_over(base: bytearray, overlay: bytearray) -> bytearray:
    out = bytearray(base)
    for offset in range(0, len(out), 4):
        alpha = overlay[offset + 3]
        if alpha == 0:
            continue
        x = (offset // 4) % PIXELS
        y = (offset // 4) // PIXELS
        blend_pixel(out, x, y, (overlay[offset], overlay[offset + 1], overlay[offset + 2], alpha))
    return out


def resize_nearest(pixels: bytearray, source_width: int, source_height: int, target: int) -> bytearray:
    out = bytearray(target * target * 4)
    for y in range(target):
        sy = min(source_height - 1, int((y + 0.5) * source_height / target))
        for x in range(target):
            sx = min(source_width - 1, int((x + 0.5) * source_width / target))
            src = (sy * source_width + sx) * 4
            dst = (y * target + x) * 4
            out[dst:dst + 4] = pixels[src:src + 4]
    return out


def map_layers_json() -> dict[str, object]:
    return {
        "schema_version": 1,
        "id": MAP_ID,
        "display_name": "Fallujah Southern Industrial Foothold 2004",
        "era": "2004",
        "real_world_size_m": [WORLD_M, WORLD_M],
        "overview_pixel_size": [PIXELS, PIXELS],
        "overview_pixels_per_meter": PPM,
        "full_combat_pixels_per_meter": PPM,
        "full_combat_pixel_size": [PIXELS, PIXELS],
        "art_style": "approved Fallujah black-and-white contemporary graphite line art; OSM-derived real street grid; 1:10,000 Al Fallujah military/image graphic and 2002/2004 satellite references used for urban fabric correction; no stick art; no schematic placeholder art; no baked static traffic vehicle ink",
        "composite": {
            "id": "minimap_composite",
            "path": "00_minimap_composite.png",
            "mode": "opaque_grayscale",
        },
        "layers": [
            {"id": "ground", "path": f"assets/map_data/{MAP_ID}_2400/01_ground_level.png", "mode": "opaque_grayscale", "z_index": 0},
            {"id": "level_2", "path": f"assets/map_data/{MAP_ID}_2400/02_level_2_alpha.png", "mode": "rgba_alpha_overlay", "z_index": 1},
            {"id": "level_3", "path": f"assets/map_data/{MAP_ID}_2400/03_level_3_alpha.png", "mode": "rgba_alpha_overlay", "z_index": 2},
            {"id": "roof_access", "path": f"assets/map_data/{MAP_ID}_2400/04_roof_access_alpha.png", "mode": "rgba_alpha_overlay", "z_index": 3},
        ],
        "multistorey_mask": {
            "path": f"assets/map_data/{MAP_ID}_2400/05_multistorey_mask.png",
            "preview_path": f"assets/map_data/{MAP_ID}_2400/preview_multistorey_mask_1400.png",
            "lineart_overview_path": "06_multistorey_lineart_overview.png",
            "mode": "opaque_grayscale",
            "black": "single-layer terrain, road, courtyard, open lot, or building without authored upper floor",
            "white": "building footprint requiring one or more transparent upper-level PNG overlays",
        },
        "display_overviews": [
            {"id": "minimap_preview", "path": "preview_1400.png", "mode": "opaque_grayscale"},
            {"id": "multistorey_lineart_overview", "path": "06_multistorey_lineart_overview.png", "mode": "opaque_grayscale"},
            {"id": "multistorey_lineart_preview", "path": "preview_multistorey_lineart_1400.png", "mode": "opaque_grayscale"},
        ],
        "source_vector_base": "vector/fallujah_sinaa_industrial_osm_20260614.json",
        "source_references": "references/reference_sources.json",
        "derived_reference_texture": "references/derived_satellite_graphite_texture.png",
    }


def map_manifest_text() -> str:
    source_root = f"assets/fallujah/source/maps/{MAP_ID}"
    source_map_data = f"{source_root}/assets/map_data/{MAP_ID}_2400"
    runtime_root = f"assets/fallujah/runtime/maps/{MAP_ID}"
    return f"""manifest_type=map
id={MAP_ID}
name=Fallujah Southern Industrial Foothold 2004
world_width_m={WORLD_M}
world_height_m={WORLD_M}
pixels_per_meter={PPM:.2f}
origin=top_left
source_root={source_root}
runtime_root={runtime_root}
layer_count=5
layer.0.id=ground
layer.0.kind=base
layer.0.path={source_map_data}/01_ground_level.png
layer.0.z=0
layer.0.alpha=opaque
layer.1.id=level_2
layer.1.kind=upper_floor
layer.1.path={source_map_data}/02_level_2_alpha.png
layer.1.z=10
layer.1.alpha=premultiplied
layer.2.id=level_3
layer.2.kind=upper_floor
layer.2.path={source_map_data}/03_level_3_alpha.png
layer.2.z=20
layer.2.alpha=premultiplied
layer.3.id=roof_access
layer.3.kind=access
layer.3.path={source_map_data}/04_roof_access_alpha.png
layer.3.z=30
layer.3.alpha=premultiplied
layer.4.id=multistorey_mask
layer.4.kind=mask
layer.4.path={source_map_data}/05_multistorey_mask.png
layer.4.z=40
layer.4.alpha=mask
overview_path={source_root}/imgs/{MAP_ID}_2400/preview_1400.png
runtime_overview_path={runtime_root}/overview.png
collision_output_path=assets/fallujah/maps/{MAP_ID}_collision.mask
navigation_output_path=assets/fallujah/maps/{MAP_ID}_navigation.grid
"""


def building_manifest(regions: list[tuple[str, float, float, float, float, int]]) -> dict[str, object]:
    levels = [
        ("level_01_ground", 1, 0, "opaque"),
        ("level_02_roofs_and_second_floor", 2, 3, "overlay"),
        ("level_03_upper_floor", 3, 6, "overlay"),
        ("level_04_roof_access", 4, 9, "overlay"),
    ]

    def feature(
        fid: str,
        level_id: str,
        kind: str,
        x: int,
        y: int,
        width: int,
        height: int,
        blocks_los: bool,
        blocks_movement: bool,
        allows_los: bool,
        allows_movement: bool,
    ) -> dict[str, object]:
        return {
            "id": fid,
            "level_id": level_id,
            "kind": kind,
            "x": x,
            "y": y,
            "width": width,
            "height": height,
            "blocks_los": blocks_los,
            "blocks_movement": blocks_movement,
            "allows_los": allows_los,
            "allows_movement": allows_movement,
        }

    features = []
    building_regions = []
    for name, x, y, width, height, storeys in regions:
        rid = name
        rx, ry, rw, rh = world_rect(x, y, width, height)
        wall = 8
        door_width = min(54, max(32, rw // 5))
        door_x = rx + rw // 2 - door_width // 2
        door_y = ry + rh - wall
        building_regions.append({
            "id": rid,
            "storeys": storeys,
            "x": rx,
            "y": ry,
            "width": rw,
            "height": rh,
            "roof_level_id": "level_04_roof_access" if storeys >= 3 else "level_02_roofs_and_second_floor",
            "art_role": "reference_aligned_graphite_building_mass",
        })
        features.extend([
            feature(f"{rid}_north_wall", "level_01_ground", "wall", rx, ry, rw, wall, True, True, False, False),
            feature(f"{rid}_west_wall", "level_01_ground", "wall", rx, ry, wall, rh, True, True, False, False),
            feature(f"{rid}_east_wall", "level_01_ground", "wall", rx + rw - wall, ry, wall, rh, True, True, False, False),
            feature(f"{rid}_south_wall_left", "level_01_ground", "wall", rx, door_y, max(0, door_x - rx), wall, True, True, False, False),
            feature(f"{rid}_south_wall_right", "level_01_ground", "wall", door_x + door_width, door_y, max(0, rx + rw - (door_x + door_width)), wall, True, True, False, False),
            feature(f"{rid}_south_door", "level_01_ground", "door", door_x, door_y, door_width, wall, False, False, True, True),
            feature(f"{rid}_north_window_left", "level_01_ground", "window", rx + rw // 3 - 14, ry, 28, wall, False, True, True, False),
            feature(f"{rid}_north_window_right", "level_01_ground", "window", rx + (rw * 2) // 3 - 14, ry, 28, wall, False, True, True, False),
            feature(f"{rid}_east_window", "level_01_ground", "window", rx + rw - wall, ry + rh // 2 - 18, wall, 36, False, True, True, False),
        ])
        if storeys >= 2:
            features.append(feature(
                f"{rid}_stair",
                "level_01_ground",
                "stair",
                rx + rw - 30,
                ry + 18,
                18,
                28,
                False,
                False,
                True,
                True,
            ))

    return {
        "schema_version": 1,
        "id": f"{MAP_ID}_building_levels",
        "map_id": MAP_ID,
        "name": "Fallujah Southern Industrial Foothold 2004 Building Levels",
        "world_width_m": WORLD_M,
        "world_height_m": WORLD_M,
        "pixel_width": PIXELS,
        "pixel_height": PIXELS,
        "pixels_per_meter": PPM,
        "origin": "top_left",
        "art_style": "Mosul-style dark graphite line art generated from Fallujah OSM streets, 1:10,000 Al Fallujah military/image graphic context, and 2002/2004 satellite-derived urban texture; LOS walls, door gaps, windows, stairs, and roof access aligned to authored building masses; no baked static traffic vehicle ink",
        "source_map_root": f"assets/fallujah/source/maps/{MAP_ID}",
        "source_vector_base": "vector/fallujah_sinaa_industrial_osm_20260614.json",
        "max_storeys": 4,
        "level_count": len(levels),
        "feature_count": len(features),
        "building_region_count": len(building_regions),
        "levels": [
            {
                "id": level_id,
                "index": index,
                "elevation_m": elevation,
                "png": f"assets/fallujah/runtime/maps/{MAP_ID}/levels/{level_id}.png",
                "alpha": alpha,
                "blocks_los_default": False,
                "blocks_movement_default": False,
            }
            for level_id, index, elevation, alpha in levels
        ],
        "features": features,
        "building_regions": building_regions,
    }


def topology_manifest() -> dict[str, object]:
    nodes = [
        ("southern_approach", "street", "level_01_ground", "", "Southern Cordon Approach", 0, 830, 1600, 440, True),
        ("industrial_west_ground", "workshop", "level_01_ground", "industrial_warehouse_west", "West Industrial Warehouse", 120, 930, 310, 210, True),
        ("repair_yards_ground", "workshop", "level_01_ground", "repair_yards_east", "Repair Yards", 1030, 1030, 340, 250, True),
        ("district_office_ground", "office", "level_01_ground", "district_office", "District Office Ground", 970, 610, 240, 210, True),
        ("district_office_second", "office", "level_02_roofs_and_second_floor", "district_office", "District Office Second Floor", 970, 610, 240, 210, True),
        ("district_office_roof", "roof", "level_04_roof_access", "district_office", "District Office Roof", 970, 610, 240, 210, True),
        ("residential_north_ground", "shelter", "level_01_ground", "residential_row_north", "North Residential Row", 520, 250, 390, 260, True),
        ("residential_roofs", "roof", "level_02_roofs_and_second_floor", "residential_row_north", "Residential Roofs", 520, 250, 390, 260, True),
        ("mosque_school_ground", "mosque", "level_01_ground", "mosque_school_edge", "Mosque School Edge", 1130, 310, 250, 220, True),
        ("clinic_compound_ground", "shelter", "level_01_ground", "clinic_compound", "Clinic Compound", 250, 620, 220, 170, True),
        ("market_shops_ground", "shop", "level_01_ground", "market_shops", "Market Shops", 580, 790, 320, 180, True),
        ("workshop_watch_ground", "workshop", "level_01_ground", "workshop_roof_watch", "Workshop Watch Ground", 1350, 760, 170, 210, True),
        ("workshop_watch_roof", "roof", "level_04_roof_access", "workshop_roof_watch", "Workshop Watch Roof", 1350, 760, 170, 210, True),
        ("east_alley", "alley", "level_01_ground", "", "East Alley Network", 1240, 700, 330, 420, True),
    ]
    portals = [
        ("approach_to_west_warehouse", "door", "open", "southern_approach", "industrial_west_ground", "level_01_ground", "industrial_warehouse_west_south_door", 260, 1132, 46, 16, True, False, 1),
        ("approach_to_repair_yards", "door", "open", "southern_approach", "repair_yards_ground", "level_01_ground", "repair_yards_east_south_door", 1180, 1272, 46, 16, True, False, 1),
        ("approach_to_market", "street_crossing", "open", "southern_approach", "market_shops_ground", "level_01_ground", "", 700, 940, 120, 20, True, False, 1),
        ("approach_to_district_office", "door", "open", "southern_approach", "district_office_ground", "level_01_ground", "district_office_south_door", 1065, 812, 46, 16, True, False, 1),
        ("approach_to_clinic", "street_crossing", "open", "southern_approach", "clinic_compound_ground", "level_01_ground", "", 310, 782, 80, 18, True, False, 1),
        ("approach_to_east_alley", "street_crossing", "open", "southern_approach", "east_alley", "level_01_ground", "", 1245, 895, 100, 20, True, False, 1),
        ("east_alley_to_workshop", "door", "open", "east_alley", "workshop_watch_ground", "level_01_ground", "workshop_roof_watch_south_door", 1420, 962, 42, 16, True, False, 1),
        ("east_alley_to_mosque_school", "street_crossing", "open", "east_alley", "mosque_school_ground", "level_01_ground", "", 1210, 520, 60, 18, True, False, 1),
        ("market_to_residential", "street_crossing", "open", "market_shops_ground", "residential_north_ground", "level_01_ground", "", 655, 510, 55, 95, True, False, 2),
        ("district_office_ground_to_second", "stair", "open", "district_office_ground", "district_office_second", "level_01_ground", "district_office_stair", 1180, 630, 18, 26, True, True, 2),
        ("district_office_second_to_roof", "stair", "open", "district_office_second", "district_office_roof", "level_02_roofs_and_second_floor", "", 1180, 630, 18, 26, True, True, 2),
        ("residential_ground_to_roofs", "stair", "open", "residential_north_ground", "residential_roofs", "level_01_ground", "residential_row_north_stair", 875, 270, 18, 26, True, True, 2),
        ("workshop_ground_to_roof", "stair", "open", "workshop_watch_ground", "workshop_watch_roof", "level_01_ground", "workshop_roof_watch_stair", 1490, 778, 18, 26, True, True, 2),
    ]
    zones = [
        ("warehouse_cache_search", "cache", "industrial_west_ground", "level_01_ground", 240, 1010, 90, 60, 70),
        ("repair_yard_breach", "search_objective", "repair_yards_ground", "level_01_ground", 1160, 1170, 80, 55, 55),
        ("district_office_search", "cache", "district_office_ground", "level_01_ground", 1030, 690, 90, 70, 60),
        ("district_rooftop_access", "overwatch_roof", "district_office_roof", "level_04_roof_access", 1040, 640, 90, 70, 80),
        ("residential_shelter", "civilian_shelter", "residential_north_ground", "level_01_ground", 610, 320, 150, 90, 95),
        ("mosque_school_sensitive", "civilian_shelter", "mosque_school_ground", "level_01_ground", 1190, 365, 120, 85, 100),
        ("clinic_civilian_shelter", "civilian_shelter", "clinic_compound_ground", "level_01_ground", 300, 675, 90, 70, 100),
        ("market_contact_area", "danger_area", "market_shops_ground", "level_01_ground", 650, 835, 120, 70, 65),
        ("workshop_rooftop_threat", "overwatch_roof", "workshop_watch_roof", "level_04_roof_access", 1400, 815, 90, 90, 85),
    ]

    def rect_entry(x: float, y: float, width: float, height: float) -> dict[str, int]:
        rx, ry, rw, rh = world_rect(x, y, width, height)
        return {"x": rx, "y": ry, "width": rw, "height": rh}

    return {
        "schema_version": 1,
        "id": f"{MAP_ID}_topology",
        "map_id": MAP_ID,
        "gameplay_area_id": f"{MAP_ID}_building_levels",
        "name": "Fallujah Southern Industrial Foothold 2004 Tactical Topology",
        "source_alignment": "Nodes and portals remain on the authored 1.6 km tactical slice, now drawn over the OSM-derived Sina’a / Industrial District street grid and reference-derived graphite city fabric.",
        "node_count": len(nodes),
        "portal_count": len(portals),
        "semantic_zone_count": len(zones),
        "nodes": [
            {
                "id": node_id,
                "kind": kind,
                "level_id": level_id,
                "region_id": region_id,
                "label": label,
                **rect_entry(x, y, width, height),
                "enterable": enterable,
            }
            for node_id, kind, level_id, region_id, label, x, y, width, height, enterable in nodes
        ],
        "portals": [
            {
                "id": portal_id,
                "kind": kind,
                "state": state,
                "from_node_id": from_node,
                "to_node_id": to_node,
                "level_id": level_id,
                "feature_id": feature_id,
                **rect_entry(x, y, width, height),
                "bidirectional": bidirectional,
                "vertical": vertical,
                "movement_cost": movement_cost,
            }
            for portal_id, kind, state, from_node, to_node, level_id, feature_id, x, y, width, height, bidirectional, vertical, movement_cost in portals
        ],
        "semantic_zones": [
            {
                "id": zone_id,
                "kind": kind,
                "node_id": node_id,
                "level_id": level_id,
                **rect_entry(x, y, width, height),
                "priority": priority,
            }
            for zone_id, kind, node_id, level_id, x, y, width, height, priority in zones
        ],
    }


def marker_manifest_text() -> str:
    source = ROOT / "assets" / "mosul" / "manifests" / "mosul_2003_markers.markermanifest"
    text = source.read_text(encoding="utf-8")
    return (
        text.replace("id=mosul_2003_markers", "id=fallujah_2004_markers")
        .replace("name=MOSUL 2003 Tactical Markers", "name=Fallujah 2004 Tactical Markers")
    )


def scenario_text() -> str:
    return """# Southern Industrial Foothold, Fallujah, April 2004
# First Battle of Fallujah playable patrol/contact slice.

format=modernerKrieg.scenario.v1
id=southern_industrial_foothold_2004
name=Southern Industrial Foothold 2004
seed=1157742004
briefing=Hold a southern industrial foothold during Operation Vigilant Resolve. Resolve contacts, protect civilians, and avoid turning the district into a larger escalation.
score.success_threshold=520
score.partial_threshold=180
score.objective_weight=110
score.civilian_risk_weight=12
score.player_casualty_weight=60
score.civilian_casualty_weight=120
score.time_weight=1
after_action.success=The cordon stabilized the industrial edge while preserving civilian safety and force cohesion.
after_action.partial=The foothold held, but risk, casualties, or unresolved contacts limited the result.
after_action.failure=The foothold failed to stabilize before the district became untenable.

asset.map_manifest=assets/fallujah/manifests/southern_industrial_foothold_2004.mapmanifest
asset.sprite_manifest=assets/shared/manifests/shared_tactical_sprites.spritemanifest
asset.building_level_manifest=assets/fallujah/manifests/southern_industrial_foothold_2004_building_levels.json
asset.topology_manifest=assets/fallujah/manifests/southern_industrial_foothold_2004_topology.json

map.name=Fallujah Southern Industrial Foothold
map.width_m=1600
map.height_m=1600
map.tile_columns=16
map.tile_rows=16
map.tile_width_m=100
map.tile_height_m=100
map.default_terrain=open

tile_range.count=5
tile_range.0.x=0
tile_range.0.y=8
tile_range.0.width=16
tile_range.0.height=2
tile_range.0.kind=road
tile_range.0.elevation=0
tile_range.0.cover=0
tile_range.0.movement_cost=1
tile_range.0.blocks_line_of_sight=false
tile_range.0.blocks_movement=false
tile_range.1.x=2
tile_range.1.y=0
tile_range.1.width=1
tile_range.1.height=16
tile_range.1.kind=road
tile_range.1.elevation=0
tile_range.1.cover=0
tile_range.1.movement_cost=1
tile_range.1.blocks_line_of_sight=false
tile_range.1.blocks_movement=false
tile_range.2.x=8
tile_range.2.y=0
tile_range.2.width=1
tile_range.2.height=16
tile_range.2.kind=road
tile_range.2.elevation=0
tile_range.2.cover=0
tile_range.2.movement_cost=1
tile_range.2.blocks_line_of_sight=false
tile_range.2.blocks_movement=false
tile_range.3.x=9
tile_range.3.y=6
tile_range.3.width=2
tile_range.3.height=3
tile_range.3.kind=building
tile_range.3.elevation=1
tile_range.3.cover=3
tile_range.3.movement_cost=2
tile_range.3.blocks_line_of_sight=true
tile_range.3.blocks_movement=false
tile_range.4.x=7
tile_range.4.y=9
tile_range.4.width=1
tile_range.4.height=1
tile_range.4.kind=rubble
tile_range.4.elevation=0
tile_range.4.cover=2
tile_range.4.movement_cost=3
tile_range.4.blocks_line_of_sight=true
tile_range.4.blocks_movement=false

terrain.count=8
terrain.0.name=Main Supply Route
terrain.0.kind=road
terrain.0.bounds=0,830,1600,120
terrain.0.cover=0
terrain.0.movement_cost=1
terrain.0.blocks_line_of_sight=false
terrain.1.name=Southern Cordon Road
terrain.1.kind=road
terrain.1.bounds=0,1165,1600,105
terrain.1.cover=0
terrain.1.movement_cost=1
terrain.1.blocks_line_of_sight=false
terrain.2.name=West Warehouse
terrain.2.kind=building
terrain.2.bounds=120,930,310,210
terrain.2.cover=3
terrain.2.movement_cost=2
terrain.2.blocks_line_of_sight=true
terrain.3.name=District Office
terrain.3.kind=building
terrain.3.bounds=970,610,240,210
terrain.3.cover=3
terrain.3.movement_cost=2
terrain.3.blocks_line_of_sight=true
terrain.4.name=Repair Yards
terrain.4.kind=building
terrain.4.bounds=1030,1030,340,250
terrain.4.cover=2
terrain.4.movement_cost=2
terrain.4.blocks_line_of_sight=true
terrain.5.name=North Residential Row
terrain.5.kind=building
terrain.5.bounds=520,250,390,260
terrain.5.cover=3
terrain.5.movement_cost=2
terrain.5.blocks_line_of_sight=true
terrain.6.name=Sensitive Mosque School Edge
terrain.6.kind=building
terrain.6.bounds=1130,310,250,220
terrain.6.cover=2
terrain.6.movement_cost=2
terrain.6.blocks_line_of_sight=true
terrain.7.name=Rubble Alley Cut
terrain.7.kind=rubble
terrain.7.bounds=735,930,90,55
terrain.7.cover=2
terrain.7.movement_cost=3
terrain.7.blocks_line_of_sight=true

controller.count=3
controller.0.name=Marine Patrol Tactical AI
controller.0.side=player
controller.0.kind=tactical_ai
controller.1.name=Fallujah Cell Tactical AI
controller.1.side=opfor
controller.1.kind=tactical_ai
controller.2.name=Civilian Observer
controller.2.side=civilian
controller.2.kind=observer

faction.count=3
faction.0.name=US Marine Foothold Patrol
faction.0.side=player
faction.0.color=80,132,170,255
faction.1.name=Fallujah Armed Cell
faction.1.side=opfor
faction.1.color=132,69,55,255
faction.2.name=Civilians
faction.2.side=civilian
faction.2.color=184,166,112,255

force.count=3
force.0.name=US Marine Patrol Force
force.0.side=player
force.0.faction_index=0
force.0.controller_index=0
force.0.command_name=US Marine Security Patrol
force.0.callsign=FOOTHOLD-1
force.1.name=Fallujah Armed Cell
force.1.side=opfor
force.1.faction_index=1
force.1.controller_index=1
force.1.command_name=Local Armed Cell
force.1.callsign=CELL-F
force.2.name=Civilian Population
force.2.side=civilian
force.2.faction_index=2
force.2.controller_index=2
force.2.command_name=Protected Noncombatants
force.2.callsign=CIV

spawn_zone.count=8
spawn_zone.0.id=marine_south_entry
spawn_zone.0.name=Marine South Entry
spawn_zone.0.kind=street_entry
spawn_zone.0.side=player
spawn_zone.0.level_id=level_01_ground
spawn_zone.0.topology_node_id=southern_approach
spawn_zone.0.bounds=120,1180,100,70
spawn_zone.0.capacity=2
spawn_zone.0.active=true
spawn_zone.1.id=warehouse_search
spawn_zone.1.name=Warehouse Search
spawn_zone.1.kind=cache_guard
spawn_zone.1.side=opfor
spawn_zone.1.level_id=level_01_ground
spawn_zone.1.topology_node_id=industrial_west_ground
spawn_zone.1.bounds=190,980,95,80
spawn_zone.1.capacity=2
spawn_zone.1.active=true
spawn_zone.2.id=district_roof_threat
spawn_zone.2.name=District Roof Threat
spawn_zone.2.kind=rooftop_threat
spawn_zone.2.side=opfor
spawn_zone.2.level_id=level_04_roof_access
spawn_zone.2.topology_node_id=district_office_roof
spawn_zone.2.bounds=1010,640,100,70
spawn_zone.2.capacity=1
spawn_zone.2.active=true
spawn_zone.3.id=workshop_roof_threat
spawn_zone.3.name=Workshop Roof Threat
spawn_zone.3.kind=rooftop_threat
spawn_zone.3.side=opfor
spawn_zone.3.level_id=level_04_roof_access
spawn_zone.3.topology_node_id=workshop_watch_roof
spawn_zone.3.bounds=1390,805,90,90
spawn_zone.3.capacity=1
spawn_zone.3.active=true
spawn_zone.4.id=residential_civilians
spawn_zone.4.name=Residential Civilians
spawn_zone.4.kind=civilian_shelter
spawn_zone.4.side=civilian
spawn_zone.4.level_id=level_01_ground
spawn_zone.4.topology_node_id=residential_north_ground
spawn_zone.4.bounds=610,320,150,90
spawn_zone.4.capacity=6
spawn_zone.4.active=true
spawn_zone.5.id=clinic_civilians
spawn_zone.5.name=Clinic Civilians
spawn_zone.5.kind=civilian_shelter
spawn_zone.5.side=civilian
spawn_zone.5.level_id=level_01_ground
spawn_zone.5.topology_node_id=clinic_compound_ground
spawn_zone.5.bounds=300,675,90,70
spawn_zone.5.capacity=4
spawn_zone.5.active=true
spawn_zone.6.id=market_contact
spawn_zone.6.name=Market Contact
spawn_zone.6.kind=hidden_scout
spawn_zone.6.side=neutral
spawn_zone.6.level_id=level_01_ground
spawn_zone.6.topology_node_id=market_shops_ground
spawn_zone.6.bounds=650,835,120,70
spawn_zone.6.capacity=2
spawn_zone.6.active=true
spawn_zone.7.id=mosque_school_civilians
spawn_zone.7.name=Mosque School Civilians
spawn_zone.7.kind=civilian_shelter
spawn_zone.7.side=civilian
spawn_zone.7.level_id=level_01_ground
spawn_zone.7.topology_node_id=mosque_school_ground
spawn_zone.7.bounds=1190,365,120,85
spawn_zone.7.capacity=2
spawn_zone.7.active=true

unit_template.count=5
unit_template.0.id=marine_patrol_element
unit_template.0.name=Marine Patrol Element
unit_template.0.role=patrol
unit_template.0.side=player
unit_template.0.training=veteran
unit_template.0.default_spawn_zone_id=marine_south_entry
unit_template.0.expected_soldiers=4
unit_template.1.id=civilian_cluster
unit_template.1.name=Civilian Cluster
unit_template.1.role=civilian_group
unit_template.1.side=civilian
unit_template.1.training=untrained
unit_template.1.default_spawn_zone_id=residential_civilians
unit_template.1.expected_soldiers=4
unit_template.2.id=hidden_cell
unit_template.2.name=Hidden Cell
unit_template.2.role=hidden_cell
unit_template.2.side=opfor
unit_template.2.training=regular
unit_template.2.default_spawn_zone_id=market_contact
unit_template.2.expected_soldiers=2
unit_template.3.id=rooftop_watcher
unit_template.3.name=Rooftop Watcher
unit_template.3.role=overwatch
unit_template.3.side=opfor
unit_template.3.training=regular
unit_template.3.default_spawn_zone_id=district_roof_threat
unit_template.3.expected_soldiers=1
unit_template.4.id=cache_guard
unit_template.4.name=Cache Guard
unit_template.4.role=cache_guard
unit_template.4.side=opfor
unit_template.4.training=regular
unit_template.4.default_spawn_zone_id=warehouse_search
unit_template.4.expected_soldiers=1

civilian_archetype.count=3
civilian_archetype.0.id=resident_adult
civilian_archetype.0.name=Resident Adult
civilian_archetype.0.sprite_id=civilian_adult_128_n
civilian_archetype.0.baseline_stress=3
civilian_archetype.0.baseline_risk=0
civilian_archetype.0.compliance=55
civilian_archetype.0.protected_noncombatant=true
civilian_archetype.1.id=clinic_patient
civilian_archetype.1.name=Clinic Patient
civilian_archetype.1.sprite_id=civilian_adult_128_n
civilian_archetype.1.baseline_stress=5
civilian_archetype.1.baseline_risk=1
civilian_archetype.1.compliance=40
civilian_archetype.1.protected_noncombatant=true
civilian_archetype.2.id=shopkeeper
civilian_archetype.2.name=Shopkeeper
civilian_archetype.2.sprite_id=civilian_adult_128_n
civilian_archetype.2.baseline_stress=2
civilian_archetype.2.baseline_risk=0
civilian_archetype.2.compliance=50
civilian_archetype.2.protected_noncombatant=true

civilian_group.count=3
civilian_group.0.id=residential_families
civilian_group.0.name=Residential Families
civilian_group.0.archetype_id=resident_adult
civilian_group.0.spawn_zone_id=residential_civilians
civilian_group.0.level_id=level_01_ground
civilian_group.0.topology_node_id=residential_north_ground
civilian_group.0.expected_count=3
civilian_group.0.baseline_stress=3
civilian_group.0.compliance=55
civilian_group.0.protected_noncombatants=true
civilian_group.1.id=clinic_group
civilian_group.1.name=Clinic Group
civilian_group.1.archetype_id=clinic_patient
civilian_group.1.spawn_zone_id=clinic_civilians
civilian_group.1.level_id=level_01_ground
civilian_group.1.topology_node_id=clinic_compound_ground
civilian_group.1.expected_count=2
civilian_group.1.baseline_stress=5
civilian_group.1.compliance=40
civilian_group.1.protected_noncombatants=true
civilian_group.2.id=market_shopkeepers
civilian_group.2.name=Market Shopkeepers
civilian_group.2.archetype_id=shopkeeper
civilian_group.2.spawn_zone_id=market_contact
civilian_group.2.level_id=level_01_ground
civilian_group.2.topology_node_id=market_shops_ground
civilian_group.2.expected_count=2
civilian_group.2.baseline_stress=2
civilian_group.2.compliance=50
civilian_group.2.protected_noncombatants=true

objective.count=2
objective.0.name=Hold Southern Foothold
objective.0.label=Foothold
objective.0.kind=control
objective.0.position=250,1195
objective.0.radius_m=70
objective.0.value=5
objective.1.name=Search Warehouse Cache
objective.1.label=Warehouse Cache
objective.1.kind=search
objective.1.position=245,1035
objective.1.radius_m=45
objective.1.value=3

weapon.count=4
weapon.0.name=M16A4
weapon.0.effective_range_m=360
weapon.0.shots_per_action=2
weapon.0.damage=35
weapon.0.suppression=8
weapon.1.name=AKM
weapon.1.effective_range_m=250
weapon.1.shots_per_action=2
weapon.1.damage=30
weapon.1.suppression=7
weapon.2.name=RPG-7
weapon.2.effective_range_m=200
weapon.2.shots_per_action=1
weapon.2.damage=80
weapon.2.suppression=12
weapon.3.name=Unarmed
weapon.3.effective_range_m=0
weapon.3.shots_per_action=0
weapon.3.damage=0
weapon.3.suppression=0

civilian.count=7
civilian.0.name=Residential Adult
civilian.0.archetype_id=resident_adult
civilian.0.group_id=residential_families
civilian.0.spawn_zone_id=residential_civilians
civilian.0.level_id=level_01_ground
civilian.0.topology_node_id=residential_north_ground
civilian.0.faction_index=2
civilian.0.position=640,350
civilian.0.state=sheltering
civilian.0.stress=3
civilian.0.risk=0
civilian.0.protected_noncombatant=true
civilian.0.compliance=55
civilian.1.name=Family Elder
civilian.1.archetype_id=resident_adult
civilian.1.group_id=residential_families
civilian.1.spawn_zone_id=residential_civilians
civilian.1.level_id=level_01_ground
civilian.1.topology_node_id=residential_north_ground
civilian.1.faction_index=2
civilian.1.position=705,382
civilian.1.state=sheltering
civilian.1.stress=4
civilian.1.risk=0
civilian.1.protected_noncombatant=true
civilian.1.compliance=60
civilian.2.name=Clinic Patient
civilian.2.archetype_id=clinic_patient
civilian.2.group_id=clinic_group
civilian.2.spawn_zone_id=clinic_civilians
civilian.2.level_id=level_01_ground
civilian.2.topology_node_id=clinic_compound_ground
civilian.2.faction_index=2
civilian.2.position=330,705
civilian.2.state=frozen
civilian.2.stress=6
civilian.2.risk=1
civilian.2.protected_noncombatant=true
civilian.2.compliance=35
civilian.3.name=Clinic Attendant
civilian.3.archetype_id=clinic_patient
civilian.3.group_id=clinic_group
civilian.3.spawn_zone_id=clinic_civilians
civilian.3.level_id=level_01_ground
civilian.3.topology_node_id=clinic_compound_ground
civilian.3.faction_index=2
civilian.3.position=375,715
civilian.3.state=sheltering
civilian.3.stress=5
civilian.3.risk=0
civilian.3.protected_noncombatant=true
civilian.3.compliance=40
civilian.4.name=Market Shopkeeper
civilian.4.archetype_id=shopkeeper
civilian.4.group_id=market_shopkeepers
civilian.4.spawn_zone_id=market_contact
civilian.4.level_id=level_01_ground
civilian.4.topology_node_id=market_shops_ground
civilian.4.faction_index=2
civilian.4.position=690,850
civilian.4.state=sheltering
civilian.4.stress=2
civilian.4.risk=0
civilian.4.protected_noncombatant=true
civilian.4.compliance=50
civilian.5.name=Market Bystander
civilian.5.archetype_id=shopkeeper
civilian.5.group_id=market_shopkeepers
civilian.5.spawn_zone_id=market_contact
civilian.5.level_id=level_01_ground
civilian.5.topology_node_id=market_shops_ground
civilian.5.faction_index=2
civilian.5.position=740,870
civilian.5.state=sheltering
civilian.5.stress=3
civilian.5.risk=0
civilian.5.protected_noncombatant=true
civilian.5.compliance=50
civilian.6.name=Mosque School Caretaker
civilian.6.archetype_id=resident_adult
civilian.6.group_id=residential_families
civilian.6.spawn_zone_id=mosque_school_civilians
civilian.6.level_id=level_01_ground
civilian.6.topology_node_id=mosque_school_ground
civilian.6.faction_index=2
civilian.6.position=1215,405
civilian.6.state=sheltering
civilian.6.stress=4
civilian.6.risk=0
civilian.6.protected_noncombatant=true
civilian.6.compliance=65

traffic_vehicle.count=0

unit.count=7
unit.0.name=Marine Patrol Element
unit.0.side=player
unit.0.training=veteran
unit.0.position=160,1195
unit.0.template_id=marine_patrol_element
unit.0.spawn_zone_id=marine_south_entry
unit.0.level_id=level_01_ground
unit.0.topology_node_id=southern_approach
unit.0.faction_index=0
unit.0.force_index=0
unit.0.controller_index=0
unit.0.command_name=Marine Patrol Element
unit.0.callsign=FOOTHOLD-1A
unit.0.soldier_count=4
unit.0.soldier.0.name=Patrol Lead
unit.0.soldier.0.role=leader
unit.0.soldier.0.weapon_index=0
unit.0.soldier.0.offset=-5,-4
unit.0.soldier.1.name=Rifleman
unit.0.soldier.1.role=rifleman
unit.0.soldier.1.weapon_index=0
unit.0.soldier.1.offset=5,-3
unit.0.soldier.2.name=Automatic Rifleman
unit.0.soldier.2.role=machine_gunner
unit.0.soldier.2.weapon_index=0
unit.0.soldier.2.offset=-2,5
unit.0.soldier.3.name=Corpsman
unit.0.soldier.3.role=medic
unit.0.soldier.3.weapon_index=0
unit.0.soldier.3.offset=6,6
unit.1.name=Warehouse Cell
unit.1.side=opfor
unit.1.training=regular
unit.1.position=260,1035
unit.1.template_id=cache_guard
unit.1.spawn_zone_id=warehouse_search
unit.1.level_id=level_01_ground
unit.1.topology_node_id=industrial_west_ground
unit.1.faction_index=1
unit.1.force_index=1
unit.1.controller_index=1
unit.1.command_name=Warehouse Cell
unit.1.callsign=CELL-F1
unit.1.hidden=true
unit.1.revealed=false
unit.1.concealment=22
unit.1.soldier_count=2
unit.1.soldier.0.name=Rifleman
unit.1.soldier.0.role=rifleman
unit.1.soldier.0.weapon_index=1
unit.1.soldier.0.offset=-4,0
unit.1.soldier.1.name=RPG Gunner
unit.1.soldier.1.role=rpg
unit.1.soldier.1.weapon_index=2
unit.1.soldier.1.offset=4,0
unit.2.name=District Roof Watcher
unit.2.side=opfor
unit.2.training=regular
unit.2.position=1065,680
unit.2.template_id=rooftop_watcher
unit.2.spawn_zone_id=district_roof_threat
unit.2.level_id=level_04_roof_access
unit.2.topology_node_id=district_office_roof
unit.2.faction_index=1
unit.2.force_index=1
unit.2.controller_index=1
unit.2.command_name=District Roof Watcher
unit.2.callsign=CELL-F2
unit.2.hidden=true
unit.2.revealed=false
unit.2.concealment=26
unit.2.soldier_count=1
unit.2.soldier.0.name=Rooftop Marksman
unit.2.soldier.0.role=marksman
unit.2.soldier.0.weapon_index=1
unit.2.soldier.0.offset=0,0
unit.3.name=Workshop Roof Watcher
unit.3.side=opfor
unit.3.training=regular
unit.3.position=1420,850
unit.3.template_id=rooftop_watcher
unit.3.spawn_zone_id=workshop_roof_threat
unit.3.level_id=level_04_roof_access
unit.3.topology_node_id=workshop_watch_roof
unit.3.faction_index=1
unit.3.force_index=1
unit.3.controller_index=1
unit.3.command_name=Workshop Roof Watcher
unit.3.callsign=CELL-F3
unit.3.hidden=true
unit.3.revealed=false
unit.3.concealment=24
unit.3.soldier_count=1
unit.3.soldier.0.name=Rooftop Rifleman
unit.3.soldier.0.role=rifleman
unit.3.soldier.0.weapon_index=1
unit.3.soldier.0.offset=0,0
unit.4.name=Market Scout
unit.4.side=opfor
unit.4.training=regular
unit.4.position=715,860
unit.4.template_id=hidden_cell
unit.4.spawn_zone_id=market_contact
unit.4.level_id=level_01_ground
unit.4.topology_node_id=market_shops_ground
unit.4.faction_index=1
unit.4.force_index=1
unit.4.controller_index=1
unit.4.command_name=Market Scout
unit.4.callsign=CELL-F4
unit.4.hidden=true
unit.4.revealed=false
unit.4.concealment=20
unit.4.soldier_count=1
unit.4.soldier.0.name=Scout Rifleman
unit.4.soldier.0.role=rifleman
unit.4.soldier.0.weapon_index=1
unit.4.soldier.0.offset=0,0
unit.5.name=Residential Civilians
unit.5.side=civilian
unit.5.training=untrained
unit.5.position=672,360
unit.5.template_id=civilian_cluster
unit.5.group_id=residential_families
unit.5.spawn_zone_id=residential_civilians
unit.5.level_id=level_01_ground
unit.5.topology_node_id=residential_north_ground
unit.5.faction_index=2
unit.5.force_index=2
unit.5.controller_index=2
unit.5.command_name=Residential Civilians
unit.5.callsign=CIV-F1
unit.5.soldier_count=3
unit.5.soldier.0.name=Resident Adult
unit.5.soldier.0.role=civilian
unit.5.soldier.0.weapon_index=3
unit.5.soldier.0.offset=0,0
unit.5.soldier.0.ammo=0
unit.5.soldier.1.name=Family Elder
unit.5.soldier.1.role=civilian
unit.5.soldier.1.weapon_index=3
unit.5.soldier.1.offset=12,8
unit.5.soldier.1.ammo=0
unit.5.soldier.2.name=Family Member
unit.5.soldier.2.role=civilian
unit.5.soldier.2.weapon_index=3
unit.5.soldier.2.offset=-14,5
unit.5.soldier.2.ammo=0
unit.6.name=Clinic Civilians
unit.6.side=civilian
unit.6.training=untrained
unit.6.position=352,710
unit.6.template_id=civilian_cluster
unit.6.group_id=clinic_group
unit.6.spawn_zone_id=clinic_civilians
unit.6.level_id=level_01_ground
unit.6.topology_node_id=clinic_compound_ground
unit.6.faction_index=2
unit.6.force_index=2
unit.6.controller_index=2
unit.6.command_name=Clinic Civilians
unit.6.callsign=CIV-F2
unit.6.soldier_count=2
unit.6.soldier.0.name=Clinic Patient
unit.6.soldier.0.role=civilian
unit.6.soldier.0.weapon_index=3
unit.6.soldier.0.offset=0,0
unit.6.soldier.0.ammo=0
unit.6.soldier.1.name=Clinic Attendant
unit.6.soldier.1.role=civilian
unit.6.soldier.1.weapon_index=3
unit.6.soldier.1.offset=12,6
unit.6.soldier.1.ammo=0
"""



def main() -> None:
    runtime = ASSET_ROOT / "runtime" / "maps" / MAP_ID
    levels = runtime / "levels"
    manifests = ASSET_ROOT / "manifests"
    regions = BUILDING_REGIONS

    ground = draw_ground()
    level_2 = draw_overlay(regions, 2)
    level_3 = draw_overlay(regions, 3)
    roof_access = draw_overlay(regions, 3)
    mask = draw_multistorey_mask(regions)
    multistorey_overview = compose_over(compose_over(ground, level_2), roof_access)

    write_png(SOURCE_MAP_DATA / "01_ground_level.png", PIXELS, PIXELS, ground)
    write_png(SOURCE_MAP_DATA / "02_level_2_alpha.png", PIXELS, PIXELS, level_2)
    write_png(SOURCE_MAP_DATA / "03_level_3_alpha.png", PIXELS, PIXELS, level_3)
    write_png(SOURCE_MAP_DATA / "04_roof_access_alpha.png", PIXELS, PIXELS, roof_access)
    write_png(SOURCE_MAP_DATA / "05_multistorey_mask.png", PIXELS, PIXELS, mask)
    write_png(SOURCE_MAP_DATA / "preview_multistorey_mask_1400.png", PREVIEW_PIXELS, PREVIEW_PIXELS, resize_nearest(mask, PIXELS, PIXELS, PREVIEW_PIXELS))

    write_png(SOURCE_IMG_DIR / "00_minimap_composite.png", PIXELS, PIXELS, ground)
    write_png(SOURCE_IMG_DIR / "06_multistorey_lineart_overview.png", PIXELS, PIXELS, multistorey_overview)
    write_png(SOURCE_IMG_DIR / "preview_1400.png", PREVIEW_PIXELS, PREVIEW_PIXELS, resize_nearest(ground, PIXELS, PIXELS, PREVIEW_PIXELS))
    write_png(SOURCE_IMG_DIR / "preview_multistorey_lineart_1400.png", PREVIEW_PIXELS, PREVIEW_PIXELS, resize_nearest(multistorey_overview, PIXELS, PIXELS, PREVIEW_PIXELS))
    (SOURCE_IMG_DIR / "map_layers.json").write_text(json.dumps(map_layers_json(), indent=2) + "\n", encoding="utf-8")

    write_png(levels / "level_01_ground.png", PIXELS, PIXELS, ground)
    write_png(runtime / "overview.png", PIXELS, PIXELS, ground)
    write_png(levels / "level_02_roofs_and_second_floor.png", PIXELS, PIXELS, level_2)
    write_png(levels / "level_03_upper_floor.png", PIXELS, PIXELS, level_3)
    write_png(levels / "level_04_roof_access.png", PIXELS, PIXELS, roof_access)

    manifests.mkdir(parents=True, exist_ok=True)
    (manifests / f"{MAP_ID}.mapmanifest").write_text(map_manifest_text(), encoding="utf-8")
    (manifests / f"{MAP_ID}_building_levels.json").write_text(
        json.dumps(building_manifest(regions), indent=2) + "\n",
        encoding="utf-8",
    )
    (manifests / f"{MAP_ID}_topology.json").write_text(
        json.dumps(topology_manifest(), indent=2) + "\n",
        encoding="utf-8",
    )
    (manifests / "fallujah_2004_markers.markermanifest").write_text(marker_manifest_text(), encoding="utf-8")

    SCENARIO_ROOT.mkdir(parents=True, exist_ok=True)
    (SCENARIO_ROOT / f"{MAP_ID}.mkscenario").write_text(scenario_text(), encoding="utf-8")


if __name__ == "__main__":
    main()
