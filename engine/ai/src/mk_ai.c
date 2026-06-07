#include "mk_ai.h"

#include <float.h>
#include <string.h>

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

static bool mk_ai_find_best_objective(
    const mk_game_t *game,
    const mk_unit_t *unit,
    mk_side_t side,
    mk_vec2_t *out_position
) {
    const mk_objective_t *best_objective = NULL;
    float best_score = -FLT_MAX;
    size_t index;

    if (game == NULL || unit == NULL || out_position == NULL || game->objective_count == 0) {
        return false;
    }

    for (index = 0; index < game->objective_count; ++index) {
        const mk_objective_t *objective = &game->objectives[index];
        float distance = mk_vec2_distance(unit->position_m, objective->position_m);
        float score = (float)objective->value * 100.0f - distance;

        if (side == MK_SIDE_PLAYER) {
            if (objective->controlling_side == MK_SIDE_PLAYER) {
                score -= 120.0f;
            } else if (objective->controlling_side == MK_SIDE_OPFOR) {
                score += 80.0f;
            }
        } else if (side == MK_SIDE_OPFOR) {
            if (objective->controlling_side == MK_SIDE_OPFOR) {
                score -= 80.0f;
            } else if (objective->controlling_side == MK_SIDE_PLAYER) {
                score += 100.0f;
            }
        }

        if (best_objective == NULL || score > best_score) {
            best_objective = objective;
            best_score = score;
        }
    }

    if (best_objective == NULL) {
        return false;
    }

    *out_position = best_objective->position_m;
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

static mk_vec2_t mk_ai_rect_center(mk_rect_t rect) {
    mk_vec2_t center;

    center.x = rect.x + rect.width * 0.5f;
    center.y = rect.y + rect.height * 0.5f;
    return center;
}

static const mk_terrain_zone_t *mk_ai_find_terrain(const mk_game_t *game, uint32_t terrain_id) {
    size_t index;

    if (game == NULL || terrain_id == 0U) {
        return NULL;
    }

    for (index = 0; index < game->map.terrain_count; ++index) {
        if (game->map.terrain[index].id == terrain_id) {
            return &game->map.terrain[index];
        }
    }

    return NULL;
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

    return mk_game_issue_withdraw_order(game, unit->id, target);
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
            || candidate->status == MK_UNIT_BROKEN
            || (candidate->hidden && !candidate->revealed)) {
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

static bool mk_ai_contact_needs_investigation(const mk_game_t *game, const mk_contact_report_t *report) {
    const mk_terrain_zone_t *terrain;

    if (game == NULL || report == NULL || report->resolved || !report->visible) {
        return false;
    }

    if (report->kind == MK_CONTACT_REPORT_SUSPECTED_DANGER) {
        return true;
    }

    if (report->kind != MK_CONTACT_REPORT_FALSE_CONTACT || report->terrain_id == 0U) {
        return false;
    }

    terrain = mk_ai_find_terrain(game, report->terrain_id);
    return terrain == NULL || !terrain->searched;
}

static const mk_contact_report_t *mk_ai_find_best_investigation_contact(const mk_game_t *game, const mk_unit_t *unit) {
    const mk_contact_report_t *best_report = NULL;
    int best_confidence = -1;
    size_t index;

    if (game == NULL || unit == NULL) {
        return NULL;
    }

    for (index = 0; index < game->contact_report_count; ++index) {
        const mk_contact_report_t *report = &game->contact_reports[index];

        if (!mk_ai_contact_needs_investigation(game, report)
            || report->side == MK_SIDE_CIVILIAN
            || report->side == unit->side) {
            continue;
        }

        if (best_report == NULL || report->confidence > best_confidence) {
            best_report = report;
            best_confidence = report->confidence;
        }
    }

    return best_report;
}

static const mk_gameplay_semantic_zone_t *mk_ai_find_nearby_unsearched_search_zone(
    const mk_game_t *game,
    const mk_unit_t *unit,
    float radius_m
) {
    const mk_gameplay_semantic_zone_t *best_zone = NULL;
    float best_distance = FLT_MAX;
    size_t index;

    if (game == NULL || unit == NULL) {
        return NULL;
    }

    for (index = 0; index < game->gameplay_area.semantic_zone_count; ++index) {
        const mk_gameplay_semantic_zone_t *zone = &game->gameplay_area.semantic_zones[index];
        mk_vec2_t center;
        float distance;
        bool searchable_kind;

        if (zone->searched) {
            continue;
        }

        searchable_kind = strcmp(zone->kind, "cache") == 0
            || strcmp(zone->kind, "search_objective") == 0
            || strcmp(zone->kind, "danger_area") == 0;
        if (!searchable_kind) {
            continue;
        }

        if (unit->level_id[0] != '\0'
            && zone->level_id[0] != '\0'
            && strcmp(unit->level_id, zone->level_id) != 0) {
            continue;
        }

        center = mk_ai_rect_center(zone->bounds_m);
        distance = mk_vec2_distance(unit->position_m, center);
        if (distance <= radius_m && distance < best_distance) {
            best_zone = zone;
            best_distance = distance;
        }
    }

    return best_zone;
}

static bool mk_ai_portal_can_be_breached(const mk_gameplay_topology_portal_t *portal) {
    if (portal == NULL) {
        return false;
    }

    return (strcmp(portal->state, "closed") == 0 || strcmp(portal->state, "locked") == 0)
        && strcmp(portal->kind, "window") != 0
        && strcmp(portal->kind, "roof_edge") != 0
        && strcmp(portal->state, "blocked") != 0
        && strcmp(portal->state, "unsafe") != 0;
}

static const mk_gameplay_topology_node_t *mk_ai_find_unit_topology_node(
    const mk_game_t *game,
    const mk_unit_t *unit
) {
    if (game == NULL || unit == NULL || unit->topology_node_id[0] == '\0') {
        return NULL;
    }

    return mk_gameplay_area_find_topology_node(&game->gameplay_area, unit->topology_node_id);
}

static bool mk_ai_unit_is_in_defensible_node(const mk_game_t *game, const mk_unit_t *unit) {
    const mk_gameplay_topology_node_t *node = mk_ai_find_unit_topology_node(game, unit);

    if (node == NULL) {
        return false;
    }

    return strcmp(node->kind, "roof") == 0
        || strcmp(node->kind, "shop") == 0
        || strcmp(node->kind, "workshop") == 0
        || strcmp(node->kind, "garage") == 0
        || strcmp(node->kind, "alley") == 0
        || strcmp(node->kind, "cache") == 0;
}

static const mk_gameplay_topology_portal_t *mk_ai_find_nearby_breachable_portal(
    const mk_game_t *game,
    const mk_unit_t *unit,
    float radius_m
) {
    const mk_gameplay_topology_portal_t *best_portal = NULL;
    float best_distance = FLT_MAX;
    size_t index;

    if (game == NULL || unit == NULL) {
        return NULL;
    }

    for (index = 0; index < game->gameplay_area.topology_portal_count; ++index) {
        const mk_gameplay_topology_portal_t *portal = &game->gameplay_area.topology_portals[index];
        mk_vec2_t center;
        float distance;
        bool same_node;

        if (!mk_ai_portal_can_be_breached(portal)) {
            continue;
        }

        if (unit->level_id[0] != '\0'
            && portal->level_id[0] != '\0'
            && strcmp(unit->level_id, portal->level_id) != 0) {
            continue;
        }

        same_node = strcmp(unit->topology_node_id, portal->from_node_id) == 0
            || strcmp(unit->topology_node_id, portal->to_node_id) == 0;
        center = mk_ai_rect_center(portal->bounds_m);
        distance = mk_vec2_distance(unit->position_m, center);
        if ((same_node || distance <= radius_m) && distance < best_distance) {
            best_portal = portal;
            best_distance = distance;
        }
    }

    return best_portal;
}

static mk_result_t mk_ai_search_contact_or_hold(mk_game_t *game, const mk_unit_t *unit, const mk_contact_report_t *contact) {
    const mk_terrain_zone_t *terrain;
    mk_search_result_t search_result;

    if (game == NULL || unit == NULL || contact == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (mk_vec2_distance(unit->position_m, contact->position_m) > 18.0f) {
        return mk_game_issue_investigate_order(game, unit->id, contact->position_m);
    }

    terrain = mk_ai_find_terrain(game, contact->terrain_id);
    if (terrain != NULL && !terrain->searched) {
        return mk_game_search_terrain(game, unit->id, terrain->id, &search_result);
    }

    return mk_game_issue_order(game, unit->id, MK_ORDER_OVERWATCH);
}

static mk_result_t mk_ai_issue_player_order(mk_game_t *game, const mk_unit_t *unit) {
    mk_vec2_t objective_position;
    const mk_unit_t *enemy = mk_ai_find_nearest_enemy(game, unit);
    const mk_contact_report_t *investigation_contact = mk_ai_find_best_investigation_contact(game, unit);
    const mk_gameplay_topology_portal_t *breach_portal;
    const mk_gameplay_semantic_zone_t *search_zone;
    mk_line_of_sight_t line_of_sight;
    mk_breach_result_t breach_result;
    mk_search_result_t search_result;

    if (mk_ai_protected_civilian_near_position(game, unit->position_m, 24.0f)) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_HOLD);
    }

    if (unit->status == MK_UNIT_PINNED || unit->suppression >= 12) {
        return mk_ai_issue_withdraw_order(game, mk_game_find_unit(game, unit->id), enemy);
    }

    if (investigation_contact != NULL) {
        return mk_ai_search_contact_or_hold(game, unit, investigation_contact);
    }

    breach_portal = mk_ai_find_nearby_breachable_portal(game, unit, 20.0f);
    if (breach_portal != NULL) {
        return mk_game_breach_portal(game, unit->id, breach_portal->id, &breach_result);
    }

    search_zone = mk_ai_find_nearby_unsearched_search_zone(game, unit, 22.0f);
    if (search_zone != NULL) {
        return mk_game_search_semantic_zone(game, unit->id, search_zone->id, &search_result);
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

    if (!mk_ai_find_best_objective(game, unit, MK_SIDE_PLAYER, &objective_position)) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_HOLD);
    }

    if (mk_vec2_distance(unit->position_m, objective_position) <= MK_UNIT_PICK_RADIUS_M) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_HOLD);
    }

    return mk_game_issue_move_order(game, unit->id, objective_position);
}

static mk_result_t mk_ai_issue_opfor_order(mk_game_t *game, const mk_unit_t *unit) {
    const mk_unit_t *enemy = mk_ai_find_nearest_enemy(game, unit);
    mk_vec2_t objective_position;
    float distance;
    mk_line_of_sight_t line_of_sight;

    if (enemy == NULL) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_HOLD);
    }

    if ((unit->hidden && unit->revealed) || mk_ai_unit_was_recent_fire_target(game, unit->id)) {
        return mk_ai_issue_withdraw_order(game, mk_game_find_unit(game, unit->id), enemy);
    }

    if (unit->hidden && !unit->revealed && mk_ai_unit_is_in_defensible_node(game, unit)) {
        return mk_game_issue_order(game, unit->id, MK_ORDER_OVERWATCH);
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

    if (mk_ai_find_best_objective(game, unit, MK_SIDE_OPFOR, &objective_position)
        && mk_vec2_distance(unit->position_m, objective_position) < distance) {
        return mk_game_issue_move_order(game, unit->id, objective_position);
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
