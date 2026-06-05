#include "mk_mosul_demo.h"

#include <stdio.h>
#include <string.h>

static mk_vec2_t mk_mosul_vec2(float x, float y) {
    mk_vec2_t value;

    value.x = x;
    value.y = y;

    return value;
}

static mk_rect_t mk_mosul_rect(float x, float y, float width, float height) {
    mk_rect_t value;

    value.x = x;
    value.y = y;
    value.width = width;
    value.height = height;

    return value;
}

static mk_result_t mk_mosul_add_core_terrain(mk_scenario_definition_t *scenario) {
    mk_terrain_zone_t road = mk_make_terrain_zone(
        "Main Road",
        MK_TERRAIN_ROAD,
        mk_mosul_rect(0.0f, 68.0f, 240.0f, 24.0f),
        0,
        1,
        false
    );
    mk_terrain_zone_t compound = mk_make_terrain_zone(
        "Objective Compound",
        MK_TERRAIN_BUILDING,
        mk_mosul_rect(148.0f, 44.0f, 52.0f, 60.0f),
        3,
        2,
        true
    );
    mk_terrain_zone_t rubble = mk_make_terrain_zone(
        "Rubble Choke",
        MK_TERRAIN_RUBBLE,
        mk_mosul_rect(96.0f, 92.0f, 38.0f, 30.0f),
        2,
        3,
        true
    );
    mk_result_t result;

    result = mk_map_add_terrain(&scenario->map, &road, NULL);
    if (result != MK_OK) {
        return result;
    }

    result = mk_map_add_terrain(&scenario->map, &compound, NULL);
    if (result != MK_OK) {
        return result;
    }

    return mk_map_add_terrain(&scenario->map, &rubble, NULL);
}

static mk_result_t mk_mosul_add_factions(
    mk_scenario_definition_t *scenario,
    uint32_t *out_cts_faction_id,
    uint32_t *out_defender_faction_id,
    uint32_t *out_civilian_faction_id
) {
    mk_faction_t cts_faction = mk_make_faction("Iraqi CTS", MK_SIDE_PLAYER, mk_make_color(80, 132, 170, 255));
    mk_faction_t defender_faction = mk_make_faction("ISIS Defensive Cell", MK_SIDE_OPFOR, mk_make_color(132, 69, 55, 255));
    mk_faction_t civilian_faction = mk_make_faction("Civilians", MK_SIDE_CIVILIAN, mk_make_color(184, 166, 112, 255));
    mk_result_t result;

    result = mk_scenario_add_faction(scenario, &cts_faction, out_cts_faction_id);
    if (result != MK_OK) {
        return result;
    }

    result = mk_scenario_add_faction(scenario, &defender_faction, out_defender_faction_id);
    if (result != MK_OK) {
        return result;
    }

    return mk_scenario_add_faction(scenario, &civilian_faction, out_civilian_faction_id);
}

static mk_result_t mk_mosul_add_objectives(mk_scenario_definition_t *scenario) {
    mk_objective_t objective = mk_make_objective(
        "Secure Compound",
        MK_OBJECTIVE_CONTROL,
        mk_mosul_vec2(174.0f, 74.0f),
        14.0f,
        5
    );

    return mk_scenario_add_objective(scenario, &objective, NULL);
}

static mk_result_t mk_mosul_add_cts_unit(mk_scenario_definition_t *scenario, uint32_t faction_id) {
    mk_weapon_profile_t m4 = mk_make_weapon("M4", 300, 2, 35, 8);
    mk_unit_t unit = mk_make_unit(
        "CTS Assault Element",
        MK_SIDE_PLAYER,
        MK_TRAINING_ELITE,
        mk_mosul_vec2(34.0f, 82.0f)
    );
    mk_soldier_t soldier;
    mk_result_t result;

    unit.faction_id = faction_id;

    soldier = mk_make_soldier("CTS Lead", MK_ROLE_LEADER, m4);
    soldier.offset_m = mk_mosul_vec2(-2.0f, -2.0f);
    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    soldier = mk_make_soldier("CTS Rifleman", MK_ROLE_RIFLEMAN, m4);
    soldier.offset_m = mk_mosul_vec2(2.0f, -1.5f);
    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    soldier = mk_make_soldier("CTS Engineer", MK_ROLE_ENGINEER, m4);
    soldier.offset_m = mk_mosul_vec2(0.0f, 2.0f);
    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    return mk_scenario_add_unit(scenario, &unit, NULL);
}

