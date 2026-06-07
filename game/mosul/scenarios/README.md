# Mosul Scenarios

Scenario files for playable MOSUL slices belong here.

The first public target is the 2003 Market / Commercial Streets demo.

Current scenarios:

- `market_commercial_streets_2003.mkscenario`: validated key/value scenario data for the first public demo slice.
- `market_control_smoke_2003.mkscenario`: compact AI-only objective-control and score-balance smoke scenario.

Scenario files reference map and sprite manifests under `assets/mosul/manifests/` and load into the portable C core through `mk_mosul_load_scenario_file`.
