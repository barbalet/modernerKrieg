#ifndef MODERNER_KRIEG_CORE_H
#define MODERNER_KRIEG_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MK_NAME_CAPACITY 64
#define MK_MAX_SOLDIERS_PER_UNIT 16
#define MK_MAX_UNITS 64

typedef enum {
    MK_OK = 0,
    MK_ERROR_INVALID_ARGUMENT = -1,
    MK_ERROR_CAPACITY = -2,
    MK_ERROR_NOT_FOUND = -3
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

typedef struct {
    float x;
    float y;
} mk_vec2_t;

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
    mk_training_t training;
    mk_order_t order;
    mk_vec2_t position_m;
    float facing_degrees;
    float cohesion_radius_m;
    int suppression;
    size_t soldier_count;
    mk_soldier_t soldiers[MK_MAX_SOLDIERS_PER_UNIT];
} mk_unit_t;

typedef struct {
    uint32_t tick;
    uint64_t rng_state;
    size_t unit_count;
    mk_unit_t units[MK_MAX_UNITS];
} mk_game_t;

const char *mk_version(void);

void mk_game_init(mk_game_t *game, uint64_t seed);
uint32_t mk_random_u32(mk_game_t *game);
float mk_random_float01(mk_game_t *game);
void mk_game_step(mk_game_t *game);

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

mk_result_t mk_game_add_unit(mk_game_t *game, const mk_unit_t *unit, uint32_t *out_unit_id);
mk_unit_t *mk_game_find_unit(mk_game_t *game, uint32_t unit_id);
const mk_unit_t *mk_game_find_unit_const(const mk_game_t *game, uint32_t unit_id);

mk_result_t mk_unit_add_soldier(mk_unit_t *unit, const mk_soldier_t *soldier, uint32_t *out_soldier_id);

#ifdef __cplusplus
}
#endif

#endif

