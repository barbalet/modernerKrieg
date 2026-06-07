#include "mk_core.h"

#include <float.h>
#include <math.h>
#include <stdarg.h>
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

static void mk_copy_scenario_text(char destination[MK_SCENARIO_TEXT_CAPACITY], const char *source) {
    mk_copy_text(destination, MK_SCENARIO_TEXT_CAPACITY, source);
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

static bool mk_float_close(float first, float second) {
    float delta = first - second;

    if (delta < 0.0f) {
        delta = -delta;
    }

    return delta < 0.01f;
}

static bool mk_text_is_present(const char *text) {
    return text != NULL && text[0] != '\0';
}

static bool mk_gameplay_area_pixel_rect_is_valid(
    int x,
    int y,
    int width,
    int height,
    int pixel_width,
    int pixel_height
) {
    return x >= 0
        && y >= 0
        && width > 0
        && height > 0
        && pixel_width > 0
        && pixel_height > 0
        && x + width <= pixel_width
        && y + height <= pixel_height;
}

static mk_rect_t mk_gameplay_area_pixel_rect_to_world(
    const mk_gameplay_area_t *area,
    int x,
    int y,
    int width,
    int height
) {
    float pixels_per_meter = area != NULL && area->pixels_per_meter > 0.0f ? area->pixels_per_meter : 1.0f;

    return mk_rect(
        (float)x / pixels_per_meter,
        (float)y / pixels_per_meter,
        (float)width / pixels_per_meter,
        (float)height / pixels_per_meter
    );
}

static bool mk_gameplay_area_rect_fits_world(const mk_gameplay_area_t *area, mk_rect_t rect) {
    if (area == NULL || !mk_rect_is_valid(rect)) {
        return false;
    }

    return rect.x >= 0.0f
        && rect.y >= 0.0f
        && rect.x + rect.width <= area->world_width_m + 0.01f
        && rect.y + rect.height <= area->world_height_m + 0.01f;
}

static bool mk_gameplay_area_ids_are_unique(const mk_gameplay_area_t *area) {
    size_t index;

    if (area == NULL) {
        return false;
    }

    for (index = 0; index < area->level_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < area->level_count; ++other_index) {
            if (strcmp(area->levels[index].id, area->levels[other_index].id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < area->feature_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < area->feature_count; ++other_index) {
            if (strcmp(area->features[index].id, area->features[other_index].id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < area->region_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < area->region_count; ++other_index) {
            if (strcmp(area->regions[index].id, area->regions[other_index].id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < area->topology_node_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < area->topology_node_count; ++other_index) {
            if (strcmp(area->topology_nodes[index].id, area->topology_nodes[other_index].id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < area->topology_portal_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < area->topology_portal_count; ++other_index) {
            if (strcmp(area->topology_portals[index].id, area->topology_portals[other_index].id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < area->semantic_zone_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < area->semantic_zone_count; ++other_index) {
            if (strcmp(area->semantic_zones[index].id, area->semantic_zones[other_index].id) == 0) {
                return false;
            }
        }
    }

    return true;
}

static bool mk_gameplay_area_topology_node_index(
    const mk_gameplay_area_t *area,
    const char *node_id,
    size_t *out_index
) {
    size_t index;

    if (area == NULL || node_id == NULL) {
        return false;
    }

    for (index = 0; index < area->topology_node_count; ++index) {
        if (strcmp(area->topology_nodes[index].id, node_id) == 0) {
            if (out_index != NULL) {
                *out_index = index;
            }
            return true;
        }
    }

    return false;
}

static bool mk_gameplay_area_region_has_topology_node(
    const mk_gameplay_area_t *area,
    const char *region_id
) {
    size_t index;

    if (area == NULL || region_id == NULL || region_id[0] == '\0') {
        return false;
    }

    for (index = 0; index < area->topology_node_count; ++index) {
        if (strcmp(area->topology_nodes[index].region_id, region_id) == 0) {
            return true;
        }
    }

    return false;
}

static size_t mk_gameplay_area_topology_unreachable_count(const mk_gameplay_area_t *area) {
    bool visited[MK_MAX_GAMEPLAY_TOPOLOGY_NODES];
    size_t queue[MK_MAX_GAMEPLAY_TOPOLOGY_NODES];
    size_t start_index = (size_t)-1;
    size_t queue_read = 0;
    size_t queue_write = 0;
    size_t unreachable = 0;
    size_t index;

    if (area == NULL || !area->topology_loaded || area->topology_node_count == 0) {
        return 0;
    }

    memset(visited, 0, sizeof(visited));
    for (index = 0; index < area->topology_node_count; ++index) {
        if (area->topology_nodes[index].enterable) {
            start_index = index;
            break;
        }
    }

    if (start_index == (size_t)-1) {
        return 0;
    }

    visited[start_index] = true;
    queue[queue_write] = start_index;
    queue_write += 1;

    while (queue_read < queue_write) {
        size_t current_index = queue[queue_read];
        queue_read += 1;

        for (index = 0; index < area->topology_portal_count; ++index) {
            const mk_gameplay_topology_portal_t *portal = &area->topology_portals[index];
            size_t from_index;
            size_t to_index;
            size_t next_index;

            if (!portal->bidirectional
                || strcmp(portal->state, "blocked") == 0
                || strcmp(portal->state, "locked") == 0
                || strcmp(portal->state, "unsafe") == 0
                || !mk_gameplay_area_topology_node_index(area, portal->from_node_id, &from_index)
                || !mk_gameplay_area_topology_node_index(area, portal->to_node_id, &to_index)) {
                continue;
            }

            if (from_index == current_index) {
                next_index = to_index;
            } else if (to_index == current_index) {
                next_index = from_index;
            } else {
                continue;
            }

            if (!area->topology_nodes[next_index].enterable || visited[next_index]) {
                continue;
            }

            visited[next_index] = true;
            queue[queue_write] = next_index;
            queue_write += 1;
        }
    }

    for (index = 0; index < area->topology_node_count; ++index) {
        if (area->topology_nodes[index].enterable && !visited[index]) {
            unreachable += 1;
        }
    }

    return unreachable;
}

static bool mk_gameplay_area_is_valid(const mk_gameplay_area_t *area) {
    size_t index;

    if (area == NULL) {
        return false;
    }

    if (!area->loaded) {
        return true;
    }

    if (area->schema_version <= 0
        || !mk_text_is_present(area->id)
        || !mk_text_is_present(area->map_id)
        || !mk_text_is_present(area->name)
        || area->world_width_m <= 0.0f
        || area->world_height_m <= 0.0f
        || area->pixel_width <= 0
        || area->pixel_height <= 0
        || area->pixels_per_meter <= 0.0f
        || strcmp(area->origin, "top_left") != 0
        || area->max_storeys <= 0
        || area->level_count == 0
        || area->level_count > MK_MAX_GAMEPLAY_AREA_LEVELS
        || area->feature_count > MK_MAX_GAMEPLAY_AREA_FEATURES
        || area->region_count > MK_MAX_GAMEPLAY_AREA_REGIONS
        || !mk_gameplay_area_ids_are_unique(area)) {
        return false;
    }

    for (index = 0; index < area->level_count; ++index) {
        const mk_gameplay_level_t *level = &area->levels[index];

        if (!mk_text_is_present(level->id)
            || !mk_text_is_present(level->image_path)
            || !mk_text_is_present(level->alpha)
            || level->index <= 0
            || level->elevation_m < 0.0f) {
            return false;
        }
    }

    for (index = 0; index < area->feature_count; ++index) {
        const mk_gameplay_feature_t *feature = &area->features[index];
        mk_rect_t expected_bounds;

        if (!mk_text_is_present(feature->id)
            || !mk_text_is_present(feature->level_id)
            || !mk_text_is_present(feature->kind)
            || mk_gameplay_area_find_level(area, feature->level_id) == NULL
            || !mk_gameplay_area_pixel_rect_is_valid(
                feature->pixel_x,
                feature->pixel_y,
                feature->pixel_width,
                feature->pixel_height,
                area->pixel_width,
                area->pixel_height
            )
            || (feature->blocks_los && feature->allows_los)
            || (feature->blocks_movement && feature->allows_movement)
            || !mk_gameplay_area_rect_fits_world(area, feature->bounds_m)) {
            return false;
        }

        expected_bounds = mk_gameplay_area_pixel_rect_to_world(
            area,
            feature->pixel_x,
            feature->pixel_y,
            feature->pixel_width,
            feature->pixel_height
        );
        if (!mk_float_close(feature->bounds_m.x, expected_bounds.x)
            || !mk_float_close(feature->bounds_m.y, expected_bounds.y)
            || !mk_float_close(feature->bounds_m.width, expected_bounds.width)
            || !mk_float_close(feature->bounds_m.height, expected_bounds.height)) {
            return false;
        }
    }

    for (index = 0; index < area->region_count; ++index) {
        const mk_gameplay_region_t *region = &area->regions[index];
        mk_rect_t expected_bounds;

        if (!mk_text_is_present(region->id)
            || !mk_text_is_present(region->roof_level_id)
            || mk_gameplay_area_find_level(area, region->roof_level_id) == NULL
            || region->storeys <= 0
            || region->storeys > area->max_storeys
            || !mk_gameplay_area_pixel_rect_is_valid(
                region->pixel_x,
                region->pixel_y,
                region->pixel_width,
                region->pixel_height,
                area->pixel_width,
                area->pixel_height
            )
            || !mk_gameplay_area_rect_fits_world(area, region->bounds_m)) {
            return false;
        }

        expected_bounds = mk_gameplay_area_pixel_rect_to_world(
            area,
            region->pixel_x,
            region->pixel_y,
            region->pixel_width,
            region->pixel_height
        );
        if (!mk_float_close(region->bounds_m.x, expected_bounds.x)
            || !mk_float_close(region->bounds_m.y, expected_bounds.y)
            || !mk_float_close(region->bounds_m.width, expected_bounds.width)
            || !mk_float_close(region->bounds_m.height, expected_bounds.height)) {
            return false;
        }
    }

    if (area->topology_loaded) {
        if (area->topology_schema_version <= 0
            || !mk_text_is_present(area->topology_id)
            || area->topology_node_count == 0
            || area->topology_node_count > MK_MAX_GAMEPLAY_TOPOLOGY_NODES
            || area->topology_portal_count > MK_MAX_GAMEPLAY_TOPOLOGY_PORTALS
            || area->semantic_zone_count > MK_MAX_GAMEPLAY_SEMANTIC_ZONES) {
            return false;
        }

        for (index = 0; index < area->topology_node_count; ++index) {
            const mk_gameplay_topology_node_t *node = &area->topology_nodes[index];
            mk_rect_t expected_bounds;

            if (!mk_text_is_present(node->id)
                || !mk_text_is_present(node->kind)
                || !mk_text_is_present(node->level_id)
                || !mk_text_is_present(node->label)
                || mk_gameplay_area_find_level(area, node->level_id) == NULL
                || (node->region_id[0] != '\0' && mk_gameplay_area_find_region(area, node->region_id) == NULL)
                || !mk_gameplay_area_pixel_rect_is_valid(
                    node->pixel_x,
                    node->pixel_y,
                    node->pixel_width,
                    node->pixel_height,
                    area->pixel_width,
                    area->pixel_height
                )
                || !mk_gameplay_area_rect_fits_world(area, node->bounds_m)) {
                return false;
            }

            expected_bounds = mk_gameplay_area_pixel_rect_to_world(
                area,
                node->pixel_x,
                node->pixel_y,
                node->pixel_width,
                node->pixel_height
            );
            if (!mk_float_close(node->bounds_m.x, expected_bounds.x)
                || !mk_float_close(node->bounds_m.y, expected_bounds.y)
                || !mk_float_close(node->bounds_m.width, expected_bounds.width)
                || !mk_float_close(node->bounds_m.height, expected_bounds.height)) {
                return false;
            }
        }

        for (index = 0; index < area->region_count; ++index) {
            if (!mk_gameplay_area_region_has_topology_node(area, area->regions[index].id)) {
                return false;
            }
        }

        for (index = 0; index < area->topology_portal_count; ++index) {
            const mk_gameplay_topology_portal_t *portal = &area->topology_portals[index];
            const mk_gameplay_topology_node_t *from_node;
            const mk_gameplay_topology_node_t *to_node;
            const mk_gameplay_feature_t *feature;
            mk_rect_t expected_bounds;

            if (!mk_text_is_present(portal->id)
                || !mk_text_is_present(portal->kind)
                || !mk_text_is_present(portal->state)
                || !mk_text_is_present(portal->from_node_id)
                || !mk_text_is_present(portal->to_node_id)
                || !mk_text_is_present(portal->level_id)
                || !portal->bidirectional
                || portal->movement_cost <= 0
                || mk_gameplay_area_find_level(area, portal->level_id) == NULL
                || !mk_gameplay_area_pixel_rect_is_valid(
                    portal->pixel_x,
                    portal->pixel_y,
                    portal->pixel_width,
                    portal->pixel_height,
                    area->pixel_width,
                    area->pixel_height
                )
                || !mk_gameplay_area_rect_fits_world(area, portal->bounds_m)) {
                return false;
            }

            from_node = mk_gameplay_area_find_topology_node(area, portal->from_node_id);
            to_node = mk_gameplay_area_find_topology_node(area, portal->to_node_id);
            if (from_node == NULL || to_node == NULL || strcmp(from_node->id, to_node->id) == 0) {
                return false;
            }

            if (portal->vertical) {
                if (strcmp(from_node->level_id, to_node->level_id) == 0) {
                    return false;
                }
            } else if (strcmp(from_node->level_id, to_node->level_id) != 0) {
                return false;
            }

            if (portal->feature_id[0] != '\0') {
                feature = mk_gameplay_area_find_feature(area, portal->feature_id);
                if (feature == NULL || strcmp(feature->level_id, portal->level_id) != 0) {
                    return false;
                }
            }

            expected_bounds = mk_gameplay_area_pixel_rect_to_world(
                area,
                portal->pixel_x,
                portal->pixel_y,
                portal->pixel_width,
                portal->pixel_height
            );
            if (!mk_float_close(portal->bounds_m.x, expected_bounds.x)
                || !mk_float_close(portal->bounds_m.y, expected_bounds.y)
                || !mk_float_close(portal->bounds_m.width, expected_bounds.width)
                || !mk_float_close(portal->bounds_m.height, expected_bounds.height)) {
                return false;
            }
        }

        for (index = 0; index < area->semantic_zone_count; ++index) {
            const mk_gameplay_semantic_zone_t *zone = &area->semantic_zones[index];
            const mk_gameplay_topology_node_t *node;
            mk_rect_t expected_bounds;

            if (!mk_text_is_present(zone->id)
                || !mk_text_is_present(zone->kind)
                || !mk_text_is_present(zone->node_id)
                || !mk_text_is_present(zone->level_id)
                || zone->priority < 0
                || mk_gameplay_area_find_level(area, zone->level_id) == NULL
                || !mk_gameplay_area_pixel_rect_is_valid(
                    zone->pixel_x,
                    zone->pixel_y,
                    zone->pixel_width,
                    zone->pixel_height,
                    area->pixel_width,
                    area->pixel_height
                )
                || !mk_gameplay_area_rect_fits_world(area, zone->bounds_m)) {
                return false;
            }

            node = mk_gameplay_area_find_topology_node(area, zone->node_id);
            if (node == NULL || strcmp(node->level_id, zone->level_id) != 0) {
                return false;
            }

            expected_bounds = mk_gameplay_area_pixel_rect_to_world(
                area,
                zone->pixel_x,
                zone->pixel_y,
                zone->pixel_width,
                zone->pixel_height
            );
            if (!mk_float_close(zone->bounds_m.x, expected_bounds.x)
                || !mk_float_close(zone->bounds_m.y, expected_bounds.y)
                || !mk_float_close(zone->bounds_m.width, expected_bounds.width)
                || !mk_float_close(zone->bounds_m.height, expected_bounds.height)) {
                return false;
            }
        }

        if (mk_gameplay_area_topology_unreachable_count(area) > 0) {
            return false;
        }
    }

    return true;
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

static mk_vec2_t mk_rect_center(mk_rect_t rect) {
    mk_vec2_t center;

    center.x = rect.x + rect.width * 0.5f;
    center.y = rect.y + rect.height * 0.5f;
    return center;
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

static int mk_gameplay_area_feature_cover_value(const mk_gameplay_feature_t *feature) {
    if (feature == NULL) {
        return 0;
    }

    if (strcmp(feature->kind, "wall") == 0) {
        return 4;
    }

    if (strcmp(feature->kind, "window") == 0) {
        return 3;
    }

    if (strcmp(feature->kind, "roof_edge") == 0) {
        return 3;
    }

    if (strcmp(feature->kind, "door") == 0 || strcmp(feature->kind, "breach_hole") == 0) {
        return 2;
    }

    if (strcmp(feature->kind, "stair") == 0) {
        return 1;
    }

    return 0;
}

static int mk_gameplay_area_feature_navigation_modifier(const mk_gameplay_feature_t *feature) {
    if (feature == NULL) {
        return 0;
    }

    if (strcmp(feature->kind, "breach_hole") == 0) {
        return 2;
    }

    if (strcmp(feature->kind, "door") == 0 || strcmp(feature->kind, "stair") == 0) {
        return 1;
    }

    return 0;
}

static int mk_gameplay_area_node_navigation_cost(const mk_gameplay_topology_node_t *node) {
    if (node == NULL) {
        return 2;
    }

    if (!node->enterable || strcmp(node->kind, "blocked_building") == 0) {
        return MK_GAMEPLAY_BLOCKED_NAVIGATION_COST;
    }

    if (strcmp(node->kind, "street") == 0) {
        return 1;
    }

    if (strcmp(node->kind, "alley") == 0 || strcmp(node->kind, "courtyard") == 0 || strcmp(node->kind, "shelter") == 0) {
        return 2;
    }

    if (strcmp(node->kind, "roof") == 0 || strcmp(node->kind, "stairwell") == 0) {
        return 3;
    }

    if (strcmp(node->kind, "shop") == 0
        || strcmp(node->kind, "workshop") == 0
        || strcmp(node->kind, "garage") == 0
        || strcmp(node->kind, "office") == 0
        || strcmp(node->kind, "mosque") == 0
        || strcmp(node->kind, "cache") == 0) {
        return 3;
    }

    return 2;
}

static int mk_gameplay_area_node_cover_value(const mk_gameplay_topology_node_t *node) {
    if (node == NULL || !node->enterable) {
        return 0;
    }

    if (strcmp(node->kind, "roof") == 0) {
        return 2;
    }

    if (strcmp(node->kind, "shop") == 0
        || strcmp(node->kind, "workshop") == 0
        || strcmp(node->kind, "garage") == 0
        || strcmp(node->kind, "office") == 0
        || strcmp(node->kind, "mosque") == 0
        || strcmp(node->kind, "shelter") == 0
        || strcmp(node->kind, "cache") == 0) {
        return 2;
    }

    if (strcmp(node->kind, "courtyard") == 0 || strcmp(node->kind, "alley") == 0 || strcmp(node->kind, "stairwell") == 0) {
        return 1;
    }

    return 0;
}

static bool mk_gameplay_area_node_is_interior(const mk_gameplay_topology_node_t *node) {
    if (node == NULL) {
        return false;
    }

    return node->region_id[0] != '\0'
        || strcmp(node->kind, "shop") == 0
        || strcmp(node->kind, "workshop") == 0
        || strcmp(node->kind, "garage") == 0
        || strcmp(node->kind, "office") == 0
        || strcmp(node->kind, "mosque") == 0
        || strcmp(node->kind, "shelter") == 0
        || strcmp(node->kind, "cache") == 0
        || strcmp(node->kind, "stairwell") == 0;
}

static bool mk_gameplay_area_portal_blocks_movement(const mk_gameplay_topology_portal_t *portal) {
    if (portal == NULL) {
        return false;
    }

    return strcmp(portal->state, "closed") == 0
        || strcmp(portal->state, "locked") == 0
        || strcmp(portal->state, "blocked") == 0
        || strcmp(portal->state, "unsafe") == 0
        || strcmp(portal->kind, "window") == 0
        || strcmp(portal->kind, "roof_edge") == 0;
}

static int mk_gameplay_area_zone_navigation_modifier(const mk_gameplay_semantic_zone_t *zone) {
    if (zone == NULL) {
        return 0;
    }

    if (strcmp(zone->kind, "market_crowd") == 0) {
        return 3;
    }

    if (strcmp(zone->kind, "restricted_fire_lane") == 0 || strcmp(zone->kind, "danger_area") == 0) {
        return 2;
    }

    if (strcmp(zone->kind, "cache") == 0
        || strcmp(zone->kind, "search_objective") == 0
        || strcmp(zone->kind, "overwatch_roof") == 0) {
        return 1;
    }

    return 0;
}

static int mk_gameplay_area_zone_cover_value(const mk_gameplay_semantic_zone_t *zone) {
    if (zone == NULL) {
        return 0;
    }

    if (strcmp(zone->kind, "civilian_shelter") == 0 || strcmp(zone->kind, "cache") == 0) {
        return 2;
    }

    if (strcmp(zone->kind, "market_crowd") == 0 || strcmp(zone->kind, "overwatch_roof") == 0) {
        return 1;
    }

    return 0;
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

static bool mk_game_contact_report_exists(
    const mk_game_t *game,
    mk_contact_report_kind_t kind,
    uint32_t observer_unit_id,
    uint32_t target_unit_id,
    uint32_t terrain_id
) {
    size_t index;

    if (game == NULL) {
        return false;
    }

    for (index = 0; index < game->contact_report_count; ++index) {
        const mk_contact_report_t *report = &game->contact_reports[index];

        if (report->kind == kind
            && report->attacker_unit_id == observer_unit_id
            && report->target_unit_id == target_unit_id
            && report->terrain_id == terrain_id) {
            return true;
        }
    }

    return false;
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
    report->confidence = 100;
    report->visible = true;
    report->resolved = true;
}

static void mk_game_record_suspected_contact(
    mk_game_t *game,
    const mk_unit_t *observer,
    const mk_unit_t *hidden_unit,
    float distance_m,
    float suspect_distance_m
) {
    mk_contact_report_t *report;
    int confidence;

    if (game == NULL || observer == NULL || hidden_unit == NULL) {
        return;
    }

    if (mk_game_contact_report_exists(
            game,
            MK_CONTACT_REPORT_SUSPECTED_DANGER,
            observer->id,
            hidden_unit->id,
            0
        )) {
        return;
    }

    report = mk_game_add_contact_report(game, MK_CONTACT_REPORT_SUSPECTED_DANGER);
    if (report == NULL) {
        return;
    }

    confidence = 25;
    if (suspect_distance_m > 0.0f && distance_m < suspect_distance_m) {
        confidence += (int)((suspect_distance_m - distance_m) / suspect_distance_m * 50.0f);
    }
    confidence = mk_clamp_int(confidence - hidden_unit->concealment / 2, 10, 80);

    report->attacker_unit_id = observer->id;
    report->target_unit_id = hidden_unit->id;
    report->side = hidden_unit->side;
    report->position_m = hidden_unit->position_m;
    report->target_position_m = hidden_unit->position_m;
    report->confidence = confidence;
    report->visible = true;
    report->resolved = false;
}

static void mk_game_reveal_unit(mk_game_t *game, mk_unit_t *unit, const mk_unit_t *observer) {
    if (game == NULL || unit == NULL || !unit->hidden || unit->revealed) {
        return;
    }

    unit->revealed = true;
    mk_game_record_reveal_contact(game, observer, unit);
}

static bool mk_game_hidden_enemy_near_position(
    const mk_game_t *game,
    const mk_unit_t *observer,
    mk_vec2_t position_m,
    float radius_m
) {
    size_t index;

    if (game == NULL || observer == NULL) {
        return false;
    }

    for (index = 0; index < game->unit_count; ++index) {
        const mk_unit_t *unit = &game->units[index];

        if (!unit->hidden
            || unit->revealed
            || unit->side == MK_SIDE_CIVILIAN
            || unit->side == observer->side
            || unit->status == MK_UNIT_BROKEN) {
            continue;
        }

        if (mk_distance(unit->position_m, position_m) <= radius_m) {
            return true;
        }
    }

    return false;
}

static mk_unit_t *mk_game_find_investigation_target(mk_game_t *game, const mk_unit_t *investigator, const mk_contact_report_t *report) {
    mk_unit_t *target;

    if (game == NULL || investigator == NULL || report == NULL || report->target_unit_id == 0) {
        return NULL;
    }

    target = mk_game_find_unit(game, report->target_unit_id);
    if (target == NULL
        || target->side == MK_SIDE_CIVILIAN
        || target->side == investigator->side
        || target->status == MK_UNIT_BROKEN) {
        return NULL;
    }

    return target;
}

static void mk_game_update_investigation_contacts(mk_game_t *game) {
    size_t unit_index;

    if (game == NULL) {
        return;
    }

    for (unit_index = 0; unit_index < game->unit_count; ++unit_index) {
        mk_unit_t *unit = &game->units[unit_index];
        size_t report_index;

        if (unit->order != MK_ORDER_INVESTIGATE
            || unit->side == MK_SIDE_CIVILIAN
            || unit->status == MK_UNIT_BROKEN) {
            continue;
        }

        for (report_index = 0; report_index < game->contact_report_count; ++report_index) {
            mk_contact_report_t *report = &game->contact_reports[report_index];
            mk_unit_t *target;

            if (report->resolved
                || !report->visible
                || (report->kind != MK_CONTACT_REPORT_SUSPECTED_DANGER
                    && report->kind != MK_CONTACT_REPORT_FALSE_CONTACT)
                || mk_distance(unit->position_m, report->position_m) > 18.0f) {
                continue;
            }

            if (report->side != MK_SIDE_NEUTRAL
                && report->side != MK_SIDE_CIVILIAN
                && report->side == unit->side
                && report->attacker_unit_id != unit->id) {
                continue;
            }

            report->resolved = true;
            report->tick = game->tick;
            if (report->kind == MK_CONTACT_REPORT_FALSE_CONTACT) {
                report->confidence = 0;
                continue;
            }

            target = mk_game_find_investigation_target(game, unit, report);
            if (target != NULL && target->hidden && !target->revealed) {
                mk_game_reveal_unit(game, target, unit);
            }
        }
    }
}

static void mk_game_record_false_contact(
    mk_game_t *game,
    const mk_unit_t *observer,
    const mk_terrain_zone_t *terrain
) {
    mk_contact_report_t *report;
    mk_vec2_t position;

    if (game == NULL || observer == NULL || terrain == NULL || terrain->id == 0) {
        return;
    }

    if (mk_game_contact_report_exists(
            game,
            MK_CONTACT_REPORT_FALSE_CONTACT,
            observer->id,
            0,
            terrain->id
        )) {
        return;
    }

    position = mk_rect_center(terrain->bounds_m);
    report = mk_game_add_contact_report(game, MK_CONTACT_REPORT_FALSE_CONTACT);
    if (report == NULL) {
        return;
    }

    report->attacker_unit_id = observer->id;
    report->terrain_id = terrain->id;
    report->side = MK_SIDE_NEUTRAL;
    report->position_m = position;
    report->target_position_m = position;
    report->confidence = 20 + mk_min_int(20, terrain->cover * 5);
    report->visible = true;
    report->resolved = false;
}

static void mk_game_update_false_terrain_contacts(mk_game_t *game) {
    size_t unit_index;

    if (game == NULL) {
        return;
    }

    for (unit_index = 0; unit_index < game->unit_count; ++unit_index) {
        const mk_unit_t *observer = &game->units[unit_index];
        size_t terrain_index;

        if (observer->side == MK_SIDE_CIVILIAN || observer->status == MK_UNIT_BROKEN) {
            continue;
        }

        for (terrain_index = 0; terrain_index < game->map.terrain_count; ++terrain_index) {
            const mk_terrain_zone_t *terrain = &game->map.terrain[terrain_index];
            mk_vec2_t terrain_center;

            if (terrain->kind != MK_TERRAIN_RUBBLE && terrain->kind != MK_TERRAIN_SUSPECTED_IED) {
                continue;
            }

            terrain_center = mk_rect_center(terrain->bounds_m);
            if (mk_distance(observer->position_m, terrain_center) > 60.0f) {
                continue;
            }

            if (mk_game_hidden_enemy_near_position(game, observer, terrain_center, 90.0f)) {
                continue;
            }

            mk_game_record_false_contact(game, observer, terrain);
        }
    }
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

static int mk_game_score_success_threshold(const mk_game_t *game) {
    if (game != NULL && game->score_success_threshold > 0) {
        return game->score_success_threshold;
    }

    return MK_DEFAULT_SCORE_SUCCESS_THRESHOLD;
}

static int mk_game_score_partial_threshold(const mk_game_t *game) {
    if (game != NULL && game->score_partial_threshold > 0) {
        return game->score_partial_threshold;
    }

    return MK_DEFAULT_SCORE_PARTIAL_THRESHOLD;
}

static int mk_game_score_objective_weight(const mk_game_t *game) {
    if (game != NULL && game->score_objective_weight > 0) {
        return game->score_objective_weight;
    }

    return MK_DEFAULT_SCORE_OBJECTIVE_WEIGHT;
}

static int mk_game_score_civilian_risk_weight(const mk_game_t *game) {
    if (game != NULL && game->score_civilian_risk_weight > 0) {
        return game->score_civilian_risk_weight;
    }

    return MK_DEFAULT_SCORE_CIVILIAN_RISK_WEIGHT;
}

static int mk_game_score_player_casualty_weight(const mk_game_t *game) {
    if (game != NULL && game->score_player_casualty_weight > 0) {
        return game->score_player_casualty_weight;
    }

    return MK_DEFAULT_SCORE_PLAYER_CASUALTY_WEIGHT;
}

static int mk_game_score_civilian_casualty_weight(const mk_game_t *game) {
    if (game != NULL && game->score_civilian_casualty_weight > 0) {
        return game->score_civilian_casualty_weight;
    }

    return MK_DEFAULT_SCORE_CIVILIAN_CASUALTY_WEIGHT;
}

static int mk_game_score_time_weight(const mk_game_t *game) {
    if (game != NULL && game->score_time_weight > 0) {
        return game->score_time_weight;
    }

    return MK_DEFAULT_SCORE_TIME_WEIGHT;
}

static void mk_unit_clear_route(mk_unit_t *unit) {
    if (unit == NULL) {
        return;
    }

    unit->has_route = false;
    unit->route_step_count = 0;
    unit->route_step_index = 0;
    unit->route_total_cost = 0;
    unit->route_uses_vertical_transition = false;
    memset(unit->route_waypoints_m, 0, sizeof(unit->route_waypoints_m));
    memset(unit->route_step_level_ids, 0, sizeof(unit->route_step_level_ids));
    memset(unit->route_step_portal_ids, 0, sizeof(unit->route_step_portal_ids));
    memset(unit->route_step_vertical_transition, 0, sizeof(unit->route_step_vertical_transition));
    unit->route_stuck_ticks = 0;
}

static void mk_unit_set_route_failure(mk_unit_t *unit, const char *reason) {
    if (unit == NULL) {
        return;
    }

    mk_copy_text(unit->route_failure_reason, sizeof(unit->route_failure_reason), reason);
    unit->route_failure_count += 1;
    mk_unit_clear_route(unit);
}

static float mk_unit_wound_move_multiplier(const mk_unit_t *unit) {
    size_t index;
    size_t movable_count = 0;
    size_t impaired_count = 0;

    if (unit == NULL || unit->soldier_count == 0) {
        return 1.0f;
    }

    for (index = 0; index < unit->soldier_count; ++index) {
        const mk_soldier_t *soldier = &unit->soldiers[index];

        if (soldier->casualty || !soldier->can_move || soldier->wound_state == MK_WOUND_KILLED) {
            impaired_count += 1;
            continue;
        }

        movable_count += 1;
        if (soldier->wound_state == MK_WOUND_SERIOUS || soldier->wound_state == MK_WOUND_INCAPACITATED) {
            impaired_count += 1;
        }
    }

    if (movable_count == 0) {
        return 0.0f;
    }

    if (impaired_count >= unit->soldier_count) {
        return 0.5f;
    }

    if (impaired_count > 0) {
        return 0.75f;
    }

    return 1.0f;
}

static bool mk_unit_assign_route(mk_unit_t *unit, const mk_gameplay_route_t *route) {
    size_t index;

    if (unit == NULL || route == NULL || !route->valid || route->step_count == 0 || route->step_count > MK_MAX_GAMEPLAY_ROUTE_STEPS) {
        return false;
    }

    mk_unit_clear_route(unit);
    unit->has_route = true;
    unit->route_step_count = route->step_count;
    unit->route_step_index = 0;
    unit->route_total_cost = route->total_cost;
    unit->route_uses_vertical_transition = route->uses_vertical_transition;
    for (index = 0; index < route->step_count; ++index) {
        unit->route_waypoints_m[index] = route->steps[index].waypoint_m;
        mk_copy_name(unit->route_step_level_ids[index], route->steps[index].level_id);
        mk_copy_name(unit->route_step_portal_ids[index], route->steps[index].portal_id);
        unit->route_step_vertical_transition[index] = route->steps[index].vertical_transition;
    }

    return true;
}


static const char *mk_game_default_level_id(const mk_game_t *game) {
    if (game != NULL
        && mk_gameplay_area_is_loaded(&game->gameplay_area)
        && game->gameplay_area.level_count > 0) {
        return game->gameplay_area.levels[0].id;
    }

    return "";
}

static const char *mk_unit_current_level_id(const mk_game_t *game, const mk_unit_t *unit) {
    if (unit != NULL && unit->level_id[0] != '\0') {
        return unit->level_id;
    }

    return mk_game_default_level_id(game);
}

static bool mk_order_is_movement_order(mk_order_t order) {
    return order == MK_ORDER_MOVE
        || order == MK_ORDER_ASSAULT_MOVE
        || order == MK_ORDER_WITHDRAW
        || order == MK_ORDER_INVESTIGATE;
}

static void mk_update_unit_movement(mk_game_t *game, mk_unit_t *unit) {
    float dx;
    float dy;
    float distance_squared;
    float speed;
    float step_squared;
    float distance;
    float move_multiplier;
    mk_vec2_t movement_target;

    if (unit == NULL || !unit->has_move_target) {
        return;
    }

    move_multiplier = mk_unit_status_move_multiplier(unit->status);
    move_multiplier *= mk_unit_wound_move_multiplier(unit);
    if (move_multiplier <= 0.0f) {
        unit->has_move_target = false;
        mk_unit_set_route_failure(unit, "immobile");
        unit->order = MK_ORDER_RALLY;
        return;
    }

    if (!mk_order_is_movement_order(unit->order)) {
        unit->has_move_target = false;
        mk_unit_clear_route(unit);
        return;
    }

    movement_target = unit->target_position_m;
    if (unit->has_route) {
        if (unit->route_step_count == 0 || unit->route_step_index >= unit->route_step_count) {
            mk_unit_set_route_failure(unit, "route_exhausted");
            unit->has_move_target = false;
            unit->order = MK_ORDER_HOLD;
            return;
        }

        movement_target = unit->route_waypoints_m[unit->route_step_index];
    }

    dx = movement_target.x - unit->position_m.x;
    dy = movement_target.y - unit->position_m.y;
    distance_squared = dx * dx + dy * dy;
    speed = (unit->move_speed_m_per_tick > 0.0f ? unit->move_speed_m_per_tick : MK_DEFAULT_MOVE_SPEED_M_PER_TICK)
        * move_multiplier;
    if (unit->order == MK_ORDER_ASSAULT_MOVE) {
        speed *= 0.8f;
    } else if (unit->order == MK_ORDER_INVESTIGATE) {
        speed *= 0.6f;
    }
    step_squared = speed * speed;

    if (distance_squared <= step_squared || distance_squared <= 0.0001f) {
        unit->position_m = movement_target;
        if (unit->has_route) {
            if (unit->route_step_level_ids[unit->route_step_index][0] != '\0') {
                mk_copy_name(unit->level_id, unit->route_step_level_ids[unit->route_step_index]);
            }
            if (unit->route_step_vertical_transition[unit->route_step_index]) {
                unit->route_vertical_transitions_completed += 1;
            }

            unit->route_step_index += 1;
            unit->route_stuck_ticks = 0;
            unit->route_last_position_m = unit->position_m;
            if (unit->route_step_index < unit->route_step_count) {
                return;
            }
        }

        unit->position_m = unit->target_position_m;
        unit->has_move_target = false;
        mk_unit_clear_route(unit);
        unit->order = unit->order == MK_ORDER_WITHDRAW ? MK_ORDER_RALLY : MK_ORDER_HOLD;
        return;
    }

    distance = sqrtf(distance_squared);
    unit->position_m.x += dx / distance * speed;
    unit->position_m.y += dy / distance * speed;
    unit->facing_degrees = atan2f(dy, dx) * 57.2957795f;

    if (unit->has_route) {
        float moved_since_last = mk_vec2_distance(unit->route_last_position_m, unit->position_m);

        if (moved_since_last < 0.01f) {
            unit->route_stuck_ticks += 1;
        } else {
            unit->route_stuck_ticks = 0;
            unit->route_last_position_m = unit->position_m;
        }

        if (unit->route_stuck_ticks > 8U) {
            mk_unit_set_route_failure(unit, "stuck");
            unit->has_move_target = false;
            unit->order = MK_ORDER_RALLY;
        }
    }

    (void)game;
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

static const mk_spawn_zone_t *mk_scenario_find_spawn_zone(
    const mk_scenario_definition_t *scenario,
    const char *spawn_zone_id
) {
    size_t index;

    if (scenario == NULL || !mk_text_is_present(spawn_zone_id)) {
        return NULL;
    }

    for (index = 0; index < scenario->spawn_zone_count; ++index) {
        if (strcmp(scenario->spawn_zones[index].scenario_id, spawn_zone_id) == 0) {
            return &scenario->spawn_zones[index];
        }
    }

    return NULL;
}

static const mk_unit_template_t *mk_scenario_find_unit_template(
    const mk_scenario_definition_t *scenario,
    const char *template_id
) {
    size_t index;

    if (scenario == NULL || !mk_text_is_present(template_id)) {
        return NULL;
    }

    for (index = 0; index < scenario->unit_template_count; ++index) {
        if (strcmp(scenario->unit_templates[index].scenario_id, template_id) == 0) {
            return &scenario->unit_templates[index];
        }
    }

    return NULL;
}

static const mk_civilian_archetype_t *mk_scenario_find_civilian_archetype(
    const mk_scenario_definition_t *scenario,
    const char *archetype_id
) {
    size_t index;

    if (scenario == NULL || !mk_text_is_present(archetype_id)) {
        return NULL;
    }

    for (index = 0; index < scenario->civilian_archetype_count; ++index) {
        if (strcmp(scenario->civilian_archetypes[index].scenario_id, archetype_id) == 0) {
            return &scenario->civilian_archetypes[index];
        }
    }

    return NULL;
}

static const mk_civilian_group_t *mk_scenario_find_civilian_group(
    const mk_scenario_definition_t *scenario,
    const char *group_id
) {
    size_t index;

    if (scenario == NULL || !mk_text_is_present(group_id)) {
        return NULL;
    }

    for (index = 0; index < scenario->civilian_group_count; ++index) {
        if (strcmp(scenario->civilian_groups[index].scenario_id, group_id) == 0) {
            return &scenario->civilian_groups[index];
        }
    }

    return NULL;
}

static bool mk_scenario_topology_reference_is_valid(
    const mk_scenario_definition_t *scenario,
    const char *level_id,
    const char *topology_node_id,
    mk_vec2_t position_m,
    bool require_position_in_node
) {
    const mk_gameplay_topology_node_t *node;

    if (!mk_text_is_present(topology_node_id)) {
        return true;
    }

    if (scenario == NULL || !mk_gameplay_area_topology_is_loaded(&scenario->gameplay_area)) {
        return false;
    }

    node = mk_gameplay_area_find_topology_node(&scenario->gameplay_area, topology_node_id);
    if (node == NULL || !node->enterable) {
        return false;
    }

    if (mk_text_is_present(level_id) && strcmp(node->level_id, level_id) != 0) {
        return false;
    }

    if (require_position_in_node && !mk_rect_contains_point(node->bounds_m, position_m)) {
        return false;
    }

    return true;
}

static bool mk_scenario_spawn_zone_reference_is_valid(
    const mk_scenario_definition_t *scenario,
    const char *spawn_zone_id,
    mk_side_t side,
    mk_vec2_t position_m
) {
    const mk_spawn_zone_t *spawn_zone;

    if (!mk_text_is_present(spawn_zone_id)) {
        return true;
    }

    spawn_zone = mk_scenario_find_spawn_zone(scenario, spawn_zone_id);
    if (spawn_zone == NULL || !spawn_zone->active) {
        return false;
    }

    if (spawn_zone->side != MK_SIDE_NEUTRAL && spawn_zone->side != side) {
        return false;
    }

    return mk_rect_contains_point(spawn_zone->bounds_m, position_m);
}

static bool mk_scenario_is_valid(const mk_scenario_definition_t *scenario) {
    size_t index;

    if (scenario == NULL) {
        return false;
    }

    if (scenario->map.width_m <= 0.0f || scenario->map.height_m <= 0.0f) {
        return false;
    }

    if (!mk_gameplay_area_is_valid(&scenario->gameplay_area)) {
        return false;
    }

    if (scenario->gameplay_area.loaded
        && (!mk_float_close(scenario->gameplay_area.world_width_m, scenario->map.width_m)
            || !mk_float_close(scenario->gameplay_area.world_height_m, scenario->map.height_m))) {
        return false;
    }

    if (scenario->controller_count > MK_MAX_CONTROLLERS
        || scenario->faction_count > MK_MAX_FACTIONS
        || scenario->force_count > MK_MAX_FORCES
        || scenario->spawn_zone_count > MK_MAX_SCENARIO_SPAWN_ZONES
        || scenario->unit_template_count > MK_MAX_SCENARIO_UNIT_TEMPLATES
        || scenario->civilian_archetype_count > MK_MAX_SCENARIO_CIVILIAN_ARCHETYPES
        || scenario->civilian_group_count > MK_MAX_SCENARIO_CIVILIAN_GROUPS
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

    for (index = 0; index < scenario->spawn_zone_count; ++index) {
        const mk_spawn_zone_t *spawn_zone = &scenario->spawn_zones[index];
        size_t other_index;

        if (spawn_zone->id == 0
            || !mk_text_is_present(spawn_zone->scenario_id)
            || !mk_text_is_present(spawn_zone->name)
            || !mk_text_is_present(spawn_zone->kind)
            || spawn_zone->capacity < 0
            || !mk_rect_fits_map(spawn_zone->bounds_m, &scenario->map)
            || (mk_text_is_present(spawn_zone->level_id)
                && mk_gameplay_area_is_loaded(&scenario->gameplay_area)
                && mk_gameplay_area_find_level(&scenario->gameplay_area, spawn_zone->level_id) == NULL)
            || !mk_scenario_topology_reference_is_valid(
                scenario,
                spawn_zone->level_id,
                spawn_zone->topology_node_id,
                mk_rect_center(spawn_zone->bounds_m),
                false
            )) {
            return false;
        }

        for (other_index = index + 1; other_index < scenario->spawn_zone_count; ++other_index) {
            if (strcmp(spawn_zone->scenario_id, scenario->spawn_zones[other_index].scenario_id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < scenario->unit_template_count; ++index) {
        const mk_unit_template_t *unit_template = &scenario->unit_templates[index];
        const mk_spawn_zone_t *default_spawn_zone =
            mk_scenario_find_spawn_zone(scenario, unit_template->default_spawn_zone_id);
        mk_rect_t default_spawn_bounds = default_spawn_zone != NULL
            ? default_spawn_zone->bounds_m
            : mk_rect(0.0f, 0.0f, scenario->map.width_m, scenario->map.height_m);
        size_t other_index;

        if (unit_template->id == 0
            || !mk_text_is_present(unit_template->scenario_id)
            || !mk_text_is_present(unit_template->name)
            || !mk_text_is_present(unit_template->role)
            || unit_template->side == MK_SIDE_NEUTRAL
            || unit_template->expected_soldiers < 0
            || !mk_scenario_spawn_zone_reference_is_valid(
                scenario,
                unit_template->default_spawn_zone_id,
                unit_template->side,
                mk_rect_center(default_spawn_bounds)
            )) {
            return false;
        }

        for (other_index = index + 1; other_index < scenario->unit_template_count; ++other_index) {
            if (strcmp(unit_template->scenario_id, scenario->unit_templates[other_index].scenario_id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < scenario->civilian_archetype_count; ++index) {
        const mk_civilian_archetype_t *archetype = &scenario->civilian_archetypes[index];
        size_t other_index;

        if (archetype->id == 0
            || !mk_text_is_present(archetype->scenario_id)
            || !mk_text_is_present(archetype->name)
            || !mk_text_is_present(archetype->sprite_id)
            || archetype->baseline_stress < 0
            || archetype->baseline_risk < 0
            || archetype->compliance < 0) {
            return false;
        }

        for (other_index = index + 1; other_index < scenario->civilian_archetype_count; ++other_index) {
            if (strcmp(archetype->scenario_id, scenario->civilian_archetypes[other_index].scenario_id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < scenario->civilian_group_count; ++index) {
        const mk_civilian_group_t *group = &scenario->civilian_groups[index];
        const mk_spawn_zone_t *spawn_zone = mk_scenario_find_spawn_zone(scenario, group->spawn_zone_id);
        size_t other_index;

        if (group->id == 0
            || !mk_text_is_present(group->scenario_id)
            || !mk_text_is_present(group->name)
            || mk_scenario_find_civilian_archetype(scenario, group->archetype_id) == NULL
            || spawn_zone == NULL
            || (spawn_zone->side != MK_SIDE_NEUTRAL && spawn_zone->side != MK_SIDE_CIVILIAN)
            || group->expected_count < 0
            || group->baseline_stress < 0
            || group->compliance < 0
            || !mk_scenario_topology_reference_is_valid(
                scenario,
                group->level_id,
                group->topology_node_id,
                mk_rect_center(spawn_zone->bounds_m),
                false
            )) {
            return false;
        }

        for (other_index = index + 1; other_index < scenario->civilian_group_count; ++other_index) {
            if (strcmp(group->scenario_id, scenario->civilian_groups[other_index].scenario_id) == 0) {
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

    if (scenario->score_success_threshold < 0
        || scenario->score_partial_threshold < 0
        || scenario->score_objective_weight < 0
        || scenario->score_civilian_risk_weight < 0
        || scenario->score_player_casualty_weight < 0
        || scenario->score_civilian_casualty_weight < 0
        || scenario->score_time_weight < 0
        || (scenario->score_success_threshold > 0
            && scenario->score_partial_threshold > 0
            && scenario->score_success_threshold < scenario->score_partial_threshold)) {
        return false;
    }

    for (index = 0; index < scenario->civilian_count; ++index) {
        const mk_civilian_t *civilian = &scenario->civilians[index];

        if (civilian->id == 0
            || !mk_faction_id_exists(scenario, civilian->faction_id)
            || !mk_position_fits_map(civilian->position_m, &scenario->map)
            || civilian->stress < 0
            || civilian->risk < 0
            || civilian->compliance < 0
            || (mk_text_is_present(civilian->level_id)
                && mk_gameplay_area_is_loaded(&scenario->gameplay_area)
                && mk_gameplay_area_find_level(&scenario->gameplay_area, civilian->level_id) == NULL)
            || (mk_text_is_present(civilian->archetype_id)
                && mk_scenario_find_civilian_archetype(scenario, civilian->archetype_id) == NULL)
            || (mk_text_is_present(civilian->group_id)
                && mk_scenario_find_civilian_group(scenario, civilian->group_id) == NULL)
            || !mk_scenario_spawn_zone_reference_is_valid(
                scenario,
                civilian->spawn_zone_id,
                MK_SIDE_CIVILIAN,
                civilian->position_m
            )
            || !mk_scenario_topology_reference_is_valid(
                scenario,
                civilian->level_id,
                civilian->topology_node_id,
                civilian->position_m,
                true
            )) {
            return false;
        }
    }

    for (index = 0; index < scenario->unit_count; ++index) {
        const mk_unit_t *unit = &scenario->units[index];

        if (unit->id == 0
            || unit->soldier_count > MK_MAX_SOLDIERS_PER_UNIT
            || !mk_position_fits_map(unit->position_m, &scenario->map)
            || (unit->has_move_target && !mk_position_fits_map(unit->target_position_m, &scenario->map))
            || (unit->level_id[0] != '\0'
                && mk_gameplay_area_is_loaded(&scenario->gameplay_area)
                && mk_gameplay_area_find_level(&scenario->gameplay_area, unit->level_id) == NULL)
            || (mk_text_is_present(unit->template_id)
                && mk_scenario_find_unit_template(scenario, unit->template_id) == NULL)
            || !mk_scenario_spawn_zone_reference_is_valid(
                scenario,
                unit->spawn_zone_id,
                unit->side,
                unit->position_m
            )
            || !mk_scenario_topology_reference_is_valid(
                scenario,
                unit->level_id,
                unit->topology_node_id,
                unit->position_m,
                true
            )
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

bool mk_gameplay_area_is_loaded(const mk_gameplay_area_t *area) {
    return area != NULL && area->loaded;
}

mk_result_t mk_gameplay_area_world_to_pixel(
    const mk_gameplay_area_t *area,
    mk_vec2_t position_m,
    mk_ivec2_t *out_pixel
) {
    int pixel_x;
    int pixel_y;

    if (area == NULL || out_pixel == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!area->loaded
        || area->pixels_per_meter <= 0.0f
        || position_m.x < 0.0f
        || position_m.y < 0.0f
        || position_m.x > area->world_width_m
        || position_m.y > area->world_height_m) {
        return MK_ERROR_INVALID_DATA;
    }

    pixel_x = (int)floorf(position_m.x * area->pixels_per_meter);
    pixel_y = (int)floorf(position_m.y * area->pixels_per_meter);
    pixel_x = mk_clamp_int(pixel_x, 0, area->pixel_width - 1);
    pixel_y = mk_clamp_int(pixel_y, 0, area->pixel_height - 1);

    *out_pixel = mk_ivec2(pixel_x, pixel_y);
    return MK_OK;
}

mk_result_t mk_gameplay_area_pixel_to_world(
    const mk_gameplay_area_t *area,
    mk_ivec2_t pixel,
    mk_vec2_t *out_position_m
) {
    if (area == NULL || out_position_m == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!area->loaded
        || area->pixels_per_meter <= 0.0f
        || pixel.x < 0
        || pixel.y < 0
        || pixel.x >= area->pixel_width
        || pixel.y >= area->pixel_height) {
        return MK_ERROR_INVALID_DATA;
    }

    *out_position_m = mk_vec2((float)pixel.x / area->pixels_per_meter, (float)pixel.y / area->pixels_per_meter);
    return MK_OK;
}

const mk_gameplay_level_t *mk_gameplay_area_find_level(
    const mk_gameplay_area_t *area,
    const char *level_id
) {
    size_t index;

    if (area == NULL || level_id == NULL) {
        return NULL;
    }

    for (index = 0; index < area->level_count; ++index) {
        if (strcmp(area->levels[index].id, level_id) == 0) {
            return &area->levels[index];
        }
    }

    return NULL;
}

const mk_gameplay_feature_t *mk_gameplay_area_find_feature(
    const mk_gameplay_area_t *area,
    const char *feature_id
) {
    size_t index;

    if (area == NULL || feature_id == NULL) {
        return NULL;
    }

    for (index = 0; index < area->feature_count; ++index) {
        if (strcmp(area->features[index].id, feature_id) == 0) {
            return &area->features[index];
        }
    }

    return NULL;
}

bool mk_gameplay_area_feature_contains_pixel(
    const mk_gameplay_feature_t *feature,
    mk_ivec2_t pixel
) {
    if (feature == NULL) {
        return false;
    }

    return pixel.x >= feature->pixel_x
        && pixel.y >= feature->pixel_y
        && pixel.x < feature->pixel_x + feature->pixel_width
        && pixel.y < feature->pixel_y + feature->pixel_height;
}

bool mk_gameplay_area_feature_contains_world(
    const mk_gameplay_area_t *area,
    const mk_gameplay_feature_t *feature,
    mk_vec2_t position_m
) {
    mk_ivec2_t pixel;

    if (area == NULL || feature == NULL) {
        return false;
    }

    if (mk_gameplay_area_world_to_pixel(area, position_m, &pixel) != MK_OK) {
        return false;
    }

    return mk_gameplay_area_feature_contains_pixel(feature, pixel);
}

const mk_gameplay_feature_t *mk_gameplay_area_find_feature_at_world(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m
) {
    const mk_gameplay_feature_t *match = NULL;
    mk_ivec2_t pixel;
    size_t index;

    if (area == NULL || level_id == NULL) {
        return NULL;
    }

    if (mk_gameplay_area_world_to_pixel(area, position_m, &pixel) != MK_OK) {
        return NULL;
    }

    for (index = 0; index < area->feature_count; ++index) {
        const mk_gameplay_feature_t *feature = &area->features[index];

        if (strcmp(feature->level_id, level_id) == 0
            && mk_gameplay_area_feature_contains_pixel(feature, pixel)) {
            match = feature;
        }
    }

    return match;
}

const mk_gameplay_region_t *mk_gameplay_area_find_region(
    const mk_gameplay_area_t *area,
    const char *region_id
) {
    size_t index;

    if (area == NULL || region_id == NULL) {
        return NULL;
    }

    for (index = 0; index < area->region_count; ++index) {
        if (strcmp(area->regions[index].id, region_id) == 0) {
            return &area->regions[index];
        }
    }

    return NULL;
}

const mk_gameplay_region_t *mk_gameplay_area_find_region_at_world(
    const mk_gameplay_area_t *area,
    mk_vec2_t position_m
) {
    size_t index;

    if (area == NULL || !area->loaded) {
        return NULL;
    }

    for (index = 0; index < area->region_count; ++index) {
        const mk_gameplay_region_t *region = &area->regions[index];

        if (mk_rect_contains_point(region->bounds_m, position_m)) {
            return region;
        }
    }

    return NULL;
}

bool mk_gameplay_area_topology_is_loaded(const mk_gameplay_area_t *area) {
    return area != NULL && area->loaded && area->topology_loaded;
}

const mk_gameplay_topology_node_t *mk_gameplay_area_find_topology_node(
    const mk_gameplay_area_t *area,
    const char *node_id
) {
    size_t index;

    if (area == NULL || node_id == NULL) {
        return NULL;
    }

    for (index = 0; index < area->topology_node_count; ++index) {
        if (strcmp(area->topology_nodes[index].id, node_id) == 0) {
            return &area->topology_nodes[index];
        }
    }

    return NULL;
}

const mk_gameplay_topology_node_t *mk_gameplay_area_find_topology_node_at_world(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m
) {
    size_t index;

    if (area == NULL || level_id == NULL || !area->topology_loaded) {
        return NULL;
    }

    for (index = 0; index < area->topology_node_count; ++index) {
        const mk_gameplay_topology_node_t *node = &area->topology_nodes[index];

        if (strcmp(node->level_id, level_id) == 0 && mk_rect_contains_point(node->bounds_m, position_m)) {
            return node;
        }
    }

    return NULL;
}

const mk_gameplay_topology_portal_t *mk_gameplay_area_find_topology_portal(
    const mk_gameplay_area_t *area,
    const char *portal_id
) {
    size_t index;

    if (area == NULL || portal_id == NULL) {
        return NULL;
    }

    for (index = 0; index < area->topology_portal_count; ++index) {
        if (strcmp(area->topology_portals[index].id, portal_id) == 0) {
            return &area->topology_portals[index];
        }
    }

    return NULL;
}

const mk_gameplay_topology_portal_t *mk_gameplay_area_find_topology_portal_at_world(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m
) {
    size_t index;

    if (area == NULL || level_id == NULL || !area->topology_loaded) {
        return NULL;
    }

    for (index = 0; index < area->topology_portal_count; ++index) {
        const mk_gameplay_topology_portal_t *portal = &area->topology_portals[index];

        if (strcmp(portal->level_id, level_id) == 0 && mk_rect_contains_point(portal->bounds_m, position_m)) {
            return portal;
        }
    }

    return NULL;
}

const mk_gameplay_semantic_zone_t *mk_gameplay_area_find_semantic_zone(
    const mk_gameplay_area_t *area,
    const char *zone_id
) {
    size_t index;

    if (area == NULL || zone_id == NULL) {
        return NULL;
    }

    for (index = 0; index < area->semantic_zone_count; ++index) {
        if (strcmp(area->semantic_zones[index].id, zone_id) == 0) {
            return &area->semantic_zones[index];
        }
    }

    return NULL;
}

static const mk_gameplay_semantic_zone_t *mk_gameplay_area_find_best_semantic_zone_at_world(
    const mk_gameplay_area_t *area,
    mk_vec2_t position_m
) {
    const mk_gameplay_semantic_zone_t *best_zone = NULL;
    size_t index;

    if (area == NULL || !area->topology_loaded) {
        return NULL;
    }

    for (index = 0; index < area->semantic_zone_count; ++index) {
        const mk_gameplay_semantic_zone_t *zone = &area->semantic_zones[index];

        if (!mk_rect_contains_point(zone->bounds_m, position_m)) {
            continue;
        }

        if (best_zone == NULL || zone->priority > best_zone->priority) {
            best_zone = zone;
        }
    }

    return best_zone;
}

const mk_gameplay_semantic_zone_t *mk_gameplay_area_find_semantic_zone_at_world(
    const mk_gameplay_area_t *area,
    const char *kind,
    mk_vec2_t position_m
) {
    size_t index;

    if (area == NULL || !area->topology_loaded) {
        return NULL;
    }

    for (index = 0; index < area->semantic_zone_count; ++index) {
        const mk_gameplay_semantic_zone_t *zone = &area->semantic_zones[index];

        if ((kind == NULL || strcmp(zone->kind, kind) == 0)
            && mk_rect_contains_point(zone->bounds_m, position_m)) {
            return zone;
        }
    }

    return NULL;
}

static bool mk_gameplay_area_debug_append(char *out_text, size_t capacity, size_t *in_out_length, const char *format, ...) {
    va_list args;
    int written;
    size_t remaining;

    if (out_text == NULL || capacity == 0 || in_out_length == NULL || *in_out_length >= capacity) {
        return false;
    }

    remaining = capacity - *in_out_length;
    va_start(args, format);
    written = vsnprintf(out_text + *in_out_length, remaining, format, args);
    va_end(args);

    if (written < 0 || (size_t)written >= remaining) {
        out_text[capacity - 1] = '\0';
        return false;
    }

    *in_out_length += (size_t)written;
    return true;
}

mk_result_t mk_gameplay_area_topology_debug_dump(
    const mk_gameplay_area_t *area,
    char *out_text,
    size_t capacity
) {
    size_t length = 0;
    size_t index;
    size_t unreachable;

    if (area == NULL || out_text == NULL || capacity == 0) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    out_text[0] = '\0';
    if (!area->loaded || !area->topology_loaded) {
        return mk_gameplay_area_debug_append(out_text, capacity, &length, "topology=none\n")
            ? MK_OK
            : MK_ERROR_CAPACITY;
    }

    unreachable = mk_gameplay_area_topology_unreachable_count(area);
    if (!mk_gameplay_area_debug_append(
            out_text,
            capacity,
            &length,
            "topology id=\"%s\" nodes=%u portals=%u zones=%u unreachable=%u\n",
            area->topology_id,
            (unsigned)area->topology_node_count,
            (unsigned)area->topology_portal_count,
            (unsigned)area->semantic_zone_count,
            (unsigned)unreachable
        )) {
        return MK_ERROR_CAPACITY;
    }

    for (index = 0; index < area->topology_node_count; ++index) {
        const mk_gameplay_topology_node_t *node = &area->topology_nodes[index];

        if (!mk_gameplay_area_debug_append(
                out_text,
                capacity,
                &length,
                "node id=\"%s\" kind=%s level=%s region=\"%s\" enterable=%d\n",
                node->id,
                node->kind,
                node->level_id,
                node->region_id,
                node->enterable ? 1 : 0
            )) {
            return MK_ERROR_CAPACITY;
        }
    }

    return MK_OK;
}

mk_result_t mk_gameplay_area_query_tactical_at(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m,
    mk_gameplay_tactical_query_t *out_query
) {
    const mk_gameplay_level_t *level;
    const mk_gameplay_feature_t *active_feature = NULL;
    const mk_gameplay_topology_node_t *node = NULL;
    const mk_gameplay_topology_portal_t *portal = NULL;
    const mk_gameplay_semantic_zone_t *zone = NULL;
    mk_gameplay_tactical_query_t query;
    mk_ivec2_t pixel;
    bool los_blocked_by_feature = false;
    bool los_allowed_by_feature = false;
    bool movement_blocked_by_feature = false;
    bool movement_allowed_by_feature = false;
    int feature_cover = 0;
    int active_feature_cover = 0;
    int navigation_cost = 1;
    size_t index;

    if (area == NULL || level_id == NULL || out_query == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!area->loaded || mk_gameplay_area_world_to_pixel(area, position_m, &pixel) != MK_OK) {
        return MK_ERROR_INVALID_DATA;
    }

    level = mk_gameplay_area_find_level(area, level_id);
    if (level == NULL) {
        return MK_ERROR_INVALID_DATA;
    }

    memset(&query, 0, sizeof(query));
    query.in_bounds = true;
    mk_copy_name(query.level_id, level_id);
    query.position_m = position_m;
    query.pixel = pixel;
    query.elevation_m = level->elevation_m;

    for (index = 0; index < area->feature_count; ++index) {
        const mk_gameplay_feature_t *feature = &area->features[index];
        int cover;

        if (strcmp(feature->level_id, level_id) != 0 || !mk_gameplay_area_feature_contains_pixel(feature, pixel)) {
            continue;
        }

        if (feature->blocks_los) {
            los_blocked_by_feature = true;
        }

        if (feature->allows_los) {
            los_allowed_by_feature = true;
        }

        if (feature->blocks_movement) {
            movement_blocked_by_feature = true;
        }

        if (feature->allows_movement) {
            movement_allowed_by_feature = true;
        }

        cover = mk_gameplay_area_feature_cover_value(feature);
        if (active_feature == NULL
            || feature->allows_los
            || feature->allows_movement
            || cover > feature_cover) {
            active_feature = feature;
            active_feature_cover = cover;
        }

        if (cover > feature_cover) {
            feature_cover = cover;
        }
    }

    query.blocks_los = (level->blocks_los_default || los_blocked_by_feature) && !los_allowed_by_feature;
    query.blocks_movement = (level->blocks_movement_default || movement_blocked_by_feature) && !movement_allowed_by_feature;

    if (active_feature != NULL) {
        mk_copy_name(query.feature_id, active_feature->id);
        mk_copy_text(query.feature_kind, sizeof(query.feature_kind), active_feature->kind);
    }

    node = mk_gameplay_area_find_topology_node_at_world(area, level_id, position_m);
    if (node != NULL) {
        mk_copy_name(query.node_id, node->id);
        mk_copy_text(query.node_kind, sizeof(query.node_kind), node->kind);
        query.interior = mk_gameplay_area_node_is_interior(node);
        query.rooftop = strcmp(node->kind, "roof") == 0;
        if (!node->enterable) {
            query.blocks_movement = true;
        }
        navigation_cost = mk_gameplay_area_node_navigation_cost(node);
        query.cover = mk_max_int(query.cover, mk_gameplay_area_node_cover_value(node));
    }

    portal = mk_gameplay_area_find_topology_portal_at_world(area, level_id, position_m);
    if (portal != NULL) {
        mk_copy_name(query.portal_id, portal->id);
        mk_copy_text(query.portal_kind, sizeof(query.portal_kind), portal->kind);
        mk_copy_text(query.portal_state, sizeof(query.portal_state), portal->state);
        query.vertical_connector = portal->vertical;
        if (mk_gameplay_area_portal_blocks_movement(portal)) {
            query.blocks_movement = true;
        } else {
            navigation_cost += portal->movement_cost;
        }
    }

    zone = mk_gameplay_area_find_best_semantic_zone_at_world(area, position_m);
    if (zone != NULL) {
        mk_copy_name(query.semantic_zone_id, zone->id);
        mk_copy_text(query.semantic_zone_kind, sizeof(query.semantic_zone_kind), zone->kind);
        query.restricted_fire_lane = strcmp(zone->kind, "restricted_fire_lane") == 0;
        query.civilian_shelter = strcmp(zone->kind, "civilian_shelter") == 0;
        query.danger_area = strcmp(zone->kind, "danger_area") == 0;
        navigation_cost += mk_gameplay_area_zone_navigation_modifier(zone);
        query.cover = mk_max_int(query.cover, mk_gameplay_area_zone_cover_value(zone));
    }

    query.cover = mk_max_int(query.cover, active_feature_cover);
    if (active_feature != NULL) {
        navigation_cost += mk_gameplay_area_feature_navigation_modifier(active_feature);
    }

    if (query.blocks_movement || navigation_cost >= MK_GAMEPLAY_BLOCKED_NAVIGATION_COST) {
        query.navigation_cost = MK_GAMEPLAY_BLOCKED_NAVIGATION_COST;
    } else {
        query.navigation_cost = mk_max_int(1, navigation_cost);
    }

    *out_query = query;
    return MK_OK;
}

mk_result_t mk_gameplay_area_navigation_cost_at(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m,
    int *out_navigation_cost
) {
    mk_gameplay_tactical_query_t query;
    mk_result_t result;

    if (out_navigation_cost == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    result = mk_gameplay_area_query_tactical_at(area, level_id, position_m, &query);
    if (result != MK_OK) {
        return result;
    }

    *out_navigation_cost = query.navigation_cost;
    return MK_OK;
}

mk_result_t mk_gameplay_area_cover_at(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m,
    int *out_cover
) {
    mk_gameplay_tactical_query_t query;
    mk_result_t result;

    if (out_cover == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    result = mk_gameplay_area_query_tactical_at(area, level_id, position_m, &query);
    if (result != MK_OK) {
        return result;
    }

    *out_cover = query.cover;
    return MK_OK;
}

mk_result_t mk_gameplay_area_trace_line_of_sight(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t from_m,
    mk_vec2_t to_m,
    mk_gameplay_los_trace_t *out_trace
) {
    mk_gameplay_los_trace_t trace;
    mk_gameplay_tactical_query_t target_query;
    float distance_m;
    size_t sample_count;
    size_t sample_index;

    if (area == NULL || level_id == NULL || out_trace == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!area->loaded
        || mk_gameplay_area_find_level(area, level_id) == NULL
        || from_m.x < 0.0f
        || from_m.y < 0.0f
        || to_m.x < 0.0f
        || to_m.y < 0.0f
        || from_m.x > area->world_width_m
        || from_m.y > area->world_height_m
        || to_m.x > area->world_width_m
        || to_m.y > area->world_height_m) {
        return MK_ERROR_INVALID_DATA;
    }

    memset(&trace, 0, sizeof(trace));
    trace.visible = true;
    trace.distance_m = mk_distance(from_m, to_m);
    mk_copy_name(trace.level_id, level_id);

    if (mk_gameplay_area_query_tactical_at(area, level_id, to_m, &target_query) == MK_OK) {
        trace.cover = target_query.cover;
        mk_copy_name(trace.cover_feature_id, target_query.feature_id);
        mk_copy_name(trace.cover_node_id, target_query.node_id);
        mk_copy_name(trace.cover_zone_id, target_query.semantic_zone_id);
    }

    distance_m = trace.distance_m;
    sample_count = (size_t)(distance_m * 2.0f) + 1;
    if (sample_count < 1) {
        sample_count = 1;
    }
    if (sample_count > MK_GAMEPLAY_LOS_MAX_SAMPLES) {
        sample_count = MK_GAMEPLAY_LOS_MAX_SAMPLES;
    }
    trace.sample_count = sample_count;

    for (sample_index = 1; sample_index < sample_count; ++sample_index) {
        float t = (float)sample_index / (float)sample_count;
        mk_vec2_t sample;
        mk_gameplay_tactical_query_t query;

        sample.x = from_m.x + (to_m.x - from_m.x) * t;
        sample.y = from_m.y + (to_m.y - from_m.y) * t;

        if (mk_gameplay_area_query_tactical_at(area, level_id, sample, &query) != MK_OK) {
            continue;
        }

        if (query.blocks_los) {
            trace.visible = false;
            trace.blocked_by_feature = query.feature_id[0] != '\0';
            mk_copy_name(trace.blocking_feature_id, query.feature_id);
            mk_copy_text(trace.blocking_feature_kind, sizeof(trace.blocking_feature_kind), query.feature_kind);
            trace.blocking_position_m = sample;
            break;
        }
    }

    *out_trace = trace;
    return MK_OK;
}

static void mk_gameplay_route_set_blocked_reason(mk_gameplay_route_t *route, const char *reason) {
    if (route != NULL) {
        mk_copy_text(route->blocked_reason, sizeof(route->blocked_reason), reason);
    }
}

static bool mk_gameplay_route_position_is_walkable(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m,
    const char *blocked_reason,
    mk_gameplay_route_t *route
) {
    mk_gameplay_tactical_query_t query;

    if (mk_gameplay_area_query_tactical_at(area, level_id, position_m, &query) != MK_OK) {
        mk_gameplay_route_set_blocked_reason(route, blocked_reason);
        return false;
    }

    if (query.blocks_movement) {
        mk_gameplay_route_set_blocked_reason(route, blocked_reason);
        return false;
    }

    return true;
}

static bool mk_gameplay_area_portal_is_routeable(const mk_gameplay_topology_portal_t *portal) {
    return portal != NULL
        && portal->bidirectional
        && !mk_gameplay_area_portal_blocks_movement(portal);
}

static bool mk_gameplay_route_add_step(
    mk_gameplay_route_t *route,
    const mk_gameplay_topology_node_t *node,
    const mk_gameplay_topology_portal_t *portal,
    mk_vec2_t waypoint_m,
    int cumulative_cost
) {
    mk_gameplay_route_step_t *step;

    if (route == NULL || node == NULL || route->step_count >= MK_MAX_GAMEPLAY_ROUTE_STEPS) {
        return false;
    }

    step = &route->steps[route->step_count];
    memset(step, 0, sizeof(*step));
    mk_copy_name(step->node_id, node->id);
    mk_copy_name(step->level_id, node->level_id);
    step->waypoint_m = waypoint_m;
    step->cumulative_cost = cumulative_cost;
    if (portal != NULL) {
        mk_copy_name(step->portal_id, portal->id);
        step->vertical_transition = portal->vertical;
        if (portal->vertical) {
            route->uses_vertical_transition = true;
        }
    }

    route->step_count += 1;
    return true;
}

mk_result_t mk_gameplay_area_plan_route(
    const mk_gameplay_area_t *area,
    const char *start_level_id,
    mk_vec2_t start_m,
    const char *goal_level_id,
    mk_vec2_t goal_m,
    mk_gameplay_route_t *out_route
) {
    int distance[MK_MAX_GAMEPLAY_TOPOLOGY_NODES];
    bool visited[MK_MAX_GAMEPLAY_TOPOLOGY_NODES];
    size_t previous_node[MK_MAX_GAMEPLAY_TOPOLOGY_NODES];
    size_t previous_portal[MK_MAX_GAMEPLAY_TOPOLOGY_NODES];
    size_t node_path[MK_MAX_GAMEPLAY_TOPOLOGY_NODES];
    size_t path_count = 0;
    size_t start_index;
    size_t goal_index;
    size_t index;
    mk_gameplay_route_t route;

    if (area == NULL || start_level_id == NULL || goal_level_id == NULL || out_route == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(&route, 0, sizeof(route));
    route.start_m = start_m;
    route.goal_m = goal_m;
    mk_copy_name(route.start_level_id, start_level_id);
    mk_copy_name(route.goal_level_id, goal_level_id);

    if (!area->loaded
        || !area->topology_loaded
        || area->topology_node_count == 0
        || mk_gameplay_area_find_level(area, start_level_id) == NULL
        || mk_gameplay_area_find_level(area, goal_level_id) == NULL) {
        mk_gameplay_route_set_blocked_reason(&route, "missing_topology");
        *out_route = route;
        return MK_ERROR_INVALID_DATA;
    }

    if (!mk_gameplay_route_position_is_walkable(area, start_level_id, start_m, "blocked_start", &route)
        || !mk_gameplay_route_position_is_walkable(area, goal_level_id, goal_m, "blocked_goal", &route)) {
        *out_route = route;
        return MK_ERROR_INVALID_DATA;
    }

    if (!mk_gameplay_area_topology_node_index(
            area,
            mk_gameplay_area_find_topology_node_at_world(area, start_level_id, start_m) != NULL
                ? mk_gameplay_area_find_topology_node_at_world(area, start_level_id, start_m)->id
                : "",
            &start_index
        )) {
        mk_gameplay_route_set_blocked_reason(&route, "missing_start_node");
        *out_route = route;
        return MK_ERROR_NOT_FOUND;
    }

    if (!mk_gameplay_area_topology_node_index(
            area,
            mk_gameplay_area_find_topology_node_at_world(area, goal_level_id, goal_m) != NULL
                ? mk_gameplay_area_find_topology_node_at_world(area, goal_level_id, goal_m)->id
                : "",
            &goal_index
        )) {
        mk_gameplay_route_set_blocked_reason(&route, "missing_goal_node");
        *out_route = route;
        return MK_ERROR_NOT_FOUND;
    }

    if (!area->topology_nodes[start_index].enterable || !area->topology_nodes[goal_index].enterable) {
        mk_gameplay_route_set_blocked_reason(&route, "blocked_node");
        *out_route = route;
        return MK_ERROR_INVALID_DATA;
    }

    for (index = 0; index < MK_MAX_GAMEPLAY_TOPOLOGY_NODES; ++index) {
        distance[index] = MK_GAMEPLAY_BLOCKED_NAVIGATION_COST;
        visited[index] = false;
        previous_node[index] = (size_t)-1;
        previous_portal[index] = (size_t)-1;
    }

    distance[start_index] = 0;
    for (;;) {
        size_t current_index = (size_t)-1;
        int best_distance = MK_GAMEPLAY_BLOCKED_NAVIGATION_COST;
        size_t portal_index;

        for (index = 0; index < area->topology_node_count; ++index) {
            if (!visited[index] && distance[index] < best_distance) {
                best_distance = distance[index];
                current_index = index;
            }
        }

        if (current_index == (size_t)-1 || current_index == goal_index) {
            break;
        }

        visited[current_index] = true;
        for (portal_index = 0; portal_index < area->topology_portal_count; ++portal_index) {
            const mk_gameplay_topology_portal_t *portal = &area->topology_portals[portal_index];
            size_t from_index;
            size_t to_index;
            size_t next_index;
            int next_cost;

            if (!mk_gameplay_area_portal_is_routeable(portal)
                || !mk_gameplay_area_topology_node_index(area, portal->from_node_id, &from_index)
                || !mk_gameplay_area_topology_node_index(area, portal->to_node_id, &to_index)) {
                continue;
            }

            if (from_index == current_index) {
                next_index = to_index;
            } else if (to_index == current_index) {
                next_index = from_index;
            } else {
                continue;
            }

            if (visited[next_index] || !area->topology_nodes[next_index].enterable) {
                continue;
            }

            next_cost = distance[current_index]
                + portal->movement_cost
                + mk_gameplay_area_node_navigation_cost(&area->topology_nodes[next_index])
                + (portal->vertical ? 2 : 0);
            if (next_cost < distance[next_index]) {
                distance[next_index] = next_cost;
                previous_node[next_index] = current_index;
                previous_portal[next_index] = portal_index;
            }
        }
    }

    if (distance[goal_index] >= MK_GAMEPLAY_BLOCKED_NAVIGATION_COST) {
        mk_gameplay_route_set_blocked_reason(&route, "unreachable");
        *out_route = route;
        return MK_ERROR_NOT_FOUND;
    }

    for (index = goal_index; index != (size_t)-1; index = previous_node[index]) {
        if (path_count >= MK_MAX_GAMEPLAY_TOPOLOGY_NODES) {
            mk_gameplay_route_set_blocked_reason(&route, "route_capacity");
            *out_route = route;
            return MK_ERROR_CAPACITY;
        }

        node_path[path_count] = index;
        path_count += 1;
        if (index == start_index) {
            break;
        }
    }

    if (path_count == 0 || node_path[path_count - 1] != start_index) {
        mk_gameplay_route_set_blocked_reason(&route, "unreachable");
        *out_route = route;
        return MK_ERROR_NOT_FOUND;
    }

    mk_copy_name(route.start_node_id, area->topology_nodes[start_index].id);
    mk_copy_name(route.goal_node_id, area->topology_nodes[goal_index].id);
    route.total_cost = distance[goal_index];

    if (start_index == goal_index) {
        if (!mk_gameplay_route_add_step(&route, &area->topology_nodes[goal_index], NULL, goal_m, route.total_cost)) {
            mk_gameplay_route_set_blocked_reason(&route, "route_capacity");
            *out_route = route;
            return MK_ERROR_CAPACITY;
        }
    } else {
        size_t reverse_index;

        for (reverse_index = path_count - 1; reverse_index > 0; --reverse_index) {
            size_t to_index = node_path[reverse_index - 1];
            size_t portal_index = previous_portal[to_index];
            const mk_gameplay_topology_portal_t *portal;

            if (portal_index == (size_t)-1 || portal_index >= area->topology_portal_count) {
                mk_gameplay_route_set_blocked_reason(&route, "missing_portal");
                *out_route = route;
                return MK_ERROR_INVALID_DATA;
            }

            portal = &area->topology_portals[portal_index];
            if (!mk_gameplay_route_add_step(
                    &route,
                    &area->topology_nodes[to_index],
                    portal,
                    mk_rect_center(portal->bounds_m),
                    distance[to_index]
                )) {
                mk_gameplay_route_set_blocked_reason(&route, "route_capacity");
                *out_route = route;
                return MK_ERROR_CAPACITY;
            }
        }

        if (!mk_gameplay_route_add_step(&route, &area->topology_nodes[goal_index], NULL, goal_m, route.total_cost)) {
            mk_gameplay_route_set_blocked_reason(&route, "route_capacity");
            *out_route = route;
            return MK_ERROR_CAPACITY;
        }
    }

    route.valid = route.step_count > 0;
    *out_route = route;
    return route.valid ? MK_OK : MK_ERROR_NOT_FOUND;
}

static bool mk_gameplay_area_blocks_at(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m,
    bool check_line_of_sight
) {
    mk_gameplay_tactical_query_t query;

    if (mk_gameplay_area_query_tactical_at(area, level_id, position_m, &query) != MK_OK) {
        return false;
    }

    return check_line_of_sight ? query.blocks_los : query.blocks_movement;
}

bool mk_gameplay_area_blocks_los_at(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m
) {
    return mk_gameplay_area_blocks_at(area, level_id, position_m, true);
}

bool mk_gameplay_area_blocks_movement_at(
    const mk_gameplay_area_t *area,
    const char *level_id,
    mk_vec2_t position_m
) {
    return mk_gameplay_area_blocks_at(area, level_id, position_m, false);
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
    mk_game_update_investigation_contacts(game);

    for (unit_index = 0; unit_index < game->unit_count; ++unit_index) {
        mk_unit_t *unit = &game->units[unit_index];
        int recovery = mk_training_recovery(unit->training);
        size_t soldier_index;

        mk_update_unit_status(unit);

        if (unit->order == MK_ORDER_RALLY) {
            recovery += 2;
        }

        mk_update_unit_movement(game, unit);
        unit->suppression = mk_subtract_floor_zero(unit->suppression, recovery);

        for (soldier_index = 0; soldier_index < unit->soldier_count; ++soldier_index) {
            mk_soldier_t *soldier = &unit->soldiers[soldier_index];
            int soldier_recovery = soldier->casualty ? 0 : recovery;
            soldier->suppression = mk_subtract_floor_zero(soldier->suppression, soldier_recovery);
        }

        mk_update_unit_status(unit);
    }

    mk_game_update_investigation_contacts(game);
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
            float suspect_distance;
            float observer_distance;

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
            suspect_distance = reveal_distance + 55.0f;
            observer_distance = mk_distance(observer->position_m, hidden_unit->position_m);

            if (observer_distance > suspect_distance) {
                continue;
            }

            if (mk_game_unit_line_of_sight(game, observer->id, hidden_unit->id, &line_of_sight) == MK_OK
                && line_of_sight.visible
                && observer_distance <= reveal_distance) {
                mk_game_reveal_unit(game, hidden_unit, observer);
                break;
            }

            mk_game_record_suspected_contact(game, observer, hidden_unit, observer_distance, suspect_distance);
        }
    }

    mk_game_update_false_terrain_contacts(game);

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
            score.objective_points += objective_value * mk_game_score_objective_weight(game);
        }
    }

    score.civilian_risk = mk_game_total_civilian_risk(game);
    score.player_casualties = mk_game_side_casualties(game, MK_SIDE_PLAYER);
    score.opfor_casualties = mk_game_side_casualties(game, MK_SIDE_OPFOR);
    score.civilian_casualties = mk_game_side_casualties(game, MK_SIDE_CIVILIAN);
    score.civilian_risk_penalty = score.civilian_risk * mk_game_score_civilian_risk_weight(game);
    score.casualty_penalty = score.player_casualties * mk_game_score_player_casualty_weight(game)
        + score.civilian_casualties * mk_game_score_civilian_casualty_weight(game);
    score.time_penalty = (int)game->tick * mk_game_score_time_weight(game);
    score.total_score = score.objective_points
        - score.civilian_risk_penalty
        - score.casualty_penalty
        - score.time_penalty;

    if (score.controlled_objectives == 0 || score.total_score < mk_game_score_partial_threshold(game)) {
        score.outcome = MK_OUTCOME_PLAYER_FAILURE;
    } else if (score.total_score >= mk_game_score_success_threshold(game)
        && score.player_casualties == 0
        && score.civilian_casualties == 0
        && score.civilian_risk < 25) {
        score.outcome = MK_OUTCOME_PLAYER_SUCCESS;
    } else {
        score.outcome = MK_OUTCOME_PLAYER_PARTIAL;
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
        "outcome=%s score=%d thresholds(success=%d,partial=%d) objectives=%u contested=%u civilian_risk=%d casualties(player=%d,opfor=%d,civilian=%d) ticks=%u",
        mk_outcome_summary_name(report.score.outcome),
        report.score.total_score,
        mk_game_score_success_threshold(game),
        mk_game_score_partial_threshold(game),
        (unsigned)report.score.controlled_objectives,
        (unsigned)report.score.contested_objectives,
        report.score.civilian_risk,
        report.score.player_casualties,
        report.score.opfor_casualties,
        report.score.civilian_casualties,
        game->tick
    );

    if (report.score.outcome == MK_OUTCOME_PLAYER_SUCCESS) {
        mk_copy_scenario_text(report.narrative, game->after_action_success);
    } else if (report.score.outcome == MK_OUTCOME_PLAYER_PARTIAL) {
        mk_copy_scenario_text(report.narrative, game->after_action_partial);
    } else {
        mk_copy_scenario_text(report.narrative, game->after_action_failure);
    }

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
    mk_copy_scenario_text(out_snapshot->briefing, game->briefing);
    mk_copy_scenario_text(out_snapshot->after_action_success, game->after_action_success);
    mk_copy_scenario_text(out_snapshot->after_action_partial, game->after_action_partial);
    mk_copy_scenario_text(out_snapshot->after_action_failure, game->after_action_failure);
    out_snapshot->tick = game->tick;
    out_snapshot->rng_state = game->rng_state;
    out_snapshot->score_success_threshold = game->score_success_threshold;
    out_snapshot->score_partial_threshold = game->score_partial_threshold;
    out_snapshot->score_objective_weight = game->score_objective_weight;
    out_snapshot->score_civilian_risk_weight = game->score_civilian_risk_weight;
    out_snapshot->score_player_casualty_weight = game->score_player_casualty_weight;
    out_snapshot->score_civilian_casualty_weight = game->score_civilian_casualty_weight;
    out_snapshot->score_time_weight = game->score_time_weight;
    out_snapshot->selected_unit_id = game->selected_unit_id;
    out_snapshot->map = game->map;
    out_snapshot->gameplay_area = game->gameplay_area;
    out_snapshot->controller_count = game->controller_count;
    memcpy(out_snapshot->controllers, game->controllers, sizeof(game->controllers));
    out_snapshot->faction_count = game->faction_count;
    memcpy(out_snapshot->factions, game->factions, sizeof(game->factions));
    out_snapshot->force_count = game->force_count;
    memcpy(out_snapshot->forces, game->forces, sizeof(game->forces));
    out_snapshot->spawn_zone_count = game->spawn_zone_count;
    memcpy(out_snapshot->spawn_zones, game->spawn_zones, sizeof(game->spawn_zones));
    out_snapshot->unit_template_count = game->unit_template_count;
    memcpy(out_snapshot->unit_templates, game->unit_templates, sizeof(game->unit_templates));
    out_snapshot->civilian_archetype_count = game->civilian_archetype_count;
    memcpy(out_snapshot->civilian_archetypes, game->civilian_archetypes, sizeof(game->civilian_archetypes));
    out_snapshot->civilian_group_count = game->civilian_group_count;
    memcpy(out_snapshot->civilian_groups, game->civilian_groups, sizeof(game->civilian_groups));
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
    mk_copy_scenario_text(game->briefing, scenario->briefing);
    mk_copy_scenario_text(game->after_action_success, scenario->after_action_success);
    mk_copy_scenario_text(game->after_action_partial, scenario->after_action_partial);
    mk_copy_scenario_text(game->after_action_failure, scenario->after_action_failure);
    game->selected_unit_id = 0;
    game->score_success_threshold = scenario->score_success_threshold;
    game->score_partial_threshold = scenario->score_partial_threshold;
    game->score_objective_weight = scenario->score_objective_weight;
    game->score_civilian_risk_weight = scenario->score_civilian_risk_weight;
    game->score_player_casualty_weight = scenario->score_player_casualty_weight;
    game->score_civilian_casualty_weight = scenario->score_civilian_casualty_weight;
    game->score_time_weight = scenario->score_time_weight;
    game->map = scenario->map;
    game->gameplay_area = scenario->gameplay_area;
    game->controller_count = scenario->controller_count;
    memcpy(game->controllers, scenario->controllers, sizeof(scenario->controllers));
    game->faction_count = scenario->faction_count;
    memcpy(game->factions, scenario->factions, sizeof(scenario->factions));
    game->force_count = scenario->force_count;
    memcpy(game->forces, scenario->forces, sizeof(scenario->forces));
    game->spawn_zone_count = scenario->spawn_zone_count;
    memcpy(game->spawn_zones, scenario->spawn_zones, sizeof(scenario->spawn_zones));
    game->unit_template_count = scenario->unit_template_count;
    memcpy(game->unit_templates, scenario->unit_templates, sizeof(scenario->unit_templates));
    game->civilian_archetype_count = scenario->civilian_archetype_count;
    memcpy(game->civilian_archetypes, scenario->civilian_archetypes, sizeof(scenario->civilian_archetypes));
    game->civilian_group_count = scenario->civilian_group_count;
    memcpy(game->civilian_groups, scenario->civilian_groups, sizeof(scenario->civilian_groups));
    game->objective_count = scenario->objective_count;
    memcpy(game->objectives, scenario->objectives, sizeof(scenario->objectives));
    game->civilian_count = scenario->civilian_count;
    memcpy(game->civilians, scenario->civilians, sizeof(scenario->civilians));
    game->unit_count = scenario->unit_count;
    memcpy(game->units, scenario->units, sizeof(scenario->units));
    game->contact_report_count = 0;
    memset(game->contact_reports, 0, sizeof(game->contact_reports));

    for (index = 0; index < game->civilian_count; ++index) {
        if (game->civilians[index].level_id[0] == '\0') {
            mk_copy_name(game->civilians[index].level_id, mk_game_default_level_id(game));
        }
    }

    for (index = 0; index < game->unit_count; ++index) {
        if (game->units[index].level_id[0] == '\0') {
            mk_copy_name(game->units[index].level_id, mk_game_default_level_id(game));
        }
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

mk_result_t mk_game_pick_contact_at(
    const mk_game_t *game,
    mk_vec2_t position_m,
    float radius_m,
    uint32_t *out_contact_report_id
) {
    float pick_radius = radius_m > 0.0f ? radius_m : MK_UNIT_PICK_RADIUS_M;
    float best_distance_squared = pick_radius * pick_radius;
    uint32_t best_report_id = 0;
    size_t index;

    if (game == NULL || out_contact_report_id == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    *out_contact_report_id = 0;

    for (index = 0; index < game->contact_report_count; ++index) {
        const mk_contact_report_t *report = &game->contact_reports[index];
        float distance_squared;

        if (report->resolved
            || !report->visible
            || (report->kind != MK_CONTACT_REPORT_SUSPECTED_DANGER
                && report->kind != MK_CONTACT_REPORT_FALSE_CONTACT)) {
            continue;
        }

        distance_squared = mk_distance_squared(position_m, report->position_m);
        if (distance_squared <= best_distance_squared) {
            best_distance_squared = distance_squared;
            best_report_id = report->id;
        }
    }

    if (best_report_id == 0) {
        return MK_ERROR_NOT_FOUND;
    }

    *out_contact_report_id = best_report_id;
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

static mk_result_t mk_game_try_assign_unit_route(
    const mk_game_t *game,
    mk_unit_t *unit,
    const char *target_level_id,
    mk_vec2_t target_position_m,
    bool require_route
) {
    const char *start_level_id;
    const char *goal_level_id;
    mk_gameplay_route_t route;
    mk_result_t result;

    if (game == NULL || unit == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    mk_unit_clear_route(unit);
    if (!mk_gameplay_area_topology_is_loaded(&game->gameplay_area)) {
        return require_route ? MK_ERROR_INVALID_DATA : MK_OK;
    }

    start_level_id = mk_unit_current_level_id(game, unit);
    goal_level_id = target_level_id != NULL && target_level_id[0] != '\0' ? target_level_id : start_level_id;
    if (start_level_id[0] == '\0' || goal_level_id[0] == '\0') {
        return require_route ? MK_ERROR_INVALID_DATA : MK_OK;
    }

    result = mk_gameplay_area_plan_route(
        &game->gameplay_area,
        start_level_id,
        unit->position_m,
        goal_level_id,
        target_position_m,
        &route
    );
    if (result != MK_OK) {
        mk_copy_text(unit->route_failure_reason, sizeof(unit->route_failure_reason), route.blocked_reason);
        return require_route ? result : MK_OK;
    }

    if (!mk_unit_assign_route(unit, &route)) {
        mk_copy_text(unit->route_failure_reason, sizeof(unit->route_failure_reason), "route_capacity");
        return MK_ERROR_CAPACITY;
    }

    unit->route_request_id += 1;
    unit->route_stuck_ticks = 0;
    unit->route_vertical_transitions_completed = 0;
    unit->route_last_position_m = unit->position_m;
    mk_copy_text(unit->route_failure_reason, sizeof(unit->route_failure_reason), "");
    if (unit->level_id[0] == '\0') {
        mk_copy_name(unit->level_id, start_level_id);
    }

    return MK_OK;
}

static mk_result_t mk_game_issue_position_order(
    mk_game_t *game,
    uint32_t unit_id,
    mk_vec2_t target_position_m,
    mk_order_t order,
    const char *target_level_id,
    bool require_route
) {
    mk_unit_t *unit;
    mk_result_t route_result;

    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!mk_order_is_movement_order(order)) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!mk_position_fits_map(target_position_m, &game->map)) {
        return MK_ERROR_INVALID_DATA;
    }

    unit = mk_game_find_unit(game, unit_id);
    if (unit == NULL) {
        return MK_ERROR_NOT_FOUND;
    }

    route_result = mk_game_try_assign_unit_route(game, unit, target_level_id, target_position_m, require_route);
    if (route_result != MK_OK) {
        return route_result;
    }

    unit->target_position_m = target_position_m;
    unit->has_move_target = true;
    unit->order = order;

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
    if (!mk_order_is_movement_order(order)) {
        unit->has_move_target = false;
        mk_unit_clear_route(unit);
    }

    return MK_OK;
}

mk_result_t mk_game_issue_move_order(mk_game_t *game, uint32_t unit_id, mk_vec2_t target_position_m) {
    return mk_game_issue_position_order(game, unit_id, target_position_m, MK_ORDER_MOVE, NULL, false);
}

mk_result_t mk_game_issue_assault_move_order(mk_game_t *game, uint32_t unit_id, mk_vec2_t target_position_m) {
    return mk_game_issue_position_order(game, unit_id, target_position_m, MK_ORDER_ASSAULT_MOVE, NULL, false);
}

mk_result_t mk_game_issue_investigate_order(mk_game_t *game, uint32_t unit_id, mk_vec2_t target_position_m) {
    return mk_game_issue_position_order(game, unit_id, target_position_m, MK_ORDER_INVESTIGATE, NULL, false);
}

mk_result_t mk_game_issue_withdraw_order(mk_game_t *game, uint32_t unit_id, mk_vec2_t target_position_m) {
    return mk_game_issue_position_order(game, unit_id, target_position_m, MK_ORDER_WITHDRAW, NULL, false);
}

mk_result_t mk_game_issue_move_order_to_level(
    mk_game_t *game,
    uint32_t unit_id,
    const char *target_level_id,
    mk_vec2_t target_position_m
) {
    return mk_game_issue_position_order(game, unit_id, target_position_m, MK_ORDER_MOVE, target_level_id, true);
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

mk_result_t mk_game_issue_selected_investigate_order(mk_game_t *game, mk_vec2_t target_position_m) {
    if (game == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (game->selected_unit_id == 0) {
        return MK_ERROR_NOT_FOUND;
    }

    return mk_game_issue_investigate_order(game, game->selected_unit_id, target_position_m);
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
    if (mk_gameplay_area_is_loaded(&game->gameplay_area) && game->gameplay_area.level_count > 0) {
        mk_gameplay_los_trace_t gameplay_trace;
        const char *level_id = game->gameplay_area.levels[0].id;

        if (mk_gameplay_area_trace_line_of_sight(
                &game->gameplay_area,
                level_id,
                from_m,
                to_m,
                &gameplay_trace
            ) == MK_OK) {
            mk_copy_name(line_of_sight.level_id, gameplay_trace.level_id);

            if (gameplay_trace.cover > line_of_sight.cover) {
                line_of_sight.cover = gameplay_trace.cover;
                line_of_sight.cover_terrain_id = 0;
                line_of_sight.cover_terrain_kind = MK_TERRAIN_OPEN;
                mk_copy_name(line_of_sight.cover_feature_id, gameplay_trace.cover_feature_id);
                mk_copy_name(line_of_sight.cover_node_id, gameplay_trace.cover_node_id);
                mk_copy_name(line_of_sight.cover_zone_id, gameplay_trace.cover_zone_id);
            }

            if (!gameplay_trace.visible) {
                line_of_sight.visible = false;
                line_of_sight.blocked_by_gameplay_area = true;
                mk_copy_name(line_of_sight.blocking_feature_id, gameplay_trace.blocking_feature_id);
                mk_copy_text(
                    line_of_sight.blocking_feature_kind,
                    sizeof(line_of_sight.blocking_feature_kind),
                    gameplay_trace.blocking_feature_kind
                );
            }
        }
    }

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
    mk_copy_name(objective.label, name);
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
    civilian.compliance = 50;
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

mk_result_t mk_scenario_set_gameplay_area(mk_scenario_definition_t *scenario, const mk_gameplay_area_t *area) {
    if (scenario == NULL || area == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!mk_gameplay_area_is_valid(area)) {
        return MK_ERROR_INVALID_DATA;
    }

    if (area->loaded
        && (!mk_float_close(area->world_width_m, scenario->map.width_m)
            || !mk_float_close(area->world_height_m, scenario->map.height_m))) {
        return MK_ERROR_INVALID_DATA;
    }

    scenario->gameplay_area = *area;
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
