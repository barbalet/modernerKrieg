#!/usr/bin/env python3
"""Run a headless Mosul pass and verify dynamic traffic vehicle replay records."""

from __future__ import annotations

import argparse
import math
import shlex
import subprocess
import sys
from pathlib import Path


EXPECTED_SPRITES = {
    "traffic_civilian_car_intact_north": "car",
    "traffic_city_bus_intact_north": "bus",
    "traffic_motorcycle_intact_north": "motorcycle",
}
EXPECTED_BOARDING = {"inside", "on"}
REQUIRED_TRAFFIC_FIELDS = {
    "id",
    "scenario",
    "name",
    "vehicle_kind",
    "boarding",
    "sprite",
    "node",
    "level",
    "x",
    "y",
    "dest_x",
    "dest_y",
    "has_destination",
    "has_route",
    "route_step",
    "route_steps",
    "route_cost",
    "route_failures",
    "route_reason",
    "facing",
    "seats",
    "occupied",
    "active",
    "blocks_movement",
}


def _parse_line(line: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for token in shlex.split(line):
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        fields[key] = value
    return fields


def _require_int(fields: dict[str, str], key: str) -> int:
    try:
        return int(fields[key])
    except (KeyError, ValueError) as exc:
        raise ValueError(f"field {key} is not an integer") from exc


def _require_float(fields: dict[str, str], key: str) -> float:
    try:
        return float(fields[key])
    except (KeyError, ValueError) as exc:
        raise ValueError(f"field {key} is not a number") from exc


def _run_headless(headless: Path, replay: Path, steps: int, scenario: str | None) -> None:
    command = [
        str(headless),
        "--steps",
        str(steps),
        "--quiet",
        "--ai-only",
        "--replay",
        str(replay),
    ]
    if scenario:
        command[1:1] = ["--scenario", scenario]

    replay.parent.mkdir(parents=True, exist_ok=True)
    if replay.exists():
        replay.unlink()

    subprocess.run(command, check=True)
    if not replay.exists():
        raise ValueError(f"headless run did not create replay {replay}")


def _validate_replay(
    replay: Path,
    steps: int,
    expect_traffic_vehicles: int,
    expect_min_moving: int,
    expect_min_static: int,
) -> tuple[int, int, int, int]:
    expected_traffic_vehicles = 0
    gameplay_area_seen = False
    records_by_tick: dict[int, list[dict[str, str]]] = {}
    positions_by_vehicle: dict[int, list[tuple[int, float, float]]] = {}
    first_records_by_vehicle: dict[int, dict[str, str]] = {}

    for line_number, raw_line in enumerate(replay.read_text(encoding="utf-8").splitlines(), start=1):
        if not raw_line.startswith("event "):
            continue

        fields = _parse_line(raw_line)
        kind = fields.get("kind")
        if kind == "start":
            expected_traffic_vehicles = _require_int(fields, "traffic_vehicles")
        elif kind == "gameplay_area":
            pixel_width = _require_int(fields, "pixel_width")
            pixel_height = _require_int(fields, "pixel_height")
            if pixel_width != 7000 or pixel_height != 7000:
                raise ValueError(f"line {line_number}: expected 7000 x 7000 gameplay area")
            if fields.get("tactical_products") != "derived":
                raise ValueError(f"line {line_number}: gameplay area did not report derived tactical products")
            gameplay_area_seen = True
        elif kind == "traffic_vehicle":
            missing = REQUIRED_TRAFFIC_FIELDS.difference(fields)
            if missing:
                raise ValueError(f"line {line_number}: traffic vehicle record missing {sorted(missing)}")

            tick = _require_int(fields, "tick")
            vehicle_id = _require_int(fields, "id")
            x = _require_float(fields, "x")
            y = _require_float(fields, "y")
            seats = _require_int(fields, "seats")
            occupied = _require_int(fields, "occupied")
            sprite = fields["sprite"]
            vehicle_kind = fields["vehicle_kind"]
            boarding = fields["boarding"]

            if sprite not in EXPECTED_SPRITES:
                raise ValueError(f"line {line_number}: unexpected traffic sprite {sprite}")
            if EXPECTED_SPRITES[sprite] != vehicle_kind:
                raise ValueError(f"line {line_number}: sprite {sprite} does not match {vehicle_kind}")
            if boarding not in EXPECTED_BOARDING:
                raise ValueError(f"line {line_number}: unexpected boarding mode {boarding}")
            if seats <= 0 or occupied < 0 or occupied > seats:
                raise ValueError(f"line {line_number}: invalid occupancy {occupied}/{seats}")
            if fields["active"] != "1" or fields["blocks_movement"] != "1":
                raise ValueError(f"line {line_number}: dynamic traffic vehicle is not active/blocking")

            records_by_tick.setdefault(tick, []).append(fields)
            positions_by_vehicle.setdefault(vehicle_id, []).append((tick, x, y))
            first_records_by_vehicle.setdefault(vehicle_id, fields)

    if expected_traffic_vehicles != expect_traffic_vehicles:
        raise ValueError(
            f"expected {expect_traffic_vehicles} traffic vehicles, replay reported {expected_traffic_vehicles}"
        )
    if not gameplay_area_seen:
        raise ValueError("replay did not include the gameplay area metadata record")

    for tick in range(1, steps + 1):
        tick_records = records_by_tick.get(tick, [])
        if len(tick_records) != expected_traffic_vehicles:
            raise ValueError(
                f"tick {tick} has {len(tick_records)} traffic vehicle records, "
                f"expected {expected_traffic_vehicles}"
            )

    moving = 0
    for positions in positions_by_vehicle.values():
        if len(positions) < 2:
            continue
        _first_tick, first_x, first_y = positions[0]
        _last_tick, last_x, last_y = positions[-1]
        if math.hypot(last_x - first_x, last_y - first_y) >= 1.0:
            moving += 1

    if moving < expect_min_moving:
        raise ValueError(f"expected at least {expect_min_moving} moving vehicles, observed {moving}")

    static = sum(
        1
        for fields in first_records_by_vehicle.values()
        if fields["has_destination"] == "0" and fields["has_route"] == "0"
    )
    if static < expect_min_static:
        raise ValueError(f"expected at least {expect_min_static} static vehicles, observed {static}")

    return expected_traffic_vehicles, len(records_by_tick), moving, static


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate dynamic traffic vehicle runtime smoke replay")
    parser.add_argument("--headless", required=True, help="path to mk_headless_run")
    parser.add_argument("--replay", required=True, help="replay path to create and inspect")
    parser.add_argument("--scenario", default=None, help="optional scenario path")
    parser.add_argument("--steps", type=int, default=12, help="headless tick count")
    parser.add_argument("--expect-traffic-vehicles", type=int, default=26)
    parser.add_argument("--expect-min-moving", type=int, default=3)
    parser.add_argument("--expect-min-static", type=int, default=20)
    args = parser.parse_args()

    try:
        replay = Path(args.replay)
        _run_headless(Path(args.headless), replay, args.steps, args.scenario)
        vehicle_count, tick_count, moving_count, static_count = _validate_replay(
            replay,
            args.steps,
            args.expect_traffic_vehicles,
            args.expect_min_moving,
            args.expect_min_static,
        )
    except (OSError, subprocess.CalledProcessError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    print("traffic vehicle runtime smoke passed")
    print(
        f"steps={args.steps} traffic_vehicles={vehicle_count} "
        f"ticks_with_records={tick_count} moving_vehicles={moving_count} "
        f"static_vehicles={static_count}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