static mk_result_t mk_mosul_add_defender_unit(mk_scenario_definition_t *scenario, uint32_t faction_id) {
    mk_weapon_profile_t akm = mk_make_weapon("AKM", 250, 2, 30, 7);
    mk_weapon_profile_t rpg = mk_make_weapon("RPG-7", 200, 1, 80, 12);
    mk_unit_t unit = mk_make_unit(
        "Hidden Defensive Cell",
        MK_SIDE_OPFOR,
        MK_TRAINING_REGULAR,
        mk_mosul_vec2(170.0f, 72.0f)
    );
    mk_soldier_t soldier;
    mk_result_t result;

    unit.faction_id = faction_id;

    soldier = mk_make_soldier("Defender Rifleman", MK_ROLE_RIFLEMAN, akm);
    soldier.offset_m = mk_mosul_vec2(-1.5f, 0.0f);
    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    soldier = mk_make_soldier("RPG Gunner", MK_ROLE_RPG, rpg);
    soldier.offset_m = mk_mosul_vec2(1.5f, 0.0f);
    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    return mk_scenario_add_unit(scenario, &unit, NULL);
}

static mk_result_t mk_mosul_add_civilians(mk_scenario_definition_t *scenario, uint32_t faction_id) {
    mk_weapon_profile_t unarmed = mk_make_weapon("Unarmed", 0, 0, 0, 0);
    mk_unit_t unit = mk_make_unit(
        "Sheltered Civilians",
        MK_SIDE_CIVILIAN,
        MK_TRAINING_UNTRAINED,
        mk_mosul_vec2(132.0f, 48.0f)
    );
    mk_soldier_t soldier = mk_make_soldier("Civilian Adult", MK_ROLE_CIVILIAN, unarmed);
    mk_result_t result;

    unit.faction_id = faction_id;
    soldier.ammo = 0;

    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    return mk_scenario_add_unit(scenario, &unit, NULL);
}

mk_result_t mk_mosul_make_east_block_scenario(mk_scenario_definition_t *out_scenario) {
    uint32_t cts_faction_id = 0;
    uint32_t defender_faction_id = 0;
    uint32_t civilian_faction_id = 0;
    mk_result_t result;

    if (out_scenario == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_scenario, 0, sizeof(*out_scenario));
    snprintf(out_scenario->name, sizeof(out_scenario->name), "%s", "East Mosul Block");
    out_scenario->seed = UINT64_C(0x4D4F53554C);
    out_scenario->map = mk_make_map("Gogjali District Edge", 240.0f, 160.0f);

    result = mk_mosul_add_core_terrain(out_scenario);
    if (result != MK_OK) {
        return result;
    }

    result = mk_mosul_add_factions(out_scenario, &cts_faction_id, &defender_faction_id, &civilian_faction_id);
    if (result != MK_OK) {
        return result;
    }

    result = mk_mosul_add_objectives(out_scenario);
    if (result != MK_OK) {
        return result;
    }

    result = mk_mosul_add_cts_unit(out_scenario, cts_faction_id);
    if (result != MK_OK) {
        return result;
    }

    result = mk_mosul_add_defender_unit(out_scenario, defender_faction_id);
    if (result != MK_OK) {
        return result;
    }

    return mk_mosul_add_civilians(out_scenario, civilian_faction_id);
}
