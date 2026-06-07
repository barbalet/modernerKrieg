#ifndef MODERNER_KRIEG_CORE_H
#define MODERNER_KRIEG_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MK_NAME_CAPACITY 64
#define MK_PATH_CAPACITY 256
#define MK_KIND_CAPACITY 32
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
#define MK_MAX_SCENARIO_SPAWN_ZONES 16
#define MK_MAX_SCENARIO_UNIT_TEMPLATES 16
#define MK_MAX_SCENARIO_CIVILIAN_ARCHETYPES 16
#define MK_MAX_SCENARIO_CIVILIAN_GROUPS 16
#define MK_MAX_GAMEPLAY_ROUTE_STEPS 16
#define MK_MAX_GAMEPLAY_AREA_LEVELS 8
#define MK_MAX_GAMEPLAY_AREA_FEATURES 128
#define MK_MAX_GAMEPLAY_AREA_REGIONS 64
#define MK_MAX_GAMEPLAY_TOPOLOGY_NODES 64
#define MK_MAX_GAMEPLAY_TOPOLOGY_PORTALS 128
#define MK_MAX_GAMEPLAY_SEMANTIC_ZONES 64
#define MK_GAMEPLAY_BLOCKED_NAVIGATION_COST 100000
#define MK_GAMEPLAY_LOS_MAX_SAMPLES 512
#define MK_SCENARIO_TEXT_CAPACITY 256
#define MK_AFTER_ACTION_SUMMARY_CAPACITY 256
#define MK_UNIT_PICK_RADIUS_M 8.0f
#define MK_DEFAULT_MOVE_SPEED_M_PER_TICK 6.0f
#define MK_DEFAULT_CIVILIAN_SPEED_M_PER_TICK 2.0f
#define MK_DEFAULT_SCORE_SUCCESS_THRESHOLD 450
#define MK_DEFAULT_SCORE_PARTIAL_THRESHOLD 150
#define MK_DEFAULT_SCORE_OBJECTIVE_WEIGHT 100
#define MK_DEFAULT_SCORE_CIVILIAN_RISK_WEIGHT 10
#define MK_DEFAULT_SCORE_PLAYER_CASUALTY_WEIGHT 50
#define MK_DEFAULT_SCORE_CIVILIAN_CASUALTY_WEIGHT 100
#define MK_DEFAULT_SCORE_TIME_WEIGHT 1

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
    MK_ORDER_WITHDRAW = 9,
    MK_ORDER_INVESTIGATE = 10
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
    MK_CIVILIAN_FOLLOWING_INSTRUCTIONS = 3,
    MK_CIVILIAN_EVACUATED = 4,
    MK_CIVILIAN_WOUNDED = 5,
    MK_CIVILIAN_DEAD = 6
} mk_civilian_state_t;

typedef enum {
    MK_CIVILIAN_INTENT_NONE = 0,
    MK_CIVILIAN_INTENT_SHELTER = 1,
    MK_CIVILIAN_INTENT_FLEE = 2,
    MK_CIVILIAN_INTENT_EVACUATE = 3,
    MK_CIVILIAN_INTENT_FOLLOW_INSTRUCTIONS = 4,
    MK_CIVILIAN_INTENT_FREEZE = 5,
    MK_CIVILIAN_INTENT_ASSIST_GROUP = 6
} mk_civilian_intent_t;

typedef enum {
    MK_CONTACT_REPORT_FIRE = 0,
    MK_CONTACT_REPORT_REVEAL = 1,
    MK_CONTACT_REPORT_CIVILIAN_RISK = 2,
    MK_CONTACT_REPORT_SUSPECTED_DANGER = 3,
    MK_CONTACT_REPORT_FALSE_CONTACT = 4,
    MK_CONTACT_REPORT_SEARCH = 5,
    MK_CONTACT_REPORT_BREACH = 6
} mk_contact_report_kind_t;

