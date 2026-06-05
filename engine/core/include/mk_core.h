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
#define MK_MAX_FACTIONS 8
#define MK_MAX_TERRAIN_ZONES 128
#define MK_MAX_OBJECTIVES 16
#define MK_MAX_SOLDIERS_PER_UNIT 16
#define MK_MAX_UNITS 64
#define MK_UNIT_PICK_RADIUS_M 8.0f
#define MK_DEFAULT_MOVE_SPEED_M_PER_TICK 6.0f

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
    MK_ORDER_RALLY = 8
} mk_order_t;

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

typedef struct {
    float x;
    float y;
} mk_vec2_t;

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
    mk_color_t color;
} mk_faction_t;

typedef struct {
    char name[MK_NAME_CAPACITY];
    int effective_range_m;
    int shots_per_action;
    int damage;
    int suppression;
} mk_weapon_profile_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    mk_soldier_role_t role;
    mk_weapon_profile_t weapon;
    int health;
    int ammo;
    int suppression;
    bool casualty;
    mk_vec2_t offset_m;
    float facing_degrees;
} mk_soldier_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    mk_side_t side;
    uint32_t faction_id;
    mk_training_t training;
    mk_order_t order;
    mk_vec2_t position_m;
    mk_vec2_t target_position_m;
    float facing_degrees;
    float cohesion_radius_m;
    float move_speed_m_per_tick;
    int suppression;
    mk_unit_status_t status;
    bool has_move_target;
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
    char name[MK_NAME_CAPACITY];
    float width_m;
    float height_m;
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
    bool visible;
    float distance_m;
    int cover;
    uint32_t blocking_terrain_id;
    uint32_t cover_terrain_id;
    mk_terrain_kind_t cover_terrain_kind;
} mk_line_of_sight_t;

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
    mk_unit_status_t attacker_status;
    mk_unit_status_t target_status_before;
    mk_unit_status_t target_status_after;
    bool resolved;
} mk_fire_result_t;

typedef struct {
    char name[MK_SCENARIO_NAME_CAPACITY];
    uint64_t seed;
    mk_map_t map;
    size_t faction_count;
    mk_faction_t factions[MK_MAX_FACTIONS];
    size_t objective_count;
    mk_objective_t objectives[MK_MAX_OBJECTIVES];
    size_t unit_count;
    mk_unit_t units[MK_MAX_UNITS];
} mk_scenario_definition_t;

typedef struct {
    char scenario_name[MK_SCENARIO_NAME_CAPACITY];
    uint32_t tick;
    uint64_t rng_state;
    uint32_t selected_unit_id;
    mk_map_t map;
    size_t faction_count;
    mk_faction_t factions[MK_MAX_FACTIONS];
    size_t objective_count;
    mk_objective_t objectives[MK_MAX_OBJECTIVES];
    size_t unit_count;
    mk_unit_t units[MK_MAX_UNITS];
} mk_game_t;

typedef struct {
    char scenario_name[MK_SCENARIO_NAME_CAPACITY];
    uint32_t tick;
    uint64_t rng_state;
    uint32_t selected_unit_id;
    mk_map_t map;
    size_t faction_count;
    mk_faction_t factions[MK_MAX_FACTIONS];
    size_t objective_count;
    mk_objective_t objectives[MK_MAX_OBJECTIVES];
    size_t unit_count;
    mk_unit_t units[MK_MAX_UNITS];
} mk_game_snapshot_t;

const char *mk_version(void);

void mk_game_init(mk_game_t *game, uint64_t seed);
uint32_t mk_random_u32(mk_game_t *game);
float mk_random_float01(mk_game_t *game);
void mk_game_step(mk_game_t *game);
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
mk_faction_t mk_make_faction(const char *name, mk_side_t side, mk_color_t color);
mk_map_t mk_make_map(const char *name, float width_m, float height_m);
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

mk_result_t mk_map_add_terrain(mk_map_t *map, const mk_terrain_zone_t *terrain, uint32_t *out_terrain_id);
mk_result_t mk_scenario_add_faction(mk_scenario_definition_t *scenario, const mk_faction_t *faction, uint32_t *out_faction_id);
mk_result_t mk_scenario_add_objective(mk_scenario_definition_t *scenario, const mk_objective_t *objective, uint32_t *out_objective_id);
mk_result_t mk_scenario_add_unit(mk_scenario_definition_t *scenario, const mk_unit_t *unit, uint32_t *out_unit_id);

mk_result_t mk_game_add_unit(mk_game_t *game, const mk_unit_t *unit, uint32_t *out_unit_id);
mk_unit_t *mk_game_find_unit(mk_game_t *game, uint32_t unit_id);
const mk_unit_t *mk_game_find_unit_const(const mk_game_t *game, uint32_t unit_id);

mk_result_t mk_unit_add_soldier(mk_unit_t *unit, const mk_soldier_t *soldier, uint32_t *out_soldier_id);

#ifdef __cplusplus
}
#endif

#endif
