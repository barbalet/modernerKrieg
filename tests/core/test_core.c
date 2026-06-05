#include "mk_core.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

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
    position.x = 10.0f;
    position.y = 12.0f;
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
    mk_vec2_t position = { 0.0f, 0.0f };
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

    position.x = 0.0f;
    position.y = 0.0f;
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

int main(void) {
    test_version_is_present();
    test_rng_is_deterministic();
    test_unit_and_soldier_creation();
    test_capacity_limits_are_reported();
    test_step_recovers_suppression();

    puts("mk_core_tests: ok");
    return 0;
}
