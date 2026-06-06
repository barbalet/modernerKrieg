#include "mk_ai.h"

#include <float.h>

static const mk_controller_slot_t *mk_ai_find_controller(const mk_game_t *game, uint32_t controller_id) {
    size_t index;

    if (game == NULL || controller_id == 0) {
        return NULL;
    }

    for (index = 0; index < game->controller_count; ++index) {
        if (game->controllers[index].id == controller_id) {
            return &game->controllers[index];
        }
    }

    return NULL;
}

static bool mk_ai_controller_can_emit_orders(const mk_controller_slot_t *controller) {
    return controller != NULL
        && (controller->kind == MK_CONTROLLER_TACTICAL_AI || controller->kind == MK_CONTROLLER_SCRIPTED_AI);
}

static bool mk_ai_unit_can_receive_orders(const mk_unit_t *unit) {
    return unit != NULL
        && unit->side != MK_SIDE_CIVILIAN
        && unit->status != MK_UNIT_BROKEN
        && unit->soldier_count > 0;
}

static bool mk_ai_find_first_objective(const mk_game_t *game, mk_vec2_t *out_position) {
    if (game == NULL || out_position == NULL || game->objective_count == 0) {
        return false;
    }

    *out_position = game->objectives[0].position_m;
    return true;
}

static const mk_unit_t *mk_ai_find_nearest_enemy(const mk_game_t *game, const mk_unit_t *unit) {
    const mk_unit_t *nearest = NULL;
    float nearest_distance = FLT_MAX;
    size_t index;

    if (game == NULL || unit == NULL) {
        return NULL;
    }

    for (index = 0; index < game->unit_count; ++index) {
        const mk_unit_t *candidate = &game->units[index];
        float distance;

        if (candidate->id == unit->id
            || candidate->side == MK_SIDE_CIVILIAN
            || candidate->side == unit->side
            || candidate->soldier_count == 0
            || candidate->status == MK_UNIT_BROKEN) {
            continue;
        }

        distance = mk_vec2_distance(unit->position_m, candidate->position_m);
        if (distance < nearest_distance) {
            nearest = candidate;
            nearest_distance = distance;
        }
    }

    return nearest;
}

static mk_result_t mk_ai_issue_player_order(mk_game_t *game, const mk_unit_t *unit) {
    mk_vec2_t objective_position;

    if (!mk_ai_find_first_objective(game, &objective_position)) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_HOLD);
    }

    if (mk_vec2_distance(unit->position_m, objective_position) <= MK_UNIT_PICK_RADIUS_M) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_HOLD);
    }

    return mk_game_issue_move_order(game, unit->id, objective_position);
}

static mk_result_t mk_ai_issue_opfor_order(mk_game_t *game, const mk_unit_t *unit) {
    const mk_unit_t *enemy = mk_ai_find_nearest_enemy(game, unit);
    float distance;

    if (enemy == NULL) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_HOLD);
    }

    distance = mk_vec2_distance(unit->position_m, enemy->position_m);
    if (distance <= 120.0f) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_SUPPRESS);
    }

    return mk_game_issue_move_order(game, unit->id, enemy->position_m);
}

mk_result_t mk_ai_issue_basic_orders(mk_game_t *game) {
    uint32_t unit_ids[MK_MAX_UNITS];
    size_t unit_count;
    size_t index;

    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    unit_count = game->unit_count;
    for (index = 0; index < unit_count; ++index) {
        unit_ids[index] = game->units[index].id;
    }

    for (index = 0; index < unit_count; ++index) {
        mk_unit_t *unit = mk_game_find_unit(game, unit_ids[index]);
        const mk_controller_slot_t *controller;
        mk_result_t result;

        if (!mk_ai_unit_can_receive_orders(unit)) {
            continue;
        }

        controller = mk_ai_find_controller(game, unit->controller_id);
        if (!mk_ai_controller_can_emit_orders(controller)) {
            continue;
        }

        if (unit->side == MK_SIDE_PLAYER) {
            result = mk_ai_issue_player_order(game, unit);
        } else if (unit->side == MK_SIDE_OPFOR) {
            result = mk_ai_issue_opfor_order(game, unit);
        } else {
            result = MK_OK;
        }

        if (result != MK_OK) {
            return result;
        }

        unit->order_source = MK_ORDER_SOURCE_AI;
    }

    return MK_OK;
}