typedef enum {
    MK_SEARCH_OUTCOME_CLEAR = 0,
    MK_SEARCH_OUTCOME_CACHE_FOUND = 1,
    MK_SEARCH_OUTCOME_THREAT_REVEALED = 2,
    MK_SEARCH_OUTCOME_CIVILIAN_FOUND = 3,
    MK_SEARCH_OUTCOME_BOOBY_TRAP = 4,
    MK_SEARCH_OUTCOME_INTELLIGENCE = 5
} mk_search_outcome_t;

typedef enum {
    MK_BREACH_OUTCOME_NONE = 0,
    MK_BREACH_OUTCOME_ALREADY_OPEN = 1,
    MK_BREACH_OUTCOME_BREACHED = 2,
    MK_BREACH_OUTCOME_UNSAFE = 3
} mk_breach_outcome_t;

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
    char node_id[MK_NAME_CAPACITY];
    char portal_id[MK_NAME_CAPACITY];
    char level_id[MK_NAME_CAPACITY];
    mk_vec2_t waypoint_m;
    int cumulative_cost;
    bool vertical_transition;
} mk_gameplay_route_step_t;

typedef struct {
    bool valid;
    char start_node_id[MK_NAME_CAPACITY];
    char goal_node_id[MK_NAME_CAPACITY];
    char start_level_id[MK_NAME_CAPACITY];
    char goal_level_id[MK_NAME_CAPACITY];
    mk_vec2_t start_m;
    mk_vec2_t goal_m;
    size_t step_count;
    int total_cost;
    bool uses_vertical_transition;
    char blocked_reason[MK_KIND_CAPACITY];
    mk_gameplay_route_step_t steps[MK_MAX_GAMEPLAY_ROUTE_STEPS];
} mk_gameplay_route_t;

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
    char scenario_id[MK_NAME_CAPACITY];
    char name[MK_NAME_CAPACITY];
    char kind[MK_KIND_CAPACITY];
    mk_side_t side;
    char level_id[MK_NAME_CAPACITY];
    char topology_node_id[MK_NAME_CAPACITY];
    mk_rect_t bounds_m;
    int capacity;
    bool active;
} mk_spawn_zone_t;

typedef struct {
    uint32_t id;
    char scenario_id[MK_NAME_CAPACITY];
    char name[MK_NAME_CAPACITY];
    char role[MK_KIND_CAPACITY];
    mk_side_t side;
    mk_training_t training;
    char default_spawn_zone_id[MK_NAME_CAPACITY];
    int expected_soldiers;
} mk_unit_template_t;

typedef struct {
    uint32_t id;
    char scenario_id[MK_NAME_CAPACITY];
    char name[MK_NAME_CAPACITY];
    char sprite_id[MK_NAME_CAPACITY];
    int baseline_stress;
    int baseline_risk;
    int compliance;
    bool protected_noncombatant;
} mk_civilian_archetype_t;

