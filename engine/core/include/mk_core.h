#ifndef MODERNER_KRIEG_CORE_H
#define MODERNER_KRIEG_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MK_NAME_CAPACITY 64
#define MK_SCENARIO_NAME_CAPACITY 96
#define MK_MAX_CONTROLLERS 8
#define MK_MAX_FACTIONS 8
#define MK_MAX_FORCES 8
#define MK_MAX_TERRAIN_ZONES 128
#define MK_MAX_MAP_TILES 4096
#define MK_MAX_OBJECTIVES 16
#define MK_MAX_CIVILIANS 128
#define MK_MAX_SOLDIERS_PER_UNIT 16
#define MK_MAX_UNITS 64
#define MK_MAX_CONTACT_REPORTS 64
#define MK_SCENARIO_TEXT_CAPACITY 256
#define MK_AFTER_ACTION_SUMMARY_CAPACITY 256
#define MK_UNIT_PICK_RADIUS_M 8.0f
#define MK_DEFAULT_MOVE_SPEED_M_PER_TICK 6.0f
#define MK_DEFAULT_SCORE_SUCCESS_THRESHOLD 450
#define MK_DEFAULT_SCORE_PARTIAL_THRESHOLD 150

typedef enum {
    MK_OK = 0,
    MK_ERROR_INVALID_ARGUMENT = -1,
    MK_ERROR_CAPACITY = -2,
    MK_ERROR_NOT_FOUND = -3,
    MK_ERROR_INVALID_DATA = -4
} mk_result_t;

typedef enum {
    MK_SIDE_NEUTRAL = 0,
    MK_SIDE_PLAYER = 1,
    MK_SIDE_OPFOR = 2,
    MK_SIDE_CIVILIAN = 3
} mk_side_t;

typedef enum {
    MK_CONTROLLER_NONE = 0,
    MK_CONTROLLER_HUMAN = 1,
    MK_CONTROLLER_SCRIPTED_AI = 2,
    MK_CONTROLLER_TACTICAL_AI = 3,
    MK_CONTROLLER_OBSERVER = 4
} mk_controller_kind_t;

typedef enum {
    MK_TRAINING_UNTRAINED = 0,
    MK_TRAINING_REGULAR = 1,
    MK_TRAINING_VETERAN = 2,
    MK_TRAINING_ELITE = 3
} mk_training_t;

typedef enum {
    MK_UNIT_READY = 0,
    MK_UNIT_SUPPRESSED = 1,
    MK_UNIT_PINNED = 2,
    MK_UNIT_BROKEN = 3
} mk_unit_status_t;

typedef enum {
    MK_ORDER_NONE = 0,
    MK_ORDER_HOLD = 1,
    MK_ORDER_MOVE = 2,
    MK_ORDER_ASSAULT_MOVE = 3,
    MK_ORDER_FIRE = 4,
    MK_ORDER_SUPPRESS = 5,
    MK_ORDER_OVERWATCH = 6,
    MK_ORDER_BREACH = 7,
    MK_ORDER_RALLY = 8,
    MK_ORDER_WITHDRAW = 9
} mk_order_t;

typedef enum {
    MK_ORDER_SOURCE_NONE = 0,
    MK_ORDER_SOURCE_HUMAN = 1,
    MK_ORDER_SOURCE_AI = 2,
    MK_ORDER_SOURCE_REPLAY = 3,
    MK_ORDER_SOURCE_SCRIPT = 4
} mk_order_source_t;

typedef enum {
    MK_ROLE_RIFLEMAN = 0,
    MK_ROLE_LEADER = 1,
    MK_ROLE_MACHINE_GUNNER = 2,
    MK_ROLE_RPG = 3,
    MK_ROLE_MARKSMAN = 4,
    MK_ROLE_ENGINEER = 5,
    MK_ROLE_MEDIC = 6,
    MK_ROLE_DRONE_OPERATOR = 7,
    MK_ROLE_CIVILIAN = 8
} mk_soldier_role_t;

