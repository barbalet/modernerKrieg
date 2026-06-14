#include "mk_asset_manifest.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char key[128];
    char value[MK_ASSET_PATH_CAPACITY];
} mk_asset_entry_t;

typedef struct {
    size_t count;
    mk_asset_entry_t entries[512];
} mk_asset_entry_list_t;

static void mk_asset_copy_text(char *destination, size_t capacity, const char *source) {
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

static char *mk_asset_trim(char *text) {
    char *end;

    if (text == NULL) {
        return NULL;
    }

    while (isspace((unsigned char)*text)) {
        text += 1;
    }

    if (*text == '\0') {
        return text;
    }

    end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) {
        *end = '\0';
        end -= 1;
    }

    return text;
}

static bool mk_asset_path_is_safe(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return false;
    }

    if (path[0] == '/' || strstr(path, "..") != NULL || strstr(path, "\\") != NULL) {
        return false;
    }

    return strncmp(path, "assets/mosul/", 13) == 0
        || strncmp(path, "assets/fallujah/", 16) == 0;
}

static bool mk_asset_file_exists(const char *project_root, const char *relative_path) {
    char full_path[512];
    FILE *file;

    if (project_root == NULL || relative_path == NULL || !mk_asset_path_is_safe(relative_path)) {
        return false;
    }

    if (snprintf(full_path, sizeof(full_path), "%s/%s", project_root, relative_path) >= (int)sizeof(full_path)) {
        return false;
    }

    file = fopen(full_path, "rb");
    if (file == NULL) {
        return false;
    }

    fclose(file);
    return true;
}

static mk_result_t mk_asset_read_entries(const char *manifest_path, mk_asset_entry_list_t *entries) {
    FILE *file;
    char line[512];

    if (manifest_path == NULL || entries == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(entries, 0, sizeof(*entries));
    file = fopen(manifest_path, "r");
    if (file == NULL) {
        return MK_ERROR_NOT_FOUND;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *trimmed = mk_asset_trim(line);
        char *equals;
        char *key;
        char *value;

        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }

        equals = strchr(trimmed, '=');
        if (equals == NULL) {
            fclose(file);
            return MK_ERROR_INVALID_DATA;
        }

        *equals = '\0';
        key = mk_asset_trim(trimmed);
        value = mk_asset_trim(equals + 1);

        if (key[0] == '\0' || value[0] == '\0' || entries->count >= sizeof(entries->entries) / sizeof(entries->entries[0])) {
            fclose(file);
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_copy_text(entries->entries[entries->count].key, sizeof(entries->entries[entries->count].key), key);
        mk_asset_copy_text(entries->entries[entries->count].value, sizeof(entries->entries[entries->count].value), value);
        entries->count += 1;
    }

    fclose(file);
    return MK_OK;
}

static mk_result_t mk_asset_read_text_file(const char *path, char **out_text) {
    FILE *file;
    long length;
    char *text;
    size_t read_count;

    if (path == NULL || out_text == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    *out_text = NULL;
    file = fopen(path, "rb");
    if (file == NULL) {
        return MK_ERROR_NOT_FOUND;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return MK_ERROR_INVALID_DATA;
    }

    length = ftell(file);
    if (length < 0) {
        fclose(file);
        return MK_ERROR_INVALID_DATA;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return MK_ERROR_INVALID_DATA;
    }

    text = (char *)malloc((size_t)length + 1U);
    if (text == NULL) {
        fclose(file);
        return MK_ERROR_CAPACITY;
    }

    read_count = fread(text, 1, (size_t)length, file);
    fclose(file);
    if (read_count != (size_t)length) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    text[length] = '\0';
    *out_text = text;
    return MK_OK;
}

static const char *mk_asset_entry_value(const mk_asset_entry_list_t *entries, const char *key) {
    size_t index;

    if (entries == NULL || key == NULL) {
        return NULL;
    }

    for (index = 0; index < entries->count; ++index) {
        if (strcmp(entries->entries[index].key, key) == 0) {
            return entries->entries[index].value;
        }
    }

    return NULL;
}

static bool mk_asset_required_text(
    const mk_asset_entry_list_t *entries,
    const char *key,
    char *out_text,
    size_t capacity
) {
    const char *value = mk_asset_entry_value(entries, key);

    if (value == NULL || value[0] == '\0') {
        return false;
    }

    mk_asset_copy_text(out_text, capacity, value);
    return true;
}

static bool mk_asset_required_int(const mk_asset_entry_list_t *entries, const char *key, int *out_value) {
    const char *value = mk_asset_entry_value(entries, key);
    char *end = NULL;
    long parsed;

    if (value == NULL || out_value == NULL) {
        return false;
    }

    parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0') {
        return false;
    }

    *out_value = (int)parsed;
    return true;
}

static bool mk_asset_optional_path(
    const mk_asset_entry_list_t *entries,
    const char *key,
    const char *project_root,
    bool must_exist,
    char *out_text,
    size_t capacity
) {
    const char *value = mk_asset_entry_value(entries, key);

    if (value == NULL) {
        return true;
    }

    if (value[0] == '\0' || !mk_asset_path_is_safe(value)) {
        return false;
    }

    if (must_exist && !mk_asset_file_exists(project_root, value)) {
        return false;
    }

    mk_asset_copy_text(out_text, capacity, value);
    return true;
}

static bool mk_asset_optional_count(const mk_asset_entry_list_t *entries, const char *key, size_t *out_count) {
    const char *value = mk_asset_entry_value(entries, key);
    char *end = NULL;
    long parsed;

    if (value == NULL) {
        return true;
    }

    parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < 0) {
        return false;
    }

    *out_count = (size_t)parsed;
    return true;
}

static const char *mk_asset_json_skip_whitespace(const char *cursor, const char *limit) {
    while (cursor < limit && isspace((unsigned char)*cursor)) {
        cursor += 1;
    }

    return cursor;
}

static const char *mk_asset_json_skip_string(const char *cursor, const char *limit) {
    if (cursor >= limit || *cursor != '"') {
        return NULL;
    }

    cursor += 1;
    while (cursor < limit) {
        if (*cursor == '\\') {
            cursor += 2;
            continue;
        }

        if (*cursor == '"') {
            return cursor + 1;
        }

        cursor += 1;
    }

    return NULL;
}

static const char *mk_asset_json_find_matching(
    const char *cursor,
    const char *limit,
    char open,
    char close
) {
    int depth = 0;

    if (cursor >= limit || *cursor != open) {
        return NULL;
    }

    while (cursor < limit) {
        if (*cursor == '"') {
            cursor = mk_asset_json_skip_string(cursor, limit);
            if (cursor == NULL) {
                return NULL;
            }
            continue;
        }

        if (*cursor == open) {
            depth += 1;
        } else if (*cursor == close) {
            depth -= 1;
            if (depth == 0) {
                return cursor;
            }
        }

        cursor += 1;
    }

    return NULL;
}

static const char *mk_asset_json_find_key(
    const char *start,
    const char *limit,
    const char *key
) {
    char pattern[96];
    size_t pattern_length;
    const char *cursor = start;

    if (snprintf(pattern, sizeof(pattern), "\"%s\"", key) >= (int)sizeof(pattern)) {
        return NULL;
    }

    pattern_length = strlen(pattern);
    while (cursor < limit) {
        const char *found = strstr(cursor, pattern);
        const char *after_key;

        if (found == NULL || found >= limit) {
            return NULL;
        }

        after_key = found + pattern_length;
        if (after_key < limit) {
            after_key = mk_asset_json_skip_whitespace(after_key, limit);
        }

        if (after_key < limit && *after_key == ':') {
            return mk_asset_json_skip_whitespace(after_key + 1, limit);
        }

        cursor = found + 1;
    }

    return NULL;
}

static bool mk_asset_json_required_int(
    const char *start,
    const char *limit,
    const char *key,
    int *out_value
) {
    const char *value = mk_asset_json_find_key(start, limit, key);
    char *end = NULL;
    long parsed;

    if (value == NULL || out_value == NULL) {
        return false;
    }

    parsed = strtol(value, &end, 10);
    if (end == (char *)value || end > limit) {
        return false;
    }

    *out_value = (int)parsed;
    return true;
}

static bool mk_asset_json_required_float(
    const char *start,
    const char *limit,
    const char *key,
    float *out_value
) {
    const char *value = mk_asset_json_find_key(start, limit, key);
    char *end = NULL;
    float parsed;

    if (value == NULL || out_value == NULL) {
        return false;
    }

    parsed = strtof(value, &end);
    if (end == (char *)value || end > limit) {
        return false;
    }

    *out_value = parsed;
    return true;
}