typedef struct {
    uint32_t id;
    char scenario_id[MK_NAME_CAPACITY];
    char name[MK_NAME_CAPACITY];
    char archetype_id[MK_NAME_CAPACITY];
    char spawn_zone_id[MK_NAME_CAPACITY];
    char level_id[MK_NAME_CAPACITY];
    char topology_node_id[MK_NAME_CAPACITY];
    int expected_count;
    int baseline_stress;
    int compliance;
    bool protected_noncombatants;
} mk_civilian_group_t;

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
    char template_id[MK_NAME_CAPACITY];
    char group_id[MK_NAME_CAPACITY];
    char spawn_zone_id[MK_NAME_CAPACITY];
    char topology_node_id[MK_NAME_CAPACITY];
    char level_id[MK_NAME_CAPACITY];
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
    bool has_route;
    size_t route_step_count;
    size_t route_step_index;
    int route_total_cost;
    bool route_uses_vertical_transition;
    mk_vec2_t route_waypoints_m[MK_MAX_GAMEPLAY_ROUTE_STEPS];
    char route_step_level_ids[MK_MAX_GAMEPLAY_ROUTE_STEPS][MK_NAME_CAPACITY];
    char route_step_portal_ids[MK_MAX_GAMEPLAY_ROUTE_STEPS][MK_NAME_CAPACITY];
    bool route_step_vertical_transition[MK_MAX_GAMEPLAY_ROUTE_STEPS];
    uint32_t route_request_id;
    uint32_t route_stuck_ticks;
    uint32_t route_failure_count;
    uint32_t route_vertical_transitions_completed;
    mk_vec2_t route_last_position_m;
    char route_failure_reason[MK_KIND_CAPACITY];
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
    bool searched;
    uint32_t searched_tick;
    mk_search_outcome_t last_search_outcome;
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
    char id[MK_NAME_CAPACITY];
    int index;
    float elevation_m;
    char image_path[MK_PATH_CAPACITY];
    char alpha[MK_KIND_CAPACITY];
    bool blocks_los_default;
    bool blocks_movement_default;
} mk_gameplay_level_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    char level_id[MK_NAME_CAPACITY];
    char kind[MK_KIND_CAPACITY];
    int pixel_x;
    int pixel_y;
    int pixel_width;
    int pixel_height;
    mk_rect_t bounds_m;
    bool blocks_los;
    bool blocks_movement;
    bool allows_los;
    bool allows_movement;
} mk_gameplay_feature_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    int storeys;
    int pixel_x;
    int pixel_y;
    int pixel_width;
    int pixel_height;
    mk_rect_t bounds_m;
    char roof_level_id[MK_NAME_CAPACITY];
} mk_gameplay_region_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    char kind[MK_KIND_CAPACITY];
    char level_id[MK_NAME_CAPACITY];
    char region_id[MK_NAME_CAPACITY];
    char label[MK_NAME_CAPACITY];
    int pixel_x;
    int pixel_y;
    int pixel_width;
    int pixel_height;
    mk_rect_t bounds_m;
    bool enterable;
} mk_gameplay_topology_node_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    char kind[MK_KIND_CAPACITY];
    char state[MK_KIND_CAPACITY];
    char from_node_id[MK_NAME_CAPACITY];
    char to_node_id[MK_NAME_CAPACITY];
    char level_id[MK_NAME_CAPACITY];
    char feature_id[MK_NAME_CAPACITY];
    int pixel_x;
    int pixel_y;
    int pixel_width;
    int pixel_height;
    mk_rect_t bounds_m;
    bool bidirectional;
    bool vertical;
    int movement_cost;
    bool breached;
    uint32_t breached_tick;
    bool searched;
    uint32_t searched_tick;
} mk_gameplay_topology_portal_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    char kind[MK_KIND_CAPACITY];
    char node_id[MK_NAME_CAPACITY];
    char level_id[MK_NAME_CAPACITY];
    int pixel_x;
    int pixel_y;
    int pixel_width;
    int pixel_height;
    mk_rect_t bounds_m;
    int priority;
    bool searched;
    uint32_t searched_tick;
    mk_search_outcome_t last_search_outcome;
} mk_gameplay_semantic_zone_t;

typedef struct {
    bool in_bounds;
    char level_id[MK_NAME_CAPACITY];
    mk_vec2_t position_m;
    mk_ivec2_t pixel;
    float elevation_m;
    char feature_id[MK_NAME_CAPACITY];
    char feature_kind[MK_KIND_CAPACITY];
    char node_id[MK_NAME_CAPACITY];
    char node_kind[MK_KIND_CAPACITY];
    char portal_id[MK_NAME_CAPACITY];
    char portal_kind[MK_KIND_CAPACITY];
    char portal_state[MK_KIND_CAPACITY];
    char semantic_zone_id[MK_NAME_CAPACITY];
    char semantic_zone_kind[MK_KIND_CAPACITY];
    int navigation_cost;
    int cover;
    bool blocks_los;
    bool blocks_movement;
    bool interior;
    bool rooftop;
    bool vertical_connector;
    bool restricted_fire_lane;
    bool civilian_shelter;
    bool danger_area;
} mk_gameplay_tactical_query_t;