typedef enum {
    MK_STANCE_STANDING = 0,
    MK_STANCE_CROUCHING = 1,
    MK_STANCE_PRONE = 2
} mk_stance_t;

typedef enum {
    MK_WOUND_NONE = 0,
    MK_WOUND_LIGHT = 1,
    MK_WOUND_SERIOUS = 2,
    MK_WOUND_INCAPACITATED = 3,
    MK_WOUND_KILLED = 4
} mk_wound_state_t;

typedef enum {
    MK_FIRE_MODE_NONE = 0,
    MK_FIRE_MODE_SINGLE = 1,
    MK_FIRE_MODE_BURST = 2,
    MK_FIRE_MODE_AUTOMATIC = 3,
    MK_FIRE_MODE_EXPLOSIVE = 4
} mk_fire_mode_t;

typedef enum {
    MK_AMMO_NONE = 0,
    MK_AMMO_SMALL_ARMS = 1,
    MK_AMMO_MACHINE_GUN = 2,
    MK_AMMO_ROCKET = 3
} mk_ammo_kind_t;

typedef enum {
    MK_TERRAIN_OPEN = 0,
    MK_TERRAIN_ROAD = 1,
    MK_TERRAIN_BUILDING = 2,
    MK_TERRAIN_ROOFTOP = 3,
    MK_TERRAIN_RUBBLE = 4,
    MK_TERRAIN_ALLEY = 5,
    MK_TERRAIN_OBSTACLE = 6,
    MK_TERRAIN_BREACH_POINT = 7,
    MK_TERRAIN_SUSPECTED_IED = 8,
    MK_TERRAIN_RIVER = 9,
    MK_TERRAIN_BRIDGE = 10
} mk_terrain_kind_t;

typedef enum {
    MK_OBJECTIVE_CONTROL = 0,
    MK_OBJECTIVE_EXIT = 1,
    MK_OBJECTIVE_PROTECT_CIVILIANS = 2,
    MK_OBJECTIVE_SEARCH = 3
} mk_objective_kind_t;

typedef enum {
    MK_CIVILIAN_SHELTERING = 0,
    MK_CIVILIAN_FLEEING = 1,
    MK_CIVILIAN_FROZEN = 2,
    MK_CIVILIAN_FOLLOWING_INSTRUCTIONS = 3
} mk_civilian_state_t;

typedef enum {
    MK_CONTACT_REPORT_FIRE = 0,
    MK_CONTACT_REPORT_REVEAL = 1,
    MK_CONTACT_REPORT_CIVILIAN_RISK = 2,
    MK_CONTACT_REPORT_SUSPECTED_DANGER = 3,
    MK_CONTACT_REPORT_FALSE_CONTACT = 4
} mk_contact_report_kind_t;

typedef enum {
    MK_OUTCOME_IN_PROGRESS = 0,
    MK_OUTCOME_PLAYER_SUCCESS = 1,
    MK_OUTCOME_PLAYER_PARTIAL = 2,
    MK_OUTCOME_PLAYER_FAILURE = 3
} mk_outcome_t;

typedef struct {
    float x;
    float y;
} mk_vec2_t;

typedef struct {
    int x;
    int y;
} mk_ivec2_t;

typedef struct {
    float x;
    float y;
    float width;
    float height;
} mk_rect_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} mk_color_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    mk_side_t side;
    mk_controller_kind_t kind;
} mk_controller_slot_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    mk_side_t side;
    mk_color_t color;
} mk_faction_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    char callsign[MK_NAME_CAPACITY];
    mk_side_t side;
} mk_command_identity_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    mk_side_t side;
    uint32_t faction_id;
    uint32_t controller_id;
    mk_command_identity_t command;
} mk_force_t;

