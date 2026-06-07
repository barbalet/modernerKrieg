#include "mk_board_view.h"

#include <string.h>

static float mk_render_min_float(float a, float b) {
    return a < b ? a : b;
}

static float mk_render_clamp_float(float value, float minimum, float maximum) {
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

static bool mk_render_map_is_valid(const mk_map_t *map) {
    return map != NULL && map->width_m > 0.0f && map->height_m > 0.0f;
}

static bool mk_render_view_is_valid(const mk_board_view_t *view) {
    return view != NULL
        && view->screen_rect_px.width > 0.0f
        && view->screen_rect_px.height > 0.0f
        && view->scale_px_per_m > 0.0f;
}

static mk_tactical_overlay_t mk_board_view_make_overlay(
    const mk_board_view_t *view,
    mk_tactical_overlay_kind_t kind,
    mk_vec2_t position_m,
    float radius_m
) {
    mk_tactical_overlay_t overlay;

    memset(&overlay, 0, sizeof(overlay));
    overlay.kind = kind;
    overlay.position_m = position_m;
    overlay.target_position_m = position_m;
    overlay.screen_position_px = mk_board_view_map_to_screen(view, position_m);
    overlay.target_screen_position_px = overlay.screen_position_px;
    overlay.radius_m = radius_m;
    overlay.screen_radius_px = radius_m * view->scale_px_per_m;
    return overlay;
}

static mk_result_t mk_board_view_push_overlay(
    mk_tactical_overlay_t *out_overlays,
    size_t overlay_capacity,
    size_t *overlay_index,
    const mk_tactical_overlay_t *overlay
) {
    if (out_overlays == NULL || overlay_index == NULL || overlay == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (*overlay_index >= overlay_capacity) {
        return MK_ERROR_CAPACITY;
    }

    out_overlays[*overlay_index] = *overlay;
    *overlay_index += 1;
    return MK_OK;
}

static bool mk_board_view_unit_should_show_order_status(const mk_unit_t *unit) {
    return unit != NULL
        && unit->side != MK_SIDE_CIVILIAN
        && !(unit->hidden && !unit->revealed)
        && unit->order != MK_ORDER_NONE;
}

static bool mk_board_view_interaction_overlay_kind(
    mk_terrain_kind_t terrain_kind,
    mk_tactical_overlay_kind_t *out_overlay_kind
) {
    if (out_overlay_kind == NULL) {
        return false;
    }

    if (terrain_kind == MK_TERRAIN_BREACH_POINT) {
        *out_overlay_kind = MK_TACTICAL_OVERLAY_BREACH_SEARCH;
        return true;
    }

    if (terrain_kind == MK_TERRAIN_ROOFTOP) {
        *out_overlay_kind = MK_TACTICAL_OVERLAY_ROOFTOP_ACCESS;
        return true;
    }

    if (terrain_kind == MK_TERRAIN_SUSPECTED_IED) {
        *out_overlay_kind = MK_TACTICAL_OVERLAY_SEARCH_CACHE;
        return true;
    }

    return false;
}

static mk_vec2_t mk_board_view_rect_center(mk_rect_t rect) {
    mk_vec2_t center;

    center.x = rect.x + rect.width * 0.5f;
    center.y = rect.y + rect.height * 0.5f;
    return center;
}

static void mk_board_view_clamp_origin(mk_board_view_t *view, const mk_map_t *map) {
    float visible_width_m;
    float visible_height_m;
    float max_origin_x;
    float max_origin_y;

    if (!mk_render_view_is_valid(view) || !mk_render_map_is_valid(map)) {
        return;
    }

    visible_width_m = view->screen_rect_px.width / view->scale_px_per_m;
    visible_height_m = view->screen_rect_px.height / view->scale_px_per_m;
    max_origin_x = map->width_m > visible_width_m ? map->width_m - visible_width_m : 0.0f;
    max_origin_y = map->height_m > visible_height_m ? map->height_m - visible_height_m : 0.0f;

    view->origin_m.x = mk_render_clamp_float(view->origin_m.x, 0.0f, max_origin_x);
    view->origin_m.y = mk_render_clamp_float(view->origin_m.y, 0.0f, max_origin_y);
}

mk_result_t mk_board_view_fit_map(
    mk_board_view_t *out_view,
    const mk_map_t *map,
    float screen_width_px,
    float screen_height_px,
    float margin_px
) {
    float margin = margin_px >= 0.0f ? margin_px : MK_BOARD_VIEW_DEFAULT_MARGIN_PX;
    float available_width = screen_width_px - margin * 2.0f;
    float available_height = screen_height_px - margin * 2.0f;
    float scale_x;
    float scale_y;

    if (out_view == NULL || !mk_render_map_is_valid(map) || available_width <= 0.0f || available_height <= 0.0f) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_view, 0, sizeof(*out_view));
    out_view->screen_rect_px.x = margin;
    out_view->screen_rect_px.y = margin;
    out_view->screen_rect_px.width = available_width;
    out_view->screen_rect_px.height = available_height;

    scale_x = available_width / map->width_m;
    scale_y = available_height / map->height_m;
    out_view->scale_px_per_m = mk_render_min_float(scale_x, scale_y);
    out_view->min_scale_px_per_m = out_view->scale_px_per_m;
    out_view->max_scale_px_per_m = out_view->scale_px_per_m * MK_BOARD_VIEW_DEFAULT_MAX_ZOOM_MULTIPLIER;
    out_view->origin_m.x = 0.0f;
    out_view->origin_m.y = 0.0f;

    return MK_OK;
}

mk_result_t mk_board_view_collect_tactical_overlays(
    const mk_board_view_t *view,
    const mk_game_snapshot_t *snapshot,
    mk_tactical_overlay_t *out_overlays,
    size_t overlay_capacity,
    size_t *out_overlay_count
) {
    size_t needed = 0;
    size_t overlay_index = 0;
    size_t objective_index;
    size_t terrain_index;
    size_t unit_index;
    size_t civilian_index;
    size_t contact_index;

    if (!mk_render_view_is_valid(view) || snapshot == NULL || out_overlay_count == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    needed += snapshot->objective_count;
    for (objective_index = 0; objective_index < snapshot->objective_count; ++objective_index) {
        if (snapshot->objectives[objective_index].controlling_side != MK_SIDE_NEUTRAL) {
            needed += 1;
        }
    }

    for (terrain_index = 0; terrain_index < snapshot->map.terrain_count; ++terrain_index) {
        mk_tactical_overlay_kind_t ignored_kind;

        if (mk_board_view_interaction_overlay_kind(snapshot->map.terrain[terrain_index].kind, &ignored_kind)) {
            needed += 1;
        }
    }

    for (contact_index = 0; contact_index < snapshot->contact_report_count; ++contact_index) {
        mk_contact_report_kind_t kind = snapshot->contact_reports[contact_index].kind;

        if (kind == MK_CONTACT_REPORT_FIRE
            || ((kind == MK_CONTACT_REPORT_SUSPECTED_DANGER || kind == MK_CONTACT_REPORT_FALSE_CONTACT)
                && !snapshot->contact_reports[contact_index].resolved)) {
            needed += 1;
        }
    }

    for (unit_index = 0; unit_index < snapshot->unit_count; ++unit_index) {
        const mk_unit_t *unit = &snapshot->units[unit_index];
        size_t soldier_index;

        if (unit->hidden && !unit->revealed) {
            needed += 1;
        }

        if (mk_board_view_unit_should_show_order_status(unit)) {
            needed += 1;
        }

        if (unit->id == snapshot->selected_unit_id) {
            needed += 1;
        }

        if (unit->has_move_target) {
            needed += 2;
        }

        if (unit->suppression > 0 || unit->status != MK_UNIT_READY) {
            needed += 1;
        }

        for (soldier_index = 0; soldier_index < unit->soldier_count; ++soldier_index) {
            if (unit->soldiers[soldier_index].casualty) {
                needed += 1;
            }
        }
    }

    for (civilian_index = 0; civilian_index < snapshot->civilian_count; ++civilian_index) {
        if (snapshot->civilians[civilian_index].risk > 0) {
            needed += 1;
        }
    }

    *out_overlay_count = needed;

    if (out_overlays == NULL) {
        return overlay_capacity == 0 ? MK_OK : MK_ERROR_INVALID_ARGUMENT;
    }

    if (overlay_capacity < needed) {
        return MK_ERROR_CAPACITY;
    }

    for (terrain_index = 0; terrain_index < snapshot->map.terrain_count; ++terrain_index) {
        const mk_terrain_zone_t *terrain = &snapshot->map.terrain[terrain_index];
        mk_tactical_overlay_kind_t overlay_kind;

        if (mk_board_view_interaction_overlay_kind(terrain->kind, &overlay_kind)) {
            float radius_m = mk_render_min_float(terrain->bounds_m.width, terrain->bounds_m.height) * 0.5f;
            mk_tactical_overlay_t overlay = mk_board_view_make_overlay(
                view,
                overlay_kind,
                mk_board_view_rect_center(terrain->bounds_m),
                radius_m
            );
            mk_result_t result;

            overlay.terrain_id = terrain->id;
            overlay.intensity = terrain->cover + terrain->movement_cost;
            result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &overlay);
            if (result != MK_OK) {
                return result;
            }
        }
    }

    for (contact_index = 0; contact_index < snapshot->contact_report_count; ++contact_index) {
        const mk_contact_report_t *report = &snapshot->contact_reports[contact_index];

        if (report->kind == MK_CONTACT_REPORT_FIRE) {
            mk_tactical_overlay_t overlay = mk_board_view_make_overlay(
                view,
                MK_TACTICAL_OVERLAY_FIRE,
                report->position_m,
                0.0f
            );
            mk_result_t result;

            overlay.unit_id = report->attacker_unit_id;
            overlay.side = report->side;
            overlay.target_position_m = report->target_position_m;
            overlay.target_screen_position_px = mk_board_view_map_to_screen(view, report->target_position_m);
            overlay.intensity = report->suppression_added;
            result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &overlay);
            if (result != MK_OK) {
                return result;
            }
        } else if (report->kind == MK_CONTACT_REPORT_SUSPECTED_DANGER
            || report->kind == MK_CONTACT_REPORT_FALSE_CONTACT) {
            mk_tactical_overlay_kind_t kind = report->kind == MK_CONTACT_REPORT_SUSPECTED_DANGER
                ? MK_TACTICAL_OVERLAY_SUSPECTED_CONTACT
                : MK_TACTICAL_OVERLAY_FALSE_CONTACT;
            mk_tactical_overlay_t overlay = mk_board_view_make_overlay(
                view,
                kind,
                report->position_m,
                report->kind == MK_CONTACT_REPORT_SUSPECTED_DANGER ? 10.0f : 8.0f
            );
            mk_result_t result;

            if (report->resolved) {
                continue;
            }

            overlay.unit_id = report->target_unit_id;
            overlay.terrain_id = report->terrain_id;
            overlay.side = report->side;
            overlay.intensity = report->confidence;
            result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &overlay);
            if (result != MK_OK) {
                return result;
            }
        }
    }

    for (objective_index = 0; objective_index < snapshot->objective_count; ++objective_index) {
        const mk_objective_t *objective = &snapshot->objectives[objective_index];
        mk_tactical_overlay_t overlay = mk_board_view_make_overlay(
            view,
            MK_TACTICAL_OVERLAY_OBJECTIVE,
            objective->position_m,
            objective->radius_m
        );
        mk_result_t result;

        overlay.objective_id = objective->id;
        overlay.side = objective->controlling_side;
        overlay.intensity = objective->value;
        result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &overlay);
        if (result != MK_OK) {
            return result;
        }

        if (objective->controlling_side != MK_SIDE_NEUTRAL) {
            mk_tactical_overlay_t control_overlay = mk_board_view_make_overlay(
                view,
                MK_TACTICAL_OVERLAY_OBJECTIVE_CONTROL,
                objective->position_m,
                objective->radius_m * 0.5f
            );

            control_overlay.objective_id = objective->id;
            control_overlay.side = objective->controlling_side;
            control_overlay.intensity = objective->value;
            result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &control_overlay);
            if (result != MK_OK) {
                return result;
            }
        }
    }

    for (unit_index = 0; unit_index < snapshot->unit_count; ++unit_index) {
        const mk_unit_t *unit = &snapshot->units[unit_index];
        size_t soldier_index;

        if (unit->hidden && !unit->revealed) {
            mk_tactical_overlay_t overlay = mk_board_view_make_overlay(
                view,
                MK_TACTICAL_OVERLAY_HIDDEN_CONTACT,
                unit->position_m,
                6.0f
            );
            mk_result_t result;

            overlay.unit_id = unit->id;
            overlay.side = unit->side;
            overlay.intensity = unit->concealment;
            result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &overlay);
            if (result != MK_OK) {
                return result;
            }
        }

        if (mk_board_view_unit_should_show_order_status(unit)) {
            mk_tactical_overlay_t overlay = mk_board_view_make_overlay(
                view,
                MK_TACTICAL_OVERLAY_ORDER_STATUS,
                unit->position_m,
                5.0f
            );
            mk_result_t result;

            overlay.unit_id = unit->id;
            overlay.side = unit->side;
            overlay.order = unit->order;
            overlay.screen_position_px.y -= 14.0f;
            overlay.target_screen_position_px = overlay.screen_position_px;
            result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &overlay);
            if (result != MK_OK) {
                return result;
            }
        }

        if (unit->id == snapshot->selected_unit_id) {
            mk_tactical_overlay_t overlay = mk_board_view_make_overlay(
                view,
                MK_TACTICAL_OVERLAY_SELECTION,
                unit->position_m,
                MK_UNIT_PICK_RADIUS_M
            );
            mk_result_t result;

            overlay.unit_id = unit->id;
            overlay.side = unit->side;
            result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &overlay);
            if (result != MK_OK) {
                return result;
            }
        }

        if (unit->has_move_target) {
            mk_vec2_t route_target_position_m = unit->target_position_m;
            mk_tactical_overlay_t route = mk_board_view_make_overlay(
                view,
                MK_TACTICAL_OVERLAY_MOVE_ROUTE,
                unit->position_m,
                0.0f
            );
            mk_tactical_overlay_t target = mk_board_view_make_overlay(
                view,
                MK_TACTICAL_OVERLAY_MOVE_TARGET,
                unit->target_position_m,
                5.0f
            );
            mk_result_t result;

            if (unit->has_route && unit->route_step_index < unit->route_step_count) {
                route_target_position_m = unit->route_waypoints_m[unit->route_step_index];
                route.intensity = (int)(unit->route_step_count - unit->route_step_index);
            }

            route.unit_id = unit->id;
            route.side = unit->side;
            route.target_position_m = route_target_position_m;
            route.target_screen_position_px = mk_board_view_map_to_screen(view, route_target_position_m);
            target.unit_id = unit->id;
            target.side = unit->side;

            result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &route);
            if (result != MK_OK) {
                return result;
            }

            result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &target);
            if (result != MK_OK) {
                return result;
            }
        }

        if (unit->suppression > 0 || unit->status != MK_UNIT_READY) {
            mk_tactical_overlay_t overlay = mk_board_view_make_overlay(
                view,
                MK_TACTICAL_OVERLAY_SUPPRESSION,
                unit->position_m,
                7.0f
            );
            mk_result_t result;

            overlay.unit_id = unit->id;
            overlay.side = unit->side;
            overlay.intensity = unit->suppression;
            result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &overlay);
            if (result != MK_OK) {
                return result;
            }
        }

        for (soldier_index = 0; soldier_index < unit->soldier_count; ++soldier_index) {
            const mk_soldier_t *soldier = &unit->soldiers[soldier_index];

            if (soldier->casualty) {
                mk_vec2_t position_m;
                mk_tactical_overlay_t overlay;
                mk_result_t result;

                position_m.x = unit->position_m.x + soldier->offset_m.x;
                position_m.y = unit->position_m.y + soldier->offset_m.y;
                overlay = mk_board_view_make_overlay(view, MK_TACTICAL_OVERLAY_CASUALTY, position_m, 5.0f);
                overlay.unit_id = unit->id;
                overlay.soldier_id = soldier->id;
                overlay.side = unit->side;
                result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &overlay);
                if (result != MK_OK) {
                    return result;
                }
            }
        }
    }

    for (civilian_index = 0; civilian_index < snapshot->civilian_count; ++civilian_index) {
        const mk_civilian_t *civilian = &snapshot->civilians[civilian_index];

        if (civilian->risk > 0) {
            mk_tactical_overlay_t overlay = mk_board_view_make_overlay(
                view,
                MK_TACTICAL_OVERLAY_CIVILIAN_RISK,
                civilian->position_m,
                10.0f
            );
            mk_result_t result;

            overlay.civilian_id = civilian->id;
            overlay.side = MK_SIDE_CIVILIAN;
            overlay.intensity = civilian->risk;
            result = mk_board_view_push_overlay(out_overlays, overlay_capacity, &overlay_index, &overlay);
            if (result != MK_OK) {
                return result;
            }
        }
    }

    return MK_OK;
}