typedef struct {
    bool visible;
    float distance_m;
    char level_id[MK_NAME_CAPACITY];
    size_t sample_count;
    int cover;
    bool blocked_by_feature;
    char blocking_feature_id[MK_NAME_CAPACITY];
    char blocking_feature_kind[MK_KIND_CAPACITY];
    mk_vec2_t blocking_position_m;
    char cover_feature_id[MK_NAME_CAPACITY];
    char cover_node_id[MK_NAME_CAPACITY];
    char cover_zone_id[MK_NAME_CAPACITY];
} mk_gameplay_los_trace_t;

typedef struct {
    bool loaded;
    int schema_version;
    char id[MK_NAME_CAPACITY];
    char map_id[MK_NAME_CAPACITY];
    char name[MK_NAME_CAPACITY];
    float world_width_m;
    float world_height_m;
    int pixel_width;
    int pixel_height;
    float pixels_per_meter;
    char origin[MK_KIND_CAPACITY];
    int max_storeys;
    size_t level_count;
    mk_gameplay_level_t levels[MK_MAX_GAMEPLAY_AREA_LEVELS];
    size_t feature_count;
    mk_gameplay_feature_t features[MK_MAX_GAMEPLAY_AREA_FEATURES];
    size_t region_count;
    mk_gameplay_region_t regions[MK_MAX_GAMEPLAY_AREA_REGIONS];
    bool topology_loaded;
    int topology_schema_version;
    char topology_id[MK_NAME_CAPACITY];
    size_t topology_node_count;
    mk_gameplay_topology_node_t topology_nodes[MK_MAX_GAMEPLAY_TOPOLOGY_NODES];
    size_t topology_portal_count;
    mk_gameplay_topology_portal_t topology_portals[MK_MAX_GAMEPLAY_TOPOLOGY_PORTALS];
    size_t semantic_zone_count;
    mk_gameplay_semantic_zone_t semantic_zones[MK_MAX_GAMEPLAY_SEMANTIC_ZONES];
} mk_gameplay_area_t;

typedef struct {
    uint32_t id;
    char name[MK_NAME_CAPACITY];
    char label[MK_NAME_CAPACITY];
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
    char archetype_id[MK_NAME_CAPACITY];
    char group_id[MK_NAME_CAPACITY];
    char spawn_zone_id[MK_NAME_CAPACITY];
    char level_id[MK_NAME_CAPACITY];
    char topology_node_id[MK_NAME_CAPACITY];
    char sprite_id[MK_NAME_CAPACITY];
    mk_vec2_t position_m;
    mk_civilian_state_t state;
    mk_civilian_intent_t intent;
    mk_vec2_t destination_m;
    bool has_destination;
    float speed_m_per_tick;
    bool has_route;
    size_t route_step_count;
    size_t route_step_index;
    int route_total_cost;
    bool route_uses_vertical_transition;
    mk_vec2_t route_waypoints_m[MK_MAX_GAMEPLAY_ROUTE_STEPS];
    char route_step_level_ids[MK_MAX_GAMEPLAY_ROUTE_STEPS][MK_NAME_CAPACITY];
    char route_step_portal_ids[MK_MAX_GAMEPLAY_ROUTE_STEPS][MK_NAME_CAPACITY];
    bool route_step_vertical_transition[MK_MAX_GAMEPLAY_ROUTE_STEPS];
    uint32_t route_request_id;
    uint32_t route_stuck_ticks;
    uint32_t route_failure_count;
    uint32_t route_vertical_transitions_completed;
    mk_vec2_t route_last_position_m;
    char route_failure_reason[MK_KIND_CAPACITY];
    char destination_level_id[MK_NAME_CAPACITY];
    int stress;
    int risk;
    int compliance;
    bool protected_noncombatant;
} mk_civilian_t;

