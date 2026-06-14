# Traffic Vehicle Renderer And Interaction

Cycle 7 connects dynamic Mosul traffic vehicles to the demo-facing renderer and
input API. Cars, buses, and motorcycles remain scenario/runtime entities; the
base map does not get new vehicle marks added by the renderer.

## Renderer Contract

Traffic vehicle draw commands use only runtime RGBA sprite PNGs under:

- `assets/shared/runtime/sprites/rendered/traffic_vehicles_1024/`

The demo layer rejects traffic vehicle sprite IDs that cannot be mapped to an
approved runtime PNG. It does not fall back to source-angle art, source
directories, manifest IDs, or temporary placeholders for live draw commands.

Each `MK_DEMO_DRAW_TRAFFIC_VEHICLE` command exposes:

- runtime asset path
- traffic vehicle kind: car, bus, or motorcycle
- boarding mode: inside or on
- seat capacity and occupied seat count
- movement blocker state
- selected state
- position, screen position, facing, and pick radius

## Interaction Contract

Traffic vehicles are pickable through `mk_demo_session_pick_screen` and become
selected through `mk_demo_session_select_screen`. A selected traffic vehicle is
tracked separately from the selected unit so a frontend can select a unit, then
select a bus/car/motorcycle, and board the selected unit into or onto it.

The demo API now exposes explicit controls:

- `mk_demo_session_board_unit_traffic_vehicle`
- `mk_demo_session_unboard_unit_traffic_vehicle`
- `mk_demo_session_board_selected_unit_to_selected_traffic_vehicle`
- `mk_demo_session_issue_selected_traffic_vehicle_move_screen`

Units boarded inside cars and buses are suppressed from the live draw command
stream while embarked, because the vehicle sprite represents the visible body.
Units mounted on motorcycles remain a renderer concern for a later polish pass.

## Test Coverage

`mk_demo_session_tests` verifies that traffic vehicles:

- draw from runtime RGBA paths and not source paths
- expose kind, boarding mode, seat data, and movement blocking
- can be picked and selected
- preserve the selected unit while selecting a vehicle for boarding
- hide inside-boarded units from draw commands
- can unboard units and receive selected-vehicle move orders
