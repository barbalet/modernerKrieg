#include "mk_core.h"
#include "mk_log.h"
#include "mk_mosul_demo.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static float abs_float(float value) {
    return value < 0.0f ? -value : value;
}

static void assert_close(float actual, float expected) {
    assert(abs_float(actual - expected) < 0.01f);
}

static mk_vec2_t make_vec2(float x, float y) {
    mk_vec2_t value;

    value.x = x;
    value.y = y;

    return value;
}

static mk_rect_t make_rect(float x, float y, float width, float height) {
    mk_rect_t value;

    value.x = x;
    value.y = y;
    value.width = width;
    value.height = height;

    return value;
}

static mk_gameplay_area_t make_test_gameplay_area(void) {
    mk_gameplay_area_t area;

    memset(&area, 0, sizeof(area));
    area.loaded = true;
    area.schema_version = 1;
    strcpy(area.id, "test_area");
    strcpy(area.map_id, "test_map");
    strcpy(area.name, "Test Gameplay Area");
    area.world_width_m = 100.0f;
    area.world_height_m = 100.0f;
    area.pixel_width = 1000;
    area.pixel_height = 1000;
    area.pixels_per_meter = 10.0f;
    strcpy(area.origin, "top_left");
    area.max_storeys = 2;
    area.level_count = 2;
    strcpy(area.levels[0].id, "ground");
    area.levels[0].index = 1;
    area.levels[0].elevation_m = 0.0f;
    strcpy(area.levels[0].image_path, "assets/test/ground.png");
    strcpy(area.levels[0].alpha, "opaque");
    strcpy(area.levels[1].id, "roof");
    area.levels[1].index = 2;
    area.levels[1].elevation_m = 4.0f;
    strcpy(area.levels[1].image_path, "assets/test/roof.png");
    strcpy(area.levels[1].alpha, "transparent");

    area.feature_count = 4;
    strcpy(area.features[0].id, "west_wall");
    strcpy(area.features[0].level_id, "ground");
    strcpy(area.features[0].kind, "wall");
    area.features[0].pixel_x = 100;
    area.features[0].pixel_y = 200;
    area.features[0].pixel_width = 20;
    area.features[0].pixel_height = 80;
    area.features[0].bounds_m = make_rect(10.0f, 20.0f, 2.0f, 8.0f);
    area.features[0].blocks_los = true;
    area.features[0].blocks_movement = true;

    strcpy(area.features[1].id, "west_door");
    strcpy(area.features[1].level_id, "ground");
    strcpy(area.features[1].kind, "door");
    area.features[1].pixel_x = 100;
    area.features[1].pixel_y = 220;
    area.features[1].pixel_width = 20;
    area.features[1].pixel_height = 10;
    area.features[1].bounds_m = make_rect(10.0f, 22.0f, 2.0f, 1.0f);
    area.features[1].allows_los = true;
    area.features[1].allows_movement = true;

    strcpy(area.features[2].id, "west_window");
    strcpy(area.features[2].level_id, "ground");
    strcpy(area.features[2].kind, "window");
    area.features[2].pixel_x = 100;
    area.features[2].pixel_y = 250;
    area.features[2].pixel_width = 20;
    area.features[2].pixel_height = 10;
    area.features[2].bounds_m = make_rect(10.0f, 25.0f, 2.0f, 1.0f);
    area.features[2].blocks_movement = true;
    area.features[2].allows_los = true;

    strcpy(area.features[3].id, "roof_stair");
    strcpy(area.features[3].level_id, "ground");
    strcpy(area.features[3].kind, "stair");
    area.features[3].pixel_x = 145;
    area.features[3].pixel_y = 205;
    area.features[3].pixel_width = 20;
    area.features[3].pixel_height = 20;
    area.features[3].bounds_m = make_rect(14.5f, 20.5f, 2.0f, 2.0f);
    area.features[3].allows_los = true;
    area.features[3].allows_movement = true;

    area.region_count = 1;
    strcpy(area.regions[0].id, "shop_row");
    area.regions[0].storeys = 2;
    area.regions[0].pixel_x = 80;
    area.regions[0].pixel_y = 180;
    area.regions[0].pixel_width = 120;
    area.regions[0].pixel_height = 120;
    area.regions[0].bounds_m = make_rect(8.0f, 18.0f, 12.0f, 12.0f);
    strcpy(area.regions[0].roof_level_id, "roof");

    area.topology_loaded = true;
    area.topology_schema_version = 1;
    strcpy(area.topology_id, "test_topology");
    area.topology_node_count = 3;
    strcpy(area.topology_nodes[0].id, "street");
    strcpy(area.topology_nodes[0].kind, "street");
    strcpy(area.topology_nodes[0].level_id, "ground");
    strcpy(area.topology_nodes[0].region_id, "");
    strcpy(area.topology_nodes[0].label, "Street");
    area.topology_nodes[0].pixel_x = 0;
    area.topology_nodes[0].pixel_y = 0;
    area.topology_nodes[0].pixel_width = 80;
    area.topology_nodes[0].pixel_height = 80;
    area.topology_nodes[0].bounds_m = make_rect(0.0f, 0.0f, 8.0f, 8.0f);
    area.topology_nodes[0].enterable = true;

    strcpy(area.topology_nodes[1].id, "shop");
    strcpy(area.topology_nodes[1].kind, "shop");
    strcpy(area.topology_nodes[1].level_id, "ground");
    strcpy(area.topology_nodes[1].region_id, "shop_row");
    strcpy(area.topology_nodes[1].label, "Shop");
    area.topology_nodes[1].pixel_x = 80;
    area.topology_nodes[1].pixel_y = 180;
    area.topology_nodes[1].pixel_width = 120;
    area.topology_nodes[1].pixel_height = 120;
    area.topology_nodes[1].bounds_m = make_rect(8.0f, 18.0f, 12.0f, 12.0f);
    area.topology_nodes[1].enterable = true;

    strcpy(area.topology_nodes[2].id, "shop_roof");
    strcpy(area.topology_nodes[2].kind, "roof");
    strcpy(area.topology_nodes[2].level_id, "roof");
    strcpy(area.topology_nodes[2].region_id, "shop_row");
    strcpy(area.topology_nodes[2].label, "Shop Roof");
    area.topology_nodes[2].pixel_x = 80;
    area.topology_nodes[2].pixel_y = 180;
    area.topology_nodes[2].pixel_width = 120;
    area.topology_nodes[2].pixel_height = 120;
    area.topology_nodes[2].bounds_m = make_rect(8.0f, 18.0f, 12.0f, 12.0f);
    area.topology_nodes[2].enterable = true;

    area.topology_portal_count = 2;
    strcpy(area.topology_portals[0].id, "street_to_shop");
    strcpy(area.topology_portals[0].kind, "door");
    strcpy(area.topology_portals[0].state, "open");
    strcpy(area.topology_portals[0].from_node_id, "street");
    strcpy(area.topology_portals[0].to_node_id, "shop");
    strcpy(area.topology_portals[0].level_id, "ground");
    strcpy(area.topology_portals[0].feature_id, "west_door");
    area.topology_portals[0].pixel_x = 100;
    area.topology_portals[0].pixel_y = 220;
    area.topology_portals[0].pixel_width = 20;
    area.topology_portals[0].pixel_height = 10;
    area.topology_portals[0].bounds_m = make_rect(10.0f, 22.0f, 2.0f, 1.0f);
    area.topology_portals[0].bidirectional = true;
    area.topology_portals[0].vertical = false;
    area.topology_portals[0].movement_cost = 1;

    strcpy(area.topology_portals[1].id, "shop_to_roof");
    strcpy(area.topology_portals[1].kind, "stair");
    strcpy(area.topology_portals[1].state, "open");
    strcpy(area.topology_portals[1].from_node_id, "shop");
    strcpy(area.topology_portals[1].to_node_id, "shop_roof");
    strcpy(area.topology_portals[1].level_id, "ground");
    strcpy(area.topology_portals[1].feature_id, "roof_stair");
    area.topology_portals[1].pixel_x = 145;
    area.topology_portals[1].pixel_y = 205;
    area.topology_portals[1].pixel_width = 20;
    area.topology_portals[1].pixel_height = 20;
    area.topology_portals[1].bounds_m = make_rect(14.5f, 20.5f, 2.0f, 2.0f);
    area.topology_portals[1].bidirectional = true;
    area.topology_portals[1].vertical = true;
    area.topology_portals[1].movement_cost = 2;

    area.semantic_zone_count = 4;
    strcpy(area.semantic_zones[0].id, "shop_shelter");
    strcpy(area.semantic_zones[0].kind, "civilian_shelter");
    strcpy(area.semantic_zones[0].node_id, "shop");
    strcpy(area.semantic_zones[0].level_id, "ground");
    area.semantic_zones[0].pixel_x = 90;
    area.semantic_zones[0].pixel_y = 190;
    area.semantic_zones[0].pixel_width = 60;
    area.semantic_zones[0].pixel_height = 60;
    area.semantic_zones[0].bounds_m = make_rect(9.0f, 19.0f, 6.0f, 6.0f);
    area.semantic_zones[0].priority = 2;
    strcpy(area.semantic_zones[1].id, "street_exit");
    strcpy(area.semantic_zones[1].kind, "evacuation_exit");
    strcpy(area.semantic_zones[1].node_id, "street");
    strcpy(area.semantic_zones[1].level_id, "ground");
    area.semantic_zones[1].pixel_x = 10;
    area.semantic_zones[1].pixel_y = 10;
    area.semantic_zones[1].pixel_width = 50;
    area.semantic_zones[1].pixel_height = 50;
    area.semantic_zones[1].bounds_m = make_rect(1.0f, 1.0f, 5.0f, 5.0f);
    area.semantic_zones[1].priority = 3;
    strcpy(area.semantic_zones[2].id, "shop_cache");
    strcpy(area.semantic_zones[2].kind, "cache");
    strcpy(area.semantic_zones[2].node_id, "shop");
    strcpy(area.semantic_zones[2].level_id, "ground");
    area.semantic_zones[2].pixel_x = 160;
    area.semantic_zones[2].pixel_y = 220;
    area.semantic_zones[2].pixel_width = 20;
    area.semantic_zones[2].pixel_height = 20;
    area.semantic_zones[2].bounds_m = make_rect(16.0f, 22.0f, 2.0f, 2.0f);
    area.semantic_zones[2].priority = 4;
    strcpy(area.semantic_zones[3].id, "street_danger");
    strcpy(area.semantic_zones[3].kind, "danger_area");
    strcpy(area.semantic_zones[3].node_id, "street");
    strcpy(area.semantic_zones[3].level_id, "ground");
    area.semantic_zones[3].pixel_x = 62;
    area.semantic_zones[3].pixel_y = 10;
    area.semantic_zones[3].pixel_width = 10;
    area.semantic_zones[3].pixel_height = 10;
    area.semantic_zones[3].bounds_m = make_rect(6.2f, 1.0f, 1.0f, 1.0f);
    area.semantic_zones[3].priority = 4;

    return area;
}

static mk_scenario_definition_t make_east_mosul_block_scenario_fixture(void) {
    static bool cached = false;
    static mk_scenario_definition_t scenario;

    if (!cached) {
        assert(mk_mosul_make_east_block_scenario(&scenario) == MK_OK);
        cached = true;
    }

    return scenario;
}