typedef struct {
    bool visible;
    float distance_m;
    int cover;
    uint32_t blocking_terrain_id;
    uint32_t cover_terrain_id;
    mk_terrain_kind_t cover_terrain_kind;
    bool blocked_by_gameplay_area;
    char level_id[MK_NAME_CAPACITY];
    char blocking_feature_id[MK_NAME_CAPACITY];
    char blocking_feature_kind[MK_KIND_CAPACITY];
    char cover_feature_id[MK_NAME_CAPACITY];
    char cover_node_id[MK_NAME_CAPACITY];
    char cover_zone_id[MK_NAME_CAPACITY];
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
    mk_search_outcome_t outcome;
    uint32_t unit_id;
    uint32_t terrain_id;
    uint32_t contact_report_id;
    uint32_t revealed_unit_id;
    char semantic_zone_id[MK_NAME_CAPACITY];
    char topology_node_id[MK_NAME_CAPACITY];
    char level_id[MK_NAME_CAPACITY];
    mk_vec2_t position_m;
    int score_delta;
    bool resolved;
} mk_search_result_t;

typedef struct {
    mk_breach_outcome_t outcome;
    uint32_t unit_id;
    uint32_t contact_report_id;
    char portal_id[MK_NAME_CAPACITY];
    char previous_state[MK_KIND_CAPACITY];
    char new_state[MK_KIND_CAPACITY];
    char from_node_id[MK_NAME_CAPACITY];
    char to_node_id[MK_NAME_CAPACITY];
    char level_id[MK_NAME_CAPACITY];
    mk_vec2_t position_m;
    int score_delta;
    bool resolved;
} mk_breach_result_t;