static bool mk_asset_json_required_bool(
    const char *start,
    const char *limit,
    const char *key,
    bool *out_value
) {
    const char *value = mk_asset_json_find_key(start, limit, key);

    if (value == NULL || out_value == NULL) {
        return false;
    }

    if (value + 4 <= limit && strncmp(value, "true", 4) == 0) {
        *out_value = true;
        return true;
    }

    if (value + 5 <= limit && strncmp(value, "false", 5) == 0) {
        *out_value = false;
        return true;
    }

    return false;
}

static bool mk_asset_json_required_string(
    const char *start,
    const char *limit,
    const char *key,
    bool allow_empty,
    char *out_text,
    size_t capacity
) {
    const char *value = mk_asset_json_find_key(start, limit, key);
    const char *cursor;
    size_t copied = 0;

    if (value == NULL || out_text == NULL || capacity == 0 || value >= limit || *value != '"') {
        return false;
    }

    cursor = value + 1;
    while (cursor < limit && *cursor != '"') {
        if (*cursor == '\\' || copied + 1 >= capacity) {
            return false;
        }

        out_text[copied] = *cursor;
        copied += 1;
        cursor += 1;
    }

    if (cursor >= limit || *cursor != '"') {
        return false;
    }

    out_text[copied] = '\0';
    return allow_empty || copied > 0;
}

static bool mk_asset_angle_is_valid(const char *angle) {
    return strcmp(angle, "north") == 0
        || strcmp(angle, "south") == 0
        || strcmp(angle, "east") == 0
        || strcmp(angle, "west") == 0
        || strcmp(angle, "north_east") == 0
        || strcmp(angle, "north_west") == 0
        || strcmp(angle, "south_east") == 0
        || strcmp(angle, "south_west") == 0;
}

static bool mk_asset_required_float(const mk_asset_entry_list_t *entries, const char *key, float *out_value) {
    const char *value = mk_asset_entry_value(entries, key);
    char *end = NULL;
    float parsed;

    if (value == NULL || out_value == NULL) {
        return false;
    }

    parsed = strtof(value, &end);
    if (end == value || *end != '\0') {
        return false;
    }

    *out_value = parsed;
    return true;
}

static bool mk_asset_required_color(const mk_asset_entry_list_t *entries, const char *key, mk_color_t *out_color) {
    const char *value = mk_asset_entry_value(entries, key);
    int r;
    int g;
    int b;
    int a;
    char trailing;

    if (value == NULL || out_color == NULL) {
        return false;
    }

    if (sscanf(value, " %d , %d , %d , %d %c", &r, &g, &b, &a, &trailing) != 4) {
        return false;
    }

    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255) {
        return false;
    }

    *out_color = mk_make_color((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
    return true;
}

static void mk_asset_make_indexed_key(char *out_key, size_t capacity, const char *prefix, size_t index, const char *field) {
    (void)snprintf(out_key, capacity, "%s.%u.%s", prefix, (unsigned)index, field);
}

static bool mk_asset_map_layer_ids_are_unique(const mk_asset_map_manifest_t *manifest) {
    size_t index;

    for (index = 0; index < manifest->layer_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < manifest->layer_count; ++other_index) {
            if (strcmp(manifest->layers[index].id, manifest->layers[other_index].id) == 0) {
                return false;
            }
        }
    }

    return true;
}

static bool mk_asset_sprite_ids_are_unique(const mk_asset_sprite_manifest_t *manifest) {
    size_t index;

    for (index = 0; index < manifest->sheet_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < manifest->sheet_count; ++other_index) {
            if (strcmp(manifest->sheets[index].id, manifest->sheets[other_index].id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < manifest->frame_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < manifest->frame_count; ++other_index) {
            if (strcmp(manifest->frames[index].runtime_id, manifest->frames[other_index].runtime_id) == 0) {
                return false;
            }
        }
    }

    return true;
}

static bool mk_asset_marker_ids_are_unique(const mk_asset_marker_manifest_t *manifest) {
    size_t index;

    for (index = 0; index < manifest->marker_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < manifest->marker_count; ++other_index) {
            if (strcmp(manifest->markers[index].id, manifest->markers[other_index].id) == 0) {
                return false;
            }
        }
    }

    return true;
}

static bool mk_asset_sprite_render_paths_are_unique(const mk_asset_sprite_render_manifest_t *manifest) {
    size_t index;

    for (index = 0; index < manifest->rendered_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < manifest->rendered_count; ++other_index) {
            if (strcmp(manifest->rendered[index].path, manifest->rendered[other_index].path) == 0) {
                return false;
            }
        }
    }

    return true;
}

static bool mk_asset_building_ids_are_unique(const mk_asset_building_level_manifest_t *manifest) {
    size_t index;

    for (index = 0; index < manifest->level_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < manifest->level_count; ++other_index) {
            if (strcmp(manifest->levels[index].id, manifest->levels[other_index].id) == 0
                || manifest->levels[index].index == manifest->levels[other_index].index) {
                return false;
            }
        }
    }

    for (index = 0; index < manifest->feature_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < manifest->feature_count; ++other_index) {
            if (strcmp(manifest->features[index].id, manifest->features[other_index].id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < manifest->region_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < manifest->region_count; ++other_index) {
            if (strcmp(manifest->regions[index].id, manifest->regions[other_index].id) == 0) {
                return false;
            }
        }
    }

    return true;
}

static bool mk_asset_building_alpha_is_valid(const char *alpha) {
    return strcmp(alpha, "opaque") == 0 || strcmp(alpha, "overlay") == 0;
}

static bool mk_asset_building_feature_kind_is_valid(const char *kind) {
    return strcmp(kind, "wall") == 0
        || strcmp(kind, "door") == 0
        || strcmp(kind, "window") == 0
        || strcmp(kind, "breach_hole") == 0
        || strcmp(kind, "stair") == 0
        || strcmp(kind, "roof_edge") == 0;
}

static bool mk_asset_building_rect_is_valid(
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
        && x <= pixel_width - width
        && y <= pixel_height - height;
}

static bool mk_asset_topology_node_kind_is_valid(const char *kind) {
    return strcmp(kind, "street") == 0
        || strcmp(kind, "alley") == 0
        || strcmp(kind, "shop") == 0
        || strcmp(kind, "courtyard") == 0
        || strcmp(kind, "roof") == 0
        || strcmp(kind, "stairwell") == 0
        || strcmp(kind, "cache") == 0
        || strcmp(kind, "shelter") == 0
        || strcmp(kind, "blocked_building") == 0
        || strcmp(kind, "mosque") == 0
        || strcmp(kind, "workshop") == 0
        || strcmp(kind, "garage") == 0
        || strcmp(kind, "office") == 0;
}

static bool mk_asset_topology_portal_kind_is_valid(const char *kind) {
    return strcmp(kind, "door") == 0
        || strcmp(kind, "window") == 0
        || strcmp(kind, "breach_hole") == 0
        || strcmp(kind, "archway") == 0
        || strcmp(kind, "stair") == 0
        || strcmp(kind, "ladder") == 0
        || strcmp(kind, "roof_edge") == 0
        || strcmp(kind, "street_crossing") == 0
        || strcmp(kind, "rubble_passage") == 0;
}

static bool mk_asset_topology_portal_state_is_valid(const char *state) {
    return strcmp(state, "open") == 0
        || strcmp(state, "closed") == 0
        || strcmp(state, "locked") == 0
        || strcmp(state, "blocked") == 0
        || strcmp(state, "breached") == 0
        || strcmp(state, "searched") == 0
        || strcmp(state, "compromised") == 0
        || strcmp(state, "unsafe") == 0;
}

static bool mk_asset_semantic_zone_kind_is_valid(const char *kind) {
    return strcmp(kind, "civilian_shelter") == 0
        || strcmp(kind, "evacuation_exit") == 0
        || strcmp(kind, "market_crowd") == 0
        || strcmp(kind, "cache") == 0
        || strcmp(kind, "overwatch_roof") == 0
        || strcmp(kind, "search_objective") == 0
        || strcmp(kind, "restricted_fire_lane") == 0
        || strcmp(kind, "danger_area") == 0;
}

