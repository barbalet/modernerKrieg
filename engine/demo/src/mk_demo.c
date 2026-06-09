#include "mk_demo.h"

#include "mk_ai.h"
#include "mk_mosul_demo.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MK_PROJECT_SOURCE_DIR
#define MK_PROJECT_SOURCE_DIR "."
#endif

struct mk_demo_session {
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_game_snapshot_t snapshot;
    mk_board_view_t view;
    bool has_game;
    bool has_snapshot;
    bool ai_only;
    float screen_width_px;
    float screen_height_px;
    float margin_px;
    char project_root[MK_PATH_CAPACITY];
    mk_demo_performance_counters_t counters;
};

static void mk_demo_copy_text(char *destination, size_t capacity, const char *source) {
    const char *text = source == NULL ? "" : source;
    size_t index = 0;

    if (destination == NULL || capacity == 0) {
        return;
    }

    for (; index + 1 < capacity && text[index] != '\0'; ++index) {
        destination[index] = text[index];
    }

    destination[index] = '\0';
}

static bool mk_demo_copy_traffic_vehicle_runtime_path(
    char *destination,
    size_t capacity,
    const char *sprite_id
) {
    const char *item_id = NULL;
    int written;

    if (destination == NULL || capacity == 0 || sprite_id == NULL) {
        return false;
    }

    if (strcmp(sprite_id, "traffic_civilian_car_intact_north") == 0) {
        item_id = "traffic_civilian_car";
    } else if (strcmp(sprite_id, "traffic_city_bus_intact_north") == 0) {
        item_id = "traffic_city_bus";
    } else if (strcmp(sprite_id, "traffic_motorcycle_intact_north") == 0) {
        item_id = "traffic_motorcycle";
    }

    if (item_id == NULL) {
        mk_demo_copy_text(destination, capacity, sprite_id);
        return false;
    }

    written = snprintf(
        destination,
        capacity,
        "assets/mosul/runtime/sprites/rendered/traffic_vehicles_1024/civilian/%s/intact/north.png",
        item_id
    );
    return written > 0 && (size_t)written < capacity;
}

static bool mk_demo_text_is_present(const char *text) {
    return text != NULL && text[0] != '\0';
}

static bool mk_demo_append_text(char *out_text, size_t capacity, size_t *in_out_length, const char *format, ...) {
    va_list args;
    int written;

    if (out_text == NULL || capacity == 0 || in_out_length == NULL || format == NULL) {
        return false;
    }
    if (*in_out_length >= capacity) {
        return false;
    }

    va_start(args, format);
    written = vsnprintf(out_text + *in_out_length, capacity - *in_out_length, format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= capacity - *in_out_length) {
        out_text[capacity - 1] = '\0';
        return false;
    }

    *in_out_length += (size_t)written;
    return true;
}

static const char *mk_demo_side_name(mk_side_t side) {
    switch (side) {
        case MK_SIDE_PLAYER:
            return "player";
        case MK_SIDE_OPFOR:
            return "opfor";
        case MK_SIDE_CIVILIAN:
            return "civilian";
        case MK_SIDE_NEUTRAL:
            return "neutral";
        default:
            return "unknown";
    }
}

static const char *mk_demo_order_name(mk_order_t order) {
    switch (order) {
        case MK_ORDER_NONE:
            return "none";
        case MK_ORDER_MOVE:
            return "move";
        case MK_ORDER_ASSAULT_MOVE:
            return "assault_move";
        case MK_ORDER_FIRE:
            return "fire";
        case MK_ORDER_HOLD:
            return "hold";
        case MK_ORDER_BREACH:
            return "breach";
        case MK_ORDER_RALLY:
            return "rally";
        case MK_ORDER_OVERWATCH:
            return "overwatch";
        case MK_ORDER_SUPPRESS:
            return "suppress";
        case MK_ORDER_WITHDRAW:
            return "withdraw";
        case MK_ORDER_INVESTIGATE:
            return "investigate";
        default:
            return "unknown";
    }
}

static const char *mk_demo_unit_status_name(mk_unit_status_t status) {
    switch (status) {
        case MK_UNIT_READY:
            return "ready";
        case MK_UNIT_SUPPRESSED:
            return "suppressed";
        case MK_UNIT_PINNED:
            return "pinned";
        case MK_UNIT_BROKEN:
            return "broken";
        default:
            return "unknown";
    }
}

static const char *mk_demo_civilian_state_name(mk_civilian_state_t state) {
    switch (state) {
        case MK_CIVILIAN_SHELTERING:
            return "sheltering";
        case MK_CIVILIAN_FLEEING:
            return "fleeing";
        case MK_CIVILIAN_FROZEN:
            return "frozen";
        case MK_CIVILIAN_FOLLOWING_INSTRUCTIONS:
            return "following_instructions";
        case MK_CIVILIAN_WOUNDED:
            return "wounded";
        case MK_CIVILIAN_DEAD:
            return "dead";
        case MK_CIVILIAN_EVACUATED:
            return "evacuated";
        default:
            return "unknown";
    }
}

static bool mk_demo_make_project_path(
    const char *project_root,
    const char *relative_path,
    char *out_path,
    size_t capacity
) {
    const char *root = project_root == NULL || project_root[0] == '\0' ? "." : project_root;
    int written;

    if (relative_path == NULL || out_path == NULL || capacity == 0) {
        return false;
    }

    if (relative_path[0] == '/') {
        size_t length = strlen(relative_path);

        if (length >= capacity) {
            return false;
        }

        memcpy(out_path, relative_path, length + 1);
        return true;
    }

    written = snprintf(out_path, capacity, "%s/%s", root, relative_path);
    return written > 0 && (size_t)written < capacity;
}