static mk_scenario_definition_t make_traffic_vehicle_route_scenario(void) {
    mk_scenario_definition_t scenario;
    mk_gameplay_area_t area = make_test_gameplay_area();
    mk_weapon_profile_t rifle;
    mk_soldier_t soldier;
    mk_unit_t unit;
    mk_traffic_vehicle_t vehicle;

    memset(&scenario, 0, sizeof(scenario));
    strcpy(scenario.name, "Traffic Vehicle Route Scenario");
    scenario.seed = 66;
    scenario.map = mk_make_map("Traffic Route Map", 100.0f, 100.0f);
    assert(mk_scenario_set_gameplay_area(&scenario, &area) == MK_OK);

    rifle = mk_make_weapon("M4", 300, 2, 35, 8);

    unit = mk_make_unit("Route Passenger One", MK_SIDE_PLAYER, MK_TRAINING_REGULAR, mk_vec2(5.0f, 5.0f));
    soldier = mk_make_soldier("Rifleman One", MK_ROLE_RIFLEMAN, rifle);
    assert(mk_unit_add_soldier(&unit, &soldier, NULL) == MK_OK);
    assert(mk_scenario_add_unit(&scenario, &unit, NULL) == MK_OK);

    unit = mk_make_unit("Route Passenger Two", MK_SIDE_PLAYER, MK_TRAINING_REGULAR, mk_vec2(6.0f, 5.0f));
    soldier = mk_make_soldier("Rifleman Two", MK_ROLE_RIFLEMAN, rifle);
    assert(mk_unit_add_soldier(&unit, &soldier, NULL) == MK_OK);
    assert(mk_scenario_add_unit(&scenario, &unit, NULL) == MK_OK);

    unit = mk_make_unit("Route Passenger Three", MK_SIDE_PLAYER, MK_TRAINING_REGULAR, mk_vec2(7.0f, 5.0f));
    soldier = mk_make_soldier("Rifleman Three", MK_ROLE_RIFLEMAN, rifle);
    assert(mk_unit_add_soldier(&unit, &soldier, NULL) == MK_OK);
    assert(mk_scenario_add_unit(&scenario, &unit, NULL) == MK_OK);

    vehicle = mk_make_traffic_vehicle("route_bus", "Route Bus", MK_TRAFFIC_VEHICLE_BUS, mk_vec2(5.0f, 5.0f));
    strcpy(vehicle.level_id, "ground");
    assert(mk_scenario_add_traffic_vehicle(&scenario, &vehicle, NULL) == MK_OK);

    vehicle = mk_make_traffic_vehicle(
        "route_motorcycle",
        "Route Motorcycle",
        MK_TRAFFIC_VEHICLE_MOTORCYCLE,
        mk_vec2(30.0f, 5.0f)
    );
    strcpy(vehicle.level_id, "ground");
    assert(mk_scenario_add_traffic_vehicle(&scenario, &vehicle, NULL) == MK_OK);

    return scenario;
}

static mk_scenario_definition_t make_traffic_vehicle_blocking_scenario(void) {
    mk_scenario_definition_t scenario;
    mk_weapon_profile_t rifle;
    mk_soldier_t soldier;
    mk_unit_t unit;
    mk_traffic_vehicle_t vehicle;

    memset(&scenario, 0, sizeof(scenario));
    strcpy(scenario.name, "Traffic Vehicle Blocking Scenario");
    scenario.seed = 77;
    scenario.map = mk_make_map("Traffic Blocking Map", 100.0f, 100.0f);

    rifle = mk_make_weapon("M4", 300, 2, 35, 8);
    unit = mk_make_unit("Blocked Team", MK_SIDE_PLAYER, MK_TRAINING_REGULAR, mk_vec2(5.0f, 5.0f));
    soldier = mk_make_soldier("Blocked Rifleman", MK_ROLE_RIFLEMAN, rifle);
    assert(mk_unit_add_soldier(&unit, &soldier, NULL) == MK_OK);
    assert(mk_scenario_add_unit(&scenario, &unit, NULL) == MK_OK);

    vehicle = mk_make_traffic_vehicle("unit_blocker", "Unit Blocker", MK_TRAFFIC_VEHICLE_CAR, mk_vec2(14.0f, 5.0f));
    assert(mk_scenario_add_traffic_vehicle(&scenario, &vehicle, NULL) == MK_OK);

    vehicle = mk_make_traffic_vehicle("moving_car", "Moving Car", MK_TRAFFIC_VEHICLE_CAR, mk_vec2(5.0f, 20.0f));
    assert(mk_scenario_add_traffic_vehicle(&scenario, &vehicle, NULL) == MK_OK);

    vehicle = mk_make_traffic_vehicle("vehicle_blocker", "Vehicle Blocker", MK_TRAFFIC_VEHICLE_BUS, mk_vec2(16.0f, 20.0f));
    assert(mk_scenario_add_traffic_vehicle(&scenario, &vehicle, NULL) == MK_OK);

    return scenario;
}

static void test_version_is_present(void) {
    assert(strcmp(mk_version(), "0.1.0") == 0);
}

static void test_rng_is_deterministic(void) {
    mk_game_t first;
    mk_game_t second;
    int index;

    mk_game_init(&first, 42);
    mk_game_init(&second, 42);

    for (index = 0; index < 8; ++index) {
        assert(mk_random_u32(&first) == mk_random_u32(&second));
    }
}

static void test_result_names_are_stable(void) {
    assert(strcmp(mk_result_name(MK_OK), "MK_OK") == 0);
    assert(strcmp(mk_result_name(MK_ERROR_INVALID_ARGUMENT), "MK_ERROR_INVALID_ARGUMENT") == 0);
    assert(strcmp(mk_result_name(MK_ERROR_CAPACITY), "MK_ERROR_CAPACITY") == 0);
    assert(strcmp(mk_result_name(MK_ERROR_NOT_FOUND), "MK_ERROR_NOT_FOUND") == 0);
    assert(strcmp(mk_result_name(MK_ERROR_INVALID_DATA), "MK_ERROR_INVALID_DATA") == 0);
    assert(strcmp(mk_result_name((mk_result_t)-999), "MK_ERROR_UNKNOWN") == 0);
    assert(strcmp(mk_log_level_name(MK_LOG_INFO), "info") == 0);
    assert(strcmp(mk_log_level_name(MK_LOG_ERROR), "error") == 0);
}

static void test_math_value_helpers(void) {
    mk_vec2_t first = mk_vec2(3.0f, 4.0f);
    mk_vec2_t second = mk_vec2(0.0f, 0.0f);
    mk_ivec2_t tile = mk_ivec2(7, -2);
    mk_rect_t rect = mk_rect(2.0f, 3.0f, 10.0f, 8.0f);

    assert_close(first.x, 3.0f);
    assert_close(first.y, 4.0f);
    assert(tile.x == 7);
    assert(tile.y == -2);
    assert_close(rect.x, 2.0f);
    assert_close(rect.height, 8.0f);
    assert_close(mk_vec2_distance(first, second), 5.0f);
    assert(mk_clamp_i32(12, 0, 10) == 10);
    assert(mk_clamp_i32(-2, 0, 10) == 0);
    assert(mk_clamp_i32(5, 10, 0) == 5);
    assert_close(mk_clamp_f32(12.0f, 0.0f, 10.0f), 10.0f);
    assert_close(mk_clamp_f32(-2.0f, 0.0f, 10.0f), 0.0f);
    assert_close(mk_clamp_f32(5.0f, 10.0f, 0.0f), 5.0f);
    assert(mk_rect_contains_point(rect, mk_vec2(4.0f, 5.0f)));
    assert(!mk_rect_contains_point(rect, mk_vec2(20.0f, 5.0f)));
}

