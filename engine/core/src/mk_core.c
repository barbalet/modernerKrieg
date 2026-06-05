#include "mk_core.h"

#include <string.h>

static void mk_copy_name(char destination[MK_NAME_CAPACITY], const char *source) {
    const char *text = source == NULL ? "" : source;
    size_t index = 0;

    for (; index + 1 < MK_NAME_CAPACITY && text[index] != '\0'; ++index) {
        destination[index] = text[index];
    }

    destination[index] = '\0';
}

static int mk_training_recovery(mk_training_t training) {
    switch (training) {
        case MK_TRAINING_ELITE:
            return 3;
        case MK_TRAINING_VETERAN:
            return 2;
        case MK_TRAINING_REGULAR:
            return 1;
        case MK_TRAINING_UNTRAINED:
        default:
            return 0;
    }
}

static int mk_subtract_floor_zero(int value, int amount) {
    if (amount <= 0) {
        return value;
    }

    if (value <= amount) {
        return 0;
    }

    return value - amount;
}

const char *mk_version(void) {
    return "0.1.0";
}

void mk_game_init(mk_game_t *game, uint64_t seed) {
    if (game == NULL) {
        return;
    }

    memset(game, 0, sizeof(*game));
    game->rng_state = seed;
}

uint32_t mk_random_u32(mk_game_t *game) {
    uint64_t z;

    if (game == NULL) {
        return 0;
    }

    game->rng_state += UINT64_C(0x9E3779B97F4A7C15);
    z = game->rng_state;
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    z = z ^ (z >> 31);

    return (uint32_t)(z >> 32);
}

float mk_random_float01(mk_game_t *game) {
    return (float)(mk_random_u32(game) >> 8) / 16777216.0f;
}

void mk_game_step(mk_game_t *game) {
    size_t unit_index;

    if (game == NULL) {
        return;
    }

    game->tick += 1;

    for (unit_index = 0; unit_index < game->unit_count; ++unit_index) {
        mk_unit_t *unit = &game->units[unit_index];
        int recovery = mk_training_recovery(unit->training);
        size_t soldier_index;

        if (unit->order == MK_ORDER_RALLY) {
            recovery += 2;
        }

        unit->suppression = mk_subtract_floor_zero(unit->suppression, recovery);

        for (soldier_index = 0; soldier_index < unit->soldier_count; ++soldier_index) {
            mk_soldier_t *soldier = &unit->soldiers[soldier_index];
            int soldier_recovery = soldier->casualty ? 0 : recovery;
            soldier->suppression = mk_subtract_floor_zero(soldier->suppression, soldier_recovery);
        }
    }
}

mk_weapon_profile_t mk_make_weapon(
    const char *name,
    int effective_range_m,
    int shots_per_action,
    int damage,
    int suppression
) {
    mk_weapon_profile_t weapon;

    memset(&weapon, 0, sizeof(weapon));
    mk_copy_name(weapon.name, name);
    weapon.effective_range_m = effective_range_m;
    weapon.shots_per_action = shots_per_action;
    weapon.damage = damage;
    weapon.suppression = suppression;

    return weapon;
}

mk_soldier_t mk_make_soldier(
    const char *name,
    mk_soldier_role_t role,
    mk_weapon_profile_t weapon
) {
    mk_soldier_t soldier;

    memset(&soldier, 0, sizeof(soldier));
    mk_copy_name(soldier.name, name);
    soldier.role = role;
    soldier.weapon = weapon;
    soldier.health = 100;
    soldier.ammo = 120;
    soldier.facing_degrees = 0.0f;

    return soldier;
}

mk_unit_t mk_make_unit(
    const char *name,
    mk_side_t side,
    mk_training_t training,
    mk_vec2_t position_m
) {
    mk_unit_t unit;

    memset(&unit, 0, sizeof(unit));
    mk_copy_name(unit.name, name);
    unit.side = side;
    unit.training = training;
    unit.order = MK_ORDER_HOLD;
    unit.position_m = position_m;
    unit.facing_degrees = 0.0f;
    unit.cohesion_radius_m = 8.0f;

    return unit;
}

mk_result_t mk_game_add_unit(mk_game_t *game, const mk_unit_t *unit, uint32_t *out_unit_id) {
    mk_unit_t copy;

    if (game == NULL || unit == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (game->unit_count >= MK_MAX_UNITS) {
        return MK_ERROR_CAPACITY;
    }

    copy = *unit;
    copy.id = (uint32_t)(game->unit_count + 1);

    game->units[game->unit_count] = copy;
    game->unit_count += 1;

    if (out_unit_id != NULL) {
        *out_unit_id = copy.id;
    }

    return MK_OK;
}

mk_unit_t *mk_game_find_unit(mk_game_t *game, uint32_t unit_id) {
    size_t index;

    if (game == NULL) {
        return NULL;
    }

    for (index = 0; index < game->unit_count; ++index) {
        if (game->units[index].id == unit_id) {
            return &game->units[index];
        }
    }

    return NULL;
}

const mk_unit_t *mk_game_find_unit_const(const mk_game_t *game, uint32_t unit_id) {
    size_t index;

    if (game == NULL) {
        return NULL;
    }

    for (index = 0; index < game->unit_count; ++index) {
        if (game->units[index].id == unit_id) {
            return &game->units[index];
        }
    }

    return NULL;
}

mk_result_t mk_unit_add_soldier(mk_unit_t *unit, const mk_soldier_t *soldier, uint32_t *out_soldier_id) {
    mk_soldier_t copy;

    if (unit == NULL || soldier == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (unit->soldier_count >= MK_MAX_SOLDIERS_PER_UNIT) {
        return MK_ERROR_CAPACITY;
    }

    copy = *soldier;
    copy.id = (uint32_t)(unit->soldier_count + 1);

    unit->soldiers[unit->soldier_count] = copy;
    unit->soldier_count += 1;

    if (out_soldier_id != NULL) {
        *out_soldier_id = copy.id;
    }

    return MK_OK;
}

