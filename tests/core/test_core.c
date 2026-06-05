#include "mk_core.h"
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

static mk_scenario_definition_t make_east_mosul_block_scenario_fixture(void) {
    mk_scenario_definition_t scenario;

    assert(mk_mosul_make_east_block_scenario(&scenario) == MK_OK);

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
    assert(strcmp(game.scenario_name, "East Mosul Block") == 0);
    assert(game.rng_state == scenario.seed);
    assert(strcmp(game.map.name, "Gogjali District Edge") == 0);
    assert(game.map.terrain_count == 3);
    assert(game.faction_count == 3);
    assert(game.objective_count == 1);
    assert(game.unit_count == 3);
    assert(game.units[0].faction_id == 1);
    assert(game.units[0].soldier_count == 3);
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
    assert(snapshot.units[0].suppression == 3);
    assert(snapshot.units[0].soldiers[0].suppression == 0);
    assert(snapshot.map.terrain[1].blocks_line_of_sight);
    assert(strcmp(snapshot.objectives[0].name, "Secure Compound") == 0);

    game.units[0].suppression = 99;
    assert(snapshot.units[0].suppression == 3);
}

static void test_pick_select_and_move_order(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;
    mk_unit_t *unit;
    uint32_t unit_id = 0;

    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(mk_game_pick_unit_at(&game, make_vec2(35.0f, 83.0f), MK_UNIT_PICK_RADIUS_M, &unit_id) == MK_OK);
    assert(unit_id == 1);
    assert(mk_game_select_unit_at(&game, make_vec2(35.0f, 83.0f), MK_UNIT_PICK_RADIUS_M, &unit_id) == MK_OK);
    assert(unit_id == 1);
    assert(game.selected_unit_id == 1);

    assert(mk_game_issue_selected_move_order(&game, make_vec2(46.0f, 82.0f)) == MK_OK);
    unit = mk_game_find_unit(&game, 1);
    assert(unit != NULL);
    assert(unit->order == MK_ORDER_MOVE);
    assert(unit->has_move_target);

    mk_game_step(&game);
    assert(unit->position_m.x == 40.0f);
    assert(unit->position_m.y == 82.0f);
    assert(unit->order == MK_ORDER_MOVE);

    mk_game_step(&game);
    assert(unit->position_m.x == 46.0f);
    assert(unit->position_m.y == 82.0f);
    assert(!unit->has_move_target);
    assert(unit->order == MK_ORDER_HOLD);
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
    assert(mk_game_select_unit(&game, 1) == MK_OK);
    assert(mk_game_issue_selected_move_order(&game, make_vec2(999.0f, 50.0f)) == MK_ERROR_INVALID_DATA);
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
    assert_close(line_of_sight.distance_m, 136.37f);
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
        make_rect(76.0f, 74.0f, 10.0f, 12.0f),
        4,
        4,
        true
    );
    assert(mk_map_add_terrain(&scenario.map, &wall, &wall_id) == MK_OK);
    assert(wall_id == 4);

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
    assert(stored_target->suppression == 50);
    assert(stored_target->status == MK_UNIT_BROKEN);
    assert(stored_target->soldiers[0].casualty);
    assert(stored_target->soldiers[1].casualty);
    assert(game.units[0].soldiers[0].ammo == 118);
    assert(game.units[0].soldiers[1].ammo == 118);
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
        make_rect(76.0f, 74.0f, 10.0f, 12.0f),
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

static void test_invalid_scenario_is_rejected(void) {
    mk_scenario_definition_t scenario = make_east_mosul_block_scenario_fixture();
    mk_game_t game;

    scenario.map.width_m = 0.0f;
    assert(mk_game_load_scenario(&game, &scenario) == MK_ERROR_INVALID_DATA);

    scenario = make_east_mosul_block_scenario_fixture();
    scenario.objectives[0].position_m = make_vec2(999.0f, 74.0f);
    assert(mk_game_load_scenario(&game, &scenario) == MK_ERROR_INVALID_DATA);

    scenario = make_east_mosul_block_scenario_fixture();
    scenario.units[0].faction_id = 99;
    assert(mk_game_load_scenario(&game, &scenario) == MK_ERROR_INVALID_DATA);
}

int main(void) {
    test_version_is_present();
    test_rng_is_deterministic();
    test_unit_and_soldier_creation();
    test_capacity_limits_are_reported();
    test_step_recovers_suppression();
    test_scenario_loading_populates_core_state();
    test_snapshot_is_stable_copy();
    test_pick_select_and_move_order();
    test_interaction_errors_are_reported();
    test_line_of_sight_reports_target_cover();
    test_line_of_sight_reports_blocking_terrain();
    test_line_of_sight_errors_are_reported();
    test_unit_fire_resolves_damage_and_suppression();
    test_unit_fire_blocked_by_line_of_sight();
    test_selected_unit_fire_uses_loaded_scenario();
    test_suppression_status_slows_movement_and_recovers();
    test_broken_unit_halts_movement();
    test_invalid_scenario_is_rejected();

    puts("mk_core_tests: ok");
    return 0;
}
