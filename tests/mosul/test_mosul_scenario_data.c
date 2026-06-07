#include "mk_mosul_demo.h"
#include "mk_test.h"

#include <stdio.h>
#include <string.h>

#ifndef MK_TEST_PROJECT_ROOT
#define MK_TEST_PROJECT_ROOT "."
#endif

#ifndef MK_TEST_BINARY_DIR
#define MK_TEST_BINARY_DIR "."
#endif

static void make_project_path(char *out_path, size_t capacity, const char *relative_path) {
    int written = snprintf(out_path, capacity, "%s/%s", MK_TEST_PROJECT_ROOT, relative_path);

    MK_TEST_ASSERT(written > 0);
    MK_TEST_ASSERT((size_t)written < capacity);
}

static void make_binary_path(char *out_path, size_t capacity, const char *name) {
    int written = snprintf(out_path, capacity, "%s/%s", MK_TEST_BINARY_DIR, name);

    MK_TEST_ASSERT(written > 0);
    MK_TEST_ASSERT((size_t)written < capacity);
}

static void write_text_file(const char *path, const char *text) {
    FILE *file = fopen(path, "w");

    MK_TEST_ASSERT(file != NULL);
    MK_TEST_ASSERT(fputs(text, file) >= 0);
    MK_TEST_ASSERT(fclose(file) == 0);
}

static void load_default_data_scenario(mk_scenario_definition_t *out_scenario) {
    char path[512];

    make_project_path(path, sizeof(path), MK_MOSUL_DEFAULT_SCENARIO_PATH);
    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, out_scenario) == MK_OK);
}