static void test_gameplay_area_coordinate_and_blocker_queries(void) {
    mk_gameplay_area_t area = make_test_gameplay_area();
    mk_ivec2_t pixel;
    mk_vec2_t position;
    const mk_gameplay_level_t *level;
    const mk_gameplay_feature_t *feature;
    const mk_gameplay_region_t *region;
    const mk_gameplay_topology_node_t *node;
    const mk_gameplay_topology_portal_t *portal;
    const mk_gameplay_semantic_zone_t *zone;
    mk_gameplay_tactical_query_t tactical_query;
    mk_gameplay_los_trace_t gameplay_los;
    mk_gameplay_route_t route;
    mk_line_of_sight_t line_of_sight;
    int navigation_cost;
    int cover;
    char topology_dump[1024];
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_game_snapshot_t snapshot;

    assert(mk_gameplay_area_is_loaded(&area));
    assert(mk_gameplay_area_world_to_pixel(&area, mk_vec2(10.0f, 20.0f), &pixel) == MK_OK);
    assert(pixel.x == 100);
    assert(pixel.y == 200);
    assert(mk_gameplay_area_world_to_pixel(&area, mk_vec2(100.0f, 100.0f), &pixel) == MK_OK);
    assert(pixel.x == 999);
    assert(pixel.y == 999);
    assert(mk_gameplay_area_world_to_pixel(&area, mk_vec2(100.1f, 1.0f), &pixel) == MK_ERROR_INVALID_DATA);

    assert(mk_gameplay_area_pixel_to_world(&area, mk_ivec2(100, 200), &position) == MK_OK);
    assert_close(position.x, 10.0f);
    assert_close(position.y, 20.0f);
    assert(mk_gameplay_area_pixel_to_world(&area, mk_ivec2(1000, 0), &position) == MK_ERROR_INVALID_DATA);

    level = mk_gameplay_area_find_level(&area, "ground");
    assert(level != NULL);
    assert(strcmp(level->image_path, "assets/test/ground.png") == 0);

    feature = mk_gameplay_area_find_feature(&area, "west_wall");
    assert(feature != NULL);
    assert(mk_gameplay_area_feature_contains_pixel(feature, mk_ivec2(101, 201)));
    assert(mk_gameplay_area_feature_contains_world(&area, feature, mk_vec2(10.1f, 20.1f)));

    feature = mk_gameplay_area_find_feature_at_world(&area, "ground", mk_vec2(10.1f, 22.1f));
    assert(feature != NULL);
    assert(strcmp(feature->id, "west_door") == 0);

    region = mk_gameplay_area_find_region(&area, "shop_row");
    assert(region != NULL);
    assert(region->storeys == 2);
    assert(mk_gameplay_area_find_region_at_world(&area, mk_vec2(12.0f, 20.0f)) == region);

    assert(mk_gameplay_area_topology_is_loaded(&area));
    node = mk_gameplay_area_find_topology_node(&area, "shop");
    assert(node != NULL);
    assert(strcmp(node->region_id, "shop_row") == 0);
    assert(mk_gameplay_area_find_topology_node_at_world(&area, "ground", mk_vec2(9.0f, 19.0f)) == node);
    portal = mk_gameplay_area_find_topology_portal(&area, "street_to_shop");
    assert(portal != NULL);
    assert(strcmp(portal->feature_id, "west_door") == 0);
    zone = mk_gameplay_area_find_semantic_zone(&area, "shop_shelter");
    assert(zone != NULL);
    assert(zone->priority == 2);
    assert(mk_gameplay_area_find_semantic_zone_at_world(&area, "civilian_shelter", mk_vec2(10.0f, 20.0f)) == zone);
    assert(mk_gameplay_area_topology_debug_dump(&area, topology_dump, sizeof(topology_dump)) == MK_OK);
    assert(strstr(topology_dump, "topology id=\"test_topology\"") != NULL);
    assert(strstr(topology_dump, "unreachable=0") != NULL);

    assert(mk_gameplay_area_blocks_los_at(&area, "ground", mk_vec2(10.1f, 20.1f)));
    assert(mk_gameplay_area_blocks_movement_at(&area, "ground", mk_vec2(10.1f, 20.1f)));
    assert(!mk_gameplay_area_blocks_los_at(&area, "ground", mk_vec2(10.1f, 22.1f)));
    assert(!mk_gameplay_area_blocks_movement_at(&area, "ground", mk_vec2(10.1f, 22.1f)));
    assert(!mk_gameplay_area_blocks_los_at(&area, "ground", mk_vec2(10.1f, 25.1f)));
    assert(mk_gameplay_area_blocks_movement_at(&area, "ground", mk_vec2(10.1f, 25.1f)));
    assert(!mk_gameplay_area_blocks_movement_at(&area, "missing", mk_vec2(10.1f, 20.1f)));

    assert(mk_gameplay_area_query_tactical_at(&area, "ground", mk_vec2(10.1f, 20.1f), &tactical_query) == MK_OK);
    assert(tactical_query.in_bounds);
    assert(tactical_query.blocks_los);
    assert(tactical_query.blocks_movement);
    assert(tactical_query.navigation_cost == MK_GAMEPLAY_BLOCKED_NAVIGATION_COST);
    assert(tactical_query.cover == 4);
    assert(strcmp(tactical_query.feature_id, "west_wall") == 0);
    assert(strcmp(tactical_query.node_id, "shop") == 0);
    assert(tactical_query.interior);

    assert(mk_gameplay_area_query_tactical_at(&area, "ground", mk_vec2(10.1f, 22.1f), &tactical_query) == MK_OK);
    assert(!tactical_query.blocks_los);
    assert(!tactical_query.blocks_movement);
    assert(tactical_query.navigation_cost > 1);
    assert(tactical_query.navigation_cost < MK_GAMEPLAY_BLOCKED_NAVIGATION_COST);
    assert(strcmp(tactical_query.feature_id, "west_door") == 0);
    assert(strcmp(tactical_query.portal_id, "street_to_shop") == 0);

    assert(mk_gameplay_area_query_tactical_at(&area, "ground", mk_vec2(10.1f, 25.1f), &tactical_query) == MK_OK);
    assert(!tactical_query.blocks_los);
    assert(tactical_query.blocks_movement);
    assert(tactical_query.cover == 3);
    assert(strcmp(tactical_query.feature_id, "west_window") == 0);

    assert(mk_gameplay_area_navigation_cost_at(&area, "ground", mk_vec2(10.1f, 22.1f), &navigation_cost) == MK_OK);
    assert(navigation_cost > 1);
    assert(navigation_cost < MK_GAMEPLAY_BLOCKED_NAVIGATION_COST);
    assert(mk_gameplay_area_cover_at(&area, "ground", mk_vec2(10.1f, 20.1f), &cover) == MK_OK);
    assert(cover == 4);

    assert(mk_gameplay_area_trace_line_of_sight(
        &area,
        "ground",
        mk_vec2(5.0f, 20.5f),
        mk_vec2(15.0f, 20.5f),
        &gameplay_los
    ) == MK_OK);
    assert(!gameplay_los.visible);
    assert(gameplay_los.blocked_by_feature);
    assert(strcmp(gameplay_los.blocking_feature_id, "west_wall") == 0);

    assert(mk_gameplay_area_trace_line_of_sight(
        &area,
        "ground",
        mk_vec2(5.0f, 22.5f),
        mk_vec2(15.0f, 22.5f),
        &gameplay_los
    ) == MK_OK);
    assert(gameplay_los.visible);
    assert(gameplay_los.cover >= 2);

    assert(mk_gameplay_area_trace_line_of_sight(
        &area,
        "ground",
        mk_vec2(5.0f, 25.5f),
        mk_vec2(15.0f, 25.5f),
        &gameplay_los
    ) == MK_OK);
    assert(gameplay_los.visible);

    assert(mk_gameplay_area_plan_route(
        &area,
        "ground",
        mk_vec2(5.0f, 5.0f),
        "ground",
        mk_vec2(12.0f, 20.0f),
        &route
    ) == MK_OK);
    assert(route.valid);
    assert(strcmp(route.start_node_id, "street") == 0);
    assert(strcmp(route.goal_node_id, "shop") == 0);
    assert(route.step_count == 2);
    assert(strcmp(route.steps[0].portal_id, "street_to_shop") == 0);
    assert(!route.uses_vertical_transition);

    assert(mk_gameplay_area_plan_route(
        &area,
        "ground",
        mk_vec2(12.0f, 20.0f),
        "roof",
        mk_vec2(12.0f, 20.0f),
        &route
    ) == MK_OK);
    assert(route.valid);
    assert(strcmp(route.goal_node_id, "shop_roof") == 0);
    assert(route.uses_vertical_transition);
    assert(route.step_count == 2);
    assert(strcmp(route.steps[0].portal_id, "shop_to_roof") == 0);
    assert(route.steps[0].vertical_transition);
    assert(strcmp(route.steps[0].level_id, "roof") == 0);

    area.topology_portals[0].state[0] = '\0';
    strcpy(area.topology_portals[0].state, "locked");
    assert(mk_gameplay_area_plan_route(
        &area,
        "ground",
        mk_vec2(5.0f, 5.0f),
        "ground",
        mk_vec2(12.0f, 20.0f),
        &route
    ) == MK_ERROR_NOT_FOUND);
    assert(strcmp(route.blocked_reason, "unreachable") == 0);
    area = make_test_gameplay_area();

    memset(&scenario, 0, sizeof(scenario));
    strcpy(scenario.name, "Gameplay Area Scenario");
    scenario.seed = 7;
    scenario.map = mk_make_map("Test Map", 100.0f, 100.0f);
    assert(mk_scenario_set_gameplay_area(&scenario, &area) == MK_OK);
    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(mk_gameplay_area_is_loaded(&game.gameplay_area));
    assert(game.gameplay_area.level_count == 2);
    assert(mk_game_trace_line_of_sight(&game, mk_vec2(5.0f, 20.5f), mk_vec2(15.0f, 20.5f), &line_of_sight) == MK_OK);
    assert(!line_of_sight.visible);
    assert(line_of_sight.blocked_by_gameplay_area);
    assert(strcmp(line_of_sight.blocking_feature_id, "west_wall") == 0);
    assert(mk_game_snapshot(&game, &snapshot) == MK_OK);
    assert(mk_gameplay_area_is_loaded(&snapshot.gameplay_area));
    assert(strcmp(snapshot.gameplay_area.features[1].id, "west_door") == 0);

    area.world_width_m = 101.0f;
    assert(mk_scenario_set_gameplay_area(&scenario, &area) == MK_ERROR_INVALID_DATA);

    area = make_test_gameplay_area();
    area.topology_portals[0].vertical = true;
    assert(mk_scenario_set_gameplay_area(&scenario, &area) == MK_ERROR_INVALID_DATA);
}

static void test_topology_route_following_moves_units_between_levels(void) {
    mk_gameplay_area_t area = make_test_gameplay_area();
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_unit_t unit;
    mk_soldier_t soldier;
    mk_weapon_profile_t rifle;
    mk_unit_t *stored_unit;
    uint32_t unit_id = 0;
    int tick;

    memset(&scenario, 0, sizeof(scenario));
    strcpy(scenario.name, "Route Scenario");
    scenario.seed = 8;
    scenario.map = mk_make_map("Route Map", 100.0f, 100.0f);
    assert(mk_scenario_set_gameplay_area(&scenario, &area) == MK_OK);

    rifle = mk_make_weapon("M4", 300, 2, 35, 8);
    unit = mk_make_unit("Route Team", MK_SIDE_PLAYER, MK_TRAINING_REGULAR, mk_vec2(5.0f, 5.0f));
    soldier = mk_make_soldier("Rifleman", MK_ROLE_RIFLEMAN, rifle);
    assert(mk_unit_add_soldier(&unit, &soldier, NULL) == MK_OK);
    assert(mk_scenario_add_unit(&scenario, &unit, &unit_id) == MK_OK);

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    stored_unit = mk_game_find_unit(&game, unit_id);
    assert(stored_unit != NULL);
    assert(strcmp(stored_unit->level_id, "ground") == 0);

    assert(mk_game_issue_move_order_to_level(&game, unit_id, "roof", mk_vec2(12.0f, 20.0f)) == MK_OK);
    assert(stored_unit->has_route);
    assert(stored_unit->route_step_count == 3);
    assert(stored_unit->route_total_cost > 0);
    assert(stored_unit->route_uses_vertical_transition);
    assert(strcmp(stored_unit->route_step_portal_ids[0], "street_to_shop") == 0);
    assert(strcmp(stored_unit->route_step_portal_ids[1], "shop_to_roof") == 0);

    for (tick = 0; tick < 12 && stored_unit->has_move_target; ++tick) {
        mk_game_step(&game);
    }

    assert(!stored_unit->has_move_target);
    assert(!stored_unit->has_route);
    assert(stored_unit->order == MK_ORDER_HOLD);
    assert(strcmp(stored_unit->level_id, "roof") == 0);
    assert(stored_unit->route_vertical_transitions_completed == 1);
    assert_close(stored_unit->position_m.x, 12.0f);
    assert_close(stored_unit->position_m.y, 20.0f);

    stored_unit->position_m = mk_vec2(5.0f, 5.0f);
    strcpy(stored_unit->level_id, "ground");
    area = game.gameplay_area;
    area.topology_portals[0].state[0] = '\0';
    strcpy(area.topology_portals[0].state, "locked");
    game.gameplay_area = area;
    assert(mk_game_issue_move_order_to_level(&game, unit_id, "roof", mk_vec2(12.0f, 20.0f)) == MK_ERROR_NOT_FOUND);
    assert(!stored_unit->has_route);
    assert(strcmp(stored_unit->route_failure_reason, "unreachable") == 0);
}

static void test_civilian_ai_and_instruction_routes_are_deterministic(void) {
    mk_gameplay_area_t area = make_test_gameplay_area();
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_civilian_t civilian;
    uint32_t civilian_id = 0;
    int tick;

    memset(&scenario, 0, sizeof(scenario));
    strcpy(scenario.name, "Civilian Route Scenario");
    scenario.seed = 11;
    scenario.map = mk_make_map("Civilian Route Map", 100.0f, 100.0f);
    assert(mk_scenario_set_gameplay_area(&scenario, &area) == MK_OK);

    civilian = mk_make_civilian("Shopper", 0, mk_vec2(5.0f, 5.0f));
    strcpy(civilian.level_id, "ground");
    strcpy(civilian.topology_node_id, "street");
    civilian.stress = 5;
    civilian.compliance = 70;
    assert(mk_scenario_add_civilian(&scenario, &civilian, &civilian_id) == MK_OK);
    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);

    assert(mk_game_update_civilian_ai(&game) == MK_OK);
    assert(game.civilians[0].intent == MK_CIVILIAN_INTENT_SHELTER);
    assert(game.civilians[0].has_destination);
    assert(game.civilians[0].has_route);
    assert(game.civilians[0].route_step_count == 2);

    for (tick = 0; tick < 20 && game.civilians[0].has_destination; ++tick) {
        mk_game_step(&game);
    }

    assert(!game.civilians[0].has_destination);
    assert(game.civilians[0].state == MK_CIVILIAN_SHELTERING);
    assert(strcmp(game.civilians[0].topology_node_id, "shop") == 0);

    assert(mk_game_issue_civilian_instruction(
        &game,
        civilian_id,
        MK_CIVILIAN_INTENT_EVACUATE,
        "ground",
        mk_vec2(3.0f, 3.0f)
    ) == MK_OK);
    assert(game.civilians[0].intent == MK_CIVILIAN_INTENT_EVACUATE);
    assert(game.civilians[0].has_route);

    for (tick = 0; tick < 20 && game.civilians[0].has_destination; ++tick) {
        mk_game_step(&game);
    }

    assert(game.civilians[0].state == MK_CIVILIAN_EVACUATED);
    assert(!game.civilians[0].has_destination);
    assert(strcmp(game.civilians[0].topology_node_id, "street") == 0);
}