mk_vec2_t mk_board_view_map_to_screen(const mk_board_view_t *view, mk_vec2_t position_m) {
    mk_vec2_t position_px;

    position_px.x = 0.0f;
    position_px.y = 0.0f;

    if (!mk_render_view_is_valid(view)) {
        return position_px;
    }

    position_px.x = view->screen_rect_px.x + (position_m.x - view->origin_m.x) * view->scale_px_per_m;
    position_px.y = view->screen_rect_px.y + (position_m.y - view->origin_m.y) * view->scale_px_per_m;

    return position_px;
}

mk_vec2_t mk_board_view_screen_to_map(const mk_board_view_t *view, mk_vec2_t position_px) {
    mk_vec2_t position_m;

    position_m.x = 0.0f;
    position_m.y = 0.0f;

    if (!mk_render_view_is_valid(view)) {
        return position_m;
    }

    position_m.x = view->origin_m.x + (position_px.x - view->screen_rect_px.x) / view->scale_px_per_m;
    position_m.y = view->origin_m.y + (position_px.y - view->screen_rect_px.y) / view->scale_px_per_m;

    return position_m;
}

mk_rect_t mk_board_view_map_rect_to_screen(const mk_board_view_t *view, mk_rect_t rect_m) {
    mk_vec2_t rect_origin_m;
    mk_vec2_t origin_px;
    mk_rect_t rect_px;

    rect_origin_m.x = rect_m.x;
    rect_origin_m.y = rect_m.y;
    origin_px = mk_board_view_map_to_screen(view, rect_origin_m);

    rect_px.x = origin_px.x;
    rect_px.y = origin_px.y;
    rect_px.width = rect_m.width * (view != NULL ? view->scale_px_per_m : 0.0f);
    rect_px.height = rect_m.height * (view != NULL ? view->scale_px_per_m : 0.0f);

    return rect_px;
}