static void test_default_scenario_data_matches_fixture_shape(void) {
    mk_scenario_definition_t loaded;
    mk_scenario_definition_t fixture;
    const mk_gameplay_feature_t *wall;
    const mk_gameplay_feature_t *door;
    const mk_gameplay_feature_t *window;
    const mk_gameplay_topology_node_t *node;
    const mk_gameplay_topology_portal_t *portal;
    const mk_gameplay_semantic_zone_t *zone;
    mk_vec2_t wall_sample;
    mk_vec2_t door_sample;
    mk_vec2_t window_sample;
    mk_ivec2_t wall_pixel;
    char topology_dump[2048];

    load_default_data_scenario(&loaded);
    MK_TEST_ASSERT(mk_mosul_make_market_2003_fixture_scenario(&fixture) == MK_OK);

    MK_TEST_ASSERT(strcmp(loaded.name, fixture.name) == 0);
    MK_TEST_ASSERT(strcmp(loaded.briefing, fixture.briefing) == 0);
    MK_TEST_ASSERT(strcmp(loaded.after_action_success, fixture.after_action_success) == 0);
    MK_TEST_ASSERT(strcmp(loaded.after_action_partial, fixture.after_action_partial) == 0);
    MK_TEST_ASSERT(strcmp(loaded.after_action_failure, fixture.after_action_failure) == 0);
    MK_TEST_ASSERT(loaded.seed == fixture.seed);
    MK_TEST_ASSERT(loaded.score_success_threshold == fixture.score_success_threshold);
    MK_TEST_ASSERT(loaded.score_partial_threshold == fixture.score_partial_threshold);
    MK_TEST_ASSERT(loaded.score_objective_weight == fixture.score_objective_weight);
    MK_TEST_ASSERT(loaded.score_civilian_risk_weight == fixture.score_civilian_risk_weight);
    MK_TEST_ASSERT(loaded.score_player_casualty_weight == fixture.score_player_casualty_weight);
    MK_TEST_ASSERT(loaded.score_civilian_casualty_weight == fixture.score_civilian_casualty_weight);
    MK_TEST_ASSERT(loaded.score_time_weight == fixture.score_time_weight);
    MK_TEST_ASSERT(strcmp(loaded.map.name, fixture.map.name) == 0);
    MK_TEST_ASSERT_CLOSE(loaded.map.width_m, fixture.map.width_m);
    MK_TEST_ASSERT_CLOSE(loaded.map.height_m, fixture.map.height_m);
    MK_TEST_ASSERT(loaded.map.terrain_count == 6);
    MK_TEST_ASSERT(loaded.map.terrain_count == fixture.map.terrain_count);
    MK_TEST_ASSERT(loaded.map.terrain[3].kind == MK_TERRAIN_BREACH_POINT);
    MK_TEST_ASSERT(loaded.map.terrain[4].kind == MK_TERRAIN_ROOFTOP);
    MK_TEST_ASSERT(loaded.map.terrain[5].kind == MK_TERRAIN_SUSPECTED_IED);
    MK_TEST_ASSERT(loaded.map.tile_count == fixture.map.tile_count);
    MK_TEST_ASSERT(loaded.map.tiles[46].kind == fixture.map.tiles[46].kind);
    MK_TEST_ASSERT(loaded.controller_count == fixture.controller_count);
    MK_TEST_ASSERT(loaded.faction_count == fixture.faction_count);
    MK_TEST_ASSERT(loaded.force_count == fixture.force_count);
    MK_TEST_ASSERT(loaded.objective_count == fixture.objective_count);
    MK_TEST_ASSERT(strcmp(loaded.objectives[0].label, fixture.objectives[0].label) == 0);
    MK_TEST_ASSERT(loaded.civilian_count == fixture.civilian_count);
    MK_TEST_ASSERT(loaded.unit_count == fixture.unit_count);
    MK_TEST_ASSERT(strcmp(loaded.forces[0].command.callsign, fixture.forces[0].command.callsign) == 0);
    MK_TEST_ASSERT(strcmp(loaded.units[0].name, fixture.units[0].name) == 0);
    MK_TEST_ASSERT(strcmp(loaded.units[0].command.callsign, fixture.units[0].command.callsign) == 0);
    MK_TEST_ASSERT_CLOSE(loaded.units[0].position_m.x, fixture.units[0].position_m.x);
    MK_TEST_ASSERT_CLOSE(loaded.units[0].position_m.y, fixture.units[0].position_m.y);
    MK_TEST_ASSERT(loaded.units[0].soldier_count == fixture.units[0].soldier_count);
    MK_TEST_ASSERT(loaded.units[1].soldiers[1].role == fixture.units[1].soldiers[1].role);
    MK_TEST_ASSERT(loaded.units[1].hidden == fixture.units[1].hidden);
    MK_TEST_ASSERT(loaded.units[1].revealed == fixture.units[1].revealed);
    MK_TEST_ASSERT(loaded.units[1].concealment == fixture.units[1].concealment);
    MK_TEST_ASSERT(loaded.units[2].soldiers[0].ammo == 0);

    MK_TEST_ASSERT(mk_gameplay_area_is_loaded(&loaded.gameplay_area));
    MK_TEST_ASSERT(strcmp(loaded.gameplay_area.id, "market_commercial_streets_2003_building_levels") == 0);
    MK_TEST_ASSERT(strcmp(loaded.gameplay_area.map_id, "market_commercial_streets_2003") == 0);
    MK_TEST_ASSERT_CLOSE(loaded.gameplay_area.world_width_m, loaded.map.width_m);
    MK_TEST_ASSERT_CLOSE(loaded.gameplay_area.world_height_m, loaded.map.height_m);
    MK_TEST_ASSERT(loaded.gameplay_area.pixel_width == 7000);
    MK_TEST_ASSERT(loaded.gameplay_area.pixel_height == 7000);
    MK_TEST_ASSERT_CLOSE(loaded.gameplay_area.pixels_per_meter, 14.0f);
    MK_TEST_ASSERT(loaded.gameplay_area.level_count == 4);
    MK_TEST_ASSERT(loaded.gameplay_area.feature_count == 25);
    MK_TEST_ASSERT(loaded.gameplay_area.region_count == 8);
    MK_TEST_ASSERT(mk_gameplay_area_topology_is_loaded(&loaded.gameplay_area));
    MK_TEST_ASSERT(strcmp(loaded.gameplay_area.topology_id, "market_commercial_streets_2003_topology") == 0);
    MK_TEST_ASSERT(loaded.gameplay_area.topology_node_count == 14);
    MK_TEST_ASSERT(loaded.gameplay_area.topology_portal_count == 15);
    MK_TEST_ASSERT(loaded.gameplay_area.semantic_zone_count == 10);

    node = mk_gameplay_area_find_topology_node(&loaded.gameplay_area, "hotel_roof_access");
    MK_TEST_ASSERT(node != NULL);
    MK_TEST_ASSERT(strcmp(node->kind, "roof") == 0);
    MK_TEST_ASSERT(strcmp(node->level_id, "level_04_roof_access") == 0);

    portal = mk_gameplay_area_find_topology_portal(&loaded.gameplay_area, "hotel_stair_ground_to_second");
    MK_TEST_ASSERT(portal != NULL);
    MK_TEST_ASSERT(portal->vertical);
    MK_TEST_ASSERT(portal->bidirectional);

    zone = mk_gameplay_area_find_semantic_zone(&loaded.gameplay_area, "market_restricted_fire_lane");
    MK_TEST_ASSERT(zone != NULL);
    MK_TEST_ASSERT(strcmp(zone->kind, "restricted_fire_lane") == 0);
    MK_TEST_ASSERT(zone->priority == 5);
    MK_TEST_ASSERT(mk_gameplay_area_find_semantic_zone_at_world(&loaded.gameplay_area, "restricted_fire_lane", mk_vec2(180.0f, 145.0f)) == zone);
    MK_TEST_ASSERT(mk_gameplay_area_topology_debug_dump(&loaded.gameplay_area, topology_dump, sizeof(topology_dump)) == MK_OK);
    MK_TEST_ASSERT(strstr(topology_dump, "topology id=\"market_commercial_streets_2003_topology\"") != NULL);
    MK_TEST_ASSERT(strstr(topology_dump, "unreachable=0") != NULL);

    wall = mk_gameplay_area_find_feature(&loaded.gameplay_area, "souq_west_outer_wall_ground");
    door = mk_gameplay_area_find_feature(&loaded.gameplay_area, "souq_west_door_ground");
    window = mk_gameplay_area_find_feature(&loaded.gameplay_area, "souq_east_window_ground");
    MK_TEST_ASSERT(wall != NULL);
    MK_TEST_ASSERT(door != NULL);
    MK_TEST_ASSERT(window != NULL);

    MK_TEST_ASSERT(mk_gameplay_area_pixel_to_world(&loaded.gameplay_area, mk_ivec2(840, 1500), &wall_sample) == MK_OK);
    MK_TEST_ASSERT(mk_gameplay_area_pixel_to_world(&loaded.gameplay_area, mk_ivec2(840, 2200), &door_sample) == MK_OK);
    MK_TEST_ASSERT(mk_gameplay_area_pixel_to_world(&loaded.gameplay_area, mk_ivec2(2110, 1720), &window_sample) == MK_OK);

    MK_TEST_ASSERT(mk_gameplay_area_world_to_pixel(&loaded.gameplay_area, wall_sample, &wall_pixel) == MK_OK);
    MK_TEST_ASSERT(wall_pixel.x >= wall->pixel_x);
    MK_TEST_ASSERT(wall_pixel.y >= wall->pixel_y);
    MK_TEST_ASSERT(mk_gameplay_area_blocks_los_at(&loaded.gameplay_area, "level_01_ground", wall_sample));
    MK_TEST_ASSERT(mk_gameplay_area_blocks_movement_at(&loaded.gameplay_area, "level_01_ground", wall_sample));
    MK_TEST_ASSERT(!mk_gameplay_area_blocks_los_at(&loaded.gameplay_area, "level_01_ground", door_sample));
    MK_TEST_ASSERT(!mk_gameplay_area_blocks_movement_at(&loaded.gameplay_area, "level_01_ground", door_sample));
    MK_TEST_ASSERT(!mk_gameplay_area_blocks_los_at(&loaded.gameplay_area, "level_01_ground", window_sample));
    MK_TEST_ASSERT(mk_gameplay_area_blocks_movement_at(&loaded.gameplay_area, "level_01_ground", window_sample));
}