static void test_search_semantic_zone_and_cache_terrain_records_results(void) {
    mk_gameplay_area_t area = make_test_gameplay_area();
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_unit_t searcher;
    mk_unit_t hidden;
    mk_soldier_t soldier;
    mk_weapon_profile_t rifle;
    mk_terrain_zone_t cache;
    mk_search_result_t search_result;
    mk_score_t score;
    uint32_t searcher_id = 0;
    uint32_t hidden_id = 0;
    uint32_t terrain_id = 0;

    memset(&scenario, 0, sizeof(scenario));
    strcpy(scenario.name, "Search Scenario");
    scenario.seed = 12;
    scenario.map = mk_make_map("Search Map", 100.0f, 100.0f);
    assert(mk_scenario_set_gameplay_area(&scenario, &area) == MK_OK);

    rifle = mk_make_weapon("M4", 300, 2, 35, 8);
    soldier = mk_make_soldier("Searcher", MK_ROLE_RIFLEMAN, rifle);
    searcher = mk_make_unit("Search Team", MK_SIDE_PLAYER, MK_TRAINING_REGULAR, mk_vec2(12.0f, 20.0f));
    strcpy(searcher.level_id, "ground");
    strcpy(searcher.topology_node_id, "shop");
    assert(mk_unit_add_soldier(&searcher, &soldier, NULL) == MK_OK);
    assert(mk_scenario_add_unit(&scenario, &searcher, &searcher_id) == MK_OK);

    hidden = mk_make_unit("Hidden Threat", MK_SIDE_OPFOR, MK_TRAINING_REGULAR, mk_vec2(13.0f, 21.0f));
    strcpy(hidden.level_id, "ground");
    strcpy(hidden.topology_node_id, "shop");
    hidden.hidden = true;
    hidden.revealed = false;
    assert(mk_unit_add_soldier(&hidden, &soldier, NULL) == MK_OK);
    assert(mk_scenario_add_unit(&scenario, &hidden, &hidden_id) == MK_OK);

    cache = mk_make_terrain_zone("Suspected Cache", MK_TERRAIN_SUSPECTED_IED, mk_rect(50.0f, 50.0f, 5.0f, 5.0f), 1, 2, false);
    assert(mk_map_add_terrain(&scenario.map, &cache, &terrain_id) == MK_OK);
    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);

    assert(mk_game_search_semantic_zone(&game, searcher_id, "shop_shelter", &search_result) == MK_OK);
    assert(search_result.outcome == MK_SEARCH_OUTCOME_THREAT_REVEALED);
    assert(search_result.revealed_unit_id == hidden_id);
    assert(search_result.contact_report_id != 0);
    assert(search_result.score_delta > 0);
    assert(game.units[1].revealed);
    assert(game.gameplay_area.semantic_zones[0].searched);
    assert(game.gameplay_area.semantic_zones[0].last_search_outcome == MK_SEARCH_OUTCOME_THREAT_REVEALED);

    assert(mk_game_search_semantic_zone(&game, searcher_id, "shop_cache", &search_result) == MK_OK);
    assert(search_result.outcome == MK_SEARCH_OUTCOME_CACHE_FOUND);
    assert(search_result.score_delta > 0);
    assert(game.gameplay_area.semantic_zones[2].searched);

    assert(mk_game_search_terrain(&game, searcher_id, terrain_id, &search_result) == MK_OK);
    assert(search_result.outcome == MK_SEARCH_OUTCOME_CACHE_FOUND);
    assert(search_result.terrain_id == terrain_id);
    assert(search_result.contact_report_id != 0);
    assert(search_result.score_delta > 0);
    assert(game.map.terrain[0].searched);

    assert(mk_game_search_semantic_zone(&game, searcher_id, "street_danger", &search_result) == MK_OK);
    assert(search_result.outcome == MK_SEARCH_OUTCOME_BOOBY_TRAP);
    assert(search_result.score_delta < 0);
    assert(game.gameplay_area.semantic_zones[3].searched);

    assert(mk_game_score(&game, &score) == MK_OK);
    assert(score.interaction_points > 0);

    assert(mk_game_search_semantic_zone(&game, searcher_id, "shop_cache", &search_result) == MK_OK);
    assert(search_result.outcome == MK_SEARCH_OUTCOME_CACHE_FOUND);
    assert(search_result.score_delta == 0);
}

static void test_breach_portal_opens_route_and_scores(void) {
    mk_gameplay_area_t area = make_test_gameplay_area();
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_unit_t breacher;
    mk_soldier_t soldier;
    mk_weapon_profile_t rifle;
    mk_breach_result_t breach_result;
    mk_score_t score;
    mk_gameplay_route_t route;
    const mk_gameplay_topology_portal_t *portal;
    uint32_t unit_id = 0;

    memset(&scenario, 0, sizeof(scenario));
    strcpy(scenario.name, "Breach Scenario");
    scenario.seed = 21;
    scenario.map = mk_make_map("Breach Map", 100.0f, 100.0f);
    assert(mk_scenario_set_gameplay_area(&scenario, &area) == MK_OK);

    rifle = mk_make_weapon("M4", 300, 2, 35, 8);
    soldier = mk_make_soldier("Engineer", MK_ROLE_ENGINEER, rifle);
    breacher = mk_make_unit("Breach Team", MK_SIDE_PLAYER, MK_TRAINING_REGULAR, mk_vec2(4.0f, 4.0f));
    strcpy(breacher.level_id, "ground");
    strcpy(breacher.topology_node_id, "street");
    assert(mk_unit_add_soldier(&breacher, &soldier, NULL) == MK_OK);
    assert(mk_scenario_add_unit(&scenario, &breacher, &unit_id) == MK_OK);
    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    strcpy(game.gameplay_area.topology_portals[0].state, "locked");

    assert(mk_gameplay_area_plan_route(
        &game.gameplay_area,
        "ground",
        mk_vec2(4.0f, 4.0f),
        "ground",
        mk_vec2(12.0f, 20.0f),
        &route
    ) == MK_ERROR_NOT_FOUND);
    assert(mk_game_breach_portal(&game, unit_id, "street_to_shop", &breach_result) == MK_OK);
    assert(breach_result.outcome == MK_BREACH_OUTCOME_BREACHED);
    assert(breach_result.score_delta > 0);
    assert(breach_result.contact_report_id != 0);
    assert(game.contact_reports[0].kind == MK_CONTACT_REPORT_BREACH);

    portal = mk_gameplay_area_find_topology_portal(&game.gameplay_area, "street_to_shop");
    assert(portal != NULL);
    assert(strcmp(portal->state, "breached") == 0);
    assert(portal->breached);
    assert(mk_gameplay_area_plan_route(
        &game.gameplay_area,
        "ground",
        mk_vec2(4.0f, 4.0f),
        "ground",
        mk_vec2(12.0f, 20.0f),
        &route
    ) == MK_OK);
    assert(route.valid);
    assert(strcmp(route.steps[0].portal_id, "street_to_shop") == 0);
    assert(mk_game_score(&game, &score) == MK_OK);
    assert(score.interaction_points == 20);
}

static void test_unit_and_soldier_creation(void) {
    mk_game_t game;
    mk_weapon_profile_t rifle;
    mk_unit_t unit;
    mk_soldier_t soldier;
    mk_vec2_t position;
    uint32_t unit_id = 0;
    uint32_t soldier_id = 0;
    mk_unit_t *stored_unit;

    mk_game_init(&game, 7);
    rifle = mk_make_weapon("M4", 300, 2, 35, 8);
    position = make_vec2(10.0f, 12.0f);
    unit = mk_make_unit("CTS Assault Element", MK_SIDE_PLAYER, MK_TRAINING_ELITE, position);
    soldier = mk_make_soldier("Rifleman", MK_ROLE_RIFLEMAN, rifle);

    assert(mk_unit_add_soldier(&unit, &soldier, &soldier_id) == MK_OK);
    assert(soldier_id == 1);
    assert(unit.soldier_count == 1);
    assert(strcmp(unit.soldiers[0].weapon.name, "M4") == 0);

    assert(mk_game_add_unit(&game, &unit, &unit_id) == MK_OK);
    assert(unit_id == 1);
    assert(game.unit_count == 1);

    stored_unit = mk_game_find_unit(&game, unit_id);
    assert(stored_unit != NULL);
    assert(strcmp(stored_unit->name, "CTS Assault Element") == 0);
    assert(stored_unit->soldier_count == 1);
    assert(stored_unit->soldiers[0].id == 1);
    assert(stored_unit->morale == 100);
    assert(stored_unit->communications_up);
    assert(stored_unit->soldiers[0].max_health == 100);
    assert(stored_unit->soldiers[0].ammo_capacity == 120);
    assert(stored_unit->soldiers[0].stance == MK_STANCE_STANDING);
    assert(stored_unit->soldiers[0].wound_state == MK_WOUND_NONE);
    assert(stored_unit->soldiers[0].can_move);
    assert(stored_unit->soldiers[0].weapon.fire_mode == MK_FIRE_MODE_BURST);
    assert(stored_unit->soldiers[0].weapon.ammo_kind == MK_AMMO_SMALL_ARMS);
}

static void test_map_tiles_are_configurable_and_addressable(void) {
    mk_map_t map = mk_make_map("Tile Map", 40.0f, 40.0f);
    mk_map_tile_t tile;
    mk_map_tile_t *stored_tile;

    assert(mk_map_configure_tiles(&map, 4, 4, 10.0f, 10.0f, MK_TERRAIN_OPEN) == MK_OK);
    assert(map.tile_columns == 4);
    assert(map.tile_rows == 4);
    assert(map.tile_count == 16);
    assert(map.tiles[0].id == 1);
    assert(map.tiles[15].id == 16);
    assert(map.tiles[15].coordinate.x == 3);
    assert(map.tiles[15].coordinate.y == 3);

    tile = mk_make_map_tile(mk_ivec2(2, 1), MK_TERRAIN_RUBBLE, 1, 2, 3, true, false);
    assert(mk_map_set_tile(&map, &tile) == MK_OK);
    stored_tile = mk_map_get_tile(&map, mk_ivec2(2, 1));
    assert(stored_tile != NULL);
    assert(stored_tile->id == 7);
    assert(stored_tile->kind == MK_TERRAIN_RUBBLE);
    assert(stored_tile->elevation == 1);
    assert(stored_tile->cover == 2);
    assert(stored_tile->movement_cost == 3);
    assert(stored_tile->blocks_line_of_sight);
    assert(mk_map_get_tile(&map, mk_ivec2(4, 1)) == NULL);
    assert(mk_map_configure_tiles(&map, 100, 100, 1.0f, 1.0f, MK_TERRAIN_OPEN) == MK_ERROR_CAPACITY);
}