static bool mk_asset_topology_node_index(
    const mk_asset_topology_manifest_t *manifest,
    const char *node_id,
    size_t *out_index
) {
    size_t index;

    if (manifest == NULL || node_id == NULL) {
        return false;
    }

    for (index = 0; index < manifest->node_count; ++index) {
        if (strcmp(manifest->nodes[index].id, node_id) == 0) {
            if (out_index != NULL) {
                *out_index = index;
            }
            return true;
        }
    }

    return false;
}

static bool mk_asset_topology_ids_are_unique(const mk_asset_topology_manifest_t *manifest) {
    size_t index;

    if (manifest == NULL) {
        return false;
    }

    for (index = 0; index < manifest->node_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < manifest->node_count; ++other_index) {
            if (strcmp(manifest->nodes[index].id, manifest->nodes[other_index].id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < manifest->portal_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < manifest->portal_count; ++other_index) {
            if (strcmp(manifest->portals[index].id, manifest->portals[other_index].id) == 0) {
                return false;
            }
        }
    }

    for (index = 0; index < manifest->zone_count; ++index) {
        size_t other_index;

        for (other_index = index + 1; other_index < manifest->zone_count; ++other_index) {
            if (strcmp(manifest->zones[index].id, manifest->zones[other_index].id) == 0) {
                return false;
            }
        }
    }

    return true;
}

static bool mk_asset_topology_region_is_covered(
    const mk_asset_topology_manifest_t *manifest,
    const char *region_id
) {
    size_t index;

    if (manifest == NULL || region_id == NULL || region_id[0] == '\0') {
        return false;
    }

    for (index = 0; index < manifest->node_count; ++index) {
        if (strcmp(manifest->nodes[index].region_id, region_id) == 0) {
            return true;
        }
    }

    return false;
}

static size_t mk_asset_topology_unreachable_count(const mk_asset_topology_manifest_t *manifest) {
    bool visited[MK_ASSET_MAX_TOPOLOGY_NODES];
    size_t queue[MK_ASSET_MAX_TOPOLOGY_NODES];
    size_t start_index = (size_t)-1;
    size_t queue_read = 0;
    size_t queue_write = 0;
    size_t unreachable = 0;
    size_t index;

    if (manifest == NULL || manifest->node_count == 0) {
        return 0;
    }

    memset(visited, 0, sizeof(visited));
    for (index = 0; index < manifest->node_count; ++index) {
        if (manifest->nodes[index].enterable) {
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

        for (index = 0; index < manifest->portal_count; ++index) {
            const mk_asset_topology_portal_t *portal = &manifest->portals[index];
            size_t from_index;
            size_t to_index;
            size_t next_index;

            if (!portal->bidirectional
                || strcmp(portal->state, "blocked") == 0
                || strcmp(portal->state, "locked") == 0
                || strcmp(portal->state, "unsafe") == 0
                || !mk_asset_topology_node_index(manifest, portal->from_node_id, &from_index)
                || !mk_asset_topology_node_index(manifest, portal->to_node_id, &to_index)) {
                continue;
            }

            if (from_index == current_index) {
                next_index = to_index;
            } else if (to_index == current_index) {
                next_index = from_index;
            } else {
                continue;
            }

            if (!manifest->nodes[next_index].enterable || visited[next_index]) {
                continue;
            }

            visited[next_index] = true;
            queue[queue_write] = next_index;
            queue_write += 1;
        }
    }

    for (index = 0; index < manifest->node_count; ++index) {
        if (manifest->nodes[index].enterable && !visited[index]) {
            unreachable += 1;
        }
    }

    return unreachable;
}

static bool mk_asset_topology_is_valid(
    const mk_asset_topology_manifest_t *manifest,
    const mk_asset_building_level_manifest_t *building_manifest
) {
    size_t index;

    if (manifest == NULL || building_manifest == NULL) {
        return false;
    }

    if (manifest->schema_version <= 0
        || strcmp(manifest->map_id, building_manifest->map_id) != 0
        || strcmp(manifest->gameplay_area_id, building_manifest->id) != 0
        || manifest->node_count == 0
        || manifest->node_count > MK_ASSET_MAX_TOPOLOGY_NODES
        || manifest->portal_count > MK_ASSET_MAX_TOPOLOGY_PORTALS
        || manifest->zone_count > MK_ASSET_MAX_SEMANTIC_ZONES
        || !mk_asset_topology_ids_are_unique(manifest)) {
        return false;
    }

    for (index = 0; index < manifest->node_count; ++index) {
        const mk_asset_topology_node_t *node = &manifest->nodes[index];

        if (node->id[0] == '\0'
            || !mk_asset_topology_node_kind_is_valid(node->kind)
            || mk_asset_find_building_level(building_manifest, node->level_id) == NULL
            || (node->region_id[0] != '\0' && mk_asset_find_building_region(building_manifest, node->region_id) == NULL)
            || !mk_asset_building_rect_is_valid(
                node->x,
                node->y,
                node->width,
                node->height,
                building_manifest->pixel_width,
                building_manifest->pixel_height
            )) {
            return false;
        }
    }

    for (index = 0; index < building_manifest->region_count; ++index) {
        if (!mk_asset_topology_region_is_covered(manifest, building_manifest->regions[index].id)) {
            return false;
        }
    }

    for (index = 0; index < manifest->portal_count; ++index) {
        const mk_asset_topology_portal_t *portal = &manifest->portals[index];
        const mk_asset_topology_node_t *from_node;
        const mk_asset_topology_node_t *to_node;
        const mk_asset_building_feature_t *feature;
        size_t from_index;
        size_t to_index;

        if (portal->id[0] == '\0'
            || !mk_asset_topology_portal_kind_is_valid(portal->kind)
            || !mk_asset_topology_portal_state_is_valid(portal->state)
            || !portal->bidirectional
            || portal->movement_cost <= 0
            || mk_asset_find_building_level(building_manifest, portal->level_id) == NULL
            || !mk_asset_topology_node_index(manifest, portal->from_node_id, &from_index)
            || !mk_asset_topology_node_index(manifest, portal->to_node_id, &to_index)
            || from_index == to_index
            || !mk_asset_building_rect_is_valid(
                portal->x,
                portal->y,
                portal->width,
                portal->height,
                building_manifest->pixel_width,
                building_manifest->pixel_height
            )) {
            return false;
        }

        from_node = &manifest->nodes[from_index];
        to_node = &manifest->nodes[to_index];
        if (portal->vertical) {
            if (strcmp(from_node->level_id, to_node->level_id) == 0) {
                return false;
            }
        } else if (strcmp(from_node->level_id, to_node->level_id) != 0) {
            return false;
        }

        if (portal->feature_id[0] != '\0') {
            feature = mk_asset_find_building_feature(building_manifest, portal->feature_id);
            if (feature == NULL || strcmp(feature->level_id, portal->level_id) != 0) {
                return false;
            }
        }
    }

    for (index = 0; index < manifest->zone_count; ++index) {
        const mk_asset_semantic_zone_t *zone = &manifest->zones[index];
        const mk_asset_topology_node_t *node;
        size_t node_index;

        if (zone->id[0] == '\0'
            || !mk_asset_semantic_zone_kind_is_valid(zone->kind)
            || mk_asset_find_building_level(building_manifest, zone->level_id) == NULL
            || !mk_asset_topology_node_index(manifest, zone->node_id, &node_index)
            || zone->priority < 0
            || !mk_asset_building_rect_is_valid(
                zone->x,
                zone->y,
                zone->width,
                zone->height,
                building_manifest->pixel_width,
                building_manifest->pixel_height
            )) {
            return false;
        }

        node = &manifest->nodes[node_index];
        if (strcmp(node->level_id, zone->level_id) != 0) {
            return false;
        }
    }

    return mk_asset_topology_unreachable_count(manifest) == 0;
}

static bool mk_asset_sprite_render_entry_is_valid(
    const mk_asset_sprite_render_entry_t *entry,
    const char *project_root
) {
    if (entry == NULL
        || entry->path[0] == '\0'
        || entry->kind[0] == '\0'
        || entry->item_id[0] == '\0'
        || entry->angle[0] == '\0'
        || !mk_asset_angle_is_valid(entry->angle)
        || !mk_asset_file_exists(project_root, entry->path)) {
        return false;
    }

    if (strcmp(entry->kind, "infantry") == 0
        || strcmp(entry->kind, "civilian") == 0
        || strcmp(entry->kind, "vehicle") == 0) {
        return entry->state[0] != '\0' && entry->faction[0] != '\0';
    }

    if (strcmp(entry->kind, "weapon") == 0) {
        return entry->state[0] == '\0' && entry->faction[0] == '\0';
    }

    return false;
}

mk_result_t mk_asset_load_map_manifest(
    const char *manifest_path,
    const char *project_root,
    mk_asset_map_manifest_t *out_manifest
) {
    mk_asset_entry_list_t entries;
    const char *manifest_type;
    int layer_count;
    int layer_index;
    bool runtime_overview_available;
    mk_result_t result;

    if (out_manifest == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_manifest, 0, sizeof(*out_manifest));
    result = mk_asset_read_entries(manifest_path, &entries);
    if (result != MK_OK) {
        return result;
    }

    manifest_type = mk_asset_entry_value(&entries, "manifest_type");
    if (manifest_type == NULL || strcmp(manifest_type, "map") != 0) {
        return MK_ERROR_INVALID_DATA;
    }

    if (!mk_asset_required_text(&entries, "id", out_manifest->id, sizeof(out_manifest->id))
        || !mk_asset_required_text(&entries, "name", out_manifest->name, sizeof(out_manifest->name))
        || !mk_asset_required_float(&entries, "world_width_m", &out_manifest->world_width_m)
        || !mk_asset_required_float(&entries, "world_height_m", &out_manifest->world_height_m)
        || !mk_asset_required_float(&entries, "pixels_per_meter", &out_manifest->pixels_per_meter)
        || !mk_asset_required_text(&entries, "origin", out_manifest->origin, sizeof(out_manifest->origin))
        || !mk_asset_required_text(&entries, "source_root", out_manifest->source_root, sizeof(out_manifest->source_root))
        || !mk_asset_required_text(&entries, "runtime_root", out_manifest->runtime_root, sizeof(out_manifest->runtime_root))
        || !mk_asset_required_int(&entries, "layer_count", &layer_count)
        || !mk_asset_required_text(&entries, "overview_path", out_manifest->overview_path, sizeof(out_manifest->overview_path))
        || !mk_asset_required_text(&entries, "runtime_overview_path", out_manifest->runtime_overview_path, sizeof(out_manifest->runtime_overview_path))
        || !mk_asset_required_text(&entries, "collision_output_path", out_manifest->collision_output_path, sizeof(out_manifest->collision_output_path))
        || !mk_asset_required_text(&entries, "navigation_output_path", out_manifest->navigation_output_path, sizeof(out_manifest->navigation_output_path))) {
        return MK_ERROR_INVALID_DATA;
    }

    runtime_overview_available = mk_asset_file_exists(project_root, out_manifest->runtime_overview_path);

    if (out_manifest->world_width_m <= 0.0f
        || out_manifest->world_height_m <= 0.0f
        || out_manifest->pixels_per_meter <= 0.0f
        || layer_count <= 0
        || layer_count > MK_ASSET_MAX_MAP_LAYERS
        || !mk_asset_path_is_safe(out_manifest->source_root)
        || !mk_asset_path_is_safe(out_manifest->runtime_root)
        || (!runtime_overview_available && !mk_asset_file_exists(project_root, out_manifest->overview_path))) {
        return MK_ERROR_INVALID_DATA;
    }

    out_manifest->layer_count = (size_t)layer_count;
    for (layer_index = 0; layer_index < layer_count; ++layer_index) {
        mk_asset_map_layer_t *layer = &out_manifest->layers[layer_index];
        char key[64];

        mk_asset_make_indexed_key(key, sizeof(key), "layer", (size_t)layer_index, "id");
        if (!mk_asset_required_text(&entries, key, layer->id, sizeof(layer->id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "layer", (size_t)layer_index, "kind");
        if (!mk_asset_required_text(&entries, key, layer->kind, sizeof(layer->kind))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "layer", (size_t)layer_index, "path");
        if (!mk_asset_required_text(&entries, key, layer->path, sizeof(layer->path))
            || (!runtime_overview_available && !mk_asset_file_exists(project_root, layer->path))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "layer", (size_t)layer_index, "z");
        if (!mk_asset_required_int(&entries, key, &layer->z)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "layer", (size_t)layer_index, "alpha");
        if (!mk_asset_required_text(&entries, key, layer->alpha, sizeof(layer->alpha))) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    if (!mk_asset_map_layer_ids_are_unique(out_manifest)) {
        return MK_ERROR_INVALID_DATA;
    }

    return MK_OK;
}

mk_result_t mk_asset_load_sprite_manifest(
    const char *manifest_path,
    const char *project_root,
    mk_asset_sprite_manifest_t *out_manifest
) {
    mk_asset_entry_list_t entries;
    const char *manifest_type;
    int sheet_count;
    int frame_count;
    int index;
    bool runtime_sprites_available;
    mk_result_t result;

    if (out_manifest == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_manifest, 0, sizeof(*out_manifest));
    result = mk_asset_read_entries(manifest_path, &entries);
    if (result != MK_OK) {
        return result;
    }

    manifest_type = mk_asset_entry_value(&entries, "manifest_type");
    if (manifest_type == NULL || strcmp(manifest_type, "sprites") != 0) {
        return MK_ERROR_INVALID_DATA;
    }

    if (!mk_asset_required_text(&entries, "id", out_manifest->id, sizeof(out_manifest->id))
        || !mk_asset_required_text(&entries, "name", out_manifest->name, sizeof(out_manifest->name))
        || !mk_asset_required_text(&entries, "fallback_runtime_id", out_manifest->fallback_runtime_id, sizeof(out_manifest->fallback_runtime_id))
        || !mk_asset_required_int(&entries, "sheet_count", &sheet_count)
        || !mk_asset_required_int(&entries, "frame_count", &frame_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    if (sheet_count <= 0
        || frame_count <= 0
        || sheet_count > MK_ASSET_MAX_SPRITE_SHEETS
        || frame_count > MK_ASSET_MAX_SPRITE_FRAMES) {
        return MK_ERROR_INVALID_DATA;
    }

    if (!mk_asset_optional_path(&entries, "source_angle_root", project_root, false, out_manifest->source_angle_root, sizeof(out_manifest->source_angle_root))
        || !mk_asset_optional_path(&entries, "runtime_rendered_root", project_root, false, out_manifest->runtime_rendered_root, sizeof(out_manifest->runtime_rendered_root))
        || !mk_asset_optional_path(&entries, "runtime_pipeline_manifest", project_root, true, out_manifest->runtime_pipeline_manifest, sizeof(out_manifest->runtime_pipeline_manifest))
        || !mk_asset_optional_path(&entries, "runtime_render_manifest", project_root, true, out_manifest->runtime_render_manifest, sizeof(out_manifest->runtime_render_manifest))
        || !mk_asset_optional_count(&entries, "runtime_rendered_count", &out_manifest->runtime_rendered_count)
        || !mk_asset_optional_count(&entries, "runtime_infantry_count", &out_manifest->runtime_infantry_count)
        || !mk_asset_optional_count(&entries, "runtime_civilian_count", &out_manifest->runtime_civilian_count)
        || !mk_asset_optional_count(&entries, "runtime_weapon_count", &out_manifest->runtime_weapon_count)
        || !mk_asset_optional_count(&entries, "runtime_vehicle_count", &out_manifest->runtime_vehicle_count)
        || out_manifest->runtime_rendered_count > MK_ASSET_MAX_SPRITE_RENDER_ENTRIES) {
        return MK_ERROR_INVALID_DATA;
    }

    runtime_sprites_available =
        out_manifest->runtime_rendered_count > 0
        && out_manifest->runtime_render_manifest[0] != '\0';

    out_manifest->sheet_count = (size_t)sheet_count;
    for (index = 0; index < sheet_count; ++index) {
        mk_asset_sprite_sheet_t *sheet = &out_manifest->sheets[index];
        char key[64];

        mk_asset_make_indexed_key(key, sizeof(key), "sheet", (size_t)index, "id");
        if (!mk_asset_required_text(&entries, key, sheet->id, sizeof(sheet->id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "sheet", (size_t)index, "path");
        if (!mk_asset_required_text(&entries, key, sheet->path, sizeof(sheet->path))
            || (!runtime_sprites_available && !mk_asset_file_exists(project_root, sheet->path))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "sheet", (size_t)index, "tile_width");
        if (!mk_asset_required_int(&entries, key, &sheet->tile_width) || sheet->tile_width <= 0) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "sheet", (size_t)index, "tile_height");
        if (!mk_asset_required_int(&entries, key, &sheet->tile_height) || sheet->tile_height <= 0) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "sheet", (size_t)index, "pivot_x");
        if (!mk_asset_required_int(&entries, key, &sheet->pivot_x)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "sheet", (size_t)index, "pivot_y");
        if (!mk_asset_required_int(&entries, key, &sheet->pivot_y)) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    out_manifest->frame_count = (size_t)frame_count;
    for (index = 0; index < frame_count; ++index) {
        mk_asset_sprite_frame_t *frame = &out_manifest->frames[index];
        char key[64];

        mk_asset_make_indexed_key(key, sizeof(key), "frame", (size_t)index, "id");
        if (!mk_asset_required_text(&entries, key, frame->id, sizeof(frame->id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "frame", (size_t)index, "runtime_id");
        if (!mk_asset_required_text(&entries, key, frame->runtime_id, sizeof(frame->runtime_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "frame", (size_t)index, "sheet");
        if (!mk_asset_required_text(&entries, key, frame->sheet, sizeof(frame->sheet))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "frame", (size_t)index, "side");
        if (!mk_asset_required_text(&entries, key, frame->side, sizeof(frame->side))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "frame", (size_t)index, "role");
        if (!mk_asset_required_text(&entries, key, frame->role, sizeof(frame->role))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "frame", (size_t)index, "state");
        if (!mk_asset_required_text(&entries, key, frame->state, sizeof(frame->state))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "frame", (size_t)index, "facing");
        if (!mk_asset_required_text(&entries, key, frame->facing, sizeof(frame->facing))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "frame", (size_t)index, "x");
        if (!mk_asset_required_int(&entries, key, &frame->x) || frame->x < 0) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "frame", (size_t)index, "y");
        if (!mk_asset_required_int(&entries, key, &frame->y) || frame->y < 0) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "frame", (size_t)index, "scale_m");
        if (!mk_asset_required_float(&entries, key, &frame->scale_m) || frame->scale_m <= 0.0f) {
            return MK_ERROR_INVALID_DATA;
        }

        if (mk_asset_find_sprite_sheet(out_manifest, frame->sheet) == NULL) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    if (!mk_asset_sprite_ids_are_unique(out_manifest)
        || mk_asset_find_sprite_frame(out_manifest, out_manifest->fallback_runtime_id) == NULL) {
        return MK_ERROR_INVALID_DATA;
    }

    return MK_OK;
}

mk_result_t mk_asset_load_sprite_render_manifest(
    const char *manifest_path,
    const char *project_root,
    mk_asset_sprite_render_manifest_t *out_manifest
) {
    char *text = NULL;
    const char *limit;
    const char *array_value;
    const char *array_end;
    const char *cursor;
    int rendered_count;
    int missing_source_count;
    int error_count;
    mk_result_t result;

    if (out_manifest == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_manifest, 0, sizeof(*out_manifest));
    result = mk_asset_read_text_file(manifest_path, &text);
    if (result != MK_OK) {
        return result;
    }

    limit = text + strlen(text);
    if (!mk_asset_json_required_int(text, limit, "schema_version", &out_manifest->schema_version)
        || out_manifest->schema_version <= 0
        || !mk_asset_json_required_string(text, limit, "source_manifest", false, out_manifest->source_manifest, sizeof(out_manifest->source_manifest))
        || !mk_asset_path_is_safe(out_manifest->source_manifest)
        || !mk_asset_file_exists(project_root, out_manifest->source_manifest)
        || !mk_asset_json_required_int(text, limit, "rendered_count", &rendered_count)
        || !mk_asset_json_required_int(text, limit, "missing_source_count", &missing_source_count)
        || !mk_asset_json_required_int(text, limit, "error_count", &error_count)
        || rendered_count <= 0
        || rendered_count > MK_ASSET_MAX_SPRITE_RENDER_ENTRIES
        || missing_source_count != 0
        || error_count != 0) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    out_manifest->missing_source_count = (size_t)missing_source_count;
    out_manifest->error_count = (size_t)error_count;

    array_value = mk_asset_json_find_key(text, limit, "rendered");
    if (array_value == NULL || array_value >= limit || *array_value != '[') {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_end = mk_asset_json_find_matching(array_value, limit, '[', ']');
    if (array_end == NULL) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    cursor = array_value + 1;
    while (cursor < array_end) {
        const char *object_end;
        mk_asset_sprite_render_entry_t *entry;

        cursor = mk_asset_json_skip_whitespace(cursor, array_end);
        if (cursor >= array_end) {
            break;
        }

        if (*cursor == ',') {
            cursor += 1;
            continue;
        }

        if (*cursor != '{' || out_manifest->rendered_count >= MK_ASSET_MAX_SPRITE_RENDER_ENTRIES) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        object_end = mk_asset_json_find_matching(cursor, array_end + 1, '{', '}');
        if (object_end == NULL) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        entry = &out_manifest->rendered[out_manifest->rendered_count];
        if (!mk_asset_json_required_string(cursor, object_end, "path", false, entry->path, sizeof(entry->path))
            || !mk_asset_json_required_string(cursor, object_end, "kind", false, entry->kind, sizeof(entry->kind))
            || !mk_asset_json_required_string(cursor, object_end, "item_id", false, entry->item_id, sizeof(entry->item_id))
            || !mk_asset_json_required_string(cursor, object_end, "state", true, entry->state, sizeof(entry->state))
            || !mk_asset_json_required_string(cursor, object_end, "faction", true, entry->faction, sizeof(entry->faction))
            || !mk_asset_json_required_string(cursor, object_end, "angle", false, entry->angle, sizeof(entry->angle))
            || !mk_asset_sprite_render_entry_is_valid(entry, project_root)) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        if (strcmp(entry->kind, "infantry") == 0) {
            out_manifest->infantry_count += 1;
        } else if (strcmp(entry->kind, "civilian") == 0) {
            out_manifest->civilian_count += 1;
        } else if (strcmp(entry->kind, "weapon") == 0) {
            out_manifest->weapon_count += 1;
        } else if (strcmp(entry->kind, "vehicle") == 0) {
            out_manifest->vehicle_count += 1;
        }

        out_manifest->rendered_count += 1;
        cursor = object_end + 1;
    }

    if (out_manifest->rendered_count != (size_t)rendered_count
        || !mk_asset_sprite_render_paths_are_unique(out_manifest)) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    free(text);
    return MK_OK;
}

mk_result_t mk_asset_load_marker_manifest(
    const char *manifest_path,
    mk_asset_marker_manifest_t *out_manifest
) {
    mk_asset_entry_list_t entries;
    const char *manifest_type;
    int marker_count;
    int marker_index;
    mk_result_t result;

    if (out_manifest == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_manifest, 0, sizeof(*out_manifest));
    result = mk_asset_read_entries(manifest_path, &entries);
    if (result != MK_OK) {
        return result;
    }

    manifest_type = mk_asset_entry_value(&entries, "manifest_type");
    if (manifest_type == NULL || strcmp(manifest_type, "markers") != 0) {
        return MK_ERROR_INVALID_DATA;
    }

    if (!mk_asset_required_text(&entries, "id", out_manifest->id, sizeof(out_manifest->id))
        || !mk_asset_required_text(&entries, "name", out_manifest->name, sizeof(out_manifest->name))
        || !mk_asset_required_int(&entries, "marker_count", &marker_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    if (marker_count <= 0 || marker_count > MK_ASSET_MAX_MARKERS) {
        return MK_ERROR_INVALID_DATA;
    }

    out_manifest->marker_count = (size_t)marker_count;
    for (marker_index = 0; marker_index < marker_count; ++marker_index) {
        mk_asset_marker_t *marker = &out_manifest->markers[marker_index];
        char key[64];

        mk_asset_make_indexed_key(key, sizeof(key), "marker", (size_t)marker_index, "id");
        if (!mk_asset_required_text(&entries, key, marker->id, sizeof(marker->id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "marker", (size_t)marker_index, "kind");
        if (!mk_asset_required_text(&entries, key, marker->kind, sizeof(marker->kind))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "marker", (size_t)marker_index, "shape");
        if (!mk_asset_required_text(&entries, key, marker->shape, sizeof(marker->shape))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "marker", (size_t)marker_index, "color");
        if (!mk_asset_required_color(&entries, key, &marker->color)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "marker", (size_t)marker_index, "radius_m");
        if (!mk_asset_required_float(&entries, key, &marker->radius_m) || marker->radius_m < 0.0f) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_asset_make_indexed_key(key, sizeof(key), "marker", (size_t)marker_index, "line_width_px");
        if (!mk_asset_required_int(&entries, key, &marker->line_width_px) || marker->line_width_px <= 0) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    if (!mk_asset_marker_ids_are_unique(out_manifest)) {
        return MK_ERROR_INVALID_DATA;
    }

    return MK_OK;
}

mk_result_t mk_asset_load_building_level_manifest(
    const char *manifest_path,
    const char *project_root,
    mk_asset_building_level_manifest_t *out_manifest
) {
    char *text = NULL;
    const char *limit;
    const char *array_value;
    const char *array_end;
    const char *cursor;
    int level_count;
    int feature_count;
    int region_count;
    mk_result_t result;

    if (out_manifest == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_manifest, 0, sizeof(*out_manifest));
    result = mk_asset_read_text_file(manifest_path, &text);
    if (result != MK_OK) {
        return result;
    }

    limit = text + strlen(text);
    if (!mk_asset_json_required_int(text, limit, "schema_version", &out_manifest->schema_version)
        || out_manifest->schema_version <= 0
        || !mk_asset_json_required_string(text, limit, "id", false, out_manifest->id, sizeof(out_manifest->id))
        || !mk_asset_json_required_string(text, limit, "map_id", false, out_manifest->map_id, sizeof(out_manifest->map_id))
        || !mk_asset_json_required_string(text, limit, "name", false, out_manifest->name, sizeof(out_manifest->name))
        || !mk_asset_json_required_float(text, limit, "world_width_m", &out_manifest->world_width_m)
        || !mk_asset_json_required_float(text, limit, "world_height_m", &out_manifest->world_height_m)
        || !mk_asset_json_required_int(text, limit, "pixel_width", &out_manifest->pixel_width)
        || !mk_asset_json_required_int(text, limit, "pixel_height", &out_manifest->pixel_height)
        || !mk_asset_json_required_float(text, limit, "pixels_per_meter", &out_manifest->pixels_per_meter)
        || !mk_asset_json_required_string(text, limit, "origin", false, out_manifest->origin, sizeof(out_manifest->origin))
        || !mk_asset_json_required_string(text, limit, "art_style", false, out_manifest->art_style, sizeof(out_manifest->art_style))
        || !mk_asset_json_required_int(text, limit, "max_storeys", &out_manifest->max_storeys)
        || !mk_asset_json_required_int(text, limit, "level_count", &level_count)
        || !mk_asset_json_required_int(text, limit, "feature_count", &feature_count)
        || !mk_asset_json_required_int(text, limit, "building_region_count", &region_count)
        || out_manifest->world_width_m <= 0.0f
        || out_manifest->world_height_m <= 0.0f
        || out_manifest->pixel_width <= 0
        || out_manifest->pixel_height <= 0
        || out_manifest->pixels_per_meter <= 0.0f
        || out_manifest->max_storeys <= 0
        || level_count <= 0
        || level_count > MK_ASSET_MAX_BUILDING_LEVELS
        || feature_count <= 0
        || feature_count > MK_ASSET_MAX_BUILDING_FEATURES
        || region_count <= 0
        || region_count > MK_ASSET_MAX_BUILDING_REGIONS) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_value = mk_asset_json_find_key(text, limit, "levels");
    if (array_value == NULL || array_value >= limit || *array_value != '[') {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_end = mk_asset_json_find_matching(array_value, limit, '[', ']');
    if (array_end == NULL) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    cursor = array_value + 1;
    while (cursor < array_end) {
        const char *object_end;
        mk_asset_building_level_t *level;

        cursor = mk_asset_json_skip_whitespace(cursor, array_end);
        if (cursor >= array_end) {
            break;
        }

        if (*cursor == ',') {
            cursor += 1;
            continue;
        }

        if (*cursor != '{' || out_manifest->level_count >= MK_ASSET_MAX_BUILDING_LEVELS) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        object_end = mk_asset_json_find_matching(cursor, array_end + 1, '{', '}');
        if (object_end == NULL) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        level = &out_manifest->levels[out_manifest->level_count];
        if (!mk_asset_json_required_string(cursor, object_end, "id", false, level->id, sizeof(level->id))
            || !mk_asset_json_required_int(cursor, object_end, "index", &level->index)
            || !mk_asset_json_required_float(cursor, object_end, "elevation_m", &level->elevation_m)
            || !mk_asset_json_required_string(cursor, object_end, "png", false, level->png_path, sizeof(level->png_path))
            || !mk_asset_json_required_string(cursor, object_end, "alpha", false, level->alpha, sizeof(level->alpha))
            || !mk_asset_json_required_bool(cursor, object_end, "blocks_los_default", &level->blocks_los_default)
            || !mk_asset_json_required_bool(cursor, object_end, "blocks_movement_default", &level->blocks_movement_default)
            || level->index <= 0
            || level->elevation_m < 0.0f
            || !mk_asset_building_alpha_is_valid(level->alpha)
            || !mk_asset_file_exists(project_root, level->png_path)) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        out_manifest->level_count += 1;
        cursor = object_end + 1;
    }

    if (out_manifest->level_count != (size_t)level_count) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_value = mk_asset_json_find_key(text, limit, "features");
    if (array_value == NULL || array_value >= limit || *array_value != '[') {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_end = mk_asset_json_find_matching(array_value, limit, '[', ']');
    if (array_end == NULL) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    cursor = array_value + 1;
    while (cursor < array_end) {
        const char *object_end;
        mk_asset_building_feature_t *feature;

        cursor = mk_asset_json_skip_whitespace(cursor, array_end);
        if (cursor >= array_end) {
            break;
        }

        if (*cursor == ',') {
            cursor += 1;
            continue;
        }

        if (*cursor != '{' || out_manifest->feature_count >= MK_ASSET_MAX_BUILDING_FEATURES) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        object_end = mk_asset_json_find_matching(cursor, array_end + 1, '{', '}');
        if (object_end == NULL) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        feature = &out_manifest->features[out_manifest->feature_count];
        if (!mk_asset_json_required_string(cursor, object_end, "id", false, feature->id, sizeof(feature->id))
            || !mk_asset_json_required_string(cursor, object_end, "level_id", false, feature->level_id, sizeof(feature->level_id))
            || !mk_asset_json_required_string(cursor, object_end, "kind", false, feature->kind, sizeof(feature->kind))
            || !mk_asset_json_required_int(cursor, object_end, "x", &feature->x)
            || !mk_asset_json_required_int(cursor, object_end, "y", &feature->y)
            || !mk_asset_json_required_int(cursor, object_end, "width", &feature->width)
            || !mk_asset_json_required_int(cursor, object_end, "height", &feature->height)
            || !mk_asset_json_required_bool(cursor, object_end, "blocks_los", &feature->blocks_los)
            || !mk_asset_json_required_bool(cursor, object_end, "blocks_movement", &feature->blocks_movement)
            || !mk_asset_json_required_bool(cursor, object_end, "allows_los", &feature->allows_los)
            || !mk_asset_json_required_bool(cursor, object_end, "allows_movement", &feature->allows_movement)
            || !mk_asset_building_feature_kind_is_valid(feature->kind)
            || !mk_asset_building_rect_is_valid(
                feature->x,
                feature->y,
                feature->width,
                feature->height,
                out_manifest->pixel_width,
                out_manifest->pixel_height
            )
            || (feature->blocks_los && feature->allows_los)
            || (feature->blocks_movement && feature->allows_movement)
            || mk_asset_find_building_level(out_manifest, feature->level_id) == NULL) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        out_manifest->feature_count += 1;
        cursor = object_end + 1;
    }

    if (out_manifest->feature_count != (size_t)feature_count) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_value = mk_asset_json_find_key(text, limit, "building_regions");
    if (array_value == NULL || array_value >= limit || *array_value != '[') {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_end = mk_asset_json_find_matching(array_value, limit, '[', ']');
    if (array_end == NULL) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    cursor = array_value + 1;
    while (cursor < array_end) {
        const char *object_end;
        mk_asset_building_region_t *region;

        cursor = mk_asset_json_skip_whitespace(cursor, array_end);
        if (cursor >= array_end) {
            break;
        }

        if (*cursor == ',') {
            cursor += 1;
            continue;
        }

        if (*cursor != '{' || out_manifest->region_count >= MK_ASSET_MAX_BUILDING_REGIONS) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        object_end = mk_asset_json_find_matching(cursor, array_end + 1, '{', '}');
        if (object_end == NULL) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        region = &out_manifest->regions[out_manifest->region_count];
        if (!mk_asset_json_required_string(cursor, object_end, "id", false, region->id, sizeof(region->id))
            || !mk_asset_json_required_int(cursor, object_end, "storeys", &region->storeys)
            || !mk_asset_json_required_int(cursor, object_end, "x", &region->x)
            || !mk_asset_json_required_int(cursor, object_end, "y", &region->y)
            || !mk_asset_json_required_int(cursor, object_end, "width", &region->width)
            || !mk_asset_json_required_int(cursor, object_end, "height", &region->height)
            || !mk_asset_json_required_string(cursor, object_end, "roof_level_id", false, region->roof_level_id, sizeof(region->roof_level_id))
            || region->storeys <= 0
            || region->storeys > out_manifest->max_storeys
            || !mk_asset_building_rect_is_valid(
                region->x,
                region->y,
                region->width,
                region->height,
                out_manifest->pixel_width,
                out_manifest->pixel_height
            )
            || mk_asset_find_building_level(out_manifest, region->roof_level_id) == NULL) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        out_manifest->region_count += 1;
        cursor = object_end + 1;
    }

    if (out_manifest->region_count != (size_t)region_count
        || !mk_asset_building_ids_are_unique(out_manifest)) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    free(text);
    return MK_OK;
}

mk_result_t mk_asset_load_topology_manifest(
    const char *manifest_path,
    const mk_asset_building_level_manifest_t *building_manifest,
    mk_asset_topology_manifest_t *out_manifest
) {
    char *text = NULL;
    const char *limit;
    const char *array_value;
    const char *array_end;
    const char *cursor;
    int node_count;
    int portal_count;
    int zone_count;
    mk_result_t result;

    if (out_manifest == NULL || building_manifest == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(out_manifest, 0, sizeof(*out_manifest));
    result = mk_asset_read_text_file(manifest_path, &text);
    if (result != MK_OK) {
        return result;
    }

    limit = text + strlen(text);
    if (!mk_asset_json_required_int(text, limit, "schema_version", &out_manifest->schema_version)
        || out_manifest->schema_version <= 0
        || !mk_asset_json_required_string(text, limit, "id", false, out_manifest->id, sizeof(out_manifest->id))
        || !mk_asset_json_required_string(text, limit, "map_id", false, out_manifest->map_id, sizeof(out_manifest->map_id))
        || !mk_asset_json_required_string(text, limit, "gameplay_area_id", false, out_manifest->gameplay_area_id, sizeof(out_manifest->gameplay_area_id))
        || !mk_asset_json_required_string(text, limit, "name", false, out_manifest->name, sizeof(out_manifest->name))
        || !mk_asset_json_required_int(text, limit, "node_count", &node_count)
        || !mk_asset_json_required_int(text, limit, "portal_count", &portal_count)
        || !mk_asset_json_required_int(text, limit, "semantic_zone_count", &zone_count)
        || node_count <= 0
        || node_count > MK_ASSET_MAX_TOPOLOGY_NODES
        || portal_count <= 0
        || portal_count > MK_ASSET_MAX_TOPOLOGY_PORTALS
        || zone_count <= 0
        || zone_count > MK_ASSET_MAX_SEMANTIC_ZONES) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_value = mk_asset_json_find_key(text, limit, "nodes");
    if (array_value == NULL || array_value >= limit || *array_value != '[') {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_end = mk_asset_json_find_matching(array_value, limit, '[', ']');
    if (array_end == NULL) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    cursor = array_value + 1;
    while (cursor < array_end) {
        const char *object_end;
        mk_asset_topology_node_t *node;

        cursor = mk_asset_json_skip_whitespace(cursor, array_end);
        if (cursor >= array_end) {
            break;
        }

        if (*cursor == ',') {
            cursor += 1;
            continue;
        }

        if (*cursor != '{' || out_manifest->node_count >= MK_ASSET_MAX_TOPOLOGY_NODES) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        object_end = mk_asset_json_find_matching(cursor, array_end + 1, '{', '}');
        if (object_end == NULL) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        node = &out_manifest->nodes[out_manifest->node_count];
        if (!mk_asset_json_required_string(cursor, object_end, "id", false, node->id, sizeof(node->id))
            || !mk_asset_json_required_string(cursor, object_end, "kind", false, node->kind, sizeof(node->kind))
            || !mk_asset_json_required_string(cursor, object_end, "level_id", false, node->level_id, sizeof(node->level_id))
            || !mk_asset_json_required_string(cursor, object_end, "region_id", true, node->region_id, sizeof(node->region_id))
            || !mk_asset_json_required_string(cursor, object_end, "label", false, node->label, sizeof(node->label))
            || !mk_asset_json_required_int(cursor, object_end, "x", &node->x)
            || !mk_asset_json_required_int(cursor, object_end, "y", &node->y)
            || !mk_asset_json_required_int(cursor, object_end, "width", &node->width)
            || !mk_asset_json_required_int(cursor, object_end, "height", &node->height)
            || !mk_asset_json_required_bool(cursor, object_end, "enterable", &node->enterable)) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        out_manifest->node_count += 1;
        cursor = object_end + 1;
    }

    if (out_manifest->node_count != (size_t)node_count) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_value = mk_asset_json_find_key(text, limit, "portals");
    if (array_value == NULL || array_value >= limit || *array_value != '[') {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_end = mk_asset_json_find_matching(array_value, limit, '[', ']');
    if (array_end == NULL) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    cursor = array_value + 1;
    while (cursor < array_end) {
        const char *object_end;
        mk_asset_topology_portal_t *portal;

        cursor = mk_asset_json_skip_whitespace(cursor, array_end);
        if (cursor >= array_end) {
            break;
        }

        if (*cursor == ',') {
            cursor += 1;
            continue;
        }

        if (*cursor != '{' || out_manifest->portal_count >= MK_ASSET_MAX_TOPOLOGY_PORTALS) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        object_end = mk_asset_json_find_matching(cursor, array_end + 1, '{', '}');
        if (object_end == NULL) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        portal = &out_manifest->portals[out_manifest->portal_count];
        if (!mk_asset_json_required_string(cursor, object_end, "id", false, portal->id, sizeof(portal->id))
            || !mk_asset_json_required_string(cursor, object_end, "kind", false, portal->kind, sizeof(portal->kind))
            || !mk_asset_json_required_string(cursor, object_end, "state", false, portal->state, sizeof(portal->state))
            || !mk_asset_json_required_string(cursor, object_end, "from_node_id", false, portal->from_node_id, sizeof(portal->from_node_id))
            || !mk_asset_json_required_string(cursor, object_end, "to_node_id", false, portal->to_node_id, sizeof(portal->to_node_id))
            || !mk_asset_json_required_string(cursor, object_end, "level_id", false, portal->level_id, sizeof(portal->level_id))
            || !mk_asset_json_required_string(cursor, object_end, "feature_id", true, portal->feature_id, sizeof(portal->feature_id))
            || !mk_asset_json_required_int(cursor, object_end, "x", &portal->x)
            || !mk_asset_json_required_int(cursor, object_end, "y", &portal->y)
            || !mk_asset_json_required_int(cursor, object_end, "width", &portal->width)
            || !mk_asset_json_required_int(cursor, object_end, "height", &portal->height)
            || !mk_asset_json_required_bool(cursor, object_end, "bidirectional", &portal->bidirectional)
            || !mk_asset_json_required_bool(cursor, object_end, "vertical", &portal->vertical)
            || !mk_asset_json_required_int(cursor, object_end, "movement_cost", &portal->movement_cost)) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        out_manifest->portal_count += 1;
        cursor = object_end + 1;
    }

    if (out_manifest->portal_count != (size_t)portal_count) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_value = mk_asset_json_find_key(text, limit, "semantic_zones");
    if (array_value == NULL || array_value >= limit || *array_value != '[') {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    array_end = mk_asset_json_find_matching(array_value, limit, '[', ']');
    if (array_end == NULL) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    cursor = array_value + 1;
    while (cursor < array_end) {
        const char *object_end;
        mk_asset_semantic_zone_t *zone;

        cursor = mk_asset_json_skip_whitespace(cursor, array_end);
        if (cursor >= array_end) {
            break;
        }

        if (*cursor == ',') {
            cursor += 1;
            continue;
        }

        if (*cursor != '{' || out_manifest->zone_count >= MK_ASSET_MAX_SEMANTIC_ZONES) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        object_end = mk_asset_json_find_matching(cursor, array_end + 1, '{', '}');
        if (object_end == NULL) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        zone = &out_manifest->zones[out_manifest->zone_count];
        if (!mk_asset_json_required_string(cursor, object_end, "id", false, zone->id, sizeof(zone->id))
            || !mk_asset_json_required_string(cursor, object_end, "kind", false, zone->kind, sizeof(zone->kind))
            || !mk_asset_json_required_string(cursor, object_end, "node_id", false, zone->node_id, sizeof(zone->node_id))
            || !mk_asset_json_required_string(cursor, object_end, "level_id", false, zone->level_id, sizeof(zone->level_id))
            || !mk_asset_json_required_int(cursor, object_end, "x", &zone->x)
            || !mk_asset_json_required_int(cursor, object_end, "y", &zone->y)
            || !mk_asset_json_required_int(cursor, object_end, "width", &zone->width)
            || !mk_asset_json_required_int(cursor, object_end, "height", &zone->height)
            || !mk_asset_json_required_int(cursor, object_end, "priority", &zone->priority)) {
            free(text);
            return MK_ERROR_INVALID_DATA;
        }

        out_manifest->zone_count += 1;
        cursor = object_end + 1;
    }

    if (out_manifest->zone_count != (size_t)zone_count
        || !mk_asset_topology_is_valid(out_manifest, building_manifest)) {
        free(text);
        return MK_ERROR_INVALID_DATA;
    }

    free(text);
    return MK_OK;
}

const mk_asset_sprite_sheet_t *mk_asset_find_sprite_sheet(
    const mk_asset_sprite_manifest_t *manifest,
    const char *sheet_id
) {
    size_t index;

    if (manifest == NULL || sheet_id == NULL) {
        return NULL;
    }

    for (index = 0; index < manifest->sheet_count; ++index) {
        if (strcmp(manifest->sheets[index].id, sheet_id) == 0) {
            return &manifest->sheets[index];
        }
    }

    return NULL;
}

const mk_asset_marker_t *mk_asset_find_marker(
    const mk_asset_marker_manifest_t *manifest,
    const char *marker_id
) {
    size_t index;

    if (manifest == NULL || marker_id == NULL) {
        return NULL;
    }

    for (index = 0; index < manifest->marker_count; ++index) {
        if (strcmp(manifest->markers[index].id, marker_id) == 0) {
            return &manifest->markers[index];
        }
    }

    return NULL;
}

const mk_asset_sprite_frame_t *mk_asset_find_sprite_frame(
    const mk_asset_sprite_manifest_t *manifest,
    const char *runtime_id
) {
    size_t index;

    if (manifest == NULL || runtime_id == NULL) {
        return NULL;
    }

    for (index = 0; index < manifest->frame_count; ++index) {
        if (strcmp(manifest->frames[index].runtime_id, runtime_id) == 0) {
            return &manifest->frames[index];
        }
    }

    return NULL;
}

const mk_asset_sprite_render_entry_t *mk_asset_find_sprite_render_entry(
    const mk_asset_sprite_render_manifest_t *manifest,
    const char *kind,
    const char *item_id,
    const char *state,
    const char *faction,
    const char *angle
) {
    const char *entry_state = state == NULL ? "" : state;
    const char *entry_faction = faction == NULL ? "" : faction;
    size_t index;

    if (manifest == NULL || kind == NULL || item_id == NULL || angle == NULL) {
        return NULL;
    }

    for (index = 0; index < manifest->rendered_count; ++index) {
        const mk_asset_sprite_render_entry_t *entry = &manifest->rendered[index];

        if (strcmp(entry->kind, kind) == 0
            && strcmp(entry->item_id, item_id) == 0
            && strcmp(entry->state, entry_state) == 0
            && strcmp(entry->faction, entry_faction) == 0
            && strcmp(entry->angle, angle) == 0) {
            return entry;
        }
    }

    return NULL;
}

const mk_asset_building_level_t *mk_asset_find_building_level(
    const mk_asset_building_level_manifest_t *manifest,
    const char *level_id
) {
    size_t index;

    if (manifest == NULL || level_id == NULL) {
        return NULL;
    }

    for (index = 0; index < manifest->level_count; ++index) {
        if (strcmp(manifest->levels[index].id, level_id) == 0) {
            return &manifest->levels[index];
        }
    }

    return NULL;
}

const mk_asset_building_feature_t *mk_asset_find_building_feature(
    const mk_asset_building_level_manifest_t *manifest,
    const char *feature_id
) {
    size_t index;

    if (manifest == NULL || feature_id == NULL) {
        return NULL;
    }

    for (index = 0; index < manifest->feature_count; ++index) {
        if (strcmp(manifest->features[index].id, feature_id) == 0) {
            return &manifest->features[index];
        }
    }

    return NULL;
}

const mk_asset_building_region_t *mk_asset_find_building_region(
    const mk_asset_building_level_manifest_t *manifest,
    const char *region_id
) {
    size_t index;

    if (manifest == NULL || region_id == NULL) {
        return NULL;
    }

    for (index = 0; index < manifest->region_count; ++index) {
        if (strcmp(manifest->regions[index].id, region_id) == 0) {
            return &manifest->regions[index];
        }
    }

    return NULL;
}

const mk_asset_topology_node_t *mk_asset_find_topology_node(
    const mk_asset_topology_manifest_t *manifest,
    const char *node_id
) {
    size_t index;

    if (manifest == NULL || node_id == NULL) {
        return NULL;
    }

    for (index = 0; index < manifest->node_count; ++index) {
        if (strcmp(manifest->nodes[index].id, node_id) == 0) {
            return &manifest->nodes[index];
        }
    }

    return NULL;
}

const mk_asset_topology_portal_t *mk_asset_find_topology_portal(
    const mk_asset_topology_manifest_t *manifest,
    const char *portal_id
) {
    size_t index;

    if (manifest == NULL || portal_id == NULL) {
        return NULL;
    }

    for (index = 0; index < manifest->portal_count; ++index) {
        if (strcmp(manifest->portals[index].id, portal_id) == 0) {
            return &manifest->portals[index];
        }
    }

    return NULL;
}

const mk_asset_semantic_zone_t *mk_asset_find_semantic_zone(
    const mk_asset_topology_manifest_t *manifest,
    const char *zone_id
) {
    size_t index;

    if (manifest == NULL || zone_id == NULL) {
        return NULL;
    }

    for (index = 0; index < manifest->zone_count; ++index) {
        if (strcmp(manifest->zones[index].id, zone_id) == 0) {
            return &manifest->zones[index];
        }
    }

    return NULL;
}

bool mk_asset_building_feature_contains_pixel(
    const mk_asset_building_feature_t *feature,
    int x,
    int y
) {
    if (feature == NULL) {
        return false;
    }

    return x >= feature->x
        && y >= feature->y
        && x < feature->x + feature->width
        && y < feature->y + feature->height;
}

static bool mk_asset_building_level_blocks_at_pixel(
    const mk_asset_building_level_manifest_t *manifest,
    const char *level_id,
    int x,
    int y,
    bool check_line_of_sight
) {
    const mk_asset_building_level_t *level;
    bool blocked = false;
    size_t index;

    if (manifest == NULL || level_id == NULL || x < 0 || y < 0 || x >= manifest->pixel_width || y >= manifest->pixel_height) {
        return false;
    }

    level = mk_asset_find_building_level(manifest, level_id);
    if (level == NULL) {
        return false;
    }

    blocked = check_line_of_sight ? level->blocks_los_default : level->blocks_movement_default;
    for (index = 0; index < manifest->feature_count; ++index) {
        const mk_asset_building_feature_t *feature = &manifest->features[index];

        if (strcmp(feature->level_id, level_id) != 0 || !mk_asset_building_feature_contains_pixel(feature, x, y)) {
            continue;
        }

        if (check_line_of_sight) {
            if (feature->allows_los) {
                return false;
            }

            if (feature->blocks_los) {
                blocked = true;
            }
        } else {
            if (feature->allows_movement) {
                return false;
            }

            if (feature->blocks_movement) {
                blocked = true;
            }
        }
    }

    return blocked;
}

bool mk_asset_building_level_blocks_los_at_pixel(
    const mk_asset_building_level_manifest_t *manifest,
    const char *level_id,
    int x,
    int y
) {
    return mk_asset_building_level_blocks_at_pixel(manifest, level_id, x, y, true);
}

bool mk_asset_building_level_blocks_movement_at_pixel(
    const mk_asset_building_level_manifest_t *manifest,
    const char *level_id,
    int x,
    int y
) {
    return mk_asset_building_level_blocks_at_pixel(manifest, level_id, x, y, false);
}