static void test_public_default_scenario_uses_data_file(void) {
    mk_scenario_definition_t scenario;
    mk_game_t game;

    MK_TEST_ASSERT(mk_mosul_make_market_2003_scenario(&scenario) == MK_OK);
    MK_TEST_ASSERT(mk_game_load_scenario(&game, &scenario) == MK_OK);
    MK_TEST_ASSERT(strcmp(game.scenario_name, "Market Commercial Streets 2003") == 0);
    MK_TEST_ASSERT(game.unit_count == 3);
    MK_TEST_ASSERT(mk_gameplay_area_is_loaded(&game.gameplay_area));
    MK_TEST_ASSERT(game.gameplay_area.level_count == 4);
    MK_TEST_ASSERT(mk_gameplay_area_topology_is_loaded(&game.gameplay_area));
}

static void test_control_smoke_scenario_loads(void) {
    char path[512];
    mk_scenario_definition_t scenario;
    mk_game_t game;

    make_project_path(path, sizeof(path), "game/mosul/scenarios/market_control_smoke_2003.mkscenario");
    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_OK);
    MK_TEST_ASSERT(strcmp(scenario.objectives[0].label, "Market Junction") == 0);
    MK_TEST_ASSERT(scenario.civilian_count == 0);
    MK_TEST_ASSERT(scenario.unit_count == 2);
    MK_TEST_ASSERT(mk_game_load_scenario(&game, &scenario) == MK_OK);
}

