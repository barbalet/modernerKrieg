#include "mk_core.h"

#include <math.h>
#include <stdio.h>
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

static bool mk_map_tile_coordinate_is_valid(const mk_map_t *map, mk_ivec2_t coordinate) {
    if (map == NULL || map->tile_columns <= 0 || map->tile_rows <= 0) {
        return false;
    }

    return coordinate.x >= 0
        && coordinate.y >= 0
        && coordinate.x < map->tile_columns
        && coordinate.y < map->tile_rows;
}

static size_t mk_map_tile_index(const mk_map_t *map, mk_ivec2_t coordinate) {
    return (size_t)coordinate.y * (size_t)map->tile_columns + (size_t)coordinate.x;
}

static float mk_distance_squared(mk_vec2_t first, mk_vec2_t second) {
    float dx = first.x - second.x;
    float dy = first.y - second.y;

    return dx * dx + dy * dy;
}

static float mk_distance(mk_vec2_t first, mk_vec2_t second) {
    return sqrtf(mk_distance_squared(first, second));
}

static float mk_distance_point_to_segment(mk_vec2_t point, mk_vec2_t segment_start, mk_vec2_t segment_end) {
    float dx = segment_end.x - segment_start.x;
    float dy = segment_end.y - segment_start.y;
    float length_squared = dx * dx + dy * dy;
    float t;
    mk_vec2_t closest;

    if (length_squared <= 0.0001f) {
        return mk_distance(point, segment_start);
    }

    t = ((point.x - segment_start.x) * dx + (point.y - segment_start.y) * dy) / length_squared;
    if (t < 0.0f) {
        t = 0.0f;
    } else if (t > 1.0f) {
        t = 1.0f;
    }
    closest.x = segment_start.x + t * dx;
    closest.y = segment_start.y + t * dy;

    return mk_distance(point, closest);
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

static mk_contact_report_t *mk_game_add_contact_report(mk_game_t *game, mk_contact_report_kind_t kind) {
    mk_contact_report_t *report;

    if (game == NULL || game->contact_report_count >= MK_MAX_CONTACT_REPORTS) {
        return NULL;
    }

    report = &game->contact_reports[game->contact_report_count];
    memset(report, 0, sizeof(*report));
    report->id = (uint32_t)(game->contact_report_count + 1);
    report->tick = game->tick;
    report->kind = kind;
    game->contact_report_count += 1;

    return report;
}

static void mk_game_record_reveal_contact(mk_game_t *game, const mk_unit_t *observer, const mk_unit_t *hidden_unit) {
    mk_contact_report_t *report = mk_game_add_contact_report(game, MK_CONTACT_REPORT_REVEAL);

    if (report == NULL || hidden_unit == NULL) {
        return;
    }

    report->attacker_unit_id = observer != NULL ? observer->id : 0;
    report->target_unit_id = hidden_unit->id;
    report->side = hidden_unit->side;
    report->position_m = hidden_unit->position_m;
    report->target_position_m = hidden_unit->position_m;
    report->visible = true;
    report->resolved = true;
}

static void mk_game_reveal_unit(mk_game_t *game, mk_unit_t *unit, const mk_unit_t *observer) {
    if (game == NULL || unit == NULL || !unit->hidden || unit->revealed) {
        return;
    }

    unit->revealed = true;
    mk_game_record_reveal_contact(game, observer, unit);
}

static int mk_game_apply_civilian_fire_risk(
    mk_game_t *game,
    const mk_unit_t *attacker,
    const mk_unit_t *target,
    const mk_fire_result_t *fire_result
) {
    int total_risk_added = 0;
    size_t index;

    if (game == NULL || attacker == NULL || target == NULL || fire_result == NULL || fire_result->shots_fired <= 0) {
        return 0;
    }

    for (index = 0; index < game->civilian_count; ++index) {
        mk_civilian_t *civilian = &game->civilians[index];
        float distance_to_lane;
        int risk_added = 0;

        if (!civilian->protected_noncombatant) {
            continue;
        }

        distance_to_lane = mk_distance_point_to_segment(civilian->position_m, attacker->position_m, target->position_m);
        if (distance_to_lane <= 30.0f) {
            risk_added += 3;
        } else if (distance_to_lane <= 60.0f) {
            risk_added += 1;
        }

        if (mk_distance(civilian->position_m, target->position_m) <= 70.0f) {
            risk_added += 2;
        }

        if (risk_added > 0) {
            mk_contact_report_t *report;

            civilian->risk = mk_clamp_int(civilian->risk + risk_added, 0, 100);
            civilian->stress = mk_clamp_int(civilian->stress + risk_added, 0, 100);
            if (civilian->state == MK_CIVILIAN_SHELTERING && civilian->risk >= 6) {
                civilian->state = MK_CIVILIAN_FROZEN;
            }

            total_risk_added += risk_added;
            report = mk_game_add_contact_report(game, MK_CONTACT_REPORT_CIVILIAN_RISK);
            if (report != NULL) {
                report->attacker_unit_id = attacker->id;
                report->target_unit_id = target->id;
                report->civilian_id = civilian->id;
                report->side = MK_SIDE_CIVILIAN;
                report->position_m = civilian->position_m;
                report->target_position_m = target->position_m;
                report->civilian_risk_added = risk_added;
                report->visible = true;
                report->resolved = true;
            }
        }
    }

    return total_risk_added;
}

static int mk_unit_casualty_count(const mk_unit_t *unit) {
    int casualty_count = 0;
    size_t index;

    if (unit == NULL) {
        return 0;
    }

    for (index = 0; index < unit->soldier_count; ++index) {
        if (unit->soldiers[index].casualty) {
            casualty_count += 1;
        }
    }

    return casualty_count;
}

static int mk_game_side_casualties(const mk_game_t *game, mk_side_t side) {
    int casualty_count = 0;
    size_t index;

    if (game == NULL) {
        return 0;
    }

    for (index = 0; index < game->unit_count; ++index) {
        const mk_unit_t *unit = &game->units[index];

        if (unit->side == side) {
            casualty_count += mk_unit_casualty_count(unit);
        }
    }

    return casualty_count;
}

static int mk_game_total_civilian_risk(const mk_game_t *game) {
    int civilian_risk = 0;
    size_t index;

    if (game == NULL) {
        return 0;
    }

    for (index = 0; index < game->civilian_count; ++index) {
        civilian_risk += game->civilians[index].risk;
    }

    return civilian_risk;
}

static bool mk_objective_can_be_controlled(const mk_objective_t *objective) {
    return objective != NULL && objective->kind != MK_OBJECTIVE_PROTECT_CIVILIANS;
}

static void mk_game_objective_presence(
    const mk_game_t *game,
    const mk_objective_t *objective,
    bool *out_player_present,
    bool *out_opfor_present
) {
    bool player_present = false;
    bool opfor_present = false;
    size_t index;

    if (game != NULL && objective != NULL) {
        for (index = 0; index < game->unit_count; ++index) {
            const mk_unit_t *unit = &game->units[index];

            if (unit->side != MK_SIDE_PLAYER && unit->side != MK_SIDE_OPFOR) {
                continue;
            }

            if (unit->status == MK_UNIT_BROKEN || mk_unit_live_soldier_count(unit) == 0) {
                continue;
            }

            if (mk_distance(unit->position_m, objective->position_m) > objective->radius_m) {
                continue;
            }

            if (unit->side == MK_SIDE_PLAYER) {
                player_present = true;
            } else if (unit->side == MK_SIDE_OPFOR) {
                opfor_present = true;
            }
        }
    }

    if (out_player_present != NULL) {
        *out_player_present = player_present;
    }

    if (out_opfor_present != NULL) {
        *out_opfor_present = opfor_present;
    }
}

static const char *mk_outcome_summary_name(mk_outcome_t outcome) {
    switch (outcome) {
        case MK_OUTCOME_PLAYER_SUCCESS:
            return "success";
        case MK_OUTCOME_PLAYER_PARTIAL:
            return "partial";
        case MK_OUTCOME_PLAYER_FAILURE:
            return "failure";
        case MK_OUTCOME_IN_PROGRESS:
        default:
            return "in_progress";
    }
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

    if (unit->order != MK_ORDER_MOVE && unit->order != MK_ORDER_ASSAULT_MOVE && unit->order != MK_ORDER_WITHDRAW) {
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
        unit->order = unit->order == MK_ORDER_WITHDRAW ? MK_ORDER_RALLY : MK_ORDER_HOLD;
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

static bool mk_controller_id_exists(const mk_scenario_definition_t *scenario, uint32_t controller_id) {
    size_t index;

    if (controller_id == 0) {
        return true;
    }

    for (index = 0; index < scenario->controller_count; ++index) {
        if (scenario->controllers[index].id == controller_id) {
            return true;
        }
    }

    return false;
}

static bool mk_force_id_exists(const mk_scenario_definition_t *scenario, uint32_t force_id) {
    size_t index;

    if (force_id == 0) {
        return true;
    }

    for (index = 0; index < scenario->force_count; ++index) {
        if (scenario->forces[index].id == force_id) {
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

    if (scenario->controller_count > MK_MAX_CONTROLLERS
        || scenario->faction_count > MK_MAX_FACTIONS
        || scenario->force_count > MK_MAX_FORCES
        || scenario->map.terrain_count > MK_MAX_TERRAIN_ZONES
        || scenario->map.tile_count > MK_MAX_MAP_TILES
        || scenario->objective_count > MK_MAX_OBJECTIVES
        || scenario->civilian_count > MK_MAX_CIVILIANS
        || scenario->unit_count > MK_MAX_UNITS) {
        return false;
    }

    if (scenario->map.tile_count > 0) {
        size_t expected_tile_count;

        if (scenario->map.tile_columns <= 0
            || scenario->map.tile_rows <= 0
            || scenario->map.tile_width_m <= 0.0f
            || scenario->map.tile_height_m <= 0.0f) {
            return false;
        }

        expected_tile_count = (size_t)scenario->map.tile_columns * (size_t)scenario->map.tile_rows;
        if (expected_tile_count != scenario->map.tile_count || expected_tile_count > MK_MAX_MAP_TILES) {
            return false;
        }
    }

    for (index = 0; index < scenario->controller_count; ++index) {
        size_t other_index;

        if (scenario->controllers[index].id == 0 || scenario->controllers[index].kind == MK_CONTROLLER_NONE) {
            return false;
        }

        for (other_index = index + 1; other_index < scenario->controller_count; ++other_index) {
            if (scenario->controllers[index].id == scenario->controllers[other_index].id) {
                return false;
            }
        }
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

    for (index = 0; index < scenario->force_count; ++index) {
        size_t other_index;

        if (scenario->forces[index].id == 0
            || !mk_faction_id_exists(scenario, scenario->forces[index].faction_id)
            || !mk_controller_id_exists(scenario, scenario->forces[index].controller_id)) {
            return false;
        }

        for (other_index = index + 1; other_index < scenario->force_count; ++other_index) {
            if (scenario->forces[index].id == scenario->forces[other_index].id) {
                return false;
            }
        }
    }

    for (index = 0; index < scenario->map.tile_count; ++index) {
        const mk_map_tile_t *tile = &scenario->map.tiles[index];

        if (tile->id == 0
            || !mk_map_tile_coordinate_is_valid(&scenario->map, tile->coordinate)
            || mk_map_tile_index(&scenario->map, tile->coordinate) != index
            || tile->movement_cost < 0) {
            return false;
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

    for (index = 0; index < scenario->civilian_count; ++index) {
        if (scenario->civilians[index].id == 0
            || !mk_faction_id_exists(scenario, scenario->civilians[index].faction_id)
            || !mk_position_fits_map(scenario->civilians[index].position_m, &scenario->map)) {
            return false;
        }
    }

    for (index = 0; index < scenario->unit_count; ++index) {
        const mk_unit_t *unit = &scenario->units[index];

        if (unit->id == 0
            || unit->soldier_count > MK_MAX_SOLDIERS_PER_UNIT
            || !mk_position_fits_map(unit->position_m, &scenario->map)
            || (unit->has_move_target && !mk_position_fits_map(unit->target_position_m, &scenario->map))
            || !mk_faction_id_exists(scenario, unit->faction_id)
            || !mk_force_id_exists(scenario, unit->force_id)
            || !mk_controller_id_exists(scenario, unit->controller_id)) {
            return false;
        }
    }

    return true;
}

const char *mk_version(void) {
    return "0.1.0";
}

mk_vec2_t mk_vec2(float x, float y) {
    mk_vec2_t value;

    value.x = x;
    value.y = y;

    return value;
}

mk_ivec2_t mk_ivec2(int x, int y) {
    mk_ivec2_t value;

    value.x = x;
    value.y = y;

    return value;
}

mk_rect_t mk_rect(float x, float y, float width, float height) {
    mk_rect_t value;

    value.x = x;
    value.y = y;
    value.width = width;
    value.height = height;

    return value;
}

float mk_clamp_f32(float value, float minimum, float maximum) {
    if (minimum > maximum) {
        float swap = minimum;
        minimum = maximum;
        maximum = swap;
    }

    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

int mk_clamp_i32(int value, int minimum, int maximum) {
    if (minimum > maximum) {
        int swap = minimum;
        minimum = maximum;
        maximum = swap;
    }

    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

bool mk_rect_contains_point(mk_rect_t rect, mk_vec2_t point) {
    return mk_rect_is_valid(rect) && mk_point_in_rect(point, rect);
}

float mk_vec2_distance(mk_vec2_t first, mk_vec2_t second) {
    return mk_distance(first, second);
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

    (void)mk_game_update_hidden_contacts(game);
    (void)mk_game_update_civilian_risk(game);
    (void)mk_game_update_objective_control(game);
}

mk_result_t mk_game_update_hidden_contacts(mk_game_t *game) {
    size_t hidden_index;

    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    for (hidden_index = 0; hidden_index < game->unit_count; ++hidden_index) {
        mk_unit_t *hidden_unit = &game->units[hidden_index];
        size_t observer_index;

        if (!hidden_unit->hidden || hidden_unit->revealed || hidden_unit->side == MK_SIDE_CIVILIAN) {
            continue;
        }

        for (observer_index = 0; observer_index < game->unit_count; ++observer_index) {
            const mk_unit_t *observer = &game->units[observer_index];
            mk_line_of_sight_t line_of_sight;
            float reveal_distance;

            if (observer->id == hidden_unit->id
                || observer->side == hidden_unit->side
                || observer->side == MK_SIDE_CIVILIAN
                || observer->status == MK_UNIT_BROKEN) {
                continue;
            }

            reveal_distance = 120.0f - (float)hidden_unit->concealment;
            if (reveal_distance < 40.0f) {
                reveal_distance = 40.0f;
            }

            if (mk_distance(observer->position_m, hidden_unit->position_m) > reveal_distance) {
                continue;
            }

            if (mk_game_unit_line_of_sight(game, observer->id, hidden_unit->id, &line_of_sight) == MK_OK
                && line_of_sight.visible) {
                mk_game_reveal_unit(game, hidden_unit, observer);
                break;
            }
        }
    }

    return MK_OK;
}

mk_result_t mk_game_update_civilian_risk(mk_game_t *game) {
    size_t civilian_index;

    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    for (civilian_index = 0; civilian_index < game->civilian_count; ++civilian_index) {
        mk_civilian_t *civilian = &game->civilians[civilian_index];
        size_t unit_index;

        if (!civilian->protected_noncombatant) {
            continue;
        }

        for (unit_index = 0; unit_index < game->unit_count; ++unit_index) {
            const mk_unit_t *unit = &game->units[unit_index];
            float distance;

            if (unit->side == MK_SIDE_CIVILIAN || unit->status == MK_UNIT_BROKEN) {
                continue;
            }

            distance = mk_distance(civilian->position_m, unit->position_m);
            if (distance <= 24.0f) {
                mk_contact_report_t *report;

                civilian->risk = mk_clamp_int(civilian->risk + 1, 0, 100);
                civilian->stress = mk_clamp_int(civilian->stress + 1, 0, 100);
                if (civilian->state == MK_CIVILIAN_SHELTERING) {
                    civilian->state = MK_CIVILIAN_FROZEN;
                }

                report = mk_game_add_contact_report(game, MK_CONTACT_REPORT_CIVILIAN_RISK);
                if (report != NULL) {
                    report->attacker_unit_id = unit->id;
                    report->civilian_id = civilian->id;
                    report->side = MK_SIDE_CIVILIAN;
                    report->position_m = civilian->position_m;
                    report->target_position_m = unit->position_m;
                    report->civilian_risk_added = 1;
                    report->visible = true;
                    report->resolved = true;
                }
                break;
            }
        }
    }

    return MK_OK;
}

mk_result_t mk_game_update_objective_control(mk_game_t *game) {
    size_t objective_index;

    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    for (objective_index = 0; objective_index < game->objective_count; ++objective_index) {
        mk_objective_t *objective = &game->objectives[objective_index];
        bool player_present = false;
        bool opfor_present = false;

        if (!mk_objective_can_be_controlled(objective)) {
            continue;
        }

        mk_game_objective_presence(game, objective, &player_present, &opfor_present);

        if (player_present && opfor_present) {
            objective->controlling_side = MK_SIDE_NEUTRAL;
        } else if (player_present) {
            objective->controlling_side = MK_SIDE_PLAYER;
        } else if (opfor_present) {
            objective->controlling_side = MK_SIDE_OPFOR;
        }
    }

    return MK_OK;
}

mk_result_t mk_game_score(const mk_game_t *game, mk_score_t *out_score) {
    mk_score_t score;
    size_t objective_index;

    if (game == NULL || out_score == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(&score, 0, sizeof(score));

    for (objective_index = 0; objective_index < game->objective_count; ++objective_index) {
        const mk_objective_t *objective = &game->objectives[objective_index];
        bool player_present = false;
        bool opfor_present = false;
        int objective_value;

        if (!mk_objective_can_be_controlled(objective)) {
            continue;
        }

        mk_game_objective_presence(game, objective, &player_present, &opfor_present);
        if (player_present && opfor_present) {
            score.contested_objectives += 1;
        }

        objective_value = mk_max_int(0, objective->value);
        if (objective->controlling_side == MK_SIDE_PLAYER && !(player_present && opfor_present)) {
            score.controlled_objectives += 1;
            score.objective_points += objective_value * 100;
        }
    }

    score.civilian_risk = mk_game_total_civilian_risk(game);
    score.player_casualties = mk_game_side_casualties(game, MK_SIDE_PLAYER);
    score.opfor_casualties = mk_game_side_casualties(game, MK_SIDE_OPFOR);
    score.civilian_casualties = mk_game_side_casualties(game, MK_SIDE_CIVILIAN);
    score.civilian_risk_penalty = score.civilian_risk * 10;
    score.casualty_penalty = score.player_casualties * 50 + score.civilian_casualties * 100;
    score.time_penalty = (int)game->tick;
    score.total_score = score.objective_points
        - score.civilian_risk_penalty
        - score.casualty_penalty
        - score.time_penalty;

    if (score.controlled_objectives == 0) {
        score.outcome = MK_OUTCOME_PLAYER_FAILURE;
    } else if (score.player_casualties > 0 || score.civilian_casualties > 0 || score.civilian_risk >= 25) {
        score.outcome = MK_OUTCOME_PLAYER_PARTIAL;
    } else {
        score.outcome = MK_OUTCOME_PLAYER_SUCCESS;
    }

    *out_score = score;
    return MK_OK;
}

mk_result_t mk_game_after_action_report(const mk_game_t *game, mk_after_action_report_t *out_report) {
    mk_after_action_report_t report;
    mk_result_t result;

    if (game == NULL || out_report == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(&report, 0, sizeof(report));
    result = mk_game_score(game, &report.score);
    if (result != MK_OK) {
        return result;
    }

    (void)snprintf(
        report.summary,
        sizeof(report.summary),
        "outcome=%s score=%d objectives=%u contested=%u civilian_risk=%d casualties(player=%d,opfor=%d,civilian=%d) ticks=%u",
        mk_outcome_summary_name(report.score.outcome),
        report.score.total_score,
        (unsigned)report.score.controlled_objectives,
        (unsigned)report.score.contested_objectives,
        report.score.civilian_risk,
        report.score.player_casualties,
        report.score.opfor_casualties,
        report.score.civilian_casualties,
        game->tick
    );

    *out_report = report;
    return MK_OK;
}

mk_result_t mk_game_run_fixed_steps(
    mk_game_t *game,
    uint32_t step_count,
    mk_step_observer_fn observer,
    void *user_data
) {
    uint32_t step_index;

    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    for (step_index = 0; step_index < step_count; ++step_index) {
        mk_result_t observer_result;

        mk_game_step(game);

        if (observer == NULL) {
            continue;
        }

        observer_result = observer(game, user_data);
        if (observer_result != MK_OK) {
            return observer_result;
        }
    }

    return MK_OK;
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
    out_snapshot->controller_count = game->controller_count;
    memcpy(out_snapshot->controllers, game->controllers, sizeof(game->controllers));
    out_snapshot->faction_count = game->faction_count;
    memcpy(out_snapshot->factions, game->factions, sizeof(game->factions));
    out_snapshot->force_count = game->force_count;
    memcpy(out_snapshot->forces, game->forces, sizeof(game->forces));
    out_snapshot->objective_count = game->objective_count;
    memcpy(out_snapshot->objectives, game->objectives, sizeof(game->objectives));
    out_snapshot->civilian_count = game->civilian_count;
    memcpy(out_snapshot->civilians, game->civilians, sizeof(game->civilians));
    out_snapshot->unit_count = game->unit_count;
    memcpy(out_snapshot->units, game->units, sizeof(game->units));
    out_snapshot->contact_report_count = game->contact_report_count;
    memcpy(out_snapshot->contact_reports, game->contact_reports, sizeof(game->contact_reports));

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
    game->controller_count = scenario->controller_count;
    memcpy(game->controllers, scenario->controllers, sizeof(scenario->controllers));
    game->faction_count = scenario->faction_count;
    memcpy(game->factions, scenario->factions, sizeof(scenario->factions));
    game->force_count = scenario->force_count;
    memcpy(game->forces, scenario->forces, sizeof(scenario->forces));
    game->objective_count = scenario->objective_count;
    memcpy(game->objectives, scenario->objectives, sizeof(scenario->objectives));
    game->civilian_count = scenario->civilian_count;
    memcpy(game->civilians, scenario->civilians, sizeof(scenario->civilians));
    game->unit_count = scenario->unit_count;
    memcpy(game->units, scenario->units, sizeof(scenario->units));
    game->contact_report_count = 0;
    memset(game->contact_reports, 0, sizeof(game->contact_reports));

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

    mk_game_reveal_unit(game, attacker, target);
    mk_game_reveal_unit(game, target, attacker);

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
    fire_result.civilian_risk_added = mk_game_apply_civilian_fire_risk(game, attacker, target, &fire_result);
    if (fire_result.resolved) {
        mk_contact_report_t *report = mk_game_add_contact_report(game, MK_CONTACT_REPORT_FIRE);

        if (report != NULL) {
            report->attacker_unit_id = attacker->id;
            report->target_unit_id = target->id;
            report->side = attacker->side;
            report->position_m = attacker->position_m;
            report->target_position_m = target->position_m;
            report->shots_fired = fire_result.shots_fired;
            report->hits = fire_result.hits;
            report->suppression_added = fire_result.suppression_added;
            report->casualties = fire_result.casualties;
            report->civilian_risk_added = fire_result.civilian_risk_added;
            report->visible = fire_result.line_of_sight.visible;
            report->resolved = fire_result.resolved;
            fire_result.contact_report_id = report->id;
        }
    }
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
    weapon.fire_mode = shots_per_action > 1 ? MK_FIRE_MODE_BURST : MK_FIRE_MODE_SINGLE;
    weapon.ammo_kind = shots_per_action > 0 ? MK_AMMO_SMALL_ARMS : MK_AMMO_NONE;
    weapon.effective_range_m = effective_range_m;
    weapon.shots_per_action = shots_per_action;
    weapon.damage = damage;
    weapon.suppression = suppression;
    weapon.magazine_capacity = shots_per_action > 0 ? 30 : 0;
    weapon.reload_ticks = shots_per_action > 0 ? 2 : 0;
    weapon.cooldown_ticks = shots_per_action > 0 ? 1 : 0;

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
    soldier.max_health = 100;
    soldier.ammo = 120;
    soldier.ammo_capacity = 120;
    soldier.stance = MK_STANCE_STANDING;
    soldier.wound_state = MK_WOUND_NONE;
    soldier.can_move = true;
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
    unit.order_source = MK_ORDER_SOURCE_NONE;
    unit.position_m = position_m;
    unit.target_position_m = position_m;
    unit.facing_degrees = 0.0f;
    unit.cohesion_radius_m = 8.0f;
    unit.move_speed_m_per_tick = MK_DEFAULT_MOVE_SPEED_M_PER_TICK;
    unit.morale = 100;
    unit.status = MK_UNIT_READY;
    unit.communications_up = true;
    unit.cover_posture = false;

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

mk_controller_slot_t mk_make_controller_slot(const char *name, mk_side_t side, mk_controller_kind_t kind) {
    mk_controller_slot_t controller;

    memset(&controller, 0, sizeof(controller));
    mk_copy_name(controller.name, name);
    controller.side = side;
    controller.kind = kind;

    return controller;
}

mk_faction_t mk_make_faction(const char *name, mk_side_t side, mk_color_t color) {
    mk_faction_t faction;

    memset(&faction, 0, sizeof(faction));
    mk_copy_name(faction.name, name);
    faction.side = side;
    faction.color = color;

    return faction;
}

mk_command_identity_t mk_make_command_identity(const char *name, const char *callsign, mk_side_t side) {
    mk_command_identity_t command;

    memset(&command, 0, sizeof(command));
    mk_copy_name(command.name, name);
    mk_copy_name(command.callsign, callsign);
    command.side = side;

    return command;
}

mk_force_t mk_make_force(
    const char *name,
    mk_side_t side,
    uint32_t faction_id,
    uint32_t controller_id
) {
    mk_force_t force;

    memset(&force, 0, sizeof(force));
    mk_copy_name(force.name, name);
    force.side = side;
    force.faction_id = faction_id;
    force.controller_id = controller_id;
    force.command = mk_make_command_identity(name, name, side);

    return force;
}

mk_map_t mk_make_map(const char *name, float width_m, float height_m) {
    mk_map_t map;

    memset(&map, 0, sizeof(map));
    mk_copy_name(map.name, name);
    map.width_m = width_m;
    map.height_m = height_m;

    return map;
}

mk_map_tile_t mk_make_map_tile(
    mk_ivec2_t coordinate,
    mk_terrain_kind_t kind,
    int elevation,
    int cover,
    int movement_cost,
    bool blocks_line_of_sight,
    bool blocks_movement
) {
    mk_map_tile_t tile;

    memset(&tile, 0, sizeof(tile));
    tile.coordinate = coordinate;
    tile.kind = kind;
    tile.elevation = elevation;
    tile.cover = cover;
    tile.movement_cost = movement_cost;
    tile.blocks_line_of_sight = blocks_line_of_sight;
    tile.blocks_movement = blocks_movement;

    return tile;
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

mk_civilian_t mk_make_civilian(const char *name, uint32_t faction_id, mk_vec2_t position_m) {
    mk_civilian_t civilian;

    memset(&civilian, 0, sizeof(civilian));
    mk_copy_name(civilian.name, name);
    civilian.faction_id = faction_id;
    civilian.position_m = position_m;
    civilian.state = MK_CIVILIAN_SHELTERING;
    civilian.protected_noncombatant = true;

    return civilian;
}

mk_result_t mk_map_configure_tiles(
    mk_map_t *map,
    int columns,
    int rows,
    float tile_width_m,
    float tile_height_m,
    mk_terrain_kind_t default_kind
) {
    size_t tile_count;
    int y;
    int x;

    if (map == NULL || columns <= 0 || rows <= 0 || tile_width_m <= 0.0f || tile_height_m <= 0.0f) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    tile_count = (size_t)columns * (size_t)rows;
    if (tile_count > MK_MAX_MAP_TILES) {
        return MK_ERROR_CAPACITY;
    }

    map->tile_width_m = tile_width_m;
    map->tile_height_m = tile_height_m;
    map->tile_columns = columns;
    map->tile_rows = rows;
    map->tile_count = tile_count;

    for (y = 0; y < rows; ++y) {
        for (x = 0; x < columns; ++x) {
            size_t index = (size_t)y * (size_t)columns + (size_t)x;
            mk_map_tile_t tile = mk_make_map_tile(
                mk_ivec2(x, y),
                default_kind,
                0,
                0,
                1,
                false,
                false
            );

            tile.id = (uint32_t)(index + 1);
            map->tiles[index] = tile;
        }
    }

    return MK_OK;
}

mk_map_tile_t *mk_map_get_tile(mk_map_t *map, mk_ivec2_t coordinate) {
    if (!mk_map_tile_coordinate_is_valid(map, coordinate)) {
        return NULL;
    }

    return &map->tiles[mk_map_tile_index(map, coordinate)];
}

const mk_map_tile_t *mk_map_get_tile_const(const mk_map_t *map, mk_ivec2_t coordinate) {
    if (!mk_map_tile_coordinate_is_valid(map, coordinate)) {
        return NULL;
    }

    return &map->tiles[mk_map_tile_index(map, coordinate)];
}

mk_result_t mk_map_set_tile(mk_map_t *map, const mk_map_tile_t *tile) {
    mk_map_tile_t copy;
    size_t index;

    if (map == NULL || tile == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!mk_map_tile_coordinate_is_valid(map, tile->coordinate) || tile->movement_cost < 0) {
        return MK_ERROR_INVALID_DATA;
    }

    index = mk_map_tile_index(map, tile->coordinate);
    if (index >= map->tile_count) {
        return MK_ERROR_INVALID_DATA;
    }

    copy = *tile;
    copy.id = (uint32_t)(index + 1);
    map->tiles[index] = copy;

    return MK_OK;
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

mk_result_t mk_scenario_add_controller(
    mk_scenario_definition_t *scenario,
    const mk_controller_slot_t *controller,
    uint32_t *out_controller_id
) {
    mk_controller_slot_t copy;

    if (scenario == NULL || controller == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (controller->kind == MK_CONTROLLER_NONE) {
        return MK_ERROR_INVALID_DATA;
    }

    if (scenario->controller_count >= MK_MAX_CONTROLLERS) {
        return MK_ERROR_CAPACITY;
    }

    copy = *controller;
    copy.id = (uint32_t)(scenario->controller_count + 1);

    scenario->controllers[scenario->controller_count] = copy;
    scenario->controller_count += 1;

    if (out_controller_id != NULL) {
        *out_controller_id = copy.id;
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

mk_result_t mk_scenario_add_force(mk_scenario_definition_t *scenario, const mk_force_t *force, uint32_t *out_force_id) {
    mk_force_t copy;

    if (scenario == NULL || force == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!mk_faction_id_exists(scenario, force->faction_id)
        || !mk_controller_id_exists(scenario, force->controller_id)) {
        return MK_ERROR_INVALID_DATA;
    }

    if (scenario->force_count >= MK_MAX_FORCES) {
        return MK_ERROR_CAPACITY;
    }

    copy = *force;
    copy.id = (uint32_t)(scenario->force_count + 1);
    if (copy.command.id == 0) {
        copy.command.id = copy.id;
    }

    scenario->forces[scenario->force_count] = copy;
    scenario->force_count += 1;

    if (out_force_id != NULL) {
        *out_force_id = copy.id;
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

mk_result_t mk_scenario_add_civilian(
    mk_scenario_definition_t *scenario,
    const mk_civilian_t *civilian,
    uint32_t *out_civilian_id
) {
    mk_civilian_t copy;

    if (scenario == NULL || civilian == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (scenario->civilian_count >= MK_MAX_CIVILIANS) {
        return MK_ERROR_CAPACITY;
    }

    if (!mk_faction_id_exists(scenario, civilian->faction_id)
        || !mk_position_fits_map(civilian->position_m, &scenario->map)) {
        return MK_ERROR_INVALID_DATA;
    }

    copy = *civilian;
    copy.id = (uint32_t)(scenario->civilian_count + 1);

    scenario->civilians[scenario->civilian_count] = copy;
    scenario->civilian_count += 1;

    if (out_civilian_id != NULL) {
        *out_civilian_id = copy.id;
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
