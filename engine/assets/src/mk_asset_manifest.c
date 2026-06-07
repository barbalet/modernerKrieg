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

    return strncmp(path, "assets/mosul/", 13) == 0;
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

    if (strcmp(entry->kind, "infantry") == 0 || strcmp(entry->kind, "vehicle") == 0) {
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

    if (out_manifest->world_width_m <= 0.0f
        || out_manifest->world_height_m <= 0.0f
        || out_manifest->pixels_per_meter <= 0.0f
        || layer_count <= 0
        || layer_count > MK_ASSET_MAX_MAP_LAYERS
        || !mk_asset_path_is_safe(out_manifest->source_root)
        || !mk_asset_path_is_safe(out_manifest->runtime_root)
        || !mk_asset_file_exists(project_root, out_manifest->overview_path)) {
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
            || !mk_asset_file_exists(project_root, layer->path)) {
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
        || !mk_asset_optional_count(&entries, "runtime_weapon_count", &out_manifest->runtime_weapon_count)
        || !mk_asset_optional_count(&entries, "runtime_vehicle_count", &out_manifest->runtime_vehicle_count)
        || out_manifest->runtime_rendered_count > MK_ASSET_MAX_SPRITE_RENDER_ENTRIES) {
        return MK_ERROR_INVALID_DATA;
    }

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
            || !mk_asset_file_exists(project_root, sheet->path)) {
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