static void test_contested_risk_smoke_scenario_loads(void) {
    char path[512];
    mk_scenario_definition_t scenario;
    mk_game_t game;

    make_project_path(path, sizeof(path), "game/mosul/scenarios/market_contested_risk_smoke_2003.mkscenario");
    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_OK);
    MK_TEST_ASSERT(strcmp(scenario.name, "Market Contested Risk Smoke 2003") == 0);
    MK_TEST_ASSERT(scenario.objective_count == 1);
    MK_TEST_ASSERT(scenario.civilian_count == 1);
    MK_TEST_ASSERT(scenario.unit_count == 2);
    MK_TEST_ASSERT(mk_game_load_scenario(&game, &scenario) == MK_OK);
}

static void test_missing_asset_manifest_is_rejected(void) {
    char path[512];
    mk_scenario_definition_t scenario;

    make_binary_path(path, sizeof(path), "bad_missing_asset.mkscenario");
    write_text_file(
        path,
        "format=modernerKrieg.scenario.v1\n"
        "name=Bad Missing Asset\n"
        "seed=1\n"
        "asset.map_manifest=assets/mosul/manifests/missing.mapmanifest\n"
        "asset.sprite_manifest=assets/mosul/manifests/mosul_2003_sprites.spritemanifest\n"
        "map.name=Market / Commercial Streets\n"
        "map.width_m=500\n"
        "map.height_m=500\n"
        "map.tile_columns=10\n"
        "map.tile_rows=10\n"
        "map.tile_width_m=50\n"
        "map.tile_height_m=50\n"
        "map.default_terrain=open\n"
    );

    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_ERROR_INVALID_DATA);
}

static void test_market_scenario_requires_building_level_manifest(void) {
    char path[512];
    mk_scenario_definition_t scenario;

    make_binary_path(path, sizeof(path), "bad_missing_building_levels.mkscenario");
    write_text_file(
        path,
        "format=modernerKrieg.scenario.v1\n"
        "name=Bad Missing Building Levels\n"
        "seed=1\n"
        "asset.map_manifest=assets/mosul/manifests/market_commercial_streets_2003.mapmanifest\n"
        "asset.sprite_manifest=assets/mosul/manifests/mosul_2003_sprites.spritemanifest\n"
        "map.name=Market / Commercial Streets\n"
        "map.width_m=500\n"
        "map.height_m=500\n"
        "map.tile_columns=10\n"
        "map.tile_rows=10\n"
        "map.tile_width_m=50\n"
        "map.tile_height_m=50\n"
        "map.default_terrain=open\n"
    );

    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_ERROR_INVALID_DATA);
}

static void test_market_scenario_requires_topology_manifest(void) {
    char path[512];
    mk_scenario_definition_t scenario;

    make_binary_path(path, sizeof(path), "bad_missing_topology.mkscenario");
    write_text_file(
        path,
        "format=modernerKrieg.scenario.v1\n"
        "name=Bad Missing Topology\n"
        "seed=1\n"
        "asset.map_manifest=assets/mosul/manifests/market_commercial_streets_2003.mapmanifest\n"
        "asset.sprite_manifest=assets/mosul/manifests/mosul_2003_sprites.spritemanifest\n"
        "asset.building_level_manifest=assets/mosul/manifests/market_commercial_streets_2003_building_levels.json\n"
        "map.name=Market / Commercial Streets\n"
        "map.width_m=500\n"
        "map.height_m=500\n"
        "map.tile_columns=10\n"
        "map.tile_rows=10\n"
        "map.tile_width_m=50\n"
        "map.tile_height_m=50\n"
        "map.default_terrain=open\n"
    );

    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_ERROR_INVALID_DATA);
}