mk_rect_t mk_board_view_visible_map_bounds(const mk_board_view_t *view) {
    mk_rect_t bounds;

    memset(&bounds, 0, sizeof(bounds));

    if (!mk_render_view_is_valid(view)) {
        return bounds;
    }

    bounds.x = view->origin_m.x;
    bounds.y = view->origin_m.y;
    bounds.width = view->screen_rect_px.width / view->scale_px_per_m;
    bounds.height = view->screen_rect_px.height / view->scale_px_per_m;

    return bounds;
}

mk_result_t mk_board_view_pan_pixels(
    mk_board_view_t *view,
    const mk_map_t *map,
    float delta_x_px,
    float delta_y_px
) {
    if (!mk_render_view_is_valid(view) || !mk_render_map_is_valid(map)) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    view->origin_m.x += delta_x_px / view->scale_px_per_m;
    view->origin_m.y += delta_y_px / view->scale_px_per_m;
    mk_board_view_clamp_origin(view, map);

    return MK_OK;
}

mk_result_t mk_board_view_zoom_at(
    mk_board_view_t *view,
    const mk_map_t *map,
    float zoom_factor,
    mk_vec2_t anchor_screen_px
) {
    mk_vec2_t anchor_map_m;
    float old_scale;
    float new_scale;

    if (!mk_render_view_is_valid(view) || !mk_render_map_is_valid(map) || zoom_factor <= 0.0f) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    anchor_map_m = mk_board_view_screen_to_map(view, anchor_screen_px);
    old_scale = view->scale_px_per_m;
    new_scale = mk_render_clamp_float(
        old_scale * zoom_factor,
        view->min_scale_px_per_m,
        view->max_scale_px_per_m
    );

    view->scale_px_per_m = new_scale;
    view->origin_m.x = anchor_map_m.x - (anchor_screen_px.x - view->screen_rect_px.x) / new_scale;
    view->origin_m.y = anchor_map_m.y - (anchor_screen_px.y - view->screen_rect_px.y) / new_scale;
    mk_board_view_clamp_origin(view, map);

    return MK_OK;
}

