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

static mk_vec2_t mk_ai_clamp_to_map(const mk_game_t *game, mk_vec2_t position) {
    mk_vec2_t clamped = position;

    if (game == NULL) {
        return clamped;
    }

    clamped.x = mk_clamp_f32(clamped.x, 0.0f, game->map.width_m);
    clamped.y = mk_clamp_f32(clamped.y, 0.0f, game->map.height_m);
    return clamped;
}

static float mk_ai_distance_point_to_segment(mk_vec2_t point, mk_vec2_t segment_start, mk_vec2_t segment_end) {
    float dx = segment_end.x - segment_start.x;
    float dy = segment_end.y - segment_start.y;
    float length_squared = dx * dx + dy * dy;
    float t;
    mk_vec2_t closest;

    if (length_squared <= 0.0001f) {
        return mk_vec2_distance(point, segment_start);
    }

    t = ((point.x - segment_start.x) * dx + (point.y - segment_start.y) * dy) / length_squared;
    t = mk_clamp_f32(t, 0.0f, 1.0f);
    closest.x = segment_start.x + t * dx;
    closest.y = segment_start.y + t * dy;

    return mk_vec2_distance(point, closest);
}

static bool mk_ai_protected_civilian_near_position(const mk_game_t *game, mk_vec2_t position, float radius_m) {
    size_t index;

    if (game == NULL) {
        return false;
    }

    for (index = 0; index < game->civilian_count; ++index) {
        const mk_civilian_t *civilian = &game->civilians[index];

        if (!civilian->protected_noncombatant) {
            continue;
        }

        if (mk_vec2_distance(civilian->position_m, position) <= radius_m) {
            return true;
        }
    }

    return false;
}

static bool mk_ai_fire_lane_crosses_protected_civilian(
    const mk_game_t *game,
    const mk_unit_t *attacker,
    const mk_unit_t *target,
    float lane_radius_m
) {
    size_t index;

    if (game == NULL || attacker == NULL || target == NULL) {
        return false;
    }

    for (index = 0; index < game->civilian_count; ++index) {
        const mk_civilian_t *civilian = &game->civilians[index];

        if (!civilian->protected_noncombatant) {
            continue;
        }

        if (mk_ai_distance_point_to_segment(
                civilian->position_m,
                attacker->position_m,
                target->position_m
            ) <= lane_radius_m) {
            return true;
        }
    }

    return false;
}

static bool mk_ai_unit_was_recent_fire_target(const mk_game_t *game, uint32_t unit_id) {
    size_t index;

    if (game == NULL || unit_id == 0) {
        return false;
    }

    for (index = 0; index < game->contact_report_count; ++index) {
        const mk_contact_report_t *report = &game->contact_reports[index];

        if (report->kind != MK_CONTACT_REPORT_FIRE
            || report->target_unit_id != unit_id
            || !report->resolved
            || report->tick > game->tick
            || game->tick - report->tick > 1U) {
            continue;
        }

        return true;
    }

    return false;
}

static mk_result_t mk_ai_issue_withdraw_order(mk_game_t *game, mk_unit_t *unit, const mk_unit_t *threat) {
    mk_vec2_t target;
    float dx;
    float dy;
    float distance;

    if (game == NULL || unit == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (threat == NULL) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_RALLY);
    }

    dx = unit->position_m.x - threat->position_m.x;
    dy = unit->position_m.y - threat->position_m.y;
    distance = mk_vec2_distance(unit->position_m, threat->position_m);
    if (distance <= 0.0001f) {
        dx = 1.0f;
        dy = 0.0f;
        distance = 1.0f;
    }

    target.x = unit->position_m.x + dx / distance * 36.0f;
    target.y = unit->position_m.y + dy / distance * 36.0f;
    target = mk_ai_clamp_to_map(game, target);

    unit->target_position_m = target;
    unit->has_move_target = true;
    unit->order = MK_ORDER_WITHDRAW;
    return MK_OK;
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
    const mk_unit_t *enemy = mk_ai_find_nearest_enemy(game, unit);
    mk_line_of_sight_t line_of_sight;

    if (mk_ai_protected_civilian_near_position(game, unit->position_m, 24.0f)) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_HOLD);
    }

    if (unit->status == MK_UNIT_PINNED || unit->suppression >= 12) {
        return mk_ai_issue_withdraw_order(game, mk_game_find_unit(game, unit->id), enemy);
    }

    if (enemy != NULL
        && mk_vec2_distance(unit->position_m, enemy->position_m) <= 120.0f
        && mk_game_unit_line_of_sight(game, unit->id, enemy->id, &line_of_sight) == MK_OK
        && line_of_sight.visible) {
        if (mk_ai_fire_lane_crosses_protected_civilian(game, unit, enemy, 30.0f)) {
            return mk_game_issue_order(game, unit->id, MK_ORDER_HOLD);
        }

        return mk_game_issue_order(game, unit->id, MK_ORDER_SUPPRESS);
    }

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
    mk_line_of_sight_t line_of_sight;

    if (enemy == NULL) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_HOLD);
    }

    if ((unit->hidden && unit->revealed) || mk_ai_unit_was_recent_fire_target(game, unit->id)) {
        return mk_ai_issue_withdraw_order(game, mk_game_find_unit(game, unit->id), enemy);
    }

    if (unit->status == MK_UNIT_PINNED || unit->suppression >= 10) {
        return mk_ai_issue_withdraw_order(game, mk_game_find_unit(game, unit->id), enemy);
    }

    distance = mk_vec2_distance(unit->position_m, enemy->position_m);
    if (distance <= 120.0f
        && mk_game_unit_line_of_sight(game, unit->id, enemy->id, &line_of_sight) == MK_OK
        && line_of_sight.visible) {
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