typedef struct {
    char name[MK_NAME_CAPACITY];
    mk_fire_mode_t fire_mode;
    mk_ammo_kind_t ammo_kind;
    int effective_range_m;
    int shots_per_action;
    int damage;
    int suppression;
    int magazine_capacity;
    int reload_ticks;
    int cooldown_ticks;
} mk_weapon_profile_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    mk_soldier_role_t role;
    mk_weapon_profile_t weapon;
    int health;
    int max_health;
    int ammo;
    int ammo_capacity;
    int suppression;
    int stress;
    int exposure;
    uint32_t equipment_flags;
    uint32_t sensor_flags;
    int reload_ticks_remaining;
    int cooldown_ticks_remaining;
    mk_stance_t stance;
    mk_wound_state_t wound_state;
    bool casualty;
    bool can_move;
    mk_vec2_t offset_m;
    float facing_degrees;
} mk_soldier_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    mk_side_t side;
    uint32_t faction_id;
    uint32_t force_id;
    uint32_t controller_id;
    mk_command_identity_t command;
    mk_training_t training;
    mk_order_t order;
    mk_order_source_t order_source;
    mk_vec2_t position_m;
    mk_vec2_t target_position_m;
    float facing_degrees;
    float cohesion_radius_m;
    float move_speed_m_per_tick;
    int morale;
    int suppression;
    int fatigue;
    int command_disruption;
    mk_unit_status_t status;
    bool communications_up;
    bool cover_posture;
    bool has_move_target;
    bool hidden;
    bool revealed;
    int concealment;
    size_t soldier_count;
    mk_soldier_t soldiers[MK_MAX_SOLDIERS_PER_UNIT];
} mk_unit_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    mk_terrain_kind_t kind;
    mk_rect_t bounds_m;
    int cover;
    int movement_cost;
    bool blocks_line_of_sight;
} mk_terrain_zone_t;

typedef struct {
    uint32_t id;
    mk_ivec2_t coordinate;
    mk_terrain_kind_t kind;
    int elevation;
    int cover;
    int movement_cost;
    bool blocks_line_of_sight;
    bool blocks_movement;
} mk_map_tile_t;

typedef struct {
    char name[MK_NAME_CAPACITY];
    float width_m;
    float height_m;
    float tile_width_m;
    float tile_height_m;
    int tile_columns;
    int tile_rows;
    size_t tile_count;
    mk_map_tile_t tiles[MK_MAX_MAP_TILES];
    size_t terrain_count;
    mk_terrain_zone_t terrain[MK_MAX_TERRAIN_ZONES];
} mk_map_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    mk_objective_kind_t kind;
    mk_vec2_t position_m;
    float radius_m;
    mk_side_t controlling_side;
    int value;
} mk_objective_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    uint32_t faction_id;
    mk_vec2_t position_m;
    mk_civilian_state_t state;
    int stress;
    int risk;
    bool protected_noncombatant;
} mk_civilian_t;

typedef struct {
    bool visible;
    float distance_m;
    int cover;
    uint32_t blocking_terrain_id;
    uint32_t cover_terrain_id;
    mk_terrain_kind_t cover_terrain_kind;
} mk_line_of_sight_t;

typedef struct {
    uint32_t id;
    uint32_t tick;
    mk_contact_report_kind_t kind;
    uint32_t attacker_unit_id;
    uint32_t target_unit_id;
    uint32_t civilian_id;
    uint32_t terrain_id;
    mk_side_t side;
    mk_vec2_t position_m;
    mk_vec2_t target_position_m;
    int shots_fired;
    int hits;
    int suppression_added;
    int casualties;
    int civilian_risk_added;
    int confidence;
    bool visible;
    bool resolved;
} mk_contact_report_t;

typedef struct {
    uint32_t attacker_unit_id;
    uint32_t target_unit_id;
    mk_line_of_sight_t line_of_sight;
    int eligible_shooters;
    int shots_fired;
    int ammo_spent;
    int hits;
    int damage_applied;
    int casualties;
    int suppression_added;
    int civilian_risk_added;
    uint32_t contact_report_id;
    mk_unit_status_t attacker_status;
    mk_unit_status_t target_status_before;
    mk_unit_status_t target_status_after;
    bool resolved;
} mk_fire_result_t;