static mk_vec2_t mk_demo_rect_center(mk_rect_t rect) {
    mk_vec2_t center;

    center.x = rect.x + rect.width * 0.5f;
    center.y = rect.y + rect.height * 0.5f;
    return center;
}

static bool mk_demo_board_is_ready(const mk_demo_session_t *session) {
    return session != NULL
        && session->view.screen_rect_px.width > 0.0f
        && session->view.screen_rect_px.height > 0.0f
        && session->view.scale_px_per_m > 0.0f;
}

static mk_result_t mk_demo_refresh_snapshot(mk_demo_session_t *session) {
    mk_result_t result;

    if (session == NULL || !session->has_game) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    result = mk_game_snapshot(&session->game, &session->snapshot);
    if (result != MK_OK) {
        return result;
    }

    session->has_snapshot = true;
    return MK_OK;
}

static size_t mk_demo_ai_unit_count(const mk_game_t *game) {
    size_t index;
    size_t count = 0;

    if (game == NULL) {
        return 0;
    }

    for (index = 0; index < game->unit_count; ++index) {
        const mk_unit_t *unit = &game->units[index];

        if (unit->side != MK_SIDE_CIVILIAN && unit->status != MK_UNIT_BROKEN && unit->soldier_count > 0) {
            count += 1;
        }
    }

    return count;
}

static mk_demo_draw_command_t mk_demo_make_draw_command(
    mk_demo_draw_kind_t kind,
    uint32_t id,
    const char *stable_id,
    const char *label,
    mk_vec2_t position_m
) {
    mk_demo_draw_command_t command;

    memset(&command, 0, sizeof(command));
    command.kind = kind;
    command.id = id;
    mk_demo_copy_text(command.stable_id, sizeof(command.stable_id), stable_id);
    mk_demo_copy_text(command.label, sizeof(command.label), label);
    command.position_m = position_m;
    command.target_position_m = position_m;
    command.side = MK_SIDE_NEUTRAL;
    command.order = MK_ORDER_NONE;
    return command;
}

