#ifndef MODERNER_KRIEG_BOARD_VIEW_H
#define MODERNER_KRIEG_BOARD_VIEW_H

#include "mk_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MK_BOARD_VIEW_DEFAULT_MARGIN_PX 48.0f
#define MK_BOARD_VIEW_DEFAULT_MAX_ZOOM_MULTIPLIER 8.0f
#define MK_BOARD_VIEW_MAX_TACTICAL_OVERLAYS 256

typedef struct {
    mk_rect_t screen_rect_px;
    mk_vec2_t origin_m;
    float scale_px_per_m;
    float min_scale_px_per_m;
    float max_scale_px_per_m;
} mk_board_view_t;

typedef struct {
    uint32_t unit_id;
    uint32_t soldier_id;
    mk_side_t side;
    mk_soldier_role_t role;
    mk_vec2_t position_m;
    mk_vec2_t screen_position_px;
    bool casualty;
    bool selected_unit;
} mk_soldier_marker_t;

typedef struct {
    uint32_t vehicle_id;
    mk_traffic_vehicle_kind_t kind;
    mk_traffic_boarding_mode_t boarding_mode;
    char sprite_id[MK_NAME_CAPACITY];
    mk_vec2_t position_m;
    mk_vec2_t screen_position_px;
    float facing_degrees;
    int seat_capacity;
    int occupied_seats;
    bool active;
} mk_traffic_vehicle_marker_t;

typedef enum {
    MK_TACTICAL_OVERLAY_SELECTION = 0,
    MK_TACTICAL_OVERLAY_MOVE_ROUTE = 1,
    MK_TACTICAL_OVERLAY_MOVE_TARGET = 2,
    MK_TACTICAL_OVERLAY_FIRE = 3,
    MK_TACTICAL_OVERLAY_SUPPRESSION = 4,
    MK_TACTICAL_OVERLAY_CASUALTY = 5,
    MK_TACTICAL_OVERLAY_OBJECTIVE = 6,
    MK_TACTICAL_OVERLAY_HIDDEN_CONTACT = 7,
    MK_TACTICAL_OVERLAY_CIVILIAN_RISK = 8,
    MK_TACTICAL_OVERLAY_SUSPECTED_CONTACT = 9,
    MK_TACTICAL_OVERLAY_FALSE_CONTACT = 10,
    MK_TACTICAL_OVERLAY_OBJECTIVE_CONTROL = 11,
    MK_TACTICAL_OVERLAY_ORDER_STATUS = 12,
    MK_TACTICAL_OVERLAY_BREACH_SEARCH = 13,
    MK_TACTICAL_OVERLAY_ROOFTOP_ACCESS = 14,
    MK_TACTICAL_OVERLAY_SEARCH_CACHE = 15
} mk_tactical_overlay_kind_t;

typedef struct {
    mk_tactical_overlay_kind_t kind;
    uint32_t unit_id;
    uint32_t soldier_id;
    uint32_t objective_id;
    uint32_t civilian_id;
    uint32_t terrain_id;
    mk_side_t side;
    mk_order_t order;
    mk_vec2_t position_m;
    mk_vec2_t target_position_m;
    mk_vec2_t screen_position_px;
    mk_vec2_t target_screen_position_px;
    float radius_m;
    float screen_radius_px;
    int intensity;
} mk_tactical_overlay_t;

mk_result_t mk_board_view_fit_map(
    mk_board_view_t *out_view,
    const mk_map_t *map,
    float screen_width_px,
    float screen_height_px,
    float margin_px
);

mk_vec2_t mk_board_view_map_to_screen(const mk_board_view_t *view, mk_vec2_t position_m);
mk_vec2_t mk_board_view_screen_to_map(const mk_board_view_t *view, mk_vec2_t position_px);
mk_rect_t mk_board_view_map_rect_to_screen(const mk_board_view_t *view, mk_rect_t rect_m);
mk_rect_t mk_board_view_visible_map_bounds(const mk_board_view_t *view);

mk_result_t mk_board_view_pan_pixels(
    mk_board_view_t *view,
    const mk_map_t *map,
    float delta_x_px,
    float delta_y_px
);

mk_result_t mk_board_view_zoom_at(
    mk_board_view_t *view,
    const mk_map_t *map,
    float zoom_factor,
    mk_vec2_t anchor_screen_px
);

mk_result_t mk_board_view_collect_soldier_markers(
    const mk_board_view_t *view,
    const mk_game_snapshot_t *snapshot,
    mk_soldier_marker_t *out_markers,
    size_t marker_capacity,
    size_t *out_marker_count
);

mk_result_t mk_board_view_collect_traffic_vehicle_markers(
    const mk_board_view_t *view,
    const mk_game_snapshot_t *snapshot,
    mk_traffic_vehicle_marker_t *out_markers,
    size_t marker_capacity,
    size_t *out_marker_count
);

mk_result_t mk_board_view_collect_tactical_overlays(
    const mk_board_view_t *view,
    const mk_game_snapshot_t *snapshot,
    mk_tactical_overlay_t *out_overlays,
    size_t overlay_capacity,
    size_t *out_overlay_count
);

#ifdef __cplusplus
}
#endif

#endif