typedef struct {
    int objective_points;
    int civilian_risk_penalty;
    int casualty_penalty;
    int time_penalty;
    int total_score;
    int player_casualties;
    int opfor_casualties;
    int civilian_casualties;
    int civilian_risk;
    uint32_t controlled_objectives;
    uint32_t contested_objectives;
    mk_outcome_t outcome;
} mk_score_t;

typedef struct {
    mk_score_t score;
    char summary[MK_AFTER_ACTION_SUMMARY_CAPACITY];
    char narrative[MK_SCENARIO_TEXT_CAPACITY];
} mk_after_action_report_t;

typedef struct {
    char name[MK_SCENARIO_NAME_CAPACITY];
    char briefing[MK_SCENARIO_TEXT_CAPACITY];
    char after_action_success[MK_SCENARIO_TEXT_CAPACITY];
    char after_action_partial[MK_SCENARIO_TEXT_CAPACITY];
    char after_action_failure[MK_SCENARIO_TEXT_CAPACITY];
    uint64_t seed;
    int score_success_threshold;
    int score_partial_threshold;
    mk_map_t map;
    size_t controller_count;
    mk_controller_slot_t controllers[MK_MAX_CONTROLLERS];
    size_t faction_count;
    mk_faction_t factions[MK_MAX_FACTIONS];
    size_t force_count;
    mk_force_t forces[MK_MAX_FORCES];
    size_t objective_count;
    mk_objective_t objectives[MK_MAX_OBJECTIVES];
    size_t civilian_count;
    mk_civilian_t civilians[MK_MAX_CIVILIANS];
    size_t unit_count;
    mk_unit_t units[MK_MAX_UNITS];
} mk_scenario_definition_t;

typedef struct {
    char scenario_name[MK_SCENARIO_NAME_CAPACITY];
    char briefing[MK_SCENARIO_TEXT_CAPACITY];
    char after_action_success[MK_SCENARIO_TEXT_CAPACITY];
    char after_action_partial[MK_SCENARIO_TEXT_CAPACITY];
    char after_action_failure[MK_SCENARIO_TEXT_CAPACITY];
    uint32_t tick;
    uint64_t rng_state;
    int score_success_threshold;
    int score_partial_threshold;
    uint32_t selected_unit_id;
    mk_map_t map;
    size_t controller_count;
    mk_controller_slot_t controllers[MK_MAX_CONTROLLERS];
    size_t faction_count;
    mk_faction_t factions[MK_MAX_FACTIONS];
    size_t force_count;
    mk_force_t forces[MK_MAX_FORCES];
    size_t objective_count;
    mk_objective_t objectives[MK_MAX_OBJECTIVES];
    size_t civilian_count;
    mk_civilian_t civilians[MK_MAX_CIVILIANS];
    size_t unit_count;
    mk_unit_t units[MK_MAX_UNITS];
    size_t contact_report_count;
    mk_contact_report_t contact_reports[MK_MAX_CONTACT_REPORTS];
} mk_game_t;

typedef struct {
    char scenario_name[MK_SCENARIO_NAME_CAPACITY];
    char briefing[MK_SCENARIO_TEXT_CAPACITY];
    char after_action_success[MK_SCENARIO_TEXT_CAPACITY];
    char after_action_partial[MK_SCENARIO_TEXT_CAPACITY];
    char after_action_failure[MK_SCENARIO_TEXT_CAPACITY];
    uint32_t tick;
    uint64_t rng_state;
    int score_success_threshold;
    int score_partial_threshold;
    uint32_t selected_unit_id;
    mk_map_t map;
    size_t controller_count;
    mk_controller_slot_t controllers[MK_MAX_CONTROLLERS];
    size_t faction_count;
    mk_faction_t factions[MK_MAX_FACTIONS];
    size_t force_count;
    mk_force_t forces[MK_MAX_FORCES];
    size_t objective_count;
    mk_objective_t objectives[MK_MAX_OBJECTIVES];
    size_t civilian_count;
    mk_civilian_t civilians[MK_MAX_CIVILIANS];
    size_t unit_count;
    mk_unit_t units[MK_MAX_UNITS];
    size_t contact_report_count;
    mk_contact_report_t contact_reports[MK_MAX_CONTACT_REPORTS];
} mk_game_snapshot_t;