static mk_result_t mk_demo_push_draw_command(
    mk_demo_draw_command_t *out_commands,
    size_t command_capacity,
    size_t *in_out_index,
    const mk_demo_draw_command_t *command
) {
    if (out_commands == NULL || in_out_index == NULL || command == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (*in_out_index >= command_capacity) {
        return MK_ERROR_CAPACITY;
    }

    out_commands[*in_out_index] = *command;
    *in_out_index += 1;
    return MK_OK;
}

static bool mk_demo_unit_is_visible_to_frontend(const mk_unit_t *unit) {
    return unit != NULL && !(unit->hidden && !unit->revealed);
}

static bool mk_demo_unit_is_inside_traffic_vehicle(const mk_game_snapshot_t *snapshot, uint32_t unit_id) {
    size_t vehicle_index;

    if (snapshot == NULL || unit_id == 0U) {
        return false;
    }

    for (vehicle_index = 0; vehicle_index < snapshot->traffic_vehicle_count; ++vehicle_index) {
        const mk_traffic_vehicle_t *vehicle = &snapshot->traffic_vehicles[vehicle_index];
        size_t occupant_index;

        if (!vehicle->active || vehicle->boarding_mode != MK_TRAFFIC_BOARD_INSIDE) {
            continue;
        }

        for (occupant_index = 0; occupant_index < vehicle->embarked_unit_count; ++occupant_index) {
            if (vehicle->embarked_unit_ids[occupant_index] == unit_id) {
                return true;
            }
        }
    }

    return false;
}

static bool mk_demo_game_unit_is_inside_traffic_vehicle(const mk_game_t *game, uint32_t unit_id) {
    size_t vehicle_index;

    if (game == NULL || unit_id == 0U) {
        return false;
    }

    for (vehicle_index = 0; vehicle_index < game->traffic_vehicle_count; ++vehicle_index) {
        const mk_traffic_vehicle_t *vehicle = &game->traffic_vehicles[vehicle_index];
        size_t occupant_index;

        if (!vehicle->active || vehicle->boarding_mode != MK_TRAFFIC_BOARD_INSIDE) {
            continue;
        }

        for (occupant_index = 0; occupant_index < vehicle->embarked_unit_count; ++occupant_index) {
            if (vehicle->embarked_unit_ids[occupant_index] == unit_id) {
                return true;
            }
        }
    }

    return false;
}

static void mk_demo_fill_pick(
    mk_demo_session_t *session,
    mk_demo_pick_result_t *out_pick,
    mk_demo_pick_kind_t kind,
    uint32_t id,
    uint32_t secondary_id,
    const char *stable_id,
    const char *label,
    mk_side_t side,
    mk_vec2_t position_m
) {
    if (out_pick == NULL) {
        return;
    }

    memset(out_pick, 0, sizeof(*out_pick));
    out_pick->kind = kind;
    out_pick->id = id;
    out_pick->secondary_id = secondary_id;
    mk_demo_copy_text(out_pick->stable_id, sizeof(out_pick->stable_id), stable_id);
    mk_demo_copy_text(out_pick->label, sizeof(out_pick->label), label);
    out_pick->side = side;
    out_pick->position_m = position_m;
    out_pick->screen_position_px = mk_board_view_map_to_screen(&session->view, position_m);
}

mk_result_t mk_demo_session_create(
    const mk_demo_session_config_t *config,
    mk_demo_session_t **out_session
) {
    mk_demo_session_t *session;
    const char *project_root = MK_PROJECT_SOURCE_DIR;
    float screen_width_px = 1024.0f;
    float screen_height_px = 768.0f;
    float margin_px = MK_BOARD_VIEW_DEFAULT_MARGIN_PX;

    if (out_session == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    *out_session = NULL;
    session = (mk_demo_session_t *)calloc(1, sizeof(*session));
    if (session == NULL) {
        return MK_ERROR_CAPACITY;
    }

    if (config != NULL) {
        if (config->project_root != NULL) {
            project_root = config->project_root;
        }
        if (config->screen_width_px > 0.0f) {
            screen_width_px = config->screen_width_px;
        }
        if (config->screen_height_px > 0.0f) {
            screen_height_px = config->screen_height_px;
        }
        if (config->margin_px >= 0.0f) {
            margin_px = config->margin_px;
        }
        session->ai_only = config->ai_only;
    }

    mk_demo_copy_text(session->project_root, sizeof(session->project_root), project_root);
    session->screen_width_px = screen_width_px;
    session->screen_height_px = screen_height_px;
    session->margin_px = margin_px;
    session->view.screen_rect_px.width = screen_width_px;
    session->view.screen_rect_px.height = screen_height_px;
    session->view.scale_px_per_m = 1.0f;
    session->view.min_scale_px_per_m = 1.0f;
    session->view.max_scale_px_per_m = MK_BOARD_VIEW_DEFAULT_MAX_ZOOM_MULTIPLIER;
    session->view.origin_m = mk_vec2(0.0f, 0.0f);
    session->view.screen_rect_px.x = margin_px;
    session->view.screen_rect_px.y = margin_px;

    *out_session = session;
    return MK_OK;
}

void mk_demo_session_destroy(mk_demo_session_t *session) {
    free(session);
}

mk_result_t mk_demo_session_load_scenario(
    mk_demo_session_t *session,
    const char *scenario_path
) {
    mk_result_t result;
    char project_path[MK_PATH_CAPACITY];

    if (session == NULL || scenario_path == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    result = mk_mosul_load_scenario_file(scenario_path, session->project_root, &session->scenario);
    if (result == MK_ERROR_NOT_FOUND && scenario_path[0] != '\0' && scenario_path[0] != '/') {
        if (!mk_demo_make_project_path(session->project_root, scenario_path, project_path, sizeof(project_path))) {
            return MK_ERROR_INVALID_DATA;
        }
        result = mk_mosul_load_scenario_file(project_path, session->project_root, &session->scenario);
    }
    if (result != MK_OK) {
        return result;
    }

    result = mk_game_load_scenario(&session->game, &session->scenario);
    if (result != MK_OK) {
        return result;
    }

    session->has_game = true;
    result = mk_board_view_fit_map(
        &session->view,
        &session->game.map,
        session->screen_width_px,
        session->screen_height_px,
        session->margin_px
    );
    if (result != MK_OK) {
        return result;
    }

    return mk_demo_refresh_snapshot(session);
}

mk_result_t mk_demo_session_load_default(mk_demo_session_t *session) {
    return mk_demo_session_load_scenario(session, MK_MOSUL_DEFAULT_SCENARIO_PATH);
}

mk_result_t mk_demo_session_set_ai_only(mk_demo_session_t *session, bool ai_only) {
    if (session == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    session->ai_only = ai_only;
    return MK_OK;
}

mk_result_t mk_demo_session_step(mk_demo_session_t *session, uint32_t ticks) {
    uint32_t tick_index;

    if (session == NULL || !session->has_game) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    for (tick_index = 0; tick_index < ticks; ++tick_index) {
        if (session->ai_only) {
            session->counters.ai_order_batches += 1;
            session->counters.ai_units_considered += (uint64_t)mk_demo_ai_unit_count(&session->game);
            if (mk_ai_issue_basic_orders(&session->game) != MK_OK) {
                return MK_ERROR_INVALID_DATA;
            }
        }

        mk_game_step(&session->game);
        session->counters.fixed_ticks += 1;
    }

    return mk_demo_refresh_snapshot(session);
}

mk_result_t mk_demo_session_snapshot(mk_demo_session_t *session, mk_game_snapshot_t *out_snapshot) {
    mk_result_t result;

    if (session == NULL || out_snapshot == NULL || !session->has_game) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    result = mk_demo_refresh_snapshot(session);
    if (result != MK_OK) {
        return result;
    }

    *out_snapshot = session->snapshot;
    session->counters.snapshot_queries += 1;
    return MK_OK;
}

mk_result_t mk_demo_session_summary(mk_demo_session_t *session, mk_demo_summary_t *out_summary) {
    mk_result_t result;

    if (session == NULL || out_summary == NULL || !session->has_game) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    result = mk_demo_refresh_snapshot(session);
    if (result != MK_OK) {
        return result;
    }

    memset(out_summary, 0, sizeof(*out_summary));
    out_summary->tick = session->snapshot.tick;
    out_summary->selected_unit_id = session->snapshot.selected_unit_id;
    out_summary->unit_count = (uint32_t)session->snapshot.unit_count;
    out_summary->civilian_count = (uint32_t)session->snapshot.civilian_count;
    out_summary->objective_count = (uint32_t)session->snapshot.objective_count;
    out_summary->contact_report_count = (uint32_t)session->snapshot.contact_report_count;
    mk_demo_copy_text(out_summary->scenario_name, sizeof(out_summary->scenario_name), session->snapshot.scenario_name);
    mk_demo_copy_text(out_summary->gameplay_area_id, sizeof(out_summary->gameplay_area_id), session->snapshot.gameplay_area.id);
    mk_demo_copy_text(out_summary->map_name, sizeof(out_summary->map_name), session->snapshot.map.name);
    out_summary->map_width_m = session->snapshot.map.width_m;
    out_summary->map_height_m = session->snapshot.map.height_m;
    return mk_game_score(&session->game, &out_summary->score);
}

mk_result_t mk_demo_session_performance(
    const mk_demo_session_t *session,
    mk_demo_performance_counters_t *out_counters
) {
    if (session == NULL || out_counters == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    *out_counters = session->counters;
    return MK_OK;
}

mk_result_t mk_demo_session_reset_performance(mk_demo_session_t *session) {
    if (session == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(&session->counters, 0, sizeof(session->counters));
    return MK_OK;
}

mk_result_t mk_demo_session_after_action(
    mk_demo_session_t *session,
    mk_after_action_report_t *out_report
) {
    if (session == NULL || out_report == NULL || !session->has_game) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    return mk_game_after_action_report(&session->game, out_report);
}

mk_result_t mk_demo_session_audit(
    mk_demo_session_t *session,
    mk_demo_audit_report_t *out_report
) {
    const mk_gameplay_area_t *area;
    size_t index;

    if (session == NULL || out_report == NULL || !session->has_game) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_report, 0, sizeof(*out_report));
    area = &session->game.gameplay_area;
    out_report->objective_count = (uint32_t)session->game.objective_count;
    out_report->unit_count = (uint32_t)session->game.unit_count;
    out_report->civilian_count = (uint32_t)session->game.civilian_count;

    if (!mk_gameplay_area_is_loaded(area)) {
        out_report->warnings += 1U;
    } else {
        out_report->level_count = (uint32_t)area->level_count;
        out_report->feature_count = (uint32_t)area->feature_count;
        out_report->region_count = (uint32_t)area->region_count;
        out_report->topology_node_count = (uint32_t)area->topology_node_count;
        out_report->topology_portal_count = (uint32_t)area->topology_portal_count;
        out_report->semantic_zone_count = (uint32_t)area->semantic_zone_count;

        if (area->level_count == 0U) {
            out_report->warnings += 1U;
        }
        if (!mk_gameplay_area_topology_is_loaded(area)) {
            out_report->warnings += 1U;
        }
        if (area->semantic_zone_count == 0U) {
            out_report->warnings += 1U;
        }

        for (index = 0; index < area->level_count; ++index) {
            if (!mk_demo_text_is_present(area->levels[index].image_path)) {
                out_report->missing_level_image_paths += 1U;
                out_report->warnings += 1U;
            }
        }

        for (index = 0; index < area->topology_node_count; ++index) {
            if (!mk_demo_text_is_present(area->topology_nodes[index].id)) {
                out_report->empty_topology_node_ids += 1U;
                out_report->warnings += 1U;
            }
        }

        for (index = 0; index < area->topology_portal_count; ++index) {
            const mk_gameplay_topology_portal_t *portal = &area->topology_portals[index];

            if (strcmp(portal->state, "blocked") == 0
                || strcmp(portal->state, "locked") == 0
                || strcmp(portal->state, "unsafe") == 0) {
                out_report->blocked_or_unsafe_portals += 1U;
            }
            if (portal->breached) {
                out_report->breached_portals += 1U;
            }
            if (portal->searched) {
                out_report->searched_portals += 1U;
            }
        }

        for (index = 0; index < area->semantic_zone_count; ++index) {
            if (area->semantic_zones[index].searched) {
                out_report->searched_semantic_zones += 1U;
            }
        }
    }

    for (index = 0; index < session->game.unit_count; ++index) {
        out_report->unit_route_failures += session->game.units[index].route_failure_count;
    }
    for (index = 0; index < session->game.civilian_count; ++index) {
        out_report->civilian_route_failures += session->game.civilians[index].route_failure_count;
    }

    if (session->game.objective_count == 0U) {
        out_report->warnings += 1U;
    }
    if (session->game.unit_count == 0U) {
        out_report->warnings += 1U;
    }
    if (session->game.civilian_count == 0U) {
        out_report->warnings += 1U;
    }

    (void)snprintf(
        out_report->summary,
        sizeof(out_report->summary),
        "audit levels=%u features=%u regions=%u nodes=%u portals=%u zones=%u warnings=%u route_failures(unit=%u,civilian=%u)",
        (unsigned)out_report->level_count,
        (unsigned)out_report->feature_count,
        (unsigned)out_report->region_count,
        (unsigned)out_report->topology_node_count,
        (unsigned)out_report->topology_portal_count,
        (unsigned)out_report->semantic_zone_count,
        (unsigned)out_report->warnings,
        (unsigned)out_report->unit_route_failures,
        (unsigned)out_report->civilian_route_failures
    );

    return MK_OK;
}

mk_result_t mk_demo_session_debug_text(
    mk_demo_session_t *session,
    char *out_text,
    size_t capacity
) {
    mk_demo_summary_t summary;
    mk_after_action_report_t report;
    size_t length = 0;
    size_t index;
    const mk_unit_t *selected_unit = NULL;

    if (session == NULL || out_text == NULL || capacity == 0 || !session->has_game) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    out_text[0] = '\0';
    if (mk_demo_session_summary(session, &summary) != MK_OK) {
        return MK_ERROR_INVALID_DATA;
    }
    if (mk_game_after_action_report(&session->game, &report) != MK_OK) {
        return MK_ERROR_INVALID_DATA;
    }

    for (index = 0; index < session->snapshot.unit_count; ++index) {
        if (session->snapshot.units[index].id == session->snapshot.selected_unit_id) {
            selected_unit = &session->snapshot.units[index];
            break;
        }
    }

    if (!mk_demo_append_text(
            out_text,
            capacity,
            &length,
            "scenario=\"%s\" tick=%u score=%d outcome=%d map=\"%s\" area=\"%s\"\n",
            summary.scenario_name,
            (unsigned)summary.tick,
            summary.score.total_score,
            summary.score.outcome,
            summary.map_name,
            summary.gameplay_area_id
        )
        || !mk_demo_append_text(
            out_text,
            capacity,
            &length,
            "counts units=%u civilians=%u objectives=%u contacts=%u selected=%u\n",
            (unsigned)summary.unit_count,
            (unsigned)summary.civilian_count,
            (unsigned)summary.objective_count,
            (unsigned)summary.contact_report_count,
            (unsigned)summary.selected_unit_id
        )
        || !mk_demo_append_text(
            out_text,
            capacity,
            &length,
            "performance ticks=%llu ai_batches=%llu snapshots=%llu draws=%llu picks=%llu orders=%llu\n",
            (unsigned long long)session->counters.fixed_ticks,
            (unsigned long long)session->counters.ai_order_batches,
            (unsigned long long)session->counters.snapshot_queries,
            (unsigned long long)session->counters.draw_queries,
            (unsigned long long)session->counters.pick_queries,
            (unsigned long long)session->counters.order_requests
        )
        || !mk_demo_append_text(
            out_text,
            capacity,
            &length,
            "after_action %s\n",
            report.summary
        )) {
        return MK_ERROR_CAPACITY;
    }

    if (selected_unit != NULL) {
        if (!mk_demo_append_text(
                out_text,
                capacity,
                &length,
                "selected_unit id=%u side=%s order=%s status=%s pos=(%.2f,%.2f) target=(%.2f,%.2f) level=\"%s\" node=\"%s\" route_failures=%u reason=\"%s\"\n",
                (unsigned)selected_unit->id,
                mk_demo_side_name(selected_unit->side),
                mk_demo_order_name(selected_unit->order),
                mk_demo_unit_status_name(selected_unit->status),
                selected_unit->position_m.x,
                selected_unit->position_m.y,
                selected_unit->target_position_m.x,
                selected_unit->target_position_m.y,
                selected_unit->level_id,
                selected_unit->topology_node_id,
                (unsigned)selected_unit->route_failure_count,
                selected_unit->route_failure_reason
            )) {
            return MK_ERROR_CAPACITY;
        }
    } else if (!mk_demo_append_text(out_text, capacity, &length, "selected_unit none\n")) {
        return MK_ERROR_CAPACITY;
    }

    if (session->snapshot.civilian_count > 0U) {
        const mk_civilian_t *civilian = &session->snapshot.civilians[0];

        if (!mk_demo_append_text(
                out_text,
                capacity,
                &length,
                "civilian_sample id=%u state=%s intent=%s risk=%d stress=%d pos=(%.2f,%.2f) dest=(%.2f,%.2f) route_failures=%u reason=\"%s\"\n",
                (unsigned)civilian->id,
                mk_demo_civilian_state_name(civilian->state),
                mk_civilian_intent_name(civilian->intent),
                civilian->risk,
                civilian->stress,
                civilian->position_m.x,
                civilian->position_m.y,
                civilian->destination_m.x,
                civilian->destination_m.y,
                (unsigned)civilian->route_failure_count,
                civilian->route_failure_reason
            )) {
            return MK_ERROR_CAPACITY;
        }
    }

    return MK_OK;
}

mk_result_t mk_demo_session_topology_debug_text(
    mk_demo_session_t *session,
    char *out_text,
    size_t capacity
) {
    if (session == NULL || out_text == NULL || capacity == 0 || !session->has_game) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    return mk_gameplay_area_topology_debug_dump(&session->game.gameplay_area, out_text, capacity);
}

mk_result_t mk_demo_session_fit_board(
    mk_demo_session_t *session,
    float screen_width_px,
    float screen_height_px,
    float margin_px
) {
    if (session == NULL || !session->has_game) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    session->screen_width_px = screen_width_px;
    session->screen_height_px = screen_height_px;
    session->margin_px = margin_px >= 0.0f ? margin_px : MK_BOARD_VIEW_DEFAULT_MARGIN_PX;
    return mk_board_view_fit_map(&session->view, &session->game.map, screen_width_px, screen_height_px, margin_px);
}

mk_result_t mk_demo_session_collect_draw_commands(
    mk_demo_session_t *session,
    mk_demo_draw_command_t *out_commands,
    size_t command_capacity,
    size_t *out_command_count
) {
    mk_tactical_overlay_t overlays[MK_BOARD_VIEW_MAX_TACTICAL_OVERLAYS];
    mk_soldier_marker_t soldiers[MK_MAX_UNITS * MK_MAX_SOLDIERS_PER_UNIT];
    mk_traffic_vehicle_marker_t traffic_vehicles[MK_MAX_TRAFFIC_VEHICLES];
    size_t overlay_count = 0;
    size_t soldier_count = 0;
    size_t visible_soldier_count = 0;
    size_t traffic_vehicle_count = 0;
    size_t needed = 0;
    size_t index;
    size_t command_index = 0;
    mk_result_t result;

    if (session == NULL || out_command_count == NULL || !session->has_game || !mk_demo_board_is_ready(session)) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    result = mk_demo_refresh_snapshot(session);
    if (result != MK_OK) {
        return result;
    }

    result = mk_board_view_collect_tactical_overlays(
        &session->view,
        &session->snapshot,
        overlays,
        sizeof(overlays) / sizeof(overlays[0]),
        &overlay_count
    );
    if (result != MK_OK) {
        return result;
    }

    result = mk_board_view_collect_soldier_markers(
        &session->view,
        &session->snapshot,
        soldiers,
        sizeof(soldiers) / sizeof(soldiers[0]),
        &soldier_count
    );
    if (result != MK_OK) {
        return result;
    }

    result = mk_board_view_collect_traffic_vehicle_markers(
        &session->view,
        &session->snapshot,
        traffic_vehicles,
        sizeof(traffic_vehicles) / sizeof(traffic_vehicles[0]),
        &traffic_vehicle_count
    );
    if (result != MK_OK) {
        return result;
    }

    for (index = 0; index < soldier_count; ++index) {
        if (!mk_demo_unit_is_inside_traffic_vehicle(&session->snapshot, soldiers[index].unit_id)) {
            visible_soldier_count += 1;
        }
    }

    needed = session->snapshot.gameplay_area.level_count
        + session->snapshot.civilian_count
        + session->snapshot.objective_count
        + session->snapshot.contact_report_count
        + overlay_count
        + visible_soldier_count
        + traffic_vehicle_count;
    for (index = 0; index < session->snapshot.unit_count; ++index) {
        if (mk_demo_unit_is_visible_to_frontend(&session->snapshot.units[index])
            && !mk_demo_unit_is_inside_traffic_vehicle(&session->snapshot, session->snapshot.units[index].id)) {
            needed += 1;
        }
    }
    *out_command_count = needed;
    session->counters.draw_queries += 1;

    if (out_commands == NULL) {
        return command_capacity == 0 ? MK_OK : MK_ERROR_INVALID_ARGUMENT;
    }
    if (command_capacity < needed) {
        return MK_ERROR_CAPACITY;
    }

    for (index = 0; index < session->snapshot.gameplay_area.level_count; ++index) {
        const mk_gameplay_level_t *level = &session->snapshot.gameplay_area.levels[index];
        mk_demo_draw_command_t command = mk_demo_make_draw_command(
            MK_DEMO_DRAW_LEVEL,
            (uint32_t)(index + 1),
            level->id,
            level->id,
            mk_vec2(0.0f, 0.0f)
        );

        mk_demo_copy_text(command.asset_path, sizeof(command.asset_path), level->image_path);
        command.screen_position_px = mk_board_view_map_to_screen(&session->view, command.position_m);
        command.target_position_m = mk_vec2(session->snapshot.map.width_m, session->snapshot.map.height_m);
        command.target_screen_position_px = mk_board_view_map_to_screen(&session->view, command.target_position_m);
        command.intensity = level->index;
        result = mk_demo_push_draw_command(out_commands, command_capacity, &command_index, &command);
        if (result != MK_OK) {
            return result;
        }
    }

    for (index = 0; index < traffic_vehicle_count; ++index) {
        const mk_traffic_vehicle_marker_t *vehicle = &traffic_vehicles[index];
        mk_demo_draw_command_t command = mk_demo_make_draw_command(
            MK_DEMO_DRAW_TRAFFIC_VEHICLE,
            vehicle->vehicle_id,
            vehicle->sprite_id,
            vehicle->sprite_id,
            vehicle->position_m
        );

        (void)mk_demo_copy_traffic_vehicle_runtime_path(
            command.asset_path,
            sizeof(command.asset_path),
            vehicle->sprite_id
        );
        command.side = MK_SIDE_CIVILIAN;
        command.screen_position_px = vehicle->screen_position_px;
        command.target_position_m = vehicle->position_m;
        command.target_screen_position_px = vehicle->screen_position_px;
        command.radius_m = vehicle->seat_capacity > 4 ? 5.0f : 2.0f;
        command.screen_radius_px = command.radius_m * session->view.scale_px_per_m;
        command.facing_degrees = vehicle->facing_degrees;
        command.intensity = (int)vehicle->occupied_seats;
        result = mk_demo_push_draw_command(out_commands, command_capacity, &command_index, &command);
        if (result != MK_OK) {
            return result;
        }
    }

    for (index = 0; index < session->snapshot.unit_count; ++index) {
        const mk_unit_t *unit = &session->snapshot.units[index];
        mk_demo_draw_command_t command;

        if (!mk_demo_unit_is_visible_to_frontend(unit)
            || mk_demo_unit_is_inside_traffic_vehicle(&session->snapshot, unit->id)) {
            continue;
        }

        command = mk_demo_make_draw_command(
            MK_DEMO_DRAW_UNIT,
            unit->id,
            unit->command.callsign,
            unit->command.name[0] != '\0' ? unit->command.name : unit->name,
            unit->position_m
        );
        command.side = unit->side;
        command.order = unit->order;
        command.target_position_m = unit->has_move_target ? unit->target_position_m : unit->position_m;
        command.screen_position_px = mk_board_view_map_to_screen(&session->view, unit->position_m);
        command.target_screen_position_px = mk_board_view_map_to_screen(&session->view, command.target_position_m);
        command.selected = unit->id == session->snapshot.selected_unit_id;
        command.intensity = unit->suppression;
        result = mk_demo_push_draw_command(out_commands, command_capacity, &command_index, &command);
        if (result != MK_OK) {
            return result;
        }
    }

    for (index = 0; index < session->snapshot.civilian_count; ++index) {
        const mk_civilian_t *civilian = &session->snapshot.civilians[index];
        mk_demo_draw_command_t command = mk_demo_make_draw_command(
            MK_DEMO_DRAW_CIVILIAN,
            civilian->id,
            civilian->group_id,
            civilian->name,
            civilian->position_m
        );

        mk_demo_copy_text(command.asset_path, sizeof(command.asset_path), civilian->sprite_id);
        command.side = MK_SIDE_CIVILIAN;
        command.target_position_m = civilian->has_destination ? civilian->destination_m : civilian->position_m;
        command.screen_position_px = mk_board_view_map_to_screen(&session->view, civilian->position_m);
        command.target_screen_position_px = mk_board_view_map_to_screen(&session->view, command.target_position_m);
        command.intensity = civilian->risk;
        result = mk_demo_push_draw_command(out_commands, command_capacity, &command_index, &command);
        if (result != MK_OK) {
            return result;
        }
    }

    for (index = 0; index < soldier_count; ++index) {
        const mk_soldier_marker_t *soldier = &soldiers[index];
        mk_demo_draw_command_t command = mk_demo_make_draw_command(
            MK_DEMO_DRAW_SOLDIER,
            soldier->unit_id,
            "",
            "",
            soldier->position_m
        );

        if (mk_demo_unit_is_inside_traffic_vehicle(&session->snapshot, soldier->unit_id)) {
            continue;
        }

        command.secondary_id = soldier->soldier_id;
        command.side = soldier->side;
        command.screen_position_px = soldier->screen_position_px;
        command.target_position_m = soldier->position_m;
        command.target_screen_position_px = soldier->screen_position_px;
        command.selected = soldier->selected_unit;
        command.intensity = soldier->casualty ? 1 : 0;
        result = mk_demo_push_draw_command(out_commands, command_capacity, &command_index, &command);
        if (result != MK_OK) {
            return result;
        }
    }

    for (index = 0; index < session->snapshot.objective_count; ++index) {
        const mk_objective_t *objective = &session->snapshot.objectives[index];
        mk_demo_draw_command_t command = mk_demo_make_draw_command(
            MK_DEMO_DRAW_OBJECTIVE,
            objective->id,
            objective->name,
            objective->label,
            objective->position_m
        );

        command.side = objective->controlling_side;
        command.radius_m = objective->radius_m;
        command.screen_radius_px = objective->radius_m * session->view.scale_px_per_m;
        command.screen_position_px = mk_board_view_map_to_screen(&session->view, objective->position_m);
        command.target_position_m = objective->position_m;
        command.target_screen_position_px = command.screen_position_px;
        command.intensity = objective->value;
        result = mk_demo_push_draw_command(out_commands, command_capacity, &command_index, &command);
        if (result != MK_OK) {
            return result;
        }
    }

    for (index = 0; index < session->snapshot.contact_report_count; ++index) {
        const mk_contact_report_t *report = &session->snapshot.contact_reports[index];
        mk_demo_draw_command_t command = mk_demo_make_draw_command(
            MK_DEMO_DRAW_CONTACT,
            report->id,
            "",
            "",
            report->position_m
        );

        command.secondary_id = report->target_unit_id;
        command.side = report->side;
        command.screen_position_px = mk_board_view_map_to_screen(&session->view, report->position_m);
        command.target_position_m = report->target_position_m;
        command.target_screen_position_px = mk_board_view_map_to_screen(&session->view, report->target_position_m);
        command.intensity = report->confidence;
        result = mk_demo_push_draw_command(out_commands, command_capacity, &command_index, &command);
        if (result != MK_OK) {
            return result;
        }
    }

    for (index = 0; index < overlay_count; ++index) {
        const mk_tactical_overlay_t *overlay = &overlays[index];
        mk_demo_draw_command_t command = mk_demo_make_draw_command(
            MK_DEMO_DRAW_OVERLAY,
            overlay->unit_id != 0U ? overlay->unit_id : overlay->objective_id,
            "",
            "",
            overlay->position_m
        );

        command.overlay_kind = overlay->kind;
        command.secondary_id = overlay->terrain_id != 0U ? overlay->terrain_id : overlay->civilian_id;
        command.side = overlay->side;
        command.order = overlay->order;
        command.position_m = overlay->position_m;
        command.target_position_m = overlay->target_position_m;
        command.screen_position_px = overlay->screen_position_px;
        command.target_screen_position_px = overlay->target_screen_position_px;
        command.radius_m = overlay->radius_m;
        command.screen_radius_px = overlay->screen_radius_px;
        command.intensity = overlay->intensity;
        result = mk_demo_push_draw_command(out_commands, command_capacity, &command_index, &command);
        if (result != MK_OK) {
            return result;
        }
    }

    *out_command_count = command_index;
    return MK_OK;
}

mk_result_t mk_demo_session_pick_screen(
    mk_demo_session_t *session,
    mk_vec2_t screen_position_px,
    float radius_px,
    mk_demo_pick_result_t *out_pick
) {
    mk_vec2_t position_m;
    float radius_m;
    uint32_t id = 0;
    size_t index;
    const mk_gameplay_topology_node_t *node;
    const mk_gameplay_topology_portal_t *portal;
    const mk_gameplay_semantic_zone_t *zone;
    const char *level_id = "";

    if (session == NULL || out_pick == NULL || !session->has_game || !mk_demo_board_is_ready(session)) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_pick, 0, sizeof(*out_pick));
    out_pick->kind = MK_DEMO_PICK_NONE;
    session->counters.pick_queries += 1;
    position_m = mk_board_view_screen_to_map(&session->view, screen_position_px);
    radius_m = radius_px > 0.0f ? radius_px / session->view.scale_px_per_m : MK_UNIT_PICK_RADIUS_M;

    if (mk_game_pick_unit_at(&session->game, position_m, radius_m, &id) == MK_OK && id != 0U) {
        for (index = 0; index < session->game.unit_count; ++index) {
            const mk_unit_t *unit = &session->game.units[index];

            if (unit->id == id) {
                if (mk_demo_game_unit_is_inside_traffic_vehicle(&session->game, unit->id)) {
                    break;
                }
                mk_demo_fill_pick(
                    session,
                    out_pick,
                    MK_DEMO_PICK_UNIT,
                    unit->id,
                    0U,
                    unit->command.callsign,
                    unit->command.name[0] != '\0' ? unit->command.name : unit->name,
                    unit->side,
                    unit->position_m
                );
                return MK_OK;
            }
        }
    }

    if (mk_game_pick_contact_at(&session->game, position_m, radius_m, &id) == MK_OK && id != 0U) {
        for (index = 0; index < session->game.contact_report_count; ++index) {
            const mk_contact_report_t *report = &session->game.contact_reports[index];

            if (report->id == id) {
                mk_demo_fill_pick(session, out_pick, MK_DEMO_PICK_CONTACT, report->id, report->target_unit_id, "", "", report->side, report->position_m);
                return MK_OK;
            }
        }
    }

    for (index = 0; index < session->game.traffic_vehicle_count; ++index) {
        const mk_traffic_vehicle_t *vehicle = &session->game.traffic_vehicles[index];
        float vehicle_radius_m = vehicle->seat_capacity > 4 ? 5.0f : radius_m;

        if (!vehicle->active) {
            continue;
        }

        if (mk_vec2_distance(vehicle->position_m, position_m) <= vehicle_radius_m) {
            mk_demo_fill_pick(
                session,
                out_pick,
                MK_DEMO_PICK_TRAFFIC_VEHICLE,
                vehicle->id,
                0U,
                vehicle->scenario_id,
                vehicle->name,
                MK_SIDE_CIVILIAN,
                vehicle->position_m
            );
            return MK_OK;
        }
    }

    for (index = 0; index < session->game.civilian_count; ++index) {
        const mk_civilian_t *civilian = &session->game.civilians[index];

        if (mk_vec2_distance(civilian->position_m, position_m) <= radius_m) {
            mk_demo_fill_pick(
                session,
                out_pick,
                MK_DEMO_PICK_CIVILIAN,
                civilian->id,
                0U,
                civilian->group_id,
                civilian->name,
                MK_SIDE_CIVILIAN,
                civilian->position_m
            );
            return MK_OK;
        }
    }

    for (index = 0; index < session->game.objective_count; ++index) {
        const mk_objective_t *objective = &session->game.objectives[index];

        if (mk_vec2_distance(objective->position_m, position_m) <= objective->radius_m) {
            mk_demo_fill_pick(
                session,
                out_pick,
                MK_DEMO_PICK_OBJECTIVE,
                objective->id,
                0U,
                objective->name,
                objective->label,
                objective->controlling_side,
                objective->position_m
            );
            return MK_OK;
        }
    }

    for (index = 0; index < session->game.map.terrain_count; ++index) {
        const mk_terrain_zone_t *terrain = &session->game.map.terrain[index];

        if (mk_rect_contains_point(terrain->bounds_m, position_m)) {
            mk_demo_fill_pick(
                session,
                out_pick,
                MK_DEMO_PICK_TERRAIN,
                terrain->id,
                0U,
                terrain->name,
                terrain->name,
                MK_SIDE_NEUTRAL,
                mk_demo_rect_center(terrain->bounds_m)
            );
            return MK_OK;
        }
    }

    if (session->game.gameplay_area.level_count > 0) {
        level_id = session->game.gameplay_area.levels[0].id;
    }
    portal = mk_gameplay_area_find_topology_portal_at_world(&session->game.gameplay_area, level_id, position_m);
    if (portal != NULL) {
        mk_demo_fill_pick(session, out_pick, MK_DEMO_PICK_TOPOLOGY_PORTAL, 0U, 0U, portal->id, portal->kind, MK_SIDE_NEUTRAL, mk_demo_rect_center(portal->bounds_m));
        return MK_OK;
    }

    zone = mk_gameplay_area_find_semantic_zone_at_world(&session->game.gameplay_area, NULL, position_m);
    if (zone != NULL) {
        mk_demo_fill_pick(session, out_pick, MK_DEMO_PICK_SEMANTIC_ZONE, 0U, 0U, zone->id, zone->kind, MK_SIDE_NEUTRAL, mk_demo_rect_center(zone->bounds_m));
        return MK_OK;
    }

    node = mk_gameplay_area_find_topology_node_at_world(&session->game.gameplay_area, level_id, position_m);
    if (node != NULL) {
        mk_demo_fill_pick(session, out_pick, MK_DEMO_PICK_TOPOLOGY_NODE, 0U, 0U, node->id, node->label, MK_SIDE_NEUTRAL, mk_demo_rect_center(node->bounds_m));
        return MK_OK;
    }

    out_pick->position_m = position_m;
    out_pick->screen_position_px = screen_position_px;
    return MK_OK;
}

mk_result_t mk_demo_session_select_screen(
    mk_demo_session_t *session,
    mk_vec2_t screen_position_px,
    float radius_px,
    mk_demo_pick_result_t *out_pick
) {
    mk_demo_pick_result_t pick;
    mk_result_t result;

    if (session == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    result = mk_demo_session_pick_screen(session, screen_position_px, radius_px, &pick);
    if (result != MK_OK) {
        return result;
    }

    if (pick.kind == MK_DEMO_PICK_UNIT) {
        result = mk_game_select_unit(&session->game, pick.id);
    } else {
        result = mk_game_clear_selection(&session->game);
    }
    if (result != MK_OK) {
        return result;
    }

    if (out_pick != NULL) {
        *out_pick = pick;
    }
    return mk_demo_refresh_snapshot(session);
}

mk_result_t mk_demo_session_issue_order(
    mk_demo_session_t *session,
    uint32_t unit_id,
    mk_order_t order
) {
    mk_result_t result;

    if (session == NULL || !session->has_game) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    result = mk_game_issue_order(&session->game, unit_id, order);
    if (result == MK_OK) {
        session->counters.order_requests += 1;
        (void)mk_demo_refresh_snapshot(session);
    }

    return result;
}

mk_result_t mk_demo_session_issue_selected_move_screen(
    mk_demo_session_t *session,
    mk_vec2_t screen_position_px
) {
    mk_vec2_t position_m;
    mk_result_t result;

    if (session == NULL || !session->has_game || !mk_demo_board_is_ready(session)) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    position_m = mk_board_view_screen_to_map(&session->view, screen_position_px);
    result = mk_game_issue_selected_move_order(&session->game, position_m);
    if (result == MK_OK) {
        session->counters.order_requests += 1;
        (void)mk_demo_refresh_snapshot(session);
    }

    return result;
}