mk_result_t mk_board_view_collect_soldier_markers(
    const mk_board_view_t *view,
    const mk_game_snapshot_t *snapshot,
    mk_soldier_marker_t *out_markers,
    size_t marker_capacity,
    size_t *out_marker_count
) {
    size_t needed = 0;
    size_t unit_index;
    size_t marker_index = 0;

    if (!mk_render_view_is_valid(view) || snapshot == NULL || out_marker_count == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    for (unit_index = 0; unit_index < snapshot->unit_count; ++unit_index) {
        const mk_unit_t *unit = &snapshot->units[unit_index];

        if (unit->hidden && !unit->revealed) {
            continue;
        }

        needed += unit->soldier_count;
    }

    *out_marker_count = needed;

    if (out_markers == NULL) {
        return marker_capacity == 0 ? MK_OK : MK_ERROR_INVALID_ARGUMENT;
    }

    if (marker_capacity < needed) {
        return MK_ERROR_CAPACITY;
    }

    for (unit_index = 0; unit_index < snapshot->unit_count; ++unit_index) {
        const mk_unit_t *unit = &snapshot->units[unit_index];
        size_t soldier_index;

        if (unit->hidden && !unit->revealed) {
            continue;
        }

        for (soldier_index = 0; soldier_index < unit->soldier_count; ++soldier_index) {
            const mk_soldier_t *soldier = &unit->soldiers[soldier_index];
            mk_vec2_t position_m;

            position_m.x = unit->position_m.x + soldier->offset_m.x;
            position_m.y = unit->position_m.y + soldier->offset_m.y;

            out_markers[marker_index].unit_id = unit->id;
            out_markers[marker_index].soldier_id = soldier->id;
            out_markers[marker_index].side = unit->side;
            out_markers[marker_index].role = soldier->role;
            out_markers[marker_index].position_m = position_m;
            out_markers[marker_index].screen_position_px = mk_board_view_map_to_screen(view, position_m);
            out_markers[marker_index].casualty = soldier->casualty;
            out_markers[marker_index].selected_unit = unit->id == snapshot->selected_unit_id;
            marker_index += 1;
        }
    }

    return MK_OK;
}