typedef mk_result_t (*mk_step_observer_fn)(const mk_game_t *game, void *user_data);

const char *mk_version(void);

void mk_game_init(mk_game_t *game, uint64_t seed);
uint32_t mk_random_u32(mk_game_t *game);
float mk_random_float01(mk_game_t *game);
void mk_game_step(mk_game_t *game);
mk_result_t mk_game_run_fixed_steps(
    mk_game_t *game,
    uint32_t step_count,
    mk_step_observer_fn observer,
    void *user_data
);
mk_result_t mk_game_snapshot(const mk_game_t *game, mk_game_snapshot_t *out_snapshot);
mk_result_t mk_game_load_scenario(mk_game_t *game, const mk_scenario_definition_t *scenario);
mk_result_t mk_game_pick_unit_at(const mk_game_t *game, mk_vec2_t position_m, float radius_m, uint32_t *out_unit_id);
mk_result_t mk_game_select_unit(mk_game_t *game, uint32_t unit_id);
mk_result_t mk_game_select_unit_at(mk_game_t *game, mk_vec2_t position_m, float radius_m, uint32_t *out_unit_id);
mk_result_t mk_game_clear_selection(mk_game_t *game);
mk_result_t mk_game_issue_order(mk_game_t *game, uint32_t unit_id, mk_order_t order);
mk_result_t mk_game_issue_move_order(mk_game_t *game, uint32_t unit_id, mk_vec2_t target_position_m);
mk_result_t mk_game_issue_selected_move_order(mk_game_t *game, mk_vec2_t target_position_m);
mk_result_t mk_game_trace_line_of_sight(
    const mk_game_t *game,
    mk_vec2_t from_m,
    mk_vec2_t to_m,
    mk_line_of_sight_t *out_line_of_sight
);
mk_result_t mk_game_unit_line_of_sight(
    const mk_game_t *game,
    uint32_t observer_unit_id,
    uint32_t target_unit_id,
    mk_line_of_sight_t *out_line_of_sight
);
mk_result_t mk_game_unit_fire(
    mk_game_t *game,
    uint32_t attacker_unit_id,
    uint32_t target_unit_id,
    mk_fire_result_t *out_fire_result
);
mk_result_t mk_game_selected_unit_fire(
    mk_game_t *game,
    uint32_t target_unit_id,
    mk_fire_result_t *out_fire_result
);
mk_result_t mk_game_update_hidden_contacts(mk_game_t *game);
mk_result_t mk_game_update_civilian_risk(mk_game_t *game);
mk_result_t mk_game_update_objective_control(mk_game_t *game);
mk_result_t mk_game_score(const mk_game_t *game, mk_score_t *out_score);
mk_result_t mk_game_after_action_report(const mk_game_t *game, mk_after_action_report_t *out_report);

mk_vec2_t mk_vec2(float x, float y);
mk_ivec2_t mk_ivec2(int x, int y);
mk_rect_t mk_rect(float x, float y, float width, float height);
float mk_clamp_f32(float value, float minimum, float maximum);
int mk_clamp_i32(int value, int minimum, int maximum);
bool mk_rect_contains_point(mk_rect_t rect, mk_vec2_t point);
float mk_vec2_distance(mk_vec2_t first, mk_vec2_t second);

