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
        needed += snapshot->units[unit_index].soldier_count;
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