static void test_invalid_force_reference_is_rejected(void) {
    char path[512];
    mk_scenario_definition_t scenario;

    make_binary_path(path, sizeof(path), "bad_force_reference.mkscenario");
    write_text_file(
        path,
        "format=modernerKrieg.scenario.v1\n"
        "name=Bad Force Reference\n"
        "seed=1\n"
        "asset.map_manifest=assets/mosul/manifests/market_commercial_streets_2003.mapmanifest\n"
        "asset.sprite_manifest=assets/mosul/manifests/mosul_2003_sprites.spritemanifest\n"
        "map.name=Market / Commercial Streets\n"
        "map.width_m=500\n"
        "map.height_m=500\n"
        "map.tile_columns=10\n"
        "map.tile_rows=10\n"
        "map.tile_width_m=50\n"
        "map.tile_height_m=50\n"
        "map.default_terrain=open\n"
        "tile_range.count=0\n"
        "terrain.count=0\n"
        "controller.count=1\n"
        "controller.0.name=AI\n"
        "controller.0.side=player\n"
        "controller.0.kind=tactical_ai\n"
        "faction.count=1\n"
        "faction.0.name=Faction\n"
        "faction.0.side=player\n"
        "faction.0.color=255,255,255,255\n"
        "force.count=1\n"
        "force.0.name=Broken Force\n"
        "force.0.side=player\n"
        "force.0.faction_index=99\n"
        "force.0.controller_index=0\n"
        "force.0.command_name=Broken\n"
        "force.0.callsign=BAD\n"
    );

    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_ERROR_INVALID_DATA);
}

static void test_impossible_objective_bounds_are_rejected(void) {
    char path[512];
    mk_scenario_definition_t scenario;

    make_binary_path(path, sizeof(path), "bad_objective_bounds.mkscenario");
    write_text_file(
        path,
        "format=modernerKrieg.scenario.v1\n"
        "name=Bad Objective Bounds\n"
        "seed=1\n"
        "asset.map_manifest=assets/mosul/manifests/market_commercial_streets_2003.mapmanifest\n"
        "asset.sprite_manifest=assets/mosul/manifests/mosul_2003_sprites.spritemanifest\n"
        "map.name=Market / Commercial Streets\n"
        "map.width_m=500\n"
        "map.height_m=500\n"
        "map.tile_columns=10\n"
        "map.tile_rows=10\n"
        "map.tile_width_m=50\n"
        "map.tile_height_m=50\n"
        "map.default_terrain=open\n"
        "tile_range.count=0\n"
        "terrain.count=0\n"
        "controller.count=1\n"
        "controller.0.name=AI\n"
        "controller.0.side=player\n"
        "controller.0.kind=tactical_ai\n"
        "faction.count=1\n"
        "faction.0.name=Faction\n"
        "faction.0.side=player\n"
        "faction.0.color=255,255,255,255\n"
        "force.count=1\n"
        "force.0.name=Force\n"
        "force.0.side=player\n"
        "force.0.faction_index=0\n"
        "force.0.controller_index=0\n"
        "force.0.command_name=Command\n"
        "force.0.callsign=OK\n"
        "objective.count=1\n"
        "objective.0.name=Impossible\n"
        "objective.0.kind=control\n"
        "objective.0.position=999,238\n"
        "objective.0.radius_m=28\n"
        "objective.0.value=1\n"
        "weapon.count=0\n"
        "civilian.count=0\n"
        "unit.count=0\n"
    );

    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_ERROR_INVALID_DATA);
}

static void test_invalid_score_thresholds_are_rejected(void) {
    char path[512];
    mk_scenario_definition_t scenario;

    make_binary_path(path, sizeof(path), "bad_score_thresholds.mkscenario");
    write_text_file(
        path,
        "format=modernerKrieg.scenario.v1\n"
        "name=Bad Score Thresholds\n"
        "seed=1\n"
        "score.success_threshold=50\n"
        "score.partial_threshold=100\n"
    );

    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_ERROR_INVALID_DATA);
}

static void test_invalid_score_weight_is_rejected(void) {
    char path[512];
    mk_scenario_definition_t scenario;

    make_binary_path(path, sizeof(path), "bad_score_weight.mkscenario");
    write_text_file(
        path,
        "format=modernerKrieg.scenario.v1\n"
        "name=Bad Score Weight\n"
        "seed=1\n"
        "score.objective_weight=-1\n"
    );

    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_ERROR_INVALID_DATA);
}

int main(void) {
    test_default_scenario_data_matches_fixture_shape();
    test_public_default_scenario_uses_data_file();
    test_control_smoke_scenario_loads();
    test_contested_risk_smoke_scenario_loads();
    test_missing_asset_manifest_is_rejected();
    test_market_scenario_requires_building_level_manifest();
    test_market_scenario_requires_topology_manifest();
    test_invalid_force_reference_is_rejected();
    test_impossible_objective_bounds_are_rejected();
    test_invalid_score_thresholds_are_rejected();
    test_invalid_score_weight_is_rejected();

    puts("mk_mosul_scenario_data_tests: ok");
    return 0;
}
