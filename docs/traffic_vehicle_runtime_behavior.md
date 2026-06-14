# Traffic Vehicle Runtime Behavior

Cycle 6 turns the demo traffic vehicle records into verified runtime entities.
The C core now treats active traffic vehicles with `blocks_movement=true` as
dynamic movement blockers, emits them in replay files, and keeps boarding state
in snapshots.

## Verified Runtime Contract

- Traffic vehicles can receive deterministic move orders.
- Same-level moves may fall back to direct movement when topology is not needed.
- Cross-level vehicle move orders require a valid topology route.
- If a required topology route is blocked, the vehicle keeps no destination and
  records the route failure reason.
- Boarded units inherit vehicle position, target position, facing, level, node,
  and hold order while embarked.
- Unboarded units are left at the current vehicle position and level.
- Cars and buses use `boarding_mode=inside`; motorcycles use `boarding_mode=on`.
- Seat capacity is enforced, and boarding a unit into a new traffic vehicle
  removes it from any previous one.
- Active blocking traffic vehicles stop units and other traffic vehicles from
  stepping through their runtime footprint.
- `mk_game_snapshot` copies traffic vehicle movement, route, seat, active, and
  blocking state.
- Headless replay files emit one `traffic_vehicle` event per vehicle per tick,
  and `mk_replay_validate` validates those records.

## Regression Coverage

Run:

```sh
build/bin/mk_core_tests
build/bin/mk_headless_run --steps 3 --quiet --replay build/cycle6_traffic.mkreplay
build/bin/mk_replay_validate build/cycle6_traffic.mkreplay
```

The core tests cover route success, route failure, deterministic stepping,
occupant synchronization, entering and exiting a bus, mounting a motorcycle,
capacity rejection, vehicle-to-vehicle transfer, snapshot copying, unit
blocking, and vehicle collision blocking.