static void test_capacity_limits_are_reported(void) {
    mk_vec2_t position = make_vec2(0.0f, 0.0f);
    mk_unit_t unit = mk_make_unit("Oversized Unit", MK_SIDE_PLAYER, MK_TRAINING_REGULAR, position);
    mk_weapon_profile_t rifle = mk_make_weapon("AKM", 250, 2, 30, 7);
    mk_soldier_t soldier = mk_make_soldier("Rifleman", MK_ROLE_RIFLEMAN, rifle);
    int index;

    for (index = 0; index < MK_MAX_SOLDIERS_PER_UNIT; ++index) {
        assert(mk_unit_add_soldier(&unit, &soldier, NULL) == MK_OK);
    }

    assert(mk_unit_add_soldier(&unit, &soldier, NULL) == MK_ERROR_CAPACITY);
}

static void test_step_recovers_suppression(void) {
    mk_game_t game;
    mk_unit_t unit;
    mk_soldier_t soldier;
    mk_weapon_profile_t rifle;
    mk_vec2_t position;
    uint32_t unit_id = 0;
    mk_unit_t *stored_unit;

    mk_game_init(&game, 99);
    rifle = mk_make_weapon("M4", 300, 2, 35, 8);
    soldier = mk_make_soldier("Team Leader", MK_ROLE_LEADER, rifle);
    soldier.suppression = 4;

    position = make_vec2(0.0f, 0.0f);
    unit = mk_make_unit("Suppressed Team", MK_SIDE_PLAYER, MK_TRAINING_VETERAN, position);
    unit.order = MK_ORDER_RALLY;
    unit.suppression = 5;
    assert(mk_unit_add_soldier(&unit, &soldier, NULL) == MK_OK);
    assert(mk_game_add_unit(&game, &unit, &unit_id) == MK_OK);

    mk_game_step(&game);
    stored_unit = mk_game_find_unit(&game, unit_id);

    assert(game.tick == 1);
    assert(stored_unit != NULL);
    assert(stored_unit->suppression == 1);
    assert(stored_unit->soldiers[0].suppression == 0);
}

static void test_scenario_loading_populates_core_state(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(strcmp(game.scenario_name, "Market Commercial Streets 2003") == 0);
    assert(strstr(game.briefing, "Secure the market junction") != NULL);
    assert(game.score_success_threshold == MK_DEFAULT_SCORE_SUCCESS_THRESHOLD);
    assert(game.score_partial_threshold == MK_DEFAULT_SCORE_PARTIAL_THRESHOLD);
    assert(game.score_objective_weight == MK_DEFAULT_SCORE_OBJECTIVE_WEIGHT);
    assert(game.score_civilian_risk_weight == MK_DEFAULT_SCORE_CIVILIAN_RISK_WEIGHT);
    assert(game.score_player_casualty_weight == MK_DEFAULT_SCORE_PLAYER_CASUALTY_WEIGHT);
    assert(game.score_civilian_casualty_weight == MK_DEFAULT_SCORE_CIVILIAN_CASUALTY_WEIGHT);
    assert(game.score_time_weight == MK_DEFAULT_SCORE_TIME_WEIGHT);
    assert(game.rng_state == scenario.seed);
    assert(strcmp(game.map.name, "Market / Commercial Streets") == 0);
    assert(game.map.tile_count == 100);
    assert(game.map.tile_columns == 10);
    assert(game.map.tile_rows == 10);
    assert(game.map.tiles[46].kind == MK_TERRAIN_BUILDING);
    assert(game.map.terrain_count == 6);
    assert(game.controller_count == 3);
    assert(game.faction_count == 3);
    assert(game.force_count == 3);
    assert(game.objective_count == 1);
    assert(strcmp(game.objectives[0].label, "Market Junction") == 0);
    assert(game.spawn_zone_count == 7);
    assert(game.unit_template_count == 5);
    assert(game.civilian_archetype_count == 4);
    assert(game.civilian_group_count == 4);
    assert(game.civilian_count == 7);
    assert(game.traffic_vehicle_count == 26);
    assert(game.unit_count == 6);
    assert(game.controllers[0].kind == MK_CONTROLLER_TACTICAL_AI);
    assert(game.controllers[2].kind == MK_CONTROLLER_OBSERVER);
    assert(game.forces[0].controller_id == 1);
    assert(strcmp(game.forces[0].command.callsign, "PATROL-1") == 0);
    assert(strcmp(game.spawn_zones[1].scenario_id, "market_stalls_crowd") == 0);
    assert(strcmp(game.unit_templates[3].scenario_id, "rooftop_watcher") == 0);
    assert(strcmp(game.civilian_archetypes[0].sprite_id, "civilian_adult_128_n") == 0);
    assert(strcmp(game.civilian_groups[0].topology_node_id, "market_stalls_ground") == 0);
    assert(game.civilians[0].protected_noncombatant);
    assert(game.civilians[0].state == MK_CIVILIAN_SHELTERING);
    assert(game.civilians[0].intent == MK_CIVILIAN_INTENT_NONE);
    assert_close(game.civilians[0].speed_m_per_tick, MK_DEFAULT_CIVILIAN_SPEED_M_PER_TICK);
    assert(strcmp(game.civilians[6].archetype_id, "wounded_bystander") == 0);
    assert(game.civilians[6].risk == 1);
    assert(strcmp(game.traffic_vehicles[0].scenario_id, "north_market_bus") == 0);
    assert(game.traffic_vehicles[0].seat_capacity == 24);
    assert(game.traffic_vehicles[0].boarding_mode == MK_TRAFFIC_BOARD_INSIDE);
    assert(strcmp(game.traffic_vehicles[4].scenario_id, "souq_motorcycle") == 0);
    assert(game.traffic_vehicles[4].boarding_mode == MK_TRAFFIC_BOARD_ON);
    assert(strcmp(game.traffic_vehicles[6].scenario_id, "central_avenue_static_car_north") == 0);
    assert(!game.traffic_vehicles[6].has_destination);
    assert(game.traffic_vehicles[6].blocks_movement);
    assert(game.units[0].faction_id == 1);
    assert(game.units[0].force_id == 1);
    assert(game.units[0].controller_id == 1);
    assert(strcmp(game.units[0].command.callsign, "PATROL-1A") == 0);
    assert(game.units[0].soldier_count == 3);
    assert(game.units[1].hidden);
    assert(!game.units[1].revealed);
    assert(game.units[1].concealment == 18);
    assert(game.units[1].soldiers[1].role == MK_ROLE_RPG);
    assert(game.units[2].side == MK_SIDE_CIVILIAN);
}

static void test_snapshot_is_stable_copy(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    mk_game_snapshot_t snapshot;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(mk_game_select_unit(&game, 1) == MK_OK);
    game.units[0].suppression = 6;
    game.units[0].soldiers[0].suppression = 3;
    mk_game_step(&game);

    assert(mk_game_snapshot(&game, &snapshot) == MK_OK);
    assert(snapshot.tick == 1);
    assert(snapshot.selected_unit_id == 1);
    assert(strcmp(snapshot.briefing, game.briefing) == 0);
    assert(snapshot.score_success_threshold == game.score_success_threshold);
    assert(snapshot.score_partial_threshold == game.score_partial_threshold);
    assert(snapshot.score_objective_weight == game.score_objective_weight);
    assert(snapshot.score_civilian_risk_weight == game.score_civilian_risk_weight);
    assert(snapshot.score_player_casualty_weight == game.score_player_casualty_weight);
    assert(snapshot.score_civilian_casualty_weight == game.score_civilian_casualty_weight);
    assert(snapshot.score_time_weight == game.score_time_weight);
    assert(snapshot.units[0].suppression == 4);
    assert(snapshot.units[0].soldiers[0].suppression == 1);
    assert(snapshot.map.terrain[1].blocks_line_of_sight);
    assert(snapshot.controller_count == 3);
    assert(snapshot.force_count == 3);
    assert(snapshot.spawn_zone_count == 7);
    assert(snapshot.unit_template_count == 5);
    assert(snapshot.civilian_archetype_count == 4);
    assert(snapshot.civilian_group_count == 4);
    assert(snapshot.civilian_count == 7);
    assert(snapshot.traffic_vehicle_count == 26);
    assert(strcmp(snapshot.traffic_vehicles[0].scenario_id, "north_market_bus") == 0);
    assert(snapshot.traffic_vehicles[0].active);
    assert(snapshot.traffic_vehicles[0].blocks_movement);
    assert(strcmp(snapshot.traffic_vehicles[6].scenario_id, "central_avenue_static_car_north") == 0);
    assert(!snapshot.traffic_vehicles[6].has_destination);
    assert(snapshot.map.tile_count == 100);
    assert(snapshot.controllers[1].side == MK_SIDE_OPFOR);
    assert(snapshot.forces[1].faction_id == 2);
    assert(snapshot.civilians[0].position_m.x == 252.0f);
    assert(strcmp(snapshot.civilians[6].spawn_zone_id, "market_stalls_crowd") == 0);
    assert(strcmp(snapshot.units[3].topology_node_id, "hotel_roof_access") == 0);
    assert(strcmp(snapshot.objectives[0].name, "Secure Market Junction") == 0);
    assert(strcmp(snapshot.objectives[0].label, "Market Junction") == 0);

    game.units[0].suppression = 99;
    game.controllers[0].kind = MK_CONTROLLER_HUMAN;
    game.civilians[0].stress = 99;
    game.traffic_vehicles[0].position_m.x = 999.0f;
    assert(snapshot.units[0].suppression == 4);
    assert(snapshot.controllers[0].kind == MK_CONTROLLER_TACTICAL_AI);
    assert(snapshot.civilians[0].stress == 0);
    assert(snapshot.traffic_vehicles[0].position_m.x != 999.0f);
}

static void test_pick_select_and_move_order(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    mk_unit_t *unit;
    uint32_t unit_id = 0;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(mk_game_pick_unit_at(&game, make_vec2(80.0f, 246.0f), MK_UNIT_PICK_RADIUS_M, &unit_id) == MK_OK);
    assert(unit_id == 1);
    assert(mk_game_select_unit_at(&game, make_vec2(80.0f, 246.0f), MK_UNIT_PICK_RADIUS_M, &unit_id) == MK_OK);
    assert(unit_id == 1);
    assert(game.selected_unit_id == 1);

    assert(mk_game_issue_selected_move_order(&game, make_vec2(92.0f, 246.0f)) == MK_OK);
    unit = mk_game_find_unit(&game, 1);
    assert(unit != NULL);
    assert(unit->order == MK_ORDER_MOVE);
    assert(unit->has_move_target);

    mk_game_step(&game);
    assert(unit->position_m.x == 86.0f);
    assert(unit->position_m.y == 246.0f);
    assert(unit->order == MK_ORDER_MOVE);

    mk_game_step(&game);
    assert(unit->position_m.x == 92.0f);
    assert(unit->position_m.y == 246.0f);
    assert(!unit->has_move_target);
    assert(unit->order == MK_ORDER_HOLD);
}

