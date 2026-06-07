#include "mk_mosul_demo.h"

#include <stdio.h>
#include <string.h>

#ifndef MK_PROJECT_SOURCE_DIR
#define MK_PROJECT_SOURCE_DIR "."
#endif

static bool mk_mosul_make_default_scenario_path(char *out_path, size_t capacity) {
    int written;

    if (out_path == NULL || capacity == 0) {
        return false;
    }

    written = snprintf(out_path, capacity, "%s/%s", MK_PROJECT_SOURCE_DIR, MK_MOSUL_DEFAULT_SCENARIO_PATH);
    return written > 0 && (size_t)written < capacity;
}

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
        "Commercial Street",
        MK_TERRAIN_ROAD,
        mk_mosul_rect(0.0f, 220.0f, 500.0f, 52.0f),
        0,
        1,
        false
    );
    mk_terrain_zone_t compound = mk_make_terrain_zone(
        "Market Shops",
        MK_TERRAIN_BUILDING,
        mk_mosul_rect(286.0f, 152.0f, 104.0f, 128.0f),
        3,
        2,
        true
    );
    mk_terrain_zone_t rubble = mk_make_terrain_zone(
        "Blocked Alley",
        MK_TERRAIN_RUBBLE,
        mk_mosul_rect(180.0f, 296.0f, 76.0f, 68.0f),
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

static mk_result_t mk_mosul_configure_tiles(mk_scenario_definition_t *scenario) {
    int x;
    mk_result_t result;

    result = mk_map_configure_tiles(&scenario->map, 10, 10, 50.0f, 50.0f, MK_TERRAIN_OPEN);
    if (result != MK_OK) {
        return result;
    }

    for (x = 0; x < 10; ++x) {
        mk_map_tile_t road = mk_make_map_tile(
            mk_ivec2(x, 4),
            MK_TERRAIN_ROAD,
            0,
            0,
            1,
            false,
            false
        );
        result = mk_map_set_tile(&scenario->map, &road);
        if (result != MK_OK) {
            return result;
        }
    }

    {
        mk_map_tile_t compound = mk_make_map_tile(
            mk_ivec2(6, 4),
            MK_TERRAIN_BUILDING,
            1,
            3,
            2,
            true,
            false
        );
        result = mk_map_set_tile(&scenario->map, &compound);
        if (result != MK_OK) {
            return result;
        }
    }

    {
        mk_map_tile_t rubble = mk_make_map_tile(
            mk_ivec2(4, 6),
            MK_TERRAIN_RUBBLE,
            0,
            2,
            3,
            true,
            false
        );
        result = mk_map_set_tile(&scenario->map, &rubble);
        if (result != MK_OK) {
            return result;
        }
    }

    return MK_OK;
}

static mk_result_t mk_mosul_add_controllers(
    mk_scenario_definition_t *scenario,
    uint32_t *out_cts_controller_id,
    uint32_t *out_defender_controller_id,
    uint32_t *out_civilian_controller_id
) {
    mk_controller_slot_t cts = mk_make_controller_slot("US Patrol Tactical AI", MK_SIDE_PLAYER, MK_CONTROLLER_TACTICAL_AI);
    mk_controller_slot_t defender = mk_make_controller_slot("Armed Threat Tactical AI", MK_SIDE_OPFOR, MK_CONTROLLER_TACTICAL_AI);
    mk_controller_slot_t civilians = mk_make_controller_slot("Civilian Observer", MK_SIDE_CIVILIAN, MK_CONTROLLER_OBSERVER);
    mk_result_t result;

    result = mk_scenario_add_controller(scenario, &cts, out_cts_controller_id);
    if (result != MK_OK) {
        return result;
    }

    result = mk_scenario_add_controller(scenario, &defender, out_defender_controller_id);
    if (result != MK_OK) {
        return result;
    }

    return mk_scenario_add_controller(scenario, &civilians, out_civilian_controller_id);
}

static mk_result_t mk_mosul_add_factions(
    mk_scenario_definition_t *scenario,
    uint32_t *out_cts_faction_id,
    uint32_t *out_defender_faction_id,
    uint32_t *out_civilian_faction_id
) {
    mk_faction_t cts_faction = mk_make_faction("US Army Patrol", MK_SIDE_PLAYER, mk_make_color(80, 132, 170, 255));
    mk_faction_t defender_faction = mk_make_faction("Armed Irregular Cell", MK_SIDE_OPFOR, mk_make_color(132, 69, 55, 255));
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

static mk_result_t mk_mosul_add_forces(
    mk_scenario_definition_t *scenario,
    uint32_t cts_faction_id,
    uint32_t defender_faction_id,
    uint32_t civilian_faction_id,
    uint32_t cts_controller_id,
    uint32_t defender_controller_id,
    uint32_t civilian_controller_id,
    uint32_t *out_cts_force_id,
    uint32_t *out_defender_force_id,
    uint32_t *out_civilian_force_id
) {
    mk_force_t cts = mk_make_force("US Patrol Force", MK_SIDE_PLAYER, cts_faction_id, cts_controller_id);
    mk_force_t defender = mk_make_force("Armed Irregular Cell", MK_SIDE_OPFOR, defender_faction_id, defender_controller_id);
    mk_force_t civilians = mk_make_force("Civilian Population", MK_SIDE_CIVILIAN, civilian_faction_id, civilian_controller_id);
    mk_result_t result;

    cts.command = mk_make_command_identity("US Army Security Patrol", "PATROL-1", MK_SIDE_PLAYER);
    defender.command = mk_make_command_identity("Local Armed Cell", "CELL-1", MK_SIDE_OPFOR);
    civilians.command = mk_make_command_identity("Protected Noncombatants", "CIV", MK_SIDE_CIVILIAN);

    result = mk_scenario_add_force(scenario, &cts, out_cts_force_id);
    if (result != MK_OK) {
        return result;
    }

    result = mk_scenario_add_force(scenario, &defender, out_defender_force_id);
    if (result != MK_OK) {
        return result;
    }

    return mk_scenario_add_force(scenario, &civilians, out_civilian_force_id);
}

static mk_result_t mk_mosul_add_objectives(mk_scenario_definition_t *scenario) {
    mk_objective_t objective = mk_make_objective(
        "Secure Market Junction",
        MK_OBJECTIVE_CONTROL,
        mk_mosul_vec2(330.0f, 238.0f),
        28.0f,
        5
    );

    snprintf(objective.label, sizeof(objective.label), "%s", "Market Junction");
    return mk_scenario_add_objective(scenario, &objective, NULL);
}

static mk_result_t mk_mosul_add_cts_unit(
    mk_scenario_definition_t *scenario,
    uint32_t faction_id,
    uint32_t force_id,
    uint32_t controller_id
) {
    mk_weapon_profile_t m4 = mk_make_weapon("M4", 300, 2, 35, 8);
    mk_unit_t unit = mk_make_unit(
        "US Patrol Element",
        MK_SIDE_PLAYER,
        MK_TRAINING_VETERAN,
        mk_mosul_vec2(80.0f, 246.0f)
    );
    mk_soldier_t soldier;
    mk_result_t result;

    unit.faction_id = faction_id;
    unit.force_id = force_id;
    unit.controller_id = controller_id;
    unit.command = mk_make_command_identity("US Patrol Element", "PATROL-1A", MK_SIDE_PLAYER);

    soldier = mk_make_soldier("Patrol Lead", MK_ROLE_LEADER, m4);
    soldier.offset_m = mk_mosul_vec2(-4.0f, -3.0f);
    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    soldier = mk_make_soldier("Patrol Rifleman", MK_ROLE_RIFLEMAN, m4);
    soldier.offset_m = mk_mosul_vec2(4.0f, -2.0f);
    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    soldier = mk_make_soldier("Patrol Automatic Rifleman", MK_ROLE_MACHINE_GUNNER, m4);
    soldier.offset_m = mk_mosul_vec2(0.0f, 4.0f);
    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    return mk_scenario_add_unit(scenario, &unit, NULL);
}

static mk_result_t mk_mosul_add_defender_unit(
    mk_scenario_definition_t *scenario,
    uint32_t faction_id,
    uint32_t force_id,
    uint32_t controller_id
) {
    mk_weapon_profile_t akm = mk_make_weapon("AKM", 250, 2, 30, 7);
    mk_weapon_profile_t rpg = mk_make_weapon("RPG-7", 200, 1, 80, 12);
    mk_unit_t unit = mk_make_unit(
        "Armed Threat Cell",
        MK_SIDE_OPFOR,
        MK_TRAINING_REGULAR,
        mk_mosul_vec2(350.0f, 230.0f)
    );
    mk_soldier_t soldier;
    mk_result_t result;

    unit.faction_id = faction_id;
    unit.force_id = force_id;
    unit.controller_id = controller_id;
    unit.command = mk_make_command_identity("Armed Threat Cell", "CELL-1A", MK_SIDE_OPFOR);
    unit.hidden = true;
    unit.revealed = false;
    unit.concealment = 18;

    soldier = mk_make_soldier("Armed Rifleman", MK_ROLE_RIFLEMAN, akm);
    soldier.offset_m = mk_mosul_vec2(-3.0f, 0.0f);
    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    soldier = mk_make_soldier("RPG Gunner", MK_ROLE_RPG, rpg);
    soldier.offset_m = mk_mosul_vec2(3.0f, 0.0f);
    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    return mk_scenario_add_unit(scenario, &unit, NULL);
}

static mk_result_t mk_mosul_add_civilians(
    mk_scenario_definition_t *scenario,
    uint32_t faction_id,
    uint32_t force_id,
    uint32_t controller_id
) {
    mk_weapon_profile_t unarmed = mk_make_weapon("Unarmed", 0, 0, 0, 0);
    mk_unit_t unit = mk_make_unit(
        "Sheltered Civilians",
        MK_SIDE_CIVILIAN,
        MK_TRAINING_UNTRAINED,
        mk_mosul_vec2(252.0f, 206.0f)
    );
    mk_soldier_t soldier = mk_make_soldier("Civilian Adult", MK_ROLE_CIVILIAN, unarmed);
    mk_civilian_t civilian = mk_make_civilian("Civilian Adult", faction_id, mk_mosul_vec2(252.0f, 206.0f));
    mk_result_t result;

    unit.faction_id = faction_id;
    unit.force_id = force_id;
    unit.controller_id = controller_id;
    unit.command = mk_make_command_identity("Sheltered Civilians", "CIV-1", MK_SIDE_CIVILIAN);
    soldier.ammo = 0;

    result = mk_unit_add_soldier(&unit, &soldier, NULL);
    if (result != MK_OK) {
        return result;
    }

    result = mk_scenario_add_civilian(scenario, &civilian, NULL);
    if (result != MK_OK) {
        return result;
    }

    return mk_scenario_add_unit(scenario, &unit, NULL);
}

mk_result_t mk_mosul_make_market_2003_fixture_scenario(mk_scenario_definition_t *out_scenario) {
    uint32_t cts_controller_id = 0;
    uint32_t defender_controller_id = 0;
    uint32_t civilian_controller_id = 0;
    uint32_t cts_faction_id = 0;
    uint32_t defender_faction_id = 0;
    uint32_t civilian_faction_id = 0;
    uint32_t cts_force_id = 0;
    uint32_t defender_force_id = 0;
    uint32_t civilian_force_id = 0;
    mk_result_t result;

    if (out_scenario == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_scenario, 0, sizeof(*out_scenario));
    snprintf(out_scenario->name, sizeof(out_scenario->name), "%s", "Market Commercial Streets 2003");
    snprintf(
        out_scenario->briefing,
        sizeof(out_scenario->briefing),
        "%s",
        "Secure the market junction, identify armed threats, and keep protected civilians out of the line of fire."
    );
    snprintf(
        out_scenario->after_action_success,
        sizeof(out_scenario->after_action_success),
        "%s",
        "The patrol secured the junction while preserving civilian safety and force cohesion."
    );
    snprintf(
        out_scenario->after_action_partial,
        sizeof(out_scenario->after_action_partial),
        "%s",
        "The patrol made progress, but civilian risk, casualties, or delay kept the result limited."
    );
    snprintf(
        out_scenario->after_action_failure,
        sizeof(out_scenario->after_action_failure),
        "%s",
        "The patrol did not secure the market junction within acceptable risk."
    );
    out_scenario->seed = UINT64_C(0x4D4B32303033);
    out_scenario->score_success_threshold = MK_DEFAULT_SCORE_SUCCESS_THRESHOLD;
    out_scenario->score_partial_threshold = MK_DEFAULT_SCORE_PARTIAL_THRESHOLD;
    out_scenario->score_objective_weight = MK_DEFAULT_SCORE_OBJECTIVE_WEIGHT;
    out_scenario->score_civilian_risk_weight = MK_DEFAULT_SCORE_CIVILIAN_RISK_WEIGHT;
    out_scenario->score_player_casualty_weight = MK_DEFAULT_SCORE_PLAYER_CASUALTY_WEIGHT;
    out_scenario->score_civilian_casualty_weight = MK_DEFAULT_SCORE_CIVILIAN_CASUALTY_WEIGHT;
    out_scenario->score_time_weight = MK_DEFAULT_SCORE_TIME_WEIGHT;
    out_scenario->map = mk_make_map("Market / Commercial Streets", 500.0f, 500.0f);

    result = mk_mosul_configure_tiles(out_scenario);
    if (result != MK_OK) {
        return result;
    }

    result = mk_mosul_add_core_terrain(out_scenario);
    if (result != MK_OK) {
        return result;
    }

    result = mk_mosul_add_controllers(
        out_scenario,
        &cts_controller_id,
        &defender_controller_id,
        &civilian_controller_id
    );
    if (result != MK_OK) {
        return result;
    }

    result = mk_mosul_add_factions(out_scenario, &cts_faction_id, &defender_faction_id, &civilian_faction_id);
    if (result != MK_OK) {
        return result;
    }

    result = mk_mosul_add_forces(
        out_scenario,
        cts_faction_id,
        defender_faction_id,
        civilian_faction_id,
        cts_controller_id,
        defender_controller_id,
        civilian_controller_id,
        &cts_force_id,
        &defender_force_id,
        &civilian_force_id
    );
    if (result != MK_OK) {
        return result;
    }

    result = mk_mosul_add_objectives(out_scenario);
    if (result != MK_OK) {
        return result;
    }

    result = mk_mosul_add_cts_unit(out_scenario, cts_faction_id, cts_force_id, cts_controller_id);
    if (result != MK_OK) {
        return result;
    }

    result = mk_mosul_add_defender_unit(out_scenario, defender_faction_id, defender_force_id, defender_controller_id);
    if (result != MK_OK) {
        return result;
    }

    return mk_mosul_add_civilians(out_scenario, civilian_faction_id, civilian_force_id, civilian_controller_id);
}

mk_result_t mk_mosul_make_market_2003_scenario(mk_scenario_definition_t *out_scenario) {
    char scenario_path[512];

    if (out_scenario == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!mk_mosul_make_default_scenario_path(scenario_path, sizeof(scenario_path))) {
        return MK_ERROR_INVALID_DATA;
    }

    return mk_mosul_load_scenario_file(scenario_path, MK_PROJECT_SOURCE_DIR, out_scenario);
}

mk_result_t mk_mosul_make_east_block_scenario(mk_scenario_definition_t *out_scenario) {
    return mk_mosul_make_market_2003_scenario(out_scenario);
}
