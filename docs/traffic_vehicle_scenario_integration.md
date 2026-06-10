# Traffic Vehicle Scenario Integration

Cycle 5 validates the Market / Commercial Streets 2003 traffic vehicles as
scenario records. These are runtime entities that can move, block movement, and
carry units. The current demo also declares parked/static vehicle records along
the active moving-traffic lanes so moving traffic cannot drive through visible
vehicle ink before the final vehicle-free `7000 x 7000` map rerender is
available.

## Live Demo Records

The default Mosul 2003 demo scenario declares twenty-six traffic vehicles:

- six vehicles start with destinations and demonstrate runtime movement
- twenty parked vehicles start without destinations and act as visible blocking
  obstacles until commanded or otherwise scripted

| ID | Type | Sprite ID | Start | Destination | Speed | Facing | Seats | Boarding | Blocks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `north_market_bus` | bus | `traffic_city_bus_intact_north` | `242,382` | `242,232` | 5.5 m/tick | 270 deg | 24 | inside | yes |
| `south_market_bus` | bus | `traffic_city_bus_intact_north` | `258,472` | `258,318` | 5.0 m/tick | 270 deg | 24 | inside | yes |
| `market_west_car` | car | `traffic_civilian_car_intact_north` | `226,234` | `166,234` | 7.5 m/tick | 180 deg | 4 | inside | yes |
| `market_east_car` | car | `traffic_civilian_car_intact_north` | `322,236` | `406,236` | 7.0 m/tick | 0 deg | 4 | inside | yes |
| `souq_motorcycle` | motorcycle | `traffic_motorcycle_intact_north` | `350,252` | `284,252` | 10.0 m/tick | 180 deg | 1 | on | yes |
| `east_shopfront_car` | car | `traffic_civilian_car_intact_north` | `438,238` | `386,238` | 6.5 m/tick | 180 deg | 4 | inside | yes |

The parked records cover the central avenue, south avenue, west/east market
street, souq lane, east shopfront, west entry, north avenue, and courtyard
vehicle marks that affect the current demo movement lanes. They are active,
blocking, seat-aware records on
`level_01_ground`, but omit `destination`, so the engine treats them as static
at scenario start.

## Loader Contract

The Mosul scenario loader reads the following required or demo-required fields
for each `traffic_vehicle.*` record:

- stable scenario ID and display name
- vehicle kind and runtime sprite ID
- level ID, start position, optional destination, and destination level ID
- speed in meters per tick and facing in degrees
- seat capacity and occupied seats
- boarding mode, active state, and movement blocking state

The C core receives those values through `mk_scenario_add_traffic_vehicle`,
then copies them into `mk_game_t` during `mk_game_load_scenario`. The cycle 5
regression test checks both the raw scenario-file load and the public default
scenario/game-load path.

## Validation

Run:

```sh
build/bin/mk_mosul_scenario_data_tests
```

That test fails if any default traffic vehicle is missing one of the movement,
boarding, seating, active, or blocking fields required by cycle 5, or if the
parked/static records lose their no-destination start state.