mk_weapon_profile_t mk_make_weapon(
    const char *name,
    int effective_range_m,
    int shots_per_action,
    int damage,
    int suppression
);

mk_soldier_t mk_make_soldier(
    const char *name,
    mk_soldier_role_t role,
    mk_weapon_profile_t weapon
);

mk_unit_t mk_make_unit(
    const char *name,
    mk_side_t side,
    mk_training_t training,
    mk_vec2_t position_m
);

mk_color_t mk_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
mk_controller_slot_t mk_make_controller_slot(const char *name, mk_side_t side, mk_controller_kind_t kind);
mk_faction_t mk_make_faction(const char *name, mk_side_t side, mk_color_t color);
mk_command_identity_t mk_make_command_identity(const char *name, const char *callsign, mk_side_t side);
mk_force_t mk_make_force(
    const char *name,
    mk_side_t side,
    uint32_t faction_id,
    uint32_t controller_id
);
mk_map_t mk_make_map(const char *name, float width_m, float height_m);
mk_map_tile_t mk_make_map_tile(
    mk_ivec2_t coordinate,
    mk_terrain_kind_t kind,
    int elevation,
    int cover,
    int movement_cost,
    bool blocks_line_of_sight,
    bool blocks_movement
);
mk_terrain_zone_t mk_make_terrain_zone(
    const char *name,
    mk_terrain_kind_t kind,
    mk_rect_t bounds_m,
    int cover,
    int movement_cost,
    bool blocks_line_of_sight
);
mk_objective_t mk_make_objective(
    const char *name,
    mk_objective_kind_t kind,
    mk_vec2_t position_m,
    float radius_m,
    int value
);
mk_civilian_t mk_make_civilian(const char *name, uint32_t faction_id, mk_vec2_t position_m);

mk_result_t mk_map_configure_tiles(
    mk_map_t *map,
    int columns,
    int rows,
    float tile_width_m,
    float tile_height_m,
    mk_terrain_kind_t default_kind
);
mk_map_tile_t *mk_map_get_tile(mk_map_t *map, mk_ivec2_t coordinate);
const mk_map_tile_t *mk_map_get_tile_const(const mk_map_t *map, mk_ivec2_t coordinate);
mk_result_t mk_map_set_tile(mk_map_t *map, const mk_map_tile_t *tile);
mk_result_t mk_map_add_terrain(mk_map_t *map, const mk_terrain_zone_t *terrain, uint32_t *out_terrain_id);
mk_result_t mk_scenario_add_controller(mk_scenario_definition_t *scenario, const mk_controller_slot_t *controller, uint32_t *out_controller_id);
mk_result_t mk_scenario_add_faction(mk_scenario_definition_t *scenario, const mk_faction_t *faction, uint32_t *out_faction_id);
mk_result_t mk_scenario_add_force(mk_scenario_definition_t *scenario, const mk_force_t *force, uint32_t *out_force_id);
mk_result_t mk_scenario_add_objective(mk_scenario_definition_t *scenario, const mk_objective_t *objective, uint32_t *out_objective_id);
mk_result_t mk_scenario_add_civilian(mk_scenario_definition_t *scenario, const mk_civilian_t *civilian, uint32_t *out_civilian_id);
mk_result_t mk_scenario_add_unit(mk_scenario_definition_t *scenario, const mk_unit_t *unit, uint32_t *out_unit_id);

mk_result_t mk_game_add_unit(mk_game_t *game, const mk_unit_t *unit, uint32_t *out_unit_id);
mk_unit_t *mk_game_find_unit(mk_game_t *game, uint32_t unit_id);
const mk_unit_t *mk_game_find_unit_const(const mk_game_t *game, uint32_t unit_id);

mk_result_t mk_unit_add_soldier(mk_unit_t *unit, const mk_soldier_t *soldier, uint32_t *out_soldier_id);

#ifdef __cplusplus
}
#endif

#endif
