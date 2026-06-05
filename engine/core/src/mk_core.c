#include "mk_core.h"

#include <math.h>
#include <string.h>

static void mk_copy_text(char *destination, size_t capacity, const char *source) {
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

static void mk_copy_name(char destination[MK_NAME_CAPACITY], const char *source) {
    mk_copy_text(destination, MK_NAME_CAPACITY, source);
}

static void mk_copy_scenario_name(char destination[MK_SCENARIO_NAME_CAPACITY], const char *source) {
    mk_copy_text(destination, MK_SCENARIO_NAME_CAPACITY, source);
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

static int mk_training_morale_bonus(mk_training_t training) {
    switch (training) {
        case MK_TRAINING_ELITE:
            return 6;
        case MK_TRAINING_VETERAN:
            return 4;
        case MK_TRAINING_REGULAR:
            return 2;
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

static int mk_max_int(int a, int b) {
    return a > b ? a : b;
}

static int mk_min_int(int a, int b) {
    return a < b ? a : b;
}

static int mk_clamp_int(int value, int minimum, int maximum) {
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

static size_t mk_unit_live_soldier_count(const mk_unit_t *unit) {
    size_t index;
    size_t live_count = 0;

    if (unit == NULL) {
        return 0;
    }

    for (index = 0; index < unit->soldier_count; ++index) {
        if (!unit->soldiers[index].casualty) {
            live_count += 1;
        }
    }

    return live_count;
}

static int mk_unit_morale_budget(const mk_unit_t *unit) {
    size_t live_count = mk_unit_live_soldier_count(unit);

    if (unit == NULL || live_count == 0) {
        return 0;
    }

    return (int)live_count * 4 + mk_training_morale_bonus(unit->training);
}

static mk_unit_status_t mk_calculate_unit_status(const mk_unit_t *unit) {
    int morale_budget;

    if (unit == NULL || mk_unit_live_soldier_count(unit) == 0) {
        return MK_UNIT_BROKEN;
    }

    morale_budget = mk_unit_morale_budget(unit);

    if (unit->suppression >= morale_budget * 2) {
        return MK_UNIT_BROKEN;
    }

    if (unit->suppression >= morale_budget) {
        return MK_UNIT_PINNED;
    }

    if (unit->suppression >= mk_max_int(1, morale_budget / 2)) {
        return MK_UNIT_SUPPRESSED;
    }

    return MK_UNIT_READY;
}

static void mk_update_unit_status(mk_unit_t *unit) {
    if (unit != NULL) {
        unit->status = mk_calculate_unit_status(unit);
    }
}

static float mk_unit_status_move_multiplier(mk_unit_status_t status) {
    switch (status) {
        case MK_UNIT_SUPPRESSED:
            return 0.75f;
        case MK_UNIT_PINNED:
            return 0.35f;
        case MK_UNIT_BROKEN:
            return 0.0f;
        case MK_UNIT_READY:
        default:
            return 1.0f;
    }
}

static int mk_unit_status_hit_penalty(mk_unit_status_t status) {
    switch (status) {
        case MK_UNIT_SUPPRESSED:
            return 10;
        case MK_UNIT_PINNED:
            return 25;
        case MK_UNIT_BROKEN:
            return 95;
        case MK_UNIT_READY:
        default:
            return 0;
    }
}

static bool mk_rect_is_valid(mk_rect_t rect) {
    return rect.width > 0.0f && rect.height > 0.0f;
}

static bool mk_rect_fits_map(mk_rect_t rect, const mk_map_t *map) {
    if (map == NULL || !mk_rect_is_valid(rect)) {
        return false;
    }

    return rect.x >= 0.0f
        && rect.y >= 0.0f
        && rect.x + rect.width <= map->width_m
        && rect.y + rect.height <= map->height_m;
}

static bool mk_position_fits_map(mk_vec2_t position, const mk_map_t *map) {
    if (map == NULL) {
        return false;
    }

    return position.x >= 0.0f
        && position.y >= 0.0f
        && position.x <= map->width_m
        && position.y <= map->height_m;
}

static float mk_distance_squared(mk_vec2_t first, mk_vec2_t second) {
    float dx = first.x - second.x;
    float dy = first.y - second.y;

    return dx * dx + dy * dy;
}

static float mk_distance(mk_vec2_t first, mk_vec2_t second) {
    return sqrtf(mk_distance_squared(first, second));
}

static bool mk_point_in_rect(mk_vec2_t point, mk_rect_t rect) {
    return point.x >= rect.x
        && point.y >= rect.y
        && point.x <= rect.x + rect.width
        && point.y <= rect.y + rect.height;
}

static bool mk_clip_line_to_rect(mk_vec2_t from, mk_vec2_t to, mk_rect_t rect, float *out_enter_t) {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float p[4];
    float q[4];
    float enter_t = 0.0f;
    float exit_t = 1.0f;
    int index;

    if (!mk_rect_is_valid(rect)) {
        return false;
    }

    p[0] = -dx;
    p[1] = dx;
    p[2] = -dy;
    p[3] = dy;
    q[0] = from.x - rect.x;
    q[1] = rect.x + rect.width - from.x;
    q[2] = from.y - rect.y;
    q[3] = rect.y + rect.height - from.y;

    for (index = 0; index < 4; ++index) {
        if (p[index] == 0.0f) {
            if (q[index] < 0.0f) {
                return false;
            }
        } else {
            float ratio = q[index] / p[index];

            if (p[index] < 0.0f) {
                if (ratio > exit_t) {
                    return false;
                }

                if (ratio > enter_t) {
                    enter_t = ratio;
                }
            } else {
                if (ratio < enter_t) {
                    return false;
                }

                if (ratio < exit_t) {
                    exit_t = ratio;
                }
            }
        }
    }

    if (out_enter_t != NULL) {
        *out_enter_t = enter_t;
    }

    return true;
}

static void mk_apply_target_cover(const mk_map_t *map, mk_vec2_t target_m, mk_line_of_sight_t *line_of_sight) {
    size_t index;

    for (index = 0; index < map->terrain_count; ++index) {
        const mk_terrain_zone_t *terrain = &map->terrain[index];

        if (terrain->cover > line_of_sight->cover && mk_point_in_rect(target_m, terrain->bounds_m)) {
            line_of_sight->cover = terrain->cover;
            line_of_sight->cover_terrain_id = terrain->id;
            line_of_sight->cover_terrain_kind = terrain->kind;
        }
    }
}

static void mk_apply_blocking_terrain(
    const mk_map_t *map,
    mk_vec2_t from_m,
    mk_vec2_t to_m,
    mk_line_of_sight_t *line_of_sight
) {
    float best_enter_t = 2.0f;
    size_t index;

    for (index = 0; index < map->terrain_count; ++index) {
        const mk_terrain_zone_t *terrain = &map->terrain[index];
        float enter_t = 0.0f;
        bool from_inside;
        bool to_inside;

        if (!terrain->blocks_line_of_sight) {
            continue;
        }

        from_inside = mk_point_in_rect(from_m, terrain->bounds_m);
        to_inside = mk_point_in_rect(to_m, terrain->bounds_m);

        if (from_inside || to_inside) {
            continue;
        }

        if (mk_clip_line_to_rect(from_m, to_m, terrain->bounds_m, &enter_t) && enter_t < best_enter_t) {
            best_enter_t = enter_t;
            line_of_sight->blocking_terrain_id = terrain->id;
            line_of_sight->visible = false;
        }
    }
}

static int mk_training_hit_chance(mk_training_t training) {
    switch (training) {
        case MK_TRAINING_ELITE:
            return 65;
        case MK_TRAINING_VETERAN:
            return 55;
        case MK_TRAINING_REGULAR:
            return 45;
        case MK_TRAINING_UNTRAINED:
        default:
            return 30;
    }
}

static int mk_weapon_hit_chance(
    const mk_unit_t *attacker,
    const mk_weapon_profile_t *weapon,
    const mk_line_of_sight_t *line_of_sight
) {
    int chance = mk_training_hit_chance(attacker->training);
    int suppression_penalty = mk_min_int(25, attacker->suppression * 3);
    int status_penalty = mk_unit_status_hit_penalty(attacker->status);

    if (weapon->effective_range_m <= 0 || line_of_sight->distance_m > (float)weapon->effective_range_m) {
        return 0;
    }

    if (line_of_sight->distance_m > (float)weapon->effective_range_m * 0.5f) {
        chance -= 15;
    }

    chance -= line_of_sight->cover * 8;
    chance -= suppression_penalty;
    chance -= status_penalty;

    return mk_clamp_int(chance, 5, 95);
}

static mk_soldier_t *mk_first_live_soldier(mk_unit_t *unit) {
    size_t index;

    if (unit == NULL) {
        return NULL;
    }

    for (index = 0; index < unit->soldier_count; ++index) {
        if (!unit->soldiers[index].casualty) {
            return &unit->soldiers[index];
        }
    }

    return NULL;
}

static int mk_apply_fire_damage(mk_unit_t *target, int damage, int cover, int *out_casualty_count) {
    mk_soldier_t *soldier = mk_first_live_soldier(target);
    int reduced_damage = mk_max_int(1, damage - cover * 5);
    int damage_applied;

    if (out_casualty_count != NULL) {
        *out_casualty_count = 0;
    }

    if (soldier == NULL || damage <= 0) {
        return 0;
    }

    damage_applied = mk_min_int(soldier->health, reduced_damage);
    soldier->health -= damage_applied;
    soldier->suppression += mk_max_int(1, reduced_damage / 10);

    if (soldier->health <= 0) {
        soldier->health = 0;
        soldier->casualty = true;

        if (out_casualty_count != NULL) {
            *out_casualty_count = 1;
        }
    }

    return damage_applied;
}

static void mk_update_unit_movement(mk_unit_t *unit) {
    float dx;
    float dy;
    float distance_squared;
    float speed;
    float step_squared;
    float distance;
    float move_multiplier;

    if (unit == NULL || !unit->has_move_target) {
        return;
    }

    move_multiplier = mk_unit_status_move_multiplier(unit->status);
    if (move_multiplier <= 0.0f) {
        unit->has_move_target = false;
        unit->order = MK_ORDER_RALLY;
        return;
    }

    if (unit->order != MK_ORDER_MOVE && unit->order != MK_ORDER_ASSAULT_MOVE) {
        unit->has_move_target = false;
        return;
    }

    dx = unit->target_position_m.x - unit->position_m.x;
    dy = unit->target_position_m.y - unit->position_m.y;
    distance_squared = dx * dx + dy * dy;
    speed = (unit->move_speed_m_per_tick > 0.0f ? unit->move_speed_m_per_tick : MK_DEFAULT_MOVE_SPEED_M_PER_TICK)
        * move_multiplier;
    step_squared = speed * speed;

    if (distance_squared <= step_squared || distance_squared <= 0.0001f) {
        unit->position_m = unit->target_position_m;
        unit->has_move_target = false;
        unit->order = MK_ORDER_HOLD;
        return;
    }

    distance = sqrtf(distance_squared);
    unit->position_m.x += dx / distance * speed;
    unit->position_m.y += dy / distance * speed;
}

static bool mk_faction_id_exists(const mk_scenario_definition_t *scenario, uint32_t faction_id) {
    size_t index;

    if (faction_id == 0) {
        return true;
    }

    for (index = 0; index < scenario->faction_count; ++index) {
        if (scenario->factions[index].id == faction_id) {
            return true;
        }
    }

    return false;
}

static bool mk_scenario_is_valid(const mk_scenario_definition_t *scenario) {
    size_t index;

    if (scenario == NULL) {
        return false;
    }

    if (scenario->map.width_m <= 0.0f || scenario->map.height_m <= 0.0f) {
        return false;
    }

    if (scenario->faction_count > MK_MAX_FACTIONS
        || scenario->map.terrain_count > MK_MAX_TERRAIN_ZONES
        || scenario->objective_count > MK_MAX_OBJECTIVES
        || scenario->unit_count > MK_MAX_UNITS) {
        return false;
    }

    for (index = 0; index < scenario->faction_count; ++index) {
        size_t other_index;

        if (scenario->factions[index].id == 0) {
            return false;
        }

        for (other_index = index + 1; other_index < scenario->faction_count; ++other_index) {
            if (scenario->factions[index].id == scenario->factions[other_index].id) {
                return false;
            }
        }
    }

    for (index = 0; index < scenario->map.terrain_count; ++index) {
        if (scenario->map.terrain[index].id == 0
            || !mk_rect_fits_map(scenario->map.terrain[index].bounds_m, &scenario->map)
            || scenario->map.terrain[index].movement_cost < 0) {
            return false;
        }
    }

    for (index = 0; index < scenario->objective_count; ++index) {
        if (scenario->objectives[index].id == 0
            || scenario->objectives[index].radius_m <= 0.0f
            || !mk_position_fits_map(scenario->objectives[index].position_m, &scenario->map)) {
            return false;
        }
    }

    for (index = 0; index < scenario->unit_count; ++index) {
        const mk_unit_t *unit = &scenario->units[index];

        if (unit->id == 0
            || unit->soldier_count > MK_MAX_SOLDIERS_PER_UNIT
            || !mk_position_fits_map(unit->position_m, &scenario->map)
            || (unit->has_move_target && !mk_position_fits_map(unit->target_position_m, &scenario->map))
            || !mk_faction_id_exists(scenario, unit->faction_id)) {
            return false;
        }
    }

    return true;
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

        mk_update_unit_status(unit);

        if (unit->order == MK_ORDER_RALLY) {
            recovery += 2;
        }

        mk_update_unit_movement(unit);
        unit->suppression = mk_subtract_floor_zero(unit->suppression, recovery);

        for (soldier_index = 0; soldier_index < unit->soldier_count; ++soldier_index) {
            mk_soldier_t *soldier = &unit->soldiers[soldier_index];
            int soldier_recovery = soldier->casualty ? 0 : recovery;
            soldier->suppression = mk_subtract_floor_zero(soldier->suppression, soldier_recovery);
        }

        mk_update_unit_status(unit);
    }
}

mk_result_t mk_game_snapshot(const mk_game_t *game, mk_game_snapshot_t *out_snapshot) {
    if (game == NULL || out_snapshot == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_snapshot, 0, sizeof(*out_snapshot));
    mk_copy_scenario_name(out_snapshot->scenario_name, game->scenario_name);
    out_snapshot->tick = game->tick;
    out_snapshot->rng_state = game->rng_state;
    out_snapshot->selected_unit_id = game->selected_unit_id;
    out_snapshot->map = game->map;
    out_snapshot->faction_count = game->faction_count;
    memcpy(out_snapshot->factions, game->factions, sizeof(game->factions));
    out_snapshot->objective_count = game->objective_count;
    memcpy(out_snapshot->objectives, game->objectives, sizeof(game->objectives));
    out_snapshot->unit_count = game->unit_count;
    memcpy(out_snapshot->units, game->units, sizeof(game->units));

    return MK_OK;
}

mk_result_t mk_game_load_scenario(mk_game_t *game, const mk_scenario_definition_t *scenario) {
    size_t index;

    if (game == NULL || scenario == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!mk_scenario_is_valid(scenario)) {
        return MK_ERROR_INVALID_DATA;
    }

    mk_game_init(game, scenario->seed);
    mk_copy_scenario_name(game->scenario_name, scenario->name);
    game->selected_unit_id = 0;
    game->map = scenario->map;
    game->faction_count = scenario->faction_count;
    memcpy(game->factions, scenario->factions, sizeof(scenario->factions));
    game->objective_count = scenario->objective_count;
    memcpy(game->objectives, scenario->objectives, sizeof(scenario->objectives));
    game->unit_count = scenario->unit_count;
    memcpy(game->units, scenario->units, sizeof(scenario->units));

    for (index = 0; index < game->unit_count; ++index) {
        mk_update_unit_status(&game->units[index]);
    }

    return MK_OK;
}

mk_result_t mk_game_pick_unit_at(const mk_game_t *game, mk_vec2_t position_m, float radius_m, uint32_t *out_unit_id) {
    float pick_radius = radius_m > 0.0f ? radius_m : MK_UNIT_PICK_RADIUS_M;
    float best_distance_squared = pick_radius * pick_radius;
    uint32_t best_unit_id = 0;
    size_t index;

    if (game == NULL || out_unit_id == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    *out_unit_id = 0;

    for (index = 0; index < game->unit_count; ++index) {
        const mk_unit_t *unit = &game->units[index];
        float distance_squared = mk_distance_squared(position_m, unit->position_m);

        if (distance_squared <= best_distance_squared) {
            best_distance_squared = distance_squared;
            best_unit_id = unit->id;
        }
    }

    if (best_unit_id == 0) {
        return MK_ERROR_NOT_FOUND;
    }

    *out_unit_id = best_unit_id;
    return MK_OK;
}

mk_result_t mk_game_select_unit(mk_game_t *game, uint32_t unit_id) {
    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (mk_game_find_unit(game, unit_id) == NULL) {
        return MK_ERROR_NOT_FOUND;
    }

    game->selected_unit_id = unit_id;
    return MK_OK;
}

mk_result_t mk_game_select_unit_at(mk_game_t *game, mk_vec2_t position_m, float radius_m, uint32_t *out_unit_id) {
    uint32_t unit_id = 0;
    mk_result_t result;

    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    result = mk_game_pick_unit_at(game, position_m, radius_m, &unit_id);
    if (result != MK_OK) {
        if (out_unit_id != NULL) {
            *out_unit_id = 0;
        }
        return result;
    }

    result = mk_game_select_unit(game, unit_id);
    if (result == MK_OK && out_unit_id != NULL) {
        *out_unit_id = unit_id;
    }

    return result;
}

mk_result_t mk_game_clear_selection(mk_game_t *game) {
    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    game->selected_unit_id = 0;
    return MK_OK;
}

mk_result_t mk_game_issue_order(mk_game_t *game, uint32_t unit_id, mk_order_t order) {
    mk_unit_t *unit;

    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    unit = mk_game_find_unit(game, unit_id);
    if (unit == NULL) {
        return MK_ERROR_NOT_FOUND;
    }

    unit->order = order;
    if (order != MK_ORDER_MOVE && order != MK_ORDER_ASSAULT_MOVE) {
        unit->has_move_target = false;
    }

    return MK_OK;
}

mk_result_t mk_game_issue_move_order(mk_game_t *game, uint32_t unit_id, mk_vec2_t target_position_m) {
    mk_unit_t *unit;

    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!mk_position_fits_map(target_position_m, &game->map)) {
        return MK_ERROR_INVALID_DATA;
    }

    unit = mk_game_find_unit(game, unit_id);
    if (unit == NULL) {
        return MK_ERROR_NOT_FOUND;
    }

    unit->target_position_m = target_position_m;
    unit->has_move_target = true;
    unit->order = MK_ORDER_MOVE;

    return MK_OK;
}

mk_result_t mk_game_issue_selected_move_order(mk_game_t *game, mk_vec2_t target_position_m) {
    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (game->selected_unit_id == 0) {
        return MK_ERROR_NOT_FOUND;
    }

    return mk_game_issue_move_order(game, game->selected_unit_id, target_position_m);
}

mk_result_t mk_game_trace_line_of_sight(
    const mk_game_t *game,
    mk_vec2_t from_m,
    mk_vec2_t to_m,
    mk_line_of_sight_t *out_line_of_sight
) {
    mk_line_of_sight_t line_of_sight;

    if (game == NULL || out_line_of_sight == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!mk_position_fits_map(from_m, &game->map) || !mk_position_fits_map(to_m, &game->map)) {
        return MK_ERROR_INVALID_DATA;
    }

    memset(&line_of_sight, 0, sizeof(line_of_sight));
    line_of_sight.visible = true;
    line_of_sight.distance_m = mk_distance(from_m, to_m);
    line_of_sight.cover_terrain_kind = MK_TERRAIN_OPEN;

    mk_apply_target_cover(&game->map, to_m, &line_of_sight);
    mk_apply_blocking_terrain(&game->map, from_m, to_m, &line_of_sight);

    *out_line_of_sight = line_of_sight;
    return MK_OK;
}

mk_result_t mk_game_unit_line_of_sight(
    const mk_game_t *game,
    uint32_t observer_unit_id,
    uint32_t target_unit_id,
    mk_line_of_sight_t *out_line_of_sight
) {
    const mk_unit_t *observer;
    const mk_unit_t *target;

    if (game == NULL || out_line_of_sight == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    observer = mk_game_find_unit_const(game, observer_unit_id);
    target = mk_game_find_unit_const(game, target_unit_id);

    if (observer == NULL || target == NULL) {
        return MK_ERROR_NOT_FOUND;
    }

    return mk_game_trace_line_of_sight(game, observer->position_m, target->position_m, out_line_of_sight);
}

mk_result_t mk_game_unit_fire(
    mk_game_t *game,
    uint32_t attacker_unit_id,
    uint32_t target_unit_id,
    mk_fire_result_t *out_fire_result
) {
    mk_unit_t *attacker;
    mk_unit_t *target;
    mk_fire_result_t fire_result;
    mk_result_t los_result;
    size_t soldier_index;

    if (game == NULL || out_fire_result == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(&fire_result, 0, sizeof(fire_result));
    fire_result.attacker_unit_id = attacker_unit_id;
    fire_result.target_unit_id = target_unit_id;

    attacker = mk_game_find_unit(game, attacker_unit_id);
    target = mk_game_find_unit(game, target_unit_id);
    if (attacker == NULL || target == NULL) {
        *out_fire_result = fire_result;
        return MK_ERROR_NOT_FOUND;
    }

    mk_update_unit_status(attacker);
    mk_update_unit_status(target);
    fire_result.attacker_status = attacker->status;
    fire_result.target_status_before = target->status;
    fire_result.target_status_after = target->status;

    if (attacker->status == MK_UNIT_BROKEN || target->status == MK_UNIT_BROKEN) {
        *out_fire_result = fire_result;
        return MK_OK;
    }

    los_result = mk_game_unit_line_of_sight(game, attacker_unit_id, target_unit_id, &fire_result.line_of_sight);
    if (los_result != MK_OK) {
        *out_fire_result = fire_result;
        return los_result;
    }

    if (!fire_result.line_of_sight.visible) {
        fire_result.resolved = false;
        *out_fire_result = fire_result;
        return MK_OK;
    }

    attacker->order = MK_ORDER_FIRE;
    attacker->has_move_target = false;

    for (soldier_index = 0; soldier_index < attacker->soldier_count; ++soldier_index) {
        mk_soldier_t *soldier = &attacker->soldiers[soldier_index];
        int shots_available;
        int shot_index;

        if (soldier->casualty
            || soldier->ammo <= 0
            || soldier->weapon.shots_per_action <= 0
            || soldier->weapon.effective_range_m <= 0
            || fire_result.line_of_sight.distance_m > (float)soldier->weapon.effective_range_m) {
            continue;
        }

        fire_result.eligible_shooters += 1;
        shots_available = mk_min_int(soldier->ammo, soldier->weapon.shots_per_action);
        soldier->ammo -= shots_available;
        fire_result.ammo_spent += shots_available;

        for (shot_index = 0; shot_index < shots_available; ++shot_index) {
            int hit_chance = mk_weapon_hit_chance(attacker, &soldier->weapon, &fire_result.line_of_sight);
            int roll = (int)(mk_random_u32(game) % 100U);
            int shot_suppression = mk_max_int(1, soldier->weapon.suppression / 2);

            fire_result.shots_fired += 1;
            fire_result.suppression_added += shot_suppression;
            target->suppression += shot_suppression;

            if (roll < hit_chance) {
                int casualty_count = 0;
                int damage = mk_apply_fire_damage(
                    target,
                    soldier->weapon.damage,
                    fire_result.line_of_sight.cover,
                    &casualty_count
                );

                fire_result.hits += 1;
                fire_result.damage_applied += damage;
                fire_result.casualties += casualty_count;
                fire_result.suppression_added += soldier->weapon.suppression;
                target->suppression += soldier->weapon.suppression;
                mk_update_unit_status(target);
            }
        }
    }

    mk_update_unit_status(attacker);
    mk_update_unit_status(target);
    fire_result.attacker_status = attacker->status;
    fire_result.target_status_after = target->status;
    fire_result.resolved = fire_result.shots_fired > 0;
    *out_fire_result = fire_result;

    return MK_OK;
}

mk_result_t mk_game_selected_unit_fire(
    mk_game_t *game,
    uint32_t target_unit_id,
    mk_fire_result_t *out_fire_result
) {
    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (game->selected_unit_id == 0) {
        return MK_ERROR_NOT_FOUND;
    }

    return mk_game_unit_fire(game, game->selected_unit_id, target_unit_id, out_fire_result);
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
    unit.target_position_m = position_m;
    unit.facing_degrees = 0.0f;
    unit.cohesion_radius_m = 8.0f;
    unit.move_speed_m_per_tick = MK_DEFAULT_MOVE_SPEED_M_PER_TICK;
    unit.status = MK_UNIT_READY;

    return unit;
}

mk_color_t mk_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    mk_color_t color;

    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;

    return color;
}

mk_faction_t mk_make_faction(const char *name, mk_side_t side, mk_color_t color) {
    mk_faction_t faction;

    memset(&faction, 0, sizeof(faction));
    mk_copy_name(faction.name, name);
    faction.side = side;
    faction.color = color;

    return faction;
}

mk_map_t mk_make_map(const char *name, float width_m, float height_m) {
    mk_map_t map;

    memset(&map, 0, sizeof(map));
    mk_copy_name(map.name, name);
    map.width_m = width_m;
    map.height_m = height_m;

    return map;
}

mk_terrain_zone_t mk_make_terrain_zone(
    const char *name,
    mk_terrain_kind_t kind,
    mk_rect_t bounds_m,
    int cover,
    int movement_cost,
    bool blocks_line_of_sight
) {
    mk_terrain_zone_t terrain;

    memset(&terrain, 0, sizeof(terrain));
    mk_copy_name(terrain.name, name);
    terrain.kind = kind;
    terrain.bounds_m = bounds_m;
    terrain.cover = cover;
    terrain.movement_cost = movement_cost;
    terrain.blocks_line_of_sight = blocks_line_of_sight;

    return terrain;
}

mk_objective_t mk_make_objective(
    const char *name,
    mk_objective_kind_t kind,
    mk_vec2_t position_m,
    float radius_m,
    int value
) {
    mk_objective_t objective;

    memset(&objective, 0, sizeof(objective));
    mk_copy_name(objective.name, name);
    objective.kind = kind;
    objective.position_m = position_m;
    objective.radius_m = radius_m;
    objective.controlling_side = MK_SIDE_NEUTRAL;
    objective.value = value;

    return objective;
}

mk_result_t mk_map_add_terrain(mk_map_t *map, const mk_terrain_zone_t *terrain, uint32_t *out_terrain_id) {
    mk_terrain_zone_t copy;

    if (map == NULL || terrain == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (map->terrain_count >= MK_MAX_TERRAIN_ZONES) {
        return MK_ERROR_CAPACITY;
    }

    copy = *terrain;
    copy.id = (uint32_t)(map->terrain_count + 1);

    map->terrain[map->terrain_count] = copy;
    map->terrain_count += 1;

    if (out_terrain_id != NULL) {
        *out_terrain_id = copy.id;
    }

    return MK_OK;
}

mk_result_t mk_scenario_add_faction(mk_scenario_definition_t *scenario, const mk_faction_t *faction, uint32_t *out_faction_id) {
    mk_faction_t copy;

    if (scenario == NULL || faction == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (scenario->faction_count >= MK_MAX_FACTIONS) {
        return MK_ERROR_CAPACITY;
    }

    copy = *faction;
    copy.id = (uint32_t)(scenario->faction_count + 1);

    scenario->factions[scenario->faction_count] = copy;
    scenario->faction_count += 1;

    if (out_faction_id != NULL) {
        *out_faction_id = copy.id;
    }

    return MK_OK;
}

mk_result_t mk_scenario_add_objective(mk_scenario_definition_t *scenario, const mk_objective_t *objective, uint32_t *out_objective_id) {
    mk_objective_t copy;

    if (scenario == NULL || objective == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (scenario->objective_count >= MK_MAX_OBJECTIVES) {
        return MK_ERROR_CAPACITY;
    }

    copy = *objective;
    copy.id = (uint32_t)(scenario->objective_count + 1);

    scenario->objectives[scenario->objective_count] = copy;
    scenario->objective_count += 1;

    if (out_objective_id != NULL) {
        *out_objective_id = copy.id;
    }

    return MK_OK;
}

mk_result_t mk_scenario_add_unit(mk_scenario_definition_t *scenario, const mk_unit_t *unit, uint32_t *out_unit_id) {
    mk_unit_t copy;

    if (scenario == NULL || unit == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (scenario->unit_count >= MK_MAX_UNITS) {
        return MK_ERROR_CAPACITY;
    }

    copy = *unit;
    copy.id = (uint32_t)(scenario->unit_count + 1);
    mk_update_unit_status(&copy);

    scenario->units[scenario->unit_count] = copy;
    scenario->unit_count += 1;

    if (out_unit_id != NULL) {
        *out_unit_id = copy.id;
    }

    return MK_OK;
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
    mk_update_unit_status(&copy);

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