typedef struct {
    int objective_points;
    int interaction_points;
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
    int score_objective_weight;
    int score_civilian_risk_weight;
    int score_player_casualty_weight;
    int score_civilian_casualty_weight;
    int score_time_weight;
    mk_map_t map;
    mk_gameplay_area_t gameplay_area;
    size_t controller_count;
    mk_controller_slot_t controllers[MK_MAX_CONTROLLERS];
    size_t faction_count;
    mk_faction_t factions[MK_MAX_FACTIONS];
    size_t force_count;
    mk_force_t forces[MK_MAX_FORCES];
    size_t spawn_zone_count;
    mk_spawn_zone_t spawn_zones[MK_MAX_SCENARIO_SPAWN_ZONES];
    size_t unit_template_count;
    mk_unit_template_t unit_templates[MK_MAX_SCENARIO_UNIT_TEMPLATES];
    size_t civilian_archetype_count;
    mk_civilian_archetype_t civilian_archetypes[MK_MAX_SCENARIO_CIVILIAN_ARCHETYPES];
    size_t civilian_group_count;
    mk_civilian_group_t civilian_groups[MK_MAX_SCENARIO_CIVILIAN_GROUPS];
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
    int score_objective_weight;
    int score_civilian_risk_weight;
    int score_player_casualty_weight;
    int score_civilian_casualty_weight;
    int score_time_weight;
    uint32_t selected_unit_id;
    mk_map_t map;
    mk_gameplay_area_t gameplay_area;
    size_t controller_count;
    mk_controller_slot_t controllers[MK_MAX_CONTROLLERS];
    size_t faction_count;
    mk_faction_t factions[MK_MAX_FACTIONS];
    size_t force_count;
    mk_force_t forces[MK_MAX_FORCES];
    size_t spawn_zone_count;
    mk_spawn_zone_t spawn_zones[MK_MAX_SCENARIO_SPAWN_ZONES];
    size_t unit_template_count;
    mk_unit_template_t unit_templates[MK_MAX_SCENARIO_UNIT_TEMPLATES];
    size_t civilian_archetype_count;
    mk_civilian_archetype_t civilian_archetypes[MK_MAX_SCENARIO_CIVILIAN_ARCHETYPES];
    size_t civilian_group_count;
    mk_civilian_group_t civilian_groups[MK_MAX_SCENARIO_CIVILIAN_GROUPS];
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
    int score_objective_weight;
    int score_civilian_risk_weight;
    int score_player_casualty_weight;
    int score_civilian_casualty_weight;
    int score_time_weight;
    uint32_t selected_unit_id;
    mk_map_t map;
    mk_gameplay_area_t gameplay_area;
    size_t controller_count;
    mk_controller_slot_t controllers[MK_MAX_CONTROLLERS];
    size_t faction_count;
    mk_faction_t factions[MK_MAX_FACTIONS];
    size_t force_count;
    mk_force_t forces[MK_MAX_FORCES];
    size_t spawn_zone_count;
    mk_spawn_zone_t spawn_zones[MK_MAX_SCENARIO_SPAWN_ZONES];
    size_t unit_template_count;
    mk_unit_template_t unit_templates[MK_MAX_SCENARIO_UNIT_TEMPLATES];
    size_t civilian_archetype_count;
    mk_civilian_archetype_t civilian_archetypes[MK_MAX_SCENARIO_CIVILIAN_ARCHETYPES];
    size_t civilian_group_count;
    mk_civilian_group_t civilian_groups[MK_MAX_SCENARIO_CIVILIAN_GROUPS];
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
mk_result_t mk_game_pick_contact_at(
    const mk_game_t *game,
    mk_vec2_t position_m,
    float radius_m,
    uint32_t *out_contact_report_id
);
mk_result_t mk_game_select_unit(mk_game_t *game, uint32_t unit_id);
mk_result_t mk_game_select_unit_at(mk_game_t *game, mk_vec2_t position_m, float radius_m, uint32_t *out_unit_id);
mk_result_t mk_game_clear_selection(mk_game_t *game);
mk_result_t mk_game_issue_order(mk_game_t *game, uint32_t unit_id, mk_order_t order);
mk_result_t mk_game_issue_move_order(mk_game_t *game, uint32_t unit_id, mk_vec2_t target_position_m);
mk_result_t mk_game_issue_assault_move_order(mk_game_t *game, uint32_t unit_id, mk_vec2_t target_position_m);
mk_result_t mk_game_issue_investigate_order(mk_game_t *game, uint32_t unit_id, mk_vec2_t target_position_m);
mk_result_t mk_game_issue_withdraw_order(mk_game_t *game, uint32_t unit_id, mk_vec2_t target_position_m);
mk_result_t mk_game_issue_move_order_to_level(
    mk_game_t *game,
    uint32_t unit_id,
    const char *target_level_id,
    mk_vec2_t target_position_m
);
mk_result_t mk_game_issue_selected_move_order(mk_game_t *game, mk_vec2_t target_position_m);
mk_result_t mk_game_issue_selected_investigate_order(mk_game_t *game, mk_vec2_t target_position_m);
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
mk_result_t mk_game_update_civilian_ai(mk_game_t *game);
mk_result_t mk_game_issue_civilian_instruction(
    mk_game_t *game,
    uint32_t civilian_id,
    mk_civilian_intent_t intent,
    const char *target_level_id,
    mk_vec2_t target_position_m
);
mk_result_t mk_game_update_objective_control(mk_game_t *game);
mk_result_t mk_game_score(const mk_game_t *game, mk_score_t *out_score);
mk_result_t mk_game_after_action_report(const mk_game_t *game, mk_after_action_report_t *out_report);
mk_result_t mk_game_search_semantic_zone(
    mk_game_t *game,
    uint32_t unit_id,
    const char *semantic_zone_id,
    mk_search_result_t *out_search_result
);
mk_result_t mk_game_search_terrain(
    mk_game_t *game,
    uint32_t unit_id,
    uint32_t terrain_id,
    mk_search_result_t *out_search_result
);
mk_result_t mk_game_breach_portal(
    mk_game_t *game,
    uint32_t unit_id,
    const char *portal_id,
    mk_breach_result_t *out_breach_result
);

mk_vec2_t mk_vec2(float x, float y);
mk_ivec2_t mk_ivec2(int x, int y);
mk_rect_t mk_rect(float x, float y, float width, float height);
float mk_clamp_f32(float value, float minimum, float maximum);
int mk_clamp_i32(int value, int minimum, int maximum);
bool mk_rect_contains_point(mk_rect_t rect, mk_vec2_t point);
float mk_vec2_distance(mk_vec2_t first, mk_vec2_t second);
bool mk_gameplay_area_is_loaded(const mk_gameplay_area_t *area);
mk_result_t mk_gameplay_area_world_to_pixel(
    const mk_gameplay_area_t *area,
    mk_vec2_t position_m,
    mk_ivec2_t *out_pixel
);
mk_result_t mk_gameplay_area_pixel_to_world(
    const mk_gameplay_area_t *area,
    mk_ivec2_t pixel,
    mk_vec2_t *out_position_m
);
const mk_gameplay_level_t *mk_gameplay_area_find_level(
    const mk_gameplay_area_t *area,
    const char *level_id
);
const mk_gameplay_feature_t *mk_gameplay_area_find_feature(
    const mk_gameplay_area_t *area,
    const char *feature_id
);
const mk_gameplay_feature_t *mk_gameplay_area_find_feature_at_world(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m
);
const mk_gameplay_region_t *mk_gameplay_area_find_region(
    const mk_gameplay_area_t *area,
    const char *region_id
);
const mk_gameplay_region_t *mk_gameplay_area_find_region_at_world(
    const mk_gameplay_area_t *area,
    mk_vec2_t position_m
);
bool mk_gameplay_area_topology_is_loaded(const mk_gameplay_area_t *area);
const mk_gameplay_topology_node_t *mk_gameplay_area_find_topology_node(
    const mk_gameplay_area_t *area,
    const char *node_id
);
const mk_gameplay_topology_node_t *mk_gameplay_area_find_topology_node_at_world(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m
);
const mk_gameplay_topology_portal_t *mk_gameplay_area_find_topology_portal(
    const mk_gameplay_area_t *area,
    const char *portal_id
);
const mk_gameplay_topology_portal_t *mk_gameplay_area_find_topology_portal_at_world(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m
);
const mk_gameplay_semantic_zone_t *mk_gameplay_area_find_semantic_zone(
    const mk_gameplay_area_t *area,
    const char *zone_id
);
const mk_gameplay_semantic_zone_t *mk_gameplay_area_find_semantic_zone_at_world(
    const mk_gameplay_area_t *area,
    const char *kind,
    mk_vec2_t position_m
);
mk_result_t mk_gameplay_area_topology_debug_dump(
    const mk_gameplay_area_t *area,
    char *out_text,
    size_t capacity
);
mk_result_t mk_gameplay_area_query_tactical_at(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m,
    mk_gameplay_tactical_query_t *out_query
);
mk_result_t mk_gameplay_area_navigation_cost_at(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m,
    int *out_navigation_cost
);
mk_result_t mk_gameplay_area_cover_at(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m,
    int *out_cover
);
mk_result_t mk_gameplay_area_trace_line_of_sight(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t from_m,
    mk_vec2_t to_m,
    mk_gameplay_los_trace_t *out_trace
);
mk_result_t mk_gameplay_area_plan_route(
    const mk_gameplay_area_t *area,
    const char *start_level_id,
    mk_vec2_t start_m,
    const char *goal_level_id,
    mk_vec2_t goal_m,
    mk_gameplay_route_t *out_route
);
bool mk_gameplay_area_feature_contains_pixel(
    const mk_gameplay_feature_t *feature,
    mk_ivec2_t pixel
);
bool mk_gameplay_area_feature_contains_world(
    const mk_gameplay_area_t *area,
    const mk_gameplay_feature_t *feature,
    mk_vec2_t position_m
);
bool mk_gameplay_area_blocks_los_at(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m
);
bool mk_gameplay_area_blocks_movement_at(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m
);
const char *mk_civilian_intent_name(mk_civilian_intent_t intent);

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
mk_result_t mk_scenario_set_gameplay_area(mk_scenario_definition_t *scenario, const mk_gameplay_area_t *area);
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
