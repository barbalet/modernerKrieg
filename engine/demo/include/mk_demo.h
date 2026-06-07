#ifndef MODERNER_KRIEG_DEMO_H
#define MODERNER_KRIEG_DEMO_H

#include "mk_board_view.h"
#include "mk_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MK_DEMO_MAX_DRAW_COMMANDS 768

typedef struct mk_demo_session mk_demo_session_t;

typedef enum {
    MK_DEMO_PICK_NONE = 0,
    MK_DEMO_PICK_UNIT = 1,
    MK_DEMO_PICK_CIVILIAN = 2,
    MK_DEMO_PICK_CONTACT = 3,
    MK_DEMO_PICK_OBJECTIVE = 4,
    MK_DEMO_PICK_TERRAIN = 5,
    MK_DEMO_PICK_TOPOLOGY_NODE = 6,
    MK_DEMO_PICK_TOPOLOGY_PORTAL = 7,
    MK_DEMO_PICK_SEMANTIC_ZONE = 8
} mk_demo_pick_kind_t;

typedef enum {
    MK_DEMO_DRAW_LEVEL = 0,
    MK_DEMO_DRAW_UNIT = 1,
    MK_DEMO_DRAW_CIVILIAN = 2,
    MK_DEMO_DRAW_SOLDIER = 3,
    MK_DEMO_DRAW_OBJECTIVE = 4,
    MK_DEMO_DRAW_CONTACT = 5,
    MK_DEMO_DRAW_OVERLAY = 6
} mk_demo_draw_kind_t;

typedef struct {
    const char *project_root;
    float screen_width_px;
    float screen_height_px;
    float margin_px;
    bool ai_only;
} mk_demo_session_config_t;

typedef struct {
    uint32_t tick;
    uint32_t selected_unit_id;
    uint32_t unit_count;
    uint32_t civilian_count;
    uint32_t objective_count;
    uint32_t contact_report_count;
    char scenario_name[MK_SCENARIO_NAME_CAPACITY];
    char gameplay_area_id[MK_NAME_CAPACITY];
    char map_name[MK_NAME_CAPACITY];
    float map_width_m;
    float map_height_m;
    mk_score_t score;
} mk_demo_summary_t;

typedef struct {
    uint64_t fixed_ticks;
    uint64_t ai_order_batches;
    uint64_t ai_units_considered;
    uint64_t snapshot_queries;
    uint64_t draw_queries;
    uint64_t pick_queries;
    uint64_t order_requests;
} mk_demo_performance_counters_t;

typedef struct {
    mk_demo_pick_kind_t kind;
    uint32_t id;
    uint32_t secondary_id;
    char stable_id[MK_NAME_CAPACITY];
    char label[MK_NAME_CAPACITY];
    mk_side_t side;
    mk_vec2_t position_m;
    mk_vec2_t screen_position_px;
} mk_demo_pick_result_t;

typedef struct {
    mk_demo_draw_kind_t kind;
    mk_tactical_overlay_kind_t overlay_kind;
    uint32_t id;
    uint32_t secondary_id;
    char stable_id[MK_NAME_CAPACITY];
    char label[MK_NAME_CAPACITY];
    char asset_path[MK_PATH_CAPACITY];
    mk_side_t side;
    mk_order_t order;
    mk_vec2_t position_m;
    mk_vec2_t target_position_m;
    mk_vec2_t screen_position_px;
    mk_vec2_t target_screen_position_px;
    float radius_m;
    float screen_radius_px;
    int intensity;
    bool selected;
} mk_demo_draw_command_t;

mk_result_t mk_demo_session_create(
    const mk_demo_session_config_t *config,
    mk_demo_session_t **out_session
);
void mk_demo_session_destroy(mk_demo_session_t *session);

mk_result_t mk_demo_session_load_default(mk_demo_session_t *session);
mk_result_t mk_demo_session_load_scenario(
    mk_demo_session_t *session,
    const char *scenario_path
);
mk_result_t mk_demo_session_set_ai_only(mk_demo_session_t *session, bool ai_only);
mk_result_t mk_demo_session_step(mk_demo_session_t *session, uint32_t ticks);
mk_result_t mk_demo_session_snapshot(mk_demo_session_t *session, mk_game_snapshot_t *out_snapshot);
mk_result_t mk_demo_session_summary(mk_demo_session_t *session, mk_demo_summary_t *out_summary);
mk_result_t mk_demo_session_performance(
    const mk_demo_session_t *session,
    mk_demo_performance_counters_t *out_counters
);

mk_result_t mk_demo_session_fit_board(
    mk_demo_session_t *session,
    float screen_width_px,
    float screen_height_px,
    float margin_px
);
mk_result_t mk_demo_session_collect_draw_commands(
    mk_demo_session_t *session,
    mk_demo_draw_command_t *out_commands,
    size_t command_capacity,
    size_t *out_command_count
);
mk_result_t mk_demo_session_pick_screen(
    mk_demo_session_t *session,
    mk_vec2_t screen_position_px,
    float radius_px,
    mk_demo_pick_result_t *out_pick
);
mk_result_t mk_demo_session_select_screen(
    mk_demo_session_t *session,
    mk_vec2_t screen_position_px,
    float radius_px,
    mk_demo_pick_result_t *out_pick
);
mk_result_t mk_demo_session_issue_order(
    mk_demo_session_t *session,
    uint32_t unit_id,
    mk_order_t order
);
mk_result_t mk_demo_session_issue_selected_move_screen(
    mk_demo_session_t *session,
    mk_vec2_t screen_position_px
);

#ifdef __cplusplus
}
#endif

#endif