static void test_pick_contact_and_investigate_order(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    mk_unit_t *unit;
    uint32_t contact_report_id = 0;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    game.contact_report_count = 1;
    memset(&game.contact_reports[0], 0, sizeof(game.contact_reports[0]));
    game.contact_reports[0].id = 1;
    game.contact_reports[0].kind = MK_CONTACT_REPORT_SUSPECTED_DANGER;
    game.contact_reports[0].side = MK_SIDE_OPFOR;
    game.contact_reports[0].attacker_unit_id = 1;
    game.contact_reports[0].target_unit_id = 2;
    game.contact_reports[0].position_m = make_vec2(220.0f, 246.0f);
    game.contact_reports[0].target_position_m = game.contact_reports[0].position_m;
    game.contact_reports[0].confidence = 45;
    game.contact_reports[0].visible = true;

    assert(mk_game_pick_contact_at(&game, make_vec2(222.0f, 246.0f), 8.0f, &contact_report_id) == MK_OK);
    assert(contact_report_id == 1);
    assert(mk_game_pick_contact_at(&game, make_vec2(260.0f, 246.0f), 8.0f, &contact_report_id) == MK_ERROR_NOT_FOUND);
    assert(contact_report_id == 0);

    assert(mk_game_select_unit(&game, 1) == MK_OK);
    assert(mk_game_issue_selected_investigate_order(&game, game.contact_reports[0].position_m) == MK_OK);
    unit = mk_game_find_unit(&game, 1);
    assert(unit != NULL);
    assert(unit->order == MK_ORDER_INVESTIGATE);
    assert(unit->has_move_target);

    mk_game_step(&game);
    assert_close(unit->position_m.x, 83.6f);
    assert_close(unit->position_m.y, 246.0f);
    assert(unit->order == MK_ORDER_INVESTIGATE);
}

static void test_investigate_resolves_contact_reports(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    game.units[0].position_m = make_vec2(346.0f, 230.0f);
    game.contact_report_count = 1;
    memset(&game.contact_reports[0], 0, sizeof(game.contact_reports[0]));
    game.contact_reports[0].id = 1;
    game.contact_reports[0].kind = MK_CONTACT_REPORT_SUSPECTED_DANGER;
    game.contact_reports[0].side = MK_SIDE_OPFOR;
    game.contact_reports[0].attacker_unit_id = 1;
    game.contact_reports[0].target_unit_id = 2;
    game.contact_reports[0].position_m = game.units[1].position_m;
    game.contact_reports[0].target_position_m = game.units[1].position_m;
    game.contact_reports[0].confidence = 55;
    game.contact_reports[0].visible = true;
    assert(mk_game_issue_investigate_order(&game, 1, game.contact_reports[0].position_m) == MK_OK);

    mk_game_step(&game);
    assert(game.contact_reports[0].resolved);
    assert(game.units[1].revealed);
    {
        bool saw_reveal = false;
        size_t report_index;

        assert(game.contact_report_count >= 2);
        for (report_index = 0; report_index < game.contact_report_count; ++report_index) {
            if (game.contact_reports[report_index].kind == MK_CONTACT_REPORT_REVEAL
                && game.contact_reports[report_index].target_unit_id == 2) {
                saw_reveal = true;
            }
        }
        assert(saw_reveal);
    }

    game = (mk_game_t){0};
    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    game.units[0].position_m = make_vec2(218.0f, 330.0f);
    game.contact_report_count = 1;
    memset(&game.contact_reports[0], 0, sizeof(game.contact_reports[0]));
    game.contact_reports[0].id = 1;
    game.contact_reports[0].kind = MK_CONTACT_REPORT_FALSE_CONTACT;
    game.contact_reports[0].side = MK_SIDE_NEUTRAL;
    game.contact_reports[0].attacker_unit_id = 1;
    game.contact_reports[0].terrain_id = 3;
    game.contact_reports[0].position_m = game.units[0].position_m;
    game.contact_reports[0].target_position_m = game.units[0].position_m;
    game.contact_reports[0].confidence = 30;
    game.contact_reports[0].visible = true;
    assert(mk_game_issue_investigate_order(&game, 1, game.contact_reports[0].position_m) == MK_OK);

    mk_game_step(&game);
    assert(game.contact_reports[0].resolved);
    assert(game.contact_reports[0].confidence == 0);
    {
        bool saw_cache_false_contact = false;
        size_t report_index;

        assert(game.contact_report_count >= 2);
        for (report_index = 0; report_index < game.contact_report_count; ++report_index) {
            if (game.contact_reports[report_index].kind == MK_CONTACT_REPORT_FALSE_CONTACT
                && game.contact_reports[report_index].terrain_id == 6) {
                saw_cache_false_contact = true;
            }
        }
        assert(saw_cache_false_contact);
    }
}

static void test_interaction_errors_are_reported(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    uint32_t unit_id = 123;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(mk_game_pick_unit_at(&game, make_vec2(230.0f, 150.0f), 4.0f, &unit_id) == MK_ERROR_NOT_FOUND);
    assert(unit_id == 0);
    assert(mk_game_select_unit(&game, 99) == MK_ERROR_NOT_FOUND);
    assert(mk_game_issue_selected_move_order(&game, make_vec2(50.0f, 50.0f)) == MK_ERROR_NOT_FOUND);
    assert(mk_game_issue_selected_investigate_order(&game, make_vec2(50.0f, 50.0f)) == MK_ERROR_NOT_FOUND);
    assert(mk_game_select_unit(&game, 1) == MK_OK);
    assert(mk_game_issue_selected_move_order(&game, make_vec2(999.0f, 50.0f)) == MK_ERROR_INVALID_DATA);
    assert(mk_game_issue_selected_investigate_order(&game, make_vec2(999.0f, 50.0f)) == MK_ERROR_INVALID_DATA);
    assert(mk_game_clear_selection(&game) == MK_OK);
    assert(game.selected_unit_id == 0);
}

static void test_line_of_sight_reports_target_cover(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    mk_line_of_sight_t line_of_sight;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(mk_game_unit_line_of_sight(&game, 1, 2, &line_of_sight) == MK_OK);
    assert(line_of_sight.visible);
    assert(line_of_sight.blocking_terrain_id == 0);
    assert(line_of_sight.cover == 3);
    assert(line_of_sight.cover_terrain_id == 2);
    assert(line_of_sight.cover_terrain_kind == MK_TERRAIN_BUILDING);
    assert_close(line_of_sight.distance_m, 270.47f);
}

static void test_line_of_sight_reports_blocking_terrain(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    mk_terrain_zone_t wall;
    mk_line_of_sight_t line_of_sight;
    uint32_t wall_id = 0;

    wall = mk_make_terrain_zone(
        "Line-of-Sight Wall",
        MK_TERRAIN_OBSTACLE,
        make_rect(200.0f, 228.0f, 24.0f, 40.0f),
        4,
        4,
        true
    );
    assert(mk_map_add_terrain(&scenario.map, &wall, &wall_id) == MK_OK);
    assert(wall_id == 7);

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(mk_game_unit_line_of_sight(&game, 1, 2, &line_of_sight) == MK_OK);
    assert(!line_of_sight.visible);
    assert(line_of_sight.blocking_terrain_id == wall_id);
    assert(line_of_sight.cover == 3);
    assert(line_of_sight.cover_terrain_id == 2);
}

static void test_line_of_sight_errors_are_reported(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    mk_line_of_sight_t line_of_sight;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(mk_game_unit_line_of_sight(&game, 1, 99, &line_of_sight) == MK_ERROR_NOT_FOUND);
    assert(mk_game_trace_line_of_sight(
        &game,
        make_vec2(-1.0f, 0.0f),
        make_vec2(10.0f, 10.0f),
        &line_of_sight
    ) == MK_ERROR_INVALID_DATA);
}

static void test_unit_fire_resolves_damage_and_suppression(void) {
    mk_game_t game;
    mk_weapon_profile_t rifle;
    mk_unit_t attacker;
    mk_unit_t target;
    mk_soldier_t soldier;
    mk_fire_result_t fire_result;
    uint32_t attacker_id = 0;
    uint32_t target_id = 0;
    mk_unit_t *stored_target;

    mk_game_init(&game, 1);
    game.map = mk_make_map("Fire Test Map", 120.0f, 80.0f);
    rifle = mk_make_weapon("Test Rifle", 100, 2, 60, 10);

    attacker = mk_make_unit("Attacker", MK_SIDE_PLAYER, MK_TRAINING_ELITE, make_vec2(10.0f, 10.0f));
    soldier = mk_make_soldier("Shooter A", MK_ROLE_RIFLEMAN, rifle);
    assert(mk_unit_add_soldier(&attacker, &soldier, NULL) == MK_OK);
    soldier = mk_make_soldier("Shooter B", MK_ROLE_RIFLEMAN, rifle);
    assert(mk_unit_add_soldier(&attacker, &soldier, NULL) == MK_OK);

    target = mk_make_unit("Target", MK_SIDE_OPFOR, MK_TRAINING_REGULAR, make_vec2(18.0f, 10.0f));
    soldier = mk_make_soldier("Target A", MK_ROLE_RIFLEMAN, rifle);
    soldier.health = 35;
    assert(mk_unit_add_soldier(&target, &soldier, NULL) == MK_OK);
    soldier = mk_make_soldier("Target B", MK_ROLE_RIFLEMAN, rifle);
    soldier.health = 35;
    assert(mk_unit_add_soldier(&target, &soldier, NULL) == MK_OK);

    assert(mk_game_add_unit(&game, &attacker, &attacker_id) == MK_OK);
    assert(mk_game_add_unit(&game, &target, &target_id) == MK_OK);
    assert(mk_game_unit_fire(&game, attacker_id, target_id, &fire_result) == MK_OK);

    stored_target = mk_game_find_unit(&game, target_id);
    assert(stored_target != NULL);
    assert(fire_result.resolved);
    assert(fire_result.eligible_shooters == 2);
    assert(fire_result.shots_fired == 4);
    assert(fire_result.ammo_spent == 4);
    assert(fire_result.hits == 3);
    assert(fire_result.damage_applied == 70);
    assert(fire_result.casualties == 2);
    assert(fire_result.suppression_added == 50);
    assert(fire_result.attacker_status == MK_UNIT_READY);
    assert(fire_result.target_status_before == MK_UNIT_READY);
    assert(fire_result.target_status_after == MK_UNIT_BROKEN);
    assert(fire_result.contact_report_id == 1);
    assert(stored_target->suppression == 50);
    assert(stored_target->status == MK_UNIT_BROKEN);
    assert(stored_target->soldiers[0].casualty);
    assert(stored_target->soldiers[1].casualty);
    assert(game.units[0].soldiers[0].ammo == 118);
    assert(game.units[0].soldiers[1].ammo == 118);
    assert(game.contact_report_count == 1);
    assert(game.contact_reports[0].kind == MK_CONTACT_REPORT_FIRE);
    assert(game.contact_reports[0].shots_fired == 4);
    assert(game.contact_reports[0].hits == 3);
    assert(game.contact_reports[0].casualties == 2);
}

static void test_unit_fire_blocked_by_line_of_sight(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_terrain_zone_t wall;
    mk_game_t game;
    mk_fire_result_t fire_result;
    int ammo_before;

    wall = mk_make_terrain_zone(
        "Blocking Wall",
        MK_TERRAIN_OBSTACLE,
        make_rect(200.0f, 228.0f, 24.0f, 40.0f),
        4,
        4,
        true
    );
    assert(mk_map_add_terrain(&scenario.map, &wall, NULL) == MK_OK);
    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);

    ammo_before = game.units[0].soldiers[0].ammo;
    assert(mk_game_unit_fire(&game, 1, 2, &fire_result) == MK_OK);
    assert(!fire_result.resolved);
    assert(!fire_result.line_of_sight.visible);
    assert(fire_result.shots_fired == 0);
    assert(game.units[0].soldiers[0].ammo == ammo_before);
    assert(game.units[1].suppression == 0);
}

