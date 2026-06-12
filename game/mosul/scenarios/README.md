# Mosul Scenarios

Scenario files for playable MOSUL slices belong here.

The first public target is the 2003 Market / Commercial Streets demo.

Current scenarios:

- `market_commercial_streets_2003.mkscenario`: validated key/value scenario data for the first public demo slice, including first-pass breach/search, cache/search, and rooftop/stair interaction terrain.
- `market_commercial_streets_clear_roads_2003.mkscenario`: full demo slice with static traffic removed from the main roads while preserving moving traffic and courtyard parked cars.
- `market_commercial_streets_nonblocking_static_roads_2003.mkscenario`: full demo slice with static main-road traffic left visible but made non-blocking for movement.
- `market_control_smoke_2003.mkscenario`: compact AI-only objective-control and score-balance smoke scenario.
- `market_contested_risk_smoke_2003.mkscenario`: compact AI-only contested-objective and civilian-risk smoke scenario.

Scenario files reference map and sprite manifests under `assets/mosul/manifests/` and load into the portable C core through `mk_mosul_load_scenario_file`.