static void test_selected_unit_fire_uses_loaded_scenario(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    mk_fire_result_t fire_result;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(mk_game_selected_unit_fire(&game, 2, &fire_result) == MK_ERROR_NOT_FOUND);
    assert(mk_game_select_unit(&game, 1) == MK_OK);
    assert(mk_game_selected_unit_fire(&game, 2, &fire_result) == MK_OK);
    assert(fire_result.resolved);
    assert(fire_result.line_of_sight.visible);
    assert(fire_result.line_of_sight.cover == 3);
    assert(fire_result.target_status_before == MK_UNIT_READY);
    assert(fire_result.eligible_shooters == 3);
    assert(fire_result.shots_fired == 6);
    assert(game.units[1].suppression > 0);
    assert(game.contact_report_count >= 2);
    assert(game.units[1].revealed);
    assert(fire_result.civilian_risk_added > 0);
    assert(game.civilians[0].risk > 0);
    assert(fire_result.civilian_risk_added >= game.civilians[0].risk);
}

static void test_hidden_contact_reveals_when_observed(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(game.units[1].hidden);
    assert(!game.units[1].revealed);
    assert(mk_game_update_hidden_contacts(&game) == MK_OK);
    assert(!game.units[1].revealed);
    {
        bool saw_unit_two_reveal = false;
        size_t report_index;

        for (report_index = 0; report_index < game.contact_report_count; ++report_index) {
            if (game.contact_reports[report_index].kind == MK_CONTACT_REPORT_REVEAL
                && game.contact_reports[report_index].target_unit_id == 2) {
                saw_unit_two_reveal = true;
            }
        }
        assert(!saw_unit_two_reveal);
    }

    game.units[0].position_m = make_vec2(300.0f, 230.0f);
    assert(mk_game_update_hidden_contacts(&game) == MK_OK);
    assert(game.units[1].revealed);
    {
        bool saw_reveal = false;
        size_t report_index;

        assert(game.contact_report_count >= 1);
        for (report_index = 0; report_index < game.contact_report_count; ++report_index) {
            if (game.contact_reports[report_index].kind == MK_CONTACT_REPORT_REVEAL
                && game.contact_reports[report_index].target_unit_id == 2) {
                saw_reveal = true;
            }
        }
        assert(saw_reveal);
    }
}

static void test_hidden_contact_records_suspected_danger(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    game.units[0].position_m = make_vec2(210.0f, 230.0f);
    assert(mk_game_update_hidden_contacts(&game) == MK_OK);
    assert(!game.units[1].revealed);
    {
        bool saw_suspected_contact = false;
        size_t report_index;

        assert(game.contact_report_count >= 1);
        for (report_index = 0; report_index < game.contact_report_count; ++report_index) {
            if (game.contact_reports[report_index].kind == MK_CONTACT_REPORT_SUSPECTED_DANGER
                && game.contact_reports[report_index].attacker_unit_id == 1
                && game.contact_reports[report_index].target_unit_id == 2) {
                assert(game.contact_reports[report_index].confidence >= 10);
                assert(!game.contact_reports[report_index].resolved);
                saw_suspected_contact = true;
            }
        }
        assert(saw_suspected_contact);
    }

    assert(mk_game_update_hidden_contacts(&game) == MK_OK);
    assert(game.contact_report_count >= 1);
}

static void test_false_contact_records_noisy_terrain(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    game.units[0].position_m = make_vec2(220.0f, 330.0f);
    assert(mk_game_update_hidden_contacts(&game) == MK_OK);
    {
        bool saw_breach_false_contact = false;
        bool saw_cache_false_contact = false;
        size_t report_index;

        assert(game.contact_report_count >= 2);
        for (report_index = 0; report_index < game.contact_report_count; ++report_index) {
            const mk_contact_report_t *report = &game.contact_reports[report_index];

            if (report->kind != MK_CONTACT_REPORT_FALSE_CONTACT) {
                continue;
            }

            if (report->terrain_id == 3) {
                assert(report->attacker_unit_id == 1);
                assert(report->target_unit_id == 0);
                assert(report->confidence > 0);
                assert(!report->resolved);
                saw_breach_false_contact = true;
            } else if (report->terrain_id == 6) {
                saw_cache_false_contact = true;
            }
        }
        assert(saw_breach_false_contact);
        assert(saw_cache_false_contact);
    }

    assert(mk_game_update_hidden_contacts(&game) == MK_OK);
    assert(game.contact_report_count >= 2);
}

static void test_civilian_risk_tracks_close_armed_units(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    game.units[0].position_m = make_vec2(246.0f, 206.0f);
    assert(mk_game_update_civilian_risk(&game) == MK_OK);
    assert(game.civilians[0].risk == 1);
    assert(game.civilians[0].stress == 1);
    assert(game.civilians[0].state == MK_CIVILIAN_FROZEN);
    {
        bool saw_civilian_risk = false;
        size_t report_index;

        assert(game.contact_report_count >= 1);
        for (report_index = 0; report_index < game.contact_report_count; ++report_index) {
            if (game.contact_reports[report_index].kind == MK_CONTACT_REPORT_CIVILIAN_RISK
                && game.contact_reports[report_index].civilian_id == 1) {
                saw_civilian_risk = true;
            }
        }
        assert(saw_civilian_risk);
    }
}

static mk_unit_t make_status_test_unit(const char *name, mk_training_t training, int suppression) {
    mk_weapon_profile_t rifle = mk_make_weapon("Test Rifle", 100, 1, 20, 5);
    mk_unit_t unit = mk_make_unit(name, MK_SIDE_PLAYER, training, make_vec2(0.0f, 0.0f));
    mk_soldier_t soldier = mk_make_soldier("A", MK_ROLE_RIFLEMAN, rifle);

    assert(mk_unit_add_soldier(&unit, &soldier, NULL) == MK_OK);
    soldier = mk_make_soldier("B", MK_ROLE_RIFLEMAN, rifle);
    assert(mk_unit_add_soldier(&unit, &soldier, NULL) == MK_OK);
    unit.suppression = suppression;

    return unit;
}

static void test_suppression_status_slows_movement_and_recovers(void) {
    mk_game_t game;
    mk_unit_t unit = make_status_test_unit("Pinned Team", MK_TRAINING_REGULAR, 10);
    uint32_t unit_id = 0;
    mk_unit_t *stored_unit;

    mk_game_init(&game, 5);
    game.map = mk_make_map("Status Map", 120.0f, 80.0f);
    assert(mk_game_add_unit(&game, &unit, &unit_id) == MK_OK);

    stored_unit = mk_game_find_unit(&game, unit_id);
    assert(stored_unit != NULL);
    assert(stored_unit->status == MK_UNIT_PINNED);
    assert(mk_game_issue_move_order(&game, unit_id, make_vec2(20.0f, 0.0f)) == MK_OK);

    mk_game_step(&game);
    assert_close(stored_unit->position_m.x, 2.1f);
    assert_close(stored_unit->position_m.y, 0.0f);
    assert(stored_unit->suppression == 9);
    assert(stored_unit->status == MK_UNIT_SUPPRESSED);
    assert(stored_unit->order == MK_ORDER_MOVE);

    stored_unit->order = MK_ORDER_RALLY;
    mk_game_step(&game);
    assert(stored_unit->suppression == 6);
    assert(stored_unit->status == MK_UNIT_SUPPRESSED);
}

static void test_broken_unit_halts_movement(void) {
    mk_game_t game;
    mk_unit_t unit = make_status_test_unit("Broken Team", MK_TRAINING_REGULAR, 25);
    uint32_t unit_id = 0;
    mk_unit_t *stored_unit;

    mk_game_init(&game, 6);
    game.map = mk_make_map("Broken Map", 120.0f, 80.0f);
    assert(mk_game_add_unit(&game, &unit, &unit_id) == MK_OK);

    stored_unit = mk_game_find_unit(&game, unit_id);
    assert(stored_unit != NULL);
    assert(stored_unit->status == MK_UNIT_BROKEN);
    assert(mk_game_issue_move_order(&game, unit_id, make_vec2(20.0f, 0.0f)) == MK_OK);

    mk_game_step(&game);
    assert_close(stored_unit->position_m.x, 0.0f);
    assert(!stored_unit->has_move_target);
    assert(stored_unit->order == MK_ORDER_RALLY);
    assert(stored_unit->suppression == 24);
    assert(stored_unit->status == MK_UNIT_BROKEN);
}

static void test_objective_control_and_scoring(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    mk_score_t score;
    size_t civilian_index;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    game.units[0].position_m = game.objectives[0].position_m;
    assert(mk_game_update_objective_control(&game) == MK_OK);
    assert(game.objectives[0].controlling_side == MK_SIDE_NEUTRAL);

    game.units[1].position_m = make_vec2(420.0f, 230.0f);
    for (civilian_index = 0; civilian_index < game.civilian_count; ++civilian_index) {
        game.civilians[civilian_index].risk = 0;
    }
    game.civilians[0].risk = 3;
    game.units[0].soldiers[0].casualty = true;
    game.tick = 7;
    assert(mk_game_update_objective_control(&game) == MK_OK);
    assert(game.objectives[0].controlling_side == MK_SIDE_PLAYER);

    assert(mk_game_score(&game, &score) == MK_OK);
    assert(score.objective_points == 500);
    assert(score.civilian_risk == 3);
    assert(score.civilian_risk_penalty == 30);
    assert(score.player_casualties == 1);
    assert(score.casualty_penalty == 50);
    assert(score.time_penalty == 7);
    assert(score.total_score == 413);
    assert(score.controlled_objectives == 1);
    assert(score.contested_objectives == 0);
    assert(score.outcome == MK_OUTCOME_PLAYER_PARTIAL);
}

static void test_score_uses_scenario_weights(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    mk_score_t score;
    size_t civilian_index;

    scenario.score_objective_weight = 80;
    scenario.score_civilian_risk_weight = 5;
    scenario.score_player_casualty_weight = 40;
    scenario.score_civilian_casualty_weight = 90;
    scenario.score_time_weight = 2;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    game.units[0].position_m = game.objectives[0].position_m;
    game.units[1].position_m = make_vec2(420.0f, 230.0f);
    for (civilian_index = 0; civilian_index < game.civilian_count; ++civilian_index) {
        game.civilians[civilian_index].risk = 0;
    }
    game.civilians[0].risk = 3;
    game.units[0].soldiers[0].casualty = true;
    game.tick = 4;
    assert(mk_game_update_objective_control(&game) == MK_OK);
    assert(mk_game_score(&game, &score) == MK_OK);

    assert(score.objective_points == 400);
    assert(score.civilian_risk_penalty == 15);
    assert(score.casualty_penalty == 40);
    assert(score.time_penalty == 8);
    assert(score.total_score == 337);
}

static void test_after_action_report_is_stable(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    mk_after_action_report_t report;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    game.units[0].position_m = game.objectives[0].position_m;
    game.units[1].position_m = make_vec2(420.0f, 230.0f);
    game.tick = 5;
    assert(mk_game_update_objective_control(&game) == MK_OK);

    assert(mk_game_after_action_report(&game, &report) == MK_OK);
    assert(report.score.total_score == 485);
    assert(report.score.outcome == MK_OUTCOME_PLAYER_SUCCESS);
    assert(strstr(report.summary, "outcome=success") != NULL);
    assert(strstr(report.summary, "score=485") != NULL);
    assert(strstr(report.summary, "objectives=1") != NULL);
    assert(strstr(report.summary, "ticks=5") != NULL);
}

static void test_traffic_vehicle_runtime_routes_boarding_and_determinism(void) {
    mk_scenario_definition_t scenario = make_traffic_vehicle_route_scenario();
    mk_game_t first;
    mk_game_t second;
    mk_game_t failure_game;
    mk_game_snapshot_t snapshot;
    mk_traffic_vehicle_t *first_bus;
    mk_traffic_vehicle_t *second_bus;
    mk_traffic_vehicle_t *motorcycle;
    mk_unit_t *first_unit;
    mk_unit_t *second_unit;
    mk_unit_t *third_unit;
    int tick;

    assert(mk_game_load_scenario(&first, &scenario) == MK_OK);
    assert(mk_game_load_scenario(&second, &scenario) == MK_OK);

    assert(mk_game_issue_traffic_vehicle_move_order(&first, 1, "roof", mk_vec2(12.0f, 20.0f)) == MK_OK);
    assert(mk_game_issue_traffic_vehicle_move_order(&second, 1, "roof", mk_vec2(12.0f, 20.0f)) == MK_OK);
    assert(mk_game_board_traffic_vehicle(&first, 1, 1) == MK_OK);
    assert(mk_game_board_traffic_vehicle(&second, 1, 1) == MK_OK);

    first_bus = mk_game_find_traffic_vehicle(&first, 1);
    second_bus = mk_game_find_traffic_vehicle(&second, 1);
    first_unit = mk_game_find_unit(&first, 1);
    assert(first_bus != NULL);
    assert(second_bus != NULL);
    assert(first_unit != NULL);
    assert(first_bus->boarding_mode == MK_TRAFFIC_BOARD_INSIDE);
    assert(first_bus->has_destination);
    assert(first_bus->has_route);
    assert(first_bus->route_step_count == 3);
    assert(first_bus->route_uses_vertical_transition);
    assert(first_bus->embarked_unit_count == 1);
    assert(first_bus->occupied_seats == 1);
    assert_close(first_unit->position_m.x, first_bus->position_m.x);
    assert_close(first_unit->position_m.y, first_bus->position_m.y);

    for (tick = 0; tick < 4; ++tick) {
        mk_game_step(&first);
        mk_game_step(&second);
    }

    assert_close(first_bus->position_m.x, second_bus->position_m.x);
    assert_close(first_bus->position_m.y, second_bus->position_m.y);
    assert_close(first.units[0].position_m.x, second.units[0].position_m.x);
    assert_close(first.units[0].position_m.y, second.units[0].position_m.y);
    assert(first_bus->route_step_index == second_bus->route_step_index);
    assert(first_bus->occupied_seats == second_bus->occupied_seats);

    assert(mk_game_snapshot(&first, &snapshot) == MK_OK);
    assert(snapshot.traffic_vehicle_count == 2);
    assert(snapshot.traffic_vehicles[0].occupied_seats == 1);
    assert(snapshot.traffic_vehicles[0].blocks_movement);

    for (tick = 0; tick < 20 && first_bus->has_destination; ++tick) {
        mk_game_step(&first);
    }

    assert(!first_bus->has_destination);
    assert(!first_bus->has_route);
    assert(strcmp(first_bus->level_id, "roof") == 0);
    assert(first_bus->route_vertical_transitions_completed == 1U);
    assert_close(first_bus->position_m.x, 12.0f);
    assert_close(first_bus->position_m.y, 20.0f);
    assert_close(first_unit->position_m.x, first_bus->position_m.x);
    assert_close(first_unit->position_m.y, first_bus->position_m.y);
    assert(strcmp(first_unit->level_id, "roof") == 0);

    assert(mk_game_unboard_traffic_vehicle(&first, first_bus->id, first_unit->id) == MK_OK);
    assert(first_bus->embarked_unit_count == 0);
    assert(first_bus->occupied_seats == 0);
    assert(first_unit->order == MK_ORDER_HOLD);
    assert(strcmp(first_unit->level_id, "roof") == 0);

    motorcycle = mk_game_find_traffic_vehicle(&first, 2);
    second_unit = mk_game_find_unit(&first, 2);
    third_unit = mk_game_find_unit(&first, 3);
    assert(motorcycle != NULL);
    assert(second_unit != NULL);
    assert(third_unit != NULL);
    assert(motorcycle->boarding_mode == MK_TRAFFIC_BOARD_ON);
    assert(mk_game_board_traffic_vehicle(&first, motorcycle->id, second_unit->id) == MK_OK);
    assert(motorcycle->embarked_unit_count == 1);
    assert(motorcycle->occupied_seats == 1);
    assert(mk_game_board_traffic_vehicle(&first, motorcycle->id, third_unit->id) == MK_ERROR_CAPACITY);
    assert(motorcycle->occupied_seats == 1);

    assert(mk_game_board_traffic_vehicle(&first, first_bus->id, second_unit->id) == MK_OK);
    assert(motorcycle->embarked_unit_count == 0);
    assert(motorcycle->occupied_seats == 0);
    assert(first_bus->embarked_unit_count == 1);
    assert(first_bus->occupied_seats == 1);
    assert_close(second_unit->position_m.x, first_bus->position_m.x);
    assert_close(second_unit->position_m.y, first_bus->position_m.y);

    assert(mk_game_load_scenario(&failure_game, &scenario) == MK_OK);
    strcpy(failure_game.gameplay_area.topology_portals[0].state, "locked");
    assert(mk_game_issue_traffic_vehicle_move_order(&failure_game, 1, "roof", mk_vec2(12.0f, 20.0f))
        == MK_ERROR_NOT_FOUND);
    first_bus = mk_game_find_traffic_vehicle(&failure_game, 1);
    assert(first_bus != NULL);
    assert(!first_bus->has_destination);
    assert(!first_bus->has_route);
    assert(strcmp(first_bus->route_failure_reason, "unreachable") == 0);
}

static void test_traffic_vehicle_runtime_blocking(void) {
    mk_scenario_definition_t scenario = make_traffic_vehicle_blocking_scenario();
    mk_game_t game;
    mk_unit_t *unit;
    mk_traffic_vehicle_t *unit_blocker;
    mk_traffic_vehicle_t *moving_car;
    mk_traffic_vehicle_t *vehicle_blocker;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    unit = mk_game_find_unit(&game, 1);
    unit_blocker = mk_game_find_traffic_vehicle(&game, 1);
    moving_car = mk_game_find_traffic_vehicle(&game, 2);
    vehicle_blocker = mk_game_find_traffic_vehicle(&game, 3);
    assert(unit != NULL);
    assert(unit_blocker != NULL);
    assert(moving_car != NULL);
    assert(vehicle_blocker != NULL);
    assert(unit_blocker->blocks_movement);
    assert(vehicle_blocker->blocks_movement);

    assert(mk_game_issue_move_order(&game, unit->id, mk_vec2(18.0f, 5.0f)) == MK_OK);
    mk_game_step(&game);
    assert_close(unit->position_m.x, 5.0f);
    assert_close(unit->position_m.y, 5.0f);
    assert(!unit->has_move_target);
    assert(unit->order == MK_ORDER_HOLD);
    assert(unit->route_failure_count == 1U);
    assert(strcmp(unit->route_failure_reason, "traffic_blocked") == 0);

    unit_blocker->blocks_movement = false;
    assert(mk_game_issue_move_order(&game, unit->id, mk_vec2(18.0f, 5.0f)) == MK_OK);
    mk_game_step(&game);
    assert(unit->position_m.x > 5.0f);

    assert(mk_game_issue_traffic_vehicle_move_order(&game, moving_car->id, NULL, mk_vec2(24.0f, 20.0f)) == MK_OK);
    mk_game_step(&game);
    assert_close(moving_car->position_m.x, 5.0f);
    assert_close(moving_car->position_m.y, 20.0f);
    assert(!moving_car->has_destination);
    assert(moving_car->route_failure_count == 1U);
    assert(strcmp(moving_car->route_failure_reason, "traffic_blocked") == 0);

    vehicle_blocker->blocks_movement = false;
    assert(mk_game_issue_traffic_vehicle_move_order(&game, moving_car->id, NULL, mk_vec2(24.0f, 20.0f)) == MK_OK);
    mk_game_step(&game);
    assert(moving_car->position_m.x > 5.0f);
}

static void test_invalid_scenario_is_rejected(void) {
    mk_scenario_definition_t valid_scenario = make_east_mosul_block_scenario_fixture();
    mk_scenario_definition_t scenario;
    mk_game_t game;

    scenario = valid_scenario;
    scenario.map.width_m = 0.0f;
    assert(mk_game_load_scenario(&game, &scenario) == MK_ERROR_INVALID_DATA);

    scenario = valid_scenario;
    scenario.objectives[0].position_m = make_vec2(999.0f, 74.0f);
    assert(mk_game_load_scenario(&game, &scenario) == MK_ERROR_INVALID_DATA);

    scenario = valid_scenario;
    scenario.units[0].faction_id = 99;
    assert(mk_game_load_scenario(&game, &scenario) == MK_ERROR_INVALID_DATA);

    scenario = valid_scenario;
    scenario.controllers[0].kind = MK_CONTROLLER_NONE;
    assert(mk_game_load_scenario(&game, &scenario) == MK_ERROR_INVALID_DATA);

    scenario = valid_scenario;
    scenario.forces[0].controller_id = 99;
    assert(mk_game_load_scenario(&game, &scenario) == MK_ERROR_INVALID_DATA);

    scenario = valid_scenario;
    scenario.civilians[0].position_m = make_vec2(-1.0f, 48.0f);
    assert(mk_game_load_scenario(&game, &scenario) == MK_ERROR_INVALID_DATA);

    scenario = valid_scenario;
    scenario.map.tiles[0].movement_cost = -1;
    assert(mk_game_load_scenario(&game, &scenario) == MK_ERROR_INVALID_DATA);

    scenario = valid_scenario;
    scenario.score_objective_weight = -1;
    assert(mk_game_load_scenario(&game, &scenario) == MK_ERROR_INVALID_DATA);
}

int main(void) {
    test_version_is_present();
    test_rng_is_deterministic();
    test_result_names_are_stable();
    test_math_value_helpers();
    test_gameplay_area_coordinate_and_blocker_queries();
    test_topology_route_following_moves_units_between_levels();
    test_civilian_ai_and_instruction_routes_are_deterministic();
    test_search_semantic_zone_and_cache_terrain_records_results();
    test_breach_portal_opens_route_and_scores();
    test_unit_and_soldier_creation();
    test_map_tiles_are_configurable_and_addressable();
    test_capacity_limits_are_reported();
    test_step_recovers_suppression();
    test_scenario_loading_populates_core_state();
    test_snapshot_is_stable_copy();
    test_pick_select_and_move_order();
    test_pick_contact_and_investigate_order();
    test_investigate_resolves_contact_reports();
    test_interaction_errors_are_reported();
    test_line_of_sight_reports_target_cover();
    test_line_of_sight_reports_blocking_terrain();
    test_line_of_sight_errors_are_reported();
    test_unit_fire_resolves_damage_and_suppression();
    test_unit_fire_blocked_by_line_of_sight();
    test_selected_unit_fire_uses_loaded_scenario();
    test_hidden_contact_reveals_when_observed();
    test_hidden_contact_records_suspected_danger();
    test_false_contact_records_noisy_terrain();
    test_civilian_risk_tracks_close_armed_units();
    test_suppression_status_slows_movement_and_recovers();
    test_broken_unit_halts_movement();
    test_objective_control_and_scoring();
    test_score_uses_scenario_weights();
    test_after_action_report_is_stable();
    test_traffic_vehicle_runtime_routes_boarding_and_determinism();
    test_traffic_vehicle_runtime_blocking();
    test_invalid_scenario_is_rejected();

    puts("mk_core_tests: ok");
    return 0;
}
