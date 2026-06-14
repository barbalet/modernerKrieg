#include "mk_mosul_demo.h"

#include "mk_asset_manifest.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MK_MOSUL_SCENARIO_MAX_ENTRIES 1536
#define MK_MOSUL_SCENARIO_MAX_WEAPONS 32
#define MK_MOSUL_SCENARIO_PATH_CAPACITY 512

typedef struct {
    char key[128];
    char value[256];
} mk_mosul_scenario_entry_t;

typedef struct {
    size_t count;
    mk_mosul_scenario_entry_t entries[MK_MOSUL_SCENARIO_MAX_ENTRIES];
} mk_mosul_scenario_entry_list_t;

static void mk_mosul_copy_text(char *destination, size_t capacity, const char *source) {
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

static char *mk_mosul_trim(char *text) {
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

static mk_result_t mk_mosul_read_entries(const char *scenario_path, mk_mosul_scenario_entry_list_t *entries) {
    FILE *file;
    char line[512];

    if (scenario_path == NULL || entries == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    memset(entries, 0, sizeof(*entries));
    file = fopen(scenario_path, "r");
    if (file == NULL) {
        return MK_ERROR_NOT_FOUND;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *trimmed = mk_mosul_trim(line);
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
        key = mk_mosul_trim(trimmed);
        value = mk_mosul_trim(equals + 1);

        if (key[0] == '\0' || value[0] == '\0' || entries->count >= MK_MOSUL_SCENARIO_MAX_ENTRIES) {
            fclose(file);
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_copy_text(entries->entries[entries->count].key, sizeof(entries->entries[entries->count].key), key);
        mk_mosul_copy_text(entries->entries[entries->count].value, sizeof(entries->entries[entries->count].value), value);
        entries->count += 1;
    }

    fclose(file);
    return MK_OK;
}

static const char *mk_mosul_entry_value(const mk_mosul_scenario_entry_list_t *entries, const char *key) {
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

static void mk_mosul_make_indexed_key(char *out_key, size_t capacity, const char *prefix, size_t index, const char *field) {
    (void)snprintf(out_key, capacity, "%s.%u.%s", prefix, (unsigned)index, field);
}

static void mk_mosul_make_soldier_key(
    char *out_key,
    size_t capacity,
    size_t unit_index,
    size_t soldier_index,
    const char *field
) {
    (void)snprintf(out_key, capacity, "unit.%u.soldier.%u.%s", (unsigned)unit_index, (unsigned)soldier_index, field);
}

static bool mk_mosul_required_text(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    char *out_text,
    size_t capacity
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL || value[0] == '\0') {
        return false;
    }

    mk_mosul_copy_text(out_text, capacity, value);
    return true;
}

static bool mk_mosul_optional_text(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    const char *default_value,
    char *out_text,
    size_t capacity
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (out_text == NULL || capacity == 0) {
        return false;
    }

    mk_mosul_copy_text(out_text, capacity, value != NULL ? value : default_value);
    return true;
}

static bool mk_mosul_parse_int_text(const char *text, int *out_value) {
    char *end = NULL;
    long parsed;

    if (text == NULL || out_value == NULL || text[0] == '\0') {
        return false;
    }

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed < -2147483647L || parsed > 2147483647L) {
        return false;
    }

    *out_value = (int)parsed;
    return true;
}

static bool mk_mosul_required_int(const mk_mosul_scenario_entry_list_t *entries, const char *key, int *out_value) {
    return mk_mosul_parse_int_text(mk_mosul_entry_value(entries, key), out_value);
}

static bool mk_mosul_optional_int(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    int default_value,
    int *out_value
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL) {
        *out_value = default_value;
        return true;
    }

    return mk_mosul_parse_int_text(value, out_value);
}

static bool mk_mosul_required_u64(const mk_mosul_scenario_entry_list_t *entries, const char *key, uint64_t *out_value) {
    const char *value = mk_mosul_entry_value(entries, key);
    char *end = NULL;
    unsigned long long parsed;

    if (value == NULL || out_value == NULL || value[0] == '\0') {
        return false;
    }

    errno = 0;
    parsed = strtoull(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0') {
        return false;
    }

    *out_value = (uint64_t)parsed;
    return true;
}

static bool mk_mosul_required_float(const mk_mosul_scenario_entry_list_t *entries, const char *key, float *out_value) {
    const char *value = mk_mosul_entry_value(entries, key);
    char *end = NULL;
    float parsed;

    if (value == NULL || out_value == NULL || value[0] == '\0') {
        return false;
    }

    errno = 0;
    parsed = strtof(value, &end);
    if (errno != 0 || end == value || *end != '\0') {
        return false;
    }

    *out_value = parsed;
    return true;
}

static bool mk_mosul_optional_float(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    float default_value,
    float *out_value
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (out_value == NULL) {
        return false;
    }

    if (value == NULL) {
        *out_value = default_value;
        return true;
    }

    return mk_mosul_required_float(entries, key, out_value);
}

static bool mk_mosul_required_bool(const mk_mosul_scenario_entry_list_t *entries, const char *key, bool *out_value) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL || out_value == NULL) {
        return false;
    }

    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
        *out_value = true;
        return true;
    }

    if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
        *out_value = false;
        return true;
    }

    return false;
}

static bool mk_mosul_optional_bool(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    bool default_value,
    bool *out_value
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL) {
        *out_value = default_value;
        return true;
    }

    return mk_mosul_required_bool(entries, key, out_value);
}

static bool mk_mosul_required_count(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    size_t maximum,
    size_t *out_count
) {
    int parsed = 0;

    if (!mk_mosul_required_int(entries, key, &parsed) || parsed < 0 || (size_t)parsed > maximum) {
        return false;
    }

    *out_count = (size_t)parsed;
    return true;
}

static bool mk_mosul_optional_count(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    size_t maximum,
    size_t *out_count
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL) {
        *out_count = 0;
        return true;
    }

    return mk_mosul_required_count(entries, key, maximum, out_count);
}

static bool mk_mosul_required_index(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    size_t count,
    size_t *out_index
) {
    int parsed = 0;

    if (!mk_mosul_required_int(entries, key, &parsed) || parsed < 0 || (size_t)parsed >= count) {
        return false;
    }

    *out_index = (size_t)parsed;
    return true;
}

static bool mk_mosul_required_vec2(const mk_mosul_scenario_entry_list_t *entries, const char *key, mk_vec2_t *out_value) {
    const char *value = mk_mosul_entry_value(entries, key);
    float x;
    float y;
    char trailing;

    if (value == NULL || out_value == NULL) {
        return false;
    }

    if (sscanf(value, " %f , %f %c", &x, &y, &trailing) != 2) {
        return false;
    }

    *out_value = mk_vec2(x, y);
    return true;
}

static bool mk_mosul_optional_vec2(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    mk_vec2_t *out_value,
    bool *out_present
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (out_value == NULL || out_present == NULL) {
        return false;
    }

    if (value == NULL) {
        *out_present = false;
        *out_value = mk_vec2(0.0f, 0.0f);
        return true;
    }

    *out_present = true;
    return mk_mosul_required_vec2(entries, key, out_value);
}

static bool mk_mosul_required_rect(const mk_mosul_scenario_entry_list_t *entries, const char *key, mk_rect_t *out_value) {
    const char *value = mk_mosul_entry_value(entries, key);
    float x;
    float y;
    float width;
    float height;
    char trailing;

    if (value == NULL || out_value == NULL) {
        return false;
    }

    if (sscanf(value, " %f , %f , %f , %f %c", &x, &y, &width, &height, &trailing) != 4) {
        return false;
    }

    *out_value = mk_rect(x, y, width, height);
    return true;
}

static bool mk_mosul_required_color(const mk_mosul_scenario_entry_list_t *entries, const char *key, mk_color_t *out_value) {
    const char *value = mk_mosul_entry_value(entries, key);
    int r;
    int g;
    int b;
    int a;
    char trailing;

    if (value == NULL || out_value == NULL) {
        return false;
    }

    if (sscanf(value, " %d , %d , %d , %d %c", &r, &g, &b, &a, &trailing) != 4) {
        return false;
    }

    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255) {
        return false;
    }

    *out_value = mk_make_color((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
    return true;
}

static bool mk_mosul_parse_side(const char *text, mk_side_t *out_side) {
    if (text == NULL || out_side == NULL) {
        return false;
    }

    if (strcmp(text, "neutral") == 0) {
        *out_side = MK_SIDE_NEUTRAL;
        return true;
    }

    if (strcmp(text, "player") == 0) {
        *out_side = MK_SIDE_PLAYER;
        return true;
    }

    if (strcmp(text, "opfor") == 0) {
        *out_side = MK_SIDE_OPFOR;
        return true;
    }

    if (strcmp(text, "civilian") == 0) {
        *out_side = MK_SIDE_CIVILIAN;
        return true;
    }

    return false;
}

static bool mk_mosul_required_side(const mk_mosul_scenario_entry_list_t *entries, const char *key, mk_side_t *out_side) {
    return mk_mosul_parse_side(mk_mosul_entry_value(entries, key), out_side);
}

static bool mk_mosul_required_controller_kind(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    mk_controller_kind_t *out_kind
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL || out_kind == NULL) {
        return false;
    }

    if (strcmp(value, "human") == 0) {
        *out_kind = MK_CONTROLLER_HUMAN;
        return true;
    }

    if (strcmp(value, "scripted_ai") == 0) {
        *out_kind = MK_CONTROLLER_SCRIPTED_AI;
        return true;
    }

    if (strcmp(value, "tactical_ai") == 0) {
        *out_kind = MK_CONTROLLER_TACTICAL_AI;
        return true;
    }

    if (strcmp(value, "observer") == 0) {
        *out_kind = MK_CONTROLLER_OBSERVER;
        return true;
    }

    return false;
}

static bool mk_mosul_required_training(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    mk_training_t *out_training
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL || out_training == NULL) {
        return false;
    }

    if (strcmp(value, "untrained") == 0) {
        *out_training = MK_TRAINING_UNTRAINED;
        return true;
    }

    if (strcmp(value, "regular") == 0) {
        *out_training = MK_TRAINING_REGULAR;
        return true;
    }

    if (strcmp(value, "veteran") == 0) {
        *out_training = MK_TRAINING_VETERAN;
        return true;
    }

    if (strcmp(value, "elite") == 0) {
        *out_training = MK_TRAINING_ELITE;
        return true;
    }

    return false;
}

static bool mk_mosul_required_soldier_role(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    mk_soldier_role_t *out_role
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL || out_role == NULL) {
        return false;
    }

    if (strcmp(value, "rifleman") == 0) {
        *out_role = MK_ROLE_RIFLEMAN;
        return true;
    }

    if (strcmp(value, "leader") == 0) {
        *out_role = MK_ROLE_LEADER;
        return true;
    }

    if (strcmp(value, "machine_gunner") == 0) {
        *out_role = MK_ROLE_MACHINE_GUNNER;
        return true;
    }

    if (strcmp(value, "rpg") == 0) {
        *out_role = MK_ROLE_RPG;
        return true;
    }

    if (strcmp(value, "marksman") == 0) {
        *out_role = MK_ROLE_MARKSMAN;
        return true;
    }

    if (strcmp(value, "engineer") == 0) {
        *out_role = MK_ROLE_ENGINEER;
        return true;
    }

    if (strcmp(value, "medic") == 0) {
        *out_role = MK_ROLE_MEDIC;
        return true;
    }

    if (strcmp(value, "drone_operator") == 0) {
        *out_role = MK_ROLE_DRONE_OPERATOR;
        return true;
    }

    if (strcmp(value, "civilian") == 0) {
        *out_role = MK_ROLE_CIVILIAN;
        return true;
    }

    return false;
}

static bool mk_mosul_required_terrain_kind(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    mk_terrain_kind_t *out_kind
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL || out_kind == NULL) {
        return false;
    }

    if (strcmp(value, "open") == 0) {
        *out_kind = MK_TERRAIN_OPEN;
        return true;
    }

    if (strcmp(value, "road") == 0) {
        *out_kind = MK_TERRAIN_ROAD;
        return true;
    }

    if (strcmp(value, "building") == 0) {
        *out_kind = MK_TERRAIN_BUILDING;
        return true;
    }

    if (strcmp(value, "rooftop") == 0) {
        *out_kind = MK_TERRAIN_ROOFTOP;
        return true;
    }

    if (strcmp(value, "rubble") == 0) {
        *out_kind = MK_TERRAIN_RUBBLE;
        return true;
    }

    if (strcmp(value, "alley") == 0) {
        *out_kind = MK_TERRAIN_ALLEY;
        return true;
    }

    if (strcmp(value, "obstacle") == 0) {
        *out_kind = MK_TERRAIN_OBSTACLE;
        return true;
    }

    if (strcmp(value, "breach_point") == 0) {
        *out_kind = MK_TERRAIN_BREACH_POINT;
        return true;
    }

    if (strcmp(value, "suspected_ied") == 0) {
        *out_kind = MK_TERRAIN_SUSPECTED_IED;
        return true;
    }

    if (strcmp(value, "river") == 0) {
        *out_kind = MK_TERRAIN_RIVER;
        return true;
    }

    if (strcmp(value, "bridge") == 0) {
        *out_kind = MK_TERRAIN_BRIDGE;
        return true;
    }

    return false;
}

static bool mk_mosul_required_objective_kind(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    mk_objective_kind_t *out_kind
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL || out_kind == NULL) {
        return false;
    }

    if (strcmp(value, "control") == 0) {
        *out_kind = MK_OBJECTIVE_CONTROL;
        return true;
    }

    if (strcmp(value, "exit") == 0) {
        *out_kind = MK_OBJECTIVE_EXIT;
        return true;
    }

    if (strcmp(value, "protect_civilians") == 0) {
        *out_kind = MK_OBJECTIVE_PROTECT_CIVILIANS;
        return true;
    }

    if (strcmp(value, "search") == 0) {
        *out_kind = MK_OBJECTIVE_SEARCH;
        return true;
    }

    return false;
}

static bool mk_mosul_required_civilian_state(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    mk_civilian_state_t *out_state
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL || out_state == NULL) {
        return false;
    }

    if (strcmp(value, "sheltering") == 0) {
        *out_state = MK_CIVILIAN_SHELTERING;
        return true;
    }

    if (strcmp(value, "fleeing") == 0) {
        *out_state = MK_CIVILIAN_FLEEING;
        return true;
    }

    if (strcmp(value, "frozen") == 0) {
        *out_state = MK_CIVILIAN_FROZEN;
        return true;
    }

    if (strcmp(value, "following_instructions") == 0) {
        *out_state = MK_CIVILIAN_FOLLOWING_INSTRUCTIONS;
        return true;
    }

    if (strcmp(value, "evacuated") == 0) {
        *out_state = MK_CIVILIAN_EVACUATED;
        return true;
    }

    if (strcmp(value, "wounded") == 0) {
        *out_state = MK_CIVILIAN_WOUNDED;
        return true;
    }

    if (strcmp(value, "dead") == 0) {
        *out_state = MK_CIVILIAN_DEAD;
        return true;
    }

    return false;
}

static bool mk_mosul_optional_civilian_intent(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    mk_civilian_intent_t default_intent,
    mk_civilian_intent_t *out_intent
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (out_intent == NULL) {
        return false;
    }

    if (value == NULL) {
        *out_intent = default_intent;
        return true;
    }

    if (strcmp(value, "none") == 0) {
        *out_intent = MK_CIVILIAN_INTENT_NONE;
        return true;
    }

    if (strcmp(value, "shelter") == 0) {
        *out_intent = MK_CIVILIAN_INTENT_SHELTER;
        return true;
    }

    if (strcmp(value, "flee") == 0) {
        *out_intent = MK_CIVILIAN_INTENT_FLEE;
        return true;
    }

    if (strcmp(value, "evacuate") == 0) {
        *out_intent = MK_CIVILIAN_INTENT_EVACUATE;
        return true;
    }

    if (strcmp(value, "follow_instructions") == 0) {
        *out_intent = MK_CIVILIAN_INTENT_FOLLOW_INSTRUCTIONS;
        return true;
    }

    if (strcmp(value, "freeze") == 0) {
        *out_intent = MK_CIVILIAN_INTENT_FREEZE;
        return true;
    }

    if (strcmp(value, "assist_group") == 0) {
        *out_intent = MK_CIVILIAN_INTENT_ASSIST_GROUP;
        return true;
    }

    return false;
}

static bool mk_mosul_required_traffic_vehicle_kind(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    mk_traffic_vehicle_kind_t *out_kind
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (value == NULL || out_kind == NULL) {
        return false;
    }

    if (strcmp(value, "car") == 0) {
        *out_kind = MK_TRAFFIC_VEHICLE_CAR;
        return true;
    }

    if (strcmp(value, "bus") == 0) {
        *out_kind = MK_TRAFFIC_VEHICLE_BUS;
        return true;
    }

    if (strcmp(value, "motorcycle") == 0) {
        *out_kind = MK_TRAFFIC_VEHICLE_MOTORCYCLE;
        return true;
    }

    return false;
}

static bool mk_mosul_optional_traffic_boarding_mode(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *key,
    mk_traffic_boarding_mode_t default_mode,
    mk_traffic_boarding_mode_t *out_mode
) {
    const char *value = mk_mosul_entry_value(entries, key);

    if (out_mode == NULL) {
        return false;
    }

    if (value == NULL) {
        *out_mode = default_mode;
        return true;
    }

    if (strcmp(value, "inside") == 0) {
        *out_mode = MK_TRAFFIC_BOARD_INSIDE;
        return true;
    }

    if (strcmp(value, "on") == 0) {
        *out_mode = MK_TRAFFIC_BOARD_ON;
        return true;
    }

    return false;
}

static bool mk_mosul_asset_path_is_safe(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return false;
    }

    if (path[0] == '/' || strstr(path, "..") != NULL || strstr(path, "\\") != NULL) {
        return false;
    }

    return strncmp(path, "assets/mosul/", 13) == 0
        || strncmp(path, "assets/fallujah/", 16) == 0
        || strncmp(path, "assets/shared/", 14) == 0;
}

static bool mk_mosul_make_project_path(
    const char *project_root,
    const char *relative_path,
    char *out_path,
    size_t capacity
) {
    const char *root = project_root == NULL || project_root[0] == '\0' ? "." : project_root;
    int written;

    if (relative_path == NULL || out_path == NULL || capacity == 0) {
        return false;
    }

    if (relative_path[0] == '/') {
        mk_mosul_copy_text(out_path, capacity, relative_path);
        return strlen(relative_path) < capacity;
    }

    written = snprintf(out_path, capacity, "%s/%s", root, relative_path);
    return written > 0 && (size_t)written < capacity;
}

static bool mk_mosul_float_close(float first, float second) {
    float delta = first - second;

    if (delta < 0.0f) {
        delta = -delta;
    }

    return delta < 0.01f;
}

static mk_rect_t mk_mosul_pixel_rect_to_world(
    const mk_asset_building_level_manifest_t *manifest,
    int x,
    int y,
    int width,
    int height
) {
    float pixels_per_meter = manifest != NULL && manifest->pixels_per_meter > 0.0f ? manifest->pixels_per_meter : 1.0f;

    return mk_rect(
        (float)x / pixels_per_meter,
        (float)y / pixels_per_meter,
        (float)width / pixels_per_meter,
        (float)height / pixels_per_meter
    );
}

static mk_result_t mk_mosul_apply_building_level_manifest(
    const mk_asset_building_level_manifest_t *manifest,
    mk_scenario_definition_t *scenario
) {
    mk_gameplay_area_t area;
    size_t index;

    if (manifest == NULL || scenario == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (manifest->level_count > MK_MAX_GAMEPLAY_AREA_LEVELS
        || manifest->feature_count > MK_MAX_GAMEPLAY_AREA_FEATURES
        || manifest->region_count > MK_MAX_GAMEPLAY_AREA_REGIONS) {
        return MK_ERROR_CAPACITY;
    }

    memset(&area, 0, sizeof(area));
    area.loaded = true;
    area.schema_version = manifest->schema_version;
    mk_mosul_copy_text(area.id, sizeof(area.id), manifest->id);
    mk_mosul_copy_text(area.map_id, sizeof(area.map_id), manifest->map_id);
    mk_mosul_copy_text(area.name, sizeof(area.name), manifest->name);
    area.world_width_m = manifest->world_width_m;
    area.world_height_m = manifest->world_height_m;
    area.pixel_width = manifest->pixel_width;
    area.pixel_height = manifest->pixel_height;
    area.pixels_per_meter = manifest->pixels_per_meter;
    mk_mosul_copy_text(area.origin, sizeof(area.origin), manifest->origin);
    area.max_storeys = manifest->max_storeys;

    area.level_count = manifest->level_count;
    for (index = 0; index < manifest->level_count; ++index) {
        const mk_asset_building_level_t *source = &manifest->levels[index];
        mk_gameplay_level_t *level = &area.levels[index];

        mk_mosul_copy_text(level->id, sizeof(level->id), source->id);
        level->index = source->index;
        level->elevation_m = source->elevation_m;
        mk_mosul_copy_text(level->image_path, sizeof(level->image_path), source->png_path);
        mk_mosul_copy_text(level->alpha, sizeof(level->alpha), source->alpha);
        level->blocks_los_default = source->blocks_los_default;
        level->blocks_movement_default = source->blocks_movement_default;
    }

    area.feature_count = manifest->feature_count;
    for (index = 0; index < manifest->feature_count; ++index) {
        const mk_asset_building_feature_t *source = &manifest->features[index];
        mk_gameplay_feature_t *feature = &area.features[index];

        mk_mosul_copy_text(feature->id, sizeof(feature->id), source->id);
        mk_mosul_copy_text(feature->level_id, sizeof(feature->level_id), source->level_id);
        mk_mosul_copy_text(feature->kind, sizeof(feature->kind), source->kind);
        feature->pixel_x = source->x;
        feature->pixel_y = source->y;
        feature->pixel_width = source->width;
        feature->pixel_height = source->height;
        feature->bounds_m = mk_mosul_pixel_rect_to_world(manifest, source->x, source->y, source->width, source->height);
        feature->blocks_los = source->blocks_los;
        feature->blocks_movement = source->blocks_movement;
        feature->allows_los = source->allows_los;
        feature->allows_movement = source->allows_movement;
    }

    area.region_count = manifest->region_count;
    for (index = 0; index < manifest->region_count; ++index) {
        const mk_asset_building_region_t *source = &manifest->regions[index];
        mk_gameplay_region_t *region = &area.regions[index];

        mk_mosul_copy_text(region->id, sizeof(region->id), source->id);
        region->storeys = source->storeys;
        region->pixel_x = source->x;
        region->pixel_y = source->y;
        region->pixel_width = source->width;
        region->pixel_height = source->height;
        region->bounds_m = mk_mosul_pixel_rect_to_world(manifest, source->x, source->y, source->width, source->height);
        mk_mosul_copy_text(region->roof_level_id, sizeof(region->roof_level_id), source->roof_level_id);
    }

    return mk_scenario_set_gameplay_area(scenario, &area);
}

static mk_result_t mk_mosul_apply_topology_manifest(
    const mk_asset_building_level_manifest_t *building_manifest,
    const mk_asset_topology_manifest_t *topology_manifest,
    mk_scenario_definition_t *scenario
) {
    mk_gameplay_area_t area;
    size_t index;

    if (building_manifest == NULL || topology_manifest == NULL || scenario == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!mk_gameplay_area_is_loaded(&scenario->gameplay_area)
        || topology_manifest->node_count > MK_MAX_GAMEPLAY_TOPOLOGY_NODES
        || topology_manifest->portal_count > MK_MAX_GAMEPLAY_TOPOLOGY_PORTALS
        || topology_manifest->zone_count > MK_MAX_GAMEPLAY_SEMANTIC_ZONES) {
        return MK_ERROR_INVALID_DATA;
    }

    area = scenario->gameplay_area;
    area.topology_loaded = true;
    area.topology_schema_version = topology_manifest->schema_version;
    mk_mosul_copy_text(area.topology_id, sizeof(area.topology_id), topology_manifest->id);

    area.topology_node_count = topology_manifest->node_count;
    for (index = 0; index < topology_manifest->node_count; ++index) {
        const mk_asset_topology_node_t *source = &topology_manifest->nodes[index];
        mk_gameplay_topology_node_t *node = &area.topology_nodes[index];

        mk_mosul_copy_text(node->id, sizeof(node->id), source->id);
        mk_mosul_copy_text(node->kind, sizeof(node->kind), source->kind);
        mk_mosul_copy_text(node->level_id, sizeof(node->level_id), source->level_id);
        mk_mosul_copy_text(node->region_id, sizeof(node->region_id), source->region_id);
        mk_mosul_copy_text(node->label, sizeof(node->label), source->label);
        node->pixel_x = source->x;
        node->pixel_y = source->y;
        node->pixel_width = source->width;
        node->pixel_height = source->height;
        node->bounds_m = mk_mosul_pixel_rect_to_world(
            building_manifest,
            source->x,
            source->y,
            source->width,
            source->height
        );
        node->enterable = source->enterable;
    }

    area.topology_portal_count = topology_manifest->portal_count;
    for (index = 0; index < topology_manifest->portal_count; ++index) {
        const mk_asset_topology_portal_t *source = &topology_manifest->portals[index];
        mk_gameplay_topology_portal_t *portal = &area.topology_portals[index];

        mk_mosul_copy_text(portal->id, sizeof(portal->id), source->id);
        mk_mosul_copy_text(portal->kind, sizeof(portal->kind), source->kind);
        mk_mosul_copy_text(portal->state, sizeof(portal->state), source->state);
        mk_mosul_copy_text(portal->from_node_id, sizeof(portal->from_node_id), source->from_node_id);
        mk_mosul_copy_text(portal->to_node_id, sizeof(portal->to_node_id), source->to_node_id);
        mk_mosul_copy_text(portal->level_id, sizeof(portal->level_id), source->level_id);
        mk_mosul_copy_text(portal->feature_id, sizeof(portal->feature_id), source->feature_id);
        portal->pixel_x = source->x;
        portal->pixel_y = source->y;
        portal->pixel_width = source->width;
        portal->pixel_height = source->height;
        portal->bounds_m = mk_mosul_pixel_rect_to_world(
            building_manifest,
            source->x,
            source->y,
            source->width,
            source->height
        );
        portal->bidirectional = source->bidirectional;
        portal->vertical = source->vertical;
        portal->movement_cost = source->movement_cost;
    }

    area.semantic_zone_count = topology_manifest->zone_count;
    for (index = 0; index < topology_manifest->zone_count; ++index) {
        const mk_asset_semantic_zone_t *source = &topology_manifest->zones[index];
        mk_gameplay_semantic_zone_t *zone = &area.semantic_zones[index];

        mk_mosul_copy_text(zone->id, sizeof(zone->id), source->id);
        mk_mosul_copy_text(zone->kind, sizeof(zone->kind), source->kind);
        mk_mosul_copy_text(zone->node_id, sizeof(zone->node_id), source->node_id);
        mk_mosul_copy_text(zone->level_id, sizeof(zone->level_id), source->level_id);
        zone->pixel_x = source->x;
        zone->pixel_y = source->y;
        zone->pixel_width = source->width;
        zone->pixel_height = source->height;
        zone->bounds_m = mk_mosul_pixel_rect_to_world(
            building_manifest,
            source->x,
            source->y,
            source->width,
            source->height
        );
        zone->priority = source->priority;
    }

    return mk_scenario_set_gameplay_area(scenario, &area);
}

static mk_result_t mk_mosul_validate_population_asset_references(
    const mk_mosul_scenario_entry_list_t *entries,
    const mk_asset_sprite_manifest_t *sprite_manifest
) {
    size_t archetype_count = 0;
    size_t archetype_index;
    size_t traffic_vehicle_count = 0;
    size_t traffic_vehicle_index;

    if (!mk_mosul_optional_count(
            entries,
            "civilian_archetype.count",
            MK_MAX_SCENARIO_CIVILIAN_ARCHETYPES,
            &archetype_count
        )) {
        return MK_ERROR_INVALID_DATA;
    }

    for (archetype_index = 0; archetype_index < archetype_count; ++archetype_index) {
        char key[128];
        char sprite_id[MK_NAME_CAPACITY];

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_archetype", archetype_index, "sprite_id");
        if (!mk_mosul_required_text(entries, key, sprite_id, sizeof(sprite_id))
            || mk_asset_find_sprite_frame(sprite_manifest, sprite_id) == NULL) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    if (!mk_mosul_optional_count(entries, "traffic_vehicle.count", MK_MAX_TRAFFIC_VEHICLES, &traffic_vehicle_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    for (traffic_vehicle_index = 0; traffic_vehicle_index < traffic_vehicle_count; ++traffic_vehicle_index) {
        char key[128];
        char sprite_id[MK_NAME_CAPACITY];

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "sprite_id");
        if (!mk_mosul_required_text(entries, key, sprite_id, sizeof(sprite_id))
            || mk_asset_find_sprite_frame(sprite_manifest, sprite_id) == NULL) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    return MK_OK;
}

static mk_result_t mk_mosul_validate_asset_references(
    const mk_mosul_scenario_entry_list_t *entries,
    const char *project_root,
    mk_scenario_definition_t *scenario
) {
    char map_manifest_relative[256];
    char sprite_manifest_relative[256];
    const char *building_level_manifest_relative;
    const char *topology_manifest_relative;
    char map_manifest_path[MK_MOSUL_SCENARIO_PATH_CAPACITY];
    char sprite_manifest_path[MK_MOSUL_SCENARIO_PATH_CAPACITY];
    char building_level_manifest_path[MK_MOSUL_SCENARIO_PATH_CAPACITY];
    char topology_manifest_path[MK_MOSUL_SCENARIO_PATH_CAPACITY];
    mk_asset_map_manifest_t map_manifest;
    mk_asset_sprite_manifest_t sprite_manifest;
    mk_asset_building_level_manifest_t building_level_manifest;
    mk_asset_topology_manifest_t topology_manifest;
    const mk_map_t *map = scenario != NULL ? &scenario->map : NULL;

    if (!mk_mosul_required_text(entries, "asset.map_manifest", map_manifest_relative, sizeof(map_manifest_relative))
        || !mk_mosul_required_text(entries, "asset.sprite_manifest", sprite_manifest_relative, sizeof(sprite_manifest_relative))
        || !mk_mosul_asset_path_is_safe(map_manifest_relative)
        || !mk_mosul_asset_path_is_safe(sprite_manifest_relative)
        || !mk_mosul_make_project_path(project_root, map_manifest_relative, map_manifest_path, sizeof(map_manifest_path))
        || !mk_mosul_make_project_path(project_root, sprite_manifest_relative, sprite_manifest_path, sizeof(sprite_manifest_path))) {
        return MK_ERROR_INVALID_DATA;
    }

    if (mk_asset_load_map_manifest(map_manifest_path, project_root, &map_manifest) != MK_OK) {
        return MK_ERROR_INVALID_DATA;
    }

    if (mk_asset_load_sprite_manifest(sprite_manifest_path, project_root, &sprite_manifest) != MK_OK) {
        return MK_ERROR_INVALID_DATA;
    }

    if (mk_mosul_validate_population_asset_references(entries, &sprite_manifest) != MK_OK) {
        return MK_ERROR_INVALID_DATA;
    }

    building_level_manifest_relative = mk_mosul_entry_value(entries, "asset.building_level_manifest");
    if (building_level_manifest_relative == NULL && strcmp(map_manifest.id, "market_commercial_streets_2003") == 0) {
        return MK_ERROR_INVALID_DATA;
    }

    topology_manifest_relative = mk_mosul_entry_value(entries, "asset.topology_manifest");
    if (topology_manifest_relative == NULL && strcmp(map_manifest.id, "market_commercial_streets_2003") == 0) {
        return MK_ERROR_INVALID_DATA;
    }

    if (building_level_manifest_relative != NULL) {
        if (!mk_mosul_asset_path_is_safe(building_level_manifest_relative)
            || !mk_mosul_make_project_path(
                project_root,
                building_level_manifest_relative,
                building_level_manifest_path,
                sizeof(building_level_manifest_path)
            )
            || mk_asset_load_building_level_manifest(
                building_level_manifest_path,
                project_root,
                &building_level_manifest
            ) != MK_OK) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    if (topology_manifest_relative != NULL) {
        if (building_level_manifest_relative == NULL
            || !mk_mosul_asset_path_is_safe(topology_manifest_relative)
            || !mk_mosul_make_project_path(
                project_root,
                topology_manifest_relative,
                topology_manifest_path,
                sizeof(topology_manifest_path)
            )
            || mk_asset_load_topology_manifest(
                topology_manifest_path,
                &building_level_manifest,
                &topology_manifest
            ) != MK_OK) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    if (map != NULL
        && (!mk_mosul_float_close(map_manifest.world_width_m, map->width_m)
            || !mk_mosul_float_close(map_manifest.world_height_m, map->height_m))) {
        return MK_ERROR_INVALID_DATA;
    }

    if (building_level_manifest_relative != NULL
        && map != NULL
        && (strcmp(building_level_manifest.map_id, map_manifest.id) != 0
            || !mk_mosul_float_close(building_level_manifest.world_width_m, map->width_m)
            || !mk_mosul_float_close(building_level_manifest.world_height_m, map->height_m))) {
        return MK_ERROR_INVALID_DATA;
    }

    if (building_level_manifest_relative != NULL
        && mk_mosul_apply_building_level_manifest(&building_level_manifest, scenario) != MK_OK) {
        return MK_ERROR_INVALID_DATA;
    }

    if (topology_manifest_relative != NULL
        && mk_mosul_apply_topology_manifest(&building_level_manifest, &topology_manifest, scenario) != MK_OK) {
        return MK_ERROR_INVALID_DATA;
    }

    return MK_OK;
}

static mk_result_t mk_mosul_load_map(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario
) {
    char map_name[MK_NAME_CAPACITY];
    float width_m = 0.0f;
    float height_m = 0.0f;
    float tile_width_m = 0.0f;
    float tile_height_m = 0.0f;
    int columns = 0;
    int rows = 0;
    mk_terrain_kind_t default_kind;

    if (!mk_mosul_required_text(entries, "map.name", map_name, sizeof(map_name))
        || !mk_mosul_required_float(entries, "map.width_m", &width_m)
        || !mk_mosul_required_float(entries, "map.height_m", &height_m)
        || !mk_mosul_required_int(entries, "map.tile_columns", &columns)
        || !mk_mosul_required_int(entries, "map.tile_rows", &rows)
        || !mk_mosul_required_float(entries, "map.tile_width_m", &tile_width_m)
        || !mk_mosul_required_float(entries, "map.tile_height_m", &tile_height_m)
        || !mk_mosul_required_terrain_kind(entries, "map.default_terrain", &default_kind)) {
        return MK_ERROR_INVALID_DATA;
    }

    scenario->map = mk_make_map(map_name, width_m, height_m);
    return mk_map_configure_tiles(&scenario->map, columns, rows, tile_width_m, tile_height_m, default_kind);
}

static mk_result_t mk_mosul_load_briefing_and_scoring(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario
) {
    int success_threshold = MK_DEFAULT_SCORE_SUCCESS_THRESHOLD;
    int partial_threshold = MK_DEFAULT_SCORE_PARTIAL_THRESHOLD;
    int objective_weight = MK_DEFAULT_SCORE_OBJECTIVE_WEIGHT;
    int civilian_risk_weight = MK_DEFAULT_SCORE_CIVILIAN_RISK_WEIGHT;
    int player_casualty_weight = MK_DEFAULT_SCORE_PLAYER_CASUALTY_WEIGHT;
    int civilian_casualty_weight = MK_DEFAULT_SCORE_CIVILIAN_CASUALTY_WEIGHT;
    int time_weight = MK_DEFAULT_SCORE_TIME_WEIGHT;

    if (entries == NULL || scenario == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!mk_mosul_optional_text(
            entries,
            "briefing",
            "",
            scenario->briefing,
            sizeof(scenario->briefing)
        )
        || !mk_mosul_optional_text(
            entries,
            "after_action.success",
            "",
            scenario->after_action_success,
            sizeof(scenario->after_action_success)
        )
        || !mk_mosul_optional_text(
            entries,
            "after_action.partial",
            "",
            scenario->after_action_partial,
            sizeof(scenario->after_action_partial)
        )
        || !mk_mosul_optional_text(
            entries,
            "after_action.failure",
            "",
            scenario->after_action_failure,
            sizeof(scenario->after_action_failure)
        )
        || !mk_mosul_optional_int(entries, "score.success_threshold", success_threshold, &success_threshold)
        || !mk_mosul_optional_int(entries, "score.partial_threshold", partial_threshold, &partial_threshold)
        || !mk_mosul_optional_int(entries, "score.objective_weight", objective_weight, &objective_weight)
        || !mk_mosul_optional_int(entries, "score.civilian_risk_weight", civilian_risk_weight, &civilian_risk_weight)
        || !mk_mosul_optional_int(entries, "score.player_casualty_weight", player_casualty_weight, &player_casualty_weight)
        || !mk_mosul_optional_int(entries, "score.civilian_casualty_weight", civilian_casualty_weight, &civilian_casualty_weight)
        || !mk_mosul_optional_int(entries, "score.time_weight", time_weight, &time_weight)
        || success_threshold < partial_threshold
        || partial_threshold < 0
        || objective_weight < 0
        || civilian_risk_weight < 0
        || player_casualty_weight < 0
        || civilian_casualty_weight < 0
        || time_weight < 0) {
        return MK_ERROR_INVALID_DATA;
    }

    scenario->score_success_threshold = success_threshold;
    scenario->score_partial_threshold = partial_threshold;
    scenario->score_objective_weight = objective_weight;
    scenario->score_civilian_risk_weight = civilian_risk_weight;
    scenario->score_player_casualty_weight = player_casualty_weight;
    scenario->score_civilian_casualty_weight = civilian_casualty_weight;
    scenario->score_time_weight = time_weight;
    return MK_OK;
}

static mk_result_t mk_mosul_load_tile_ranges(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario
) {
    size_t range_count = 0;
    size_t range_index;

    if (!mk_mosul_required_count(entries, "tile_range.count", MK_MAX_MAP_TILES, &range_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    for (range_index = 0; range_index < range_count; ++range_index) {
        char key[128];
        int start_x = 0;
        int start_y = 0;
        int width = 0;
        int height = 0;
        int elevation = 0;
        int cover = 0;
        int movement_cost = 0;
        int x;
        int y;
        bool blocks_line_of_sight = false;
        bool blocks_movement = false;
        mk_terrain_kind_t kind;

        mk_mosul_make_indexed_key(key, sizeof(key), "tile_range", range_index, "x");
        if (!mk_mosul_required_int(entries, key, &start_x)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "tile_range", range_index, "y");
        if (!mk_mosul_required_int(entries, key, &start_y)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "tile_range", range_index, "width");
        if (!mk_mosul_required_int(entries, key, &width) || width <= 0) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "tile_range", range_index, "height");
        if (!mk_mosul_required_int(entries, key, &height) || height <= 0) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "tile_range", range_index, "kind");
        if (!mk_mosul_required_terrain_kind(entries, key, &kind)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "tile_range", range_index, "elevation");
        if (!mk_mosul_required_int(entries, key, &elevation)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "tile_range", range_index, "cover");
        if (!mk_mosul_required_int(entries, key, &cover)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "tile_range", range_index, "movement_cost");
        if (!mk_mosul_required_int(entries, key, &movement_cost)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "tile_range", range_index, "blocks_line_of_sight");
        if (!mk_mosul_required_bool(entries, key, &blocks_line_of_sight)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "tile_range", range_index, "blocks_movement");
        if (!mk_mosul_required_bool(entries, key, &blocks_movement)) {
            return MK_ERROR_INVALID_DATA;
        }

        for (y = start_y; y < start_y + height; ++y) {
            for (x = start_x; x < start_x + width; ++x) {
                mk_map_tile_t tile = mk_make_map_tile(
                    mk_ivec2(x, y),
                    kind,
                    elevation,
                    cover,
                    movement_cost,
                    blocks_line_of_sight,
                    blocks_movement
                );
                mk_result_t result = mk_map_set_tile(&scenario->map, &tile);

                if (result != MK_OK) {
                    return result;
                }
            }
        }
    }

    return MK_OK;
}

static mk_result_t mk_mosul_load_terrain(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario
) {
    size_t terrain_count = 0;
    size_t terrain_index;

    if (!mk_mosul_required_count(entries, "terrain.count", MK_MAX_TERRAIN_ZONES, &terrain_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    for (terrain_index = 0; terrain_index < terrain_count; ++terrain_index) {
        char key[128];
        char name[MK_NAME_CAPACITY];
        mk_terrain_kind_t kind;
        mk_rect_t bounds;
        int cover = 0;
        int movement_cost = 0;
        bool blocks_line_of_sight = false;
        mk_terrain_zone_t terrain;
        mk_result_t result;

        mk_mosul_make_indexed_key(key, sizeof(key), "terrain", terrain_index, "name");
        if (!mk_mosul_required_text(entries, key, name, sizeof(name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "terrain", terrain_index, "kind");
        if (!mk_mosul_required_terrain_kind(entries, key, &kind)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "terrain", terrain_index, "bounds");
        if (!mk_mosul_required_rect(entries, key, &bounds)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "terrain", terrain_index, "cover");
        if (!mk_mosul_required_int(entries, key, &cover)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "terrain", terrain_index, "movement_cost");
        if (!mk_mosul_required_int(entries, key, &movement_cost)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "terrain", terrain_index, "blocks_line_of_sight");
        if (!mk_mosul_required_bool(entries, key, &blocks_line_of_sight)) {
            return MK_ERROR_INVALID_DATA;
        }

        terrain = mk_make_terrain_zone(name, kind, bounds, cover, movement_cost, blocks_line_of_sight);
        result = mk_map_add_terrain(&scenario->map, &terrain, NULL);
        if (result != MK_OK) {
            return result;
        }
    }

    return MK_OK;
}

static mk_result_t mk_mosul_load_controllers(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario,
    uint32_t *controller_ids,
    size_t *out_controller_count
) {
    size_t controller_count = 0;
    size_t controller_index;

    if (!mk_mosul_required_count(entries, "controller.count", MK_MAX_CONTROLLERS, &controller_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    for (controller_index = 0; controller_index < controller_count; ++controller_index) {
        char key[128];
        char name[MK_NAME_CAPACITY];
        mk_side_t side;
        mk_controller_kind_t kind;
        mk_controller_slot_t controller;
        mk_result_t result;

        mk_mosul_make_indexed_key(key, sizeof(key), "controller", controller_index, "name");
        if (!mk_mosul_required_text(entries, key, name, sizeof(name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "controller", controller_index, "side");
        if (!mk_mosul_required_side(entries, key, &side)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "controller", controller_index, "kind");
        if (!mk_mosul_required_controller_kind(entries, key, &kind)) {
            return MK_ERROR_INVALID_DATA;
        }

        controller = mk_make_controller_slot(name, side, kind);
        result = mk_scenario_add_controller(scenario, &controller, &controller_ids[controller_index]);
        if (result != MK_OK) {
            return result;
        }
    }

    *out_controller_count = controller_count;
    return MK_OK;
}

static mk_result_t mk_mosul_load_factions(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario,
    uint32_t *faction_ids,
    size_t *out_faction_count
) {
    size_t faction_count = 0;
    size_t faction_index;

    if (!mk_mosul_required_count(entries, "faction.count", MK_MAX_FACTIONS, &faction_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    for (faction_index = 0; faction_index < faction_count; ++faction_index) {
        char key[128];
        char name[MK_NAME_CAPACITY];
        mk_side_t side;
        mk_color_t color;
        mk_faction_t faction;
        mk_result_t result;

        mk_mosul_make_indexed_key(key, sizeof(key), "faction", faction_index, "name");
        if (!mk_mosul_required_text(entries, key, name, sizeof(name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "faction", faction_index, "side");
        if (!mk_mosul_required_side(entries, key, &side)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "faction", faction_index, "color");
        if (!mk_mosul_required_color(entries, key, &color)) {
            return MK_ERROR_INVALID_DATA;
        }

        faction = mk_make_faction(name, side, color);
        result = mk_scenario_add_faction(scenario, &faction, &faction_ids[faction_index]);
        if (result != MK_OK) {
            return result;
        }
    }

    *out_faction_count = faction_count;
    return MK_OK;
}

static mk_result_t mk_mosul_load_forces(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario,
    const uint32_t *controller_ids,
    size_t controller_count,
    const uint32_t *faction_ids,
    size_t faction_count,
    uint32_t *force_ids,
    size_t *out_force_count
) {
    size_t force_count = 0;
    size_t force_index;

    if (!mk_mosul_required_count(entries, "force.count", MK_MAX_FORCES, &force_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    for (force_index = 0; force_index < force_count; ++force_index) {
        char key[128];
        char name[MK_NAME_CAPACITY];
        char command_name[MK_NAME_CAPACITY];
        char callsign[MK_NAME_CAPACITY];
        mk_side_t side;
        size_t faction_index = 0;
        size_t controller_index = 0;
        mk_force_t force;
        mk_result_t result;

        mk_mosul_make_indexed_key(key, sizeof(key), "force", force_index, "name");
        if (!mk_mosul_required_text(entries, key, name, sizeof(name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "force", force_index, "side");
        if (!mk_mosul_required_side(entries, key, &side)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "force", force_index, "faction_index");
        if (!mk_mosul_required_index(entries, key, faction_count, &faction_index)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "force", force_index, "controller_index");
        if (!mk_mosul_required_index(entries, key, controller_count, &controller_index)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "force", force_index, "command_name");
        if (!mk_mosul_required_text(entries, key, command_name, sizeof(command_name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "force", force_index, "callsign");
        if (!mk_mosul_required_text(entries, key, callsign, sizeof(callsign))) {
            return MK_ERROR_INVALID_DATA;
        }

        force = mk_make_force(name, side, faction_ids[faction_index], controller_ids[controller_index]);
        force.command = mk_make_command_identity(command_name, callsign, side);
        result = mk_scenario_add_force(scenario, &force, &force_ids[force_index]);
        if (result != MK_OK) {
            return result;
        }
    }

    *out_force_count = force_count;
    return MK_OK;
}

static mk_result_t mk_mosul_load_spawn_zones(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario
) {
    size_t spawn_zone_count = 0;
    size_t spawn_zone_index;

    if (!mk_mosul_optional_count(entries, "spawn_zone.count", MK_MAX_SCENARIO_SPAWN_ZONES, &spawn_zone_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    scenario->spawn_zone_count = spawn_zone_count;
    for (spawn_zone_index = 0; spawn_zone_index < spawn_zone_count; ++spawn_zone_index) {
        char key[128];
        mk_spawn_zone_t *spawn_zone = &scenario->spawn_zones[spawn_zone_index];

        memset(spawn_zone, 0, sizeof(*spawn_zone));
        spawn_zone->id = (uint32_t)(spawn_zone_index + 1);

        mk_mosul_make_indexed_key(key, sizeof(key), "spawn_zone", spawn_zone_index, "id");
        if (!mk_mosul_required_text(entries, key, spawn_zone->scenario_id, sizeof(spawn_zone->scenario_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "spawn_zone", spawn_zone_index, "name");
        if (!mk_mosul_required_text(entries, key, spawn_zone->name, sizeof(spawn_zone->name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "spawn_zone", spawn_zone_index, "kind");
        if (!mk_mosul_required_text(entries, key, spawn_zone->kind, sizeof(spawn_zone->kind))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "spawn_zone", spawn_zone_index, "side");
        if (!mk_mosul_required_side(entries, key, &spawn_zone->side)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "spawn_zone", spawn_zone_index, "level_id");
        if (!mk_mosul_optional_text(entries, key, "", spawn_zone->level_id, sizeof(spawn_zone->level_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "spawn_zone", spawn_zone_index, "topology_node_id");
        if (!mk_mosul_optional_text(entries, key, "", spawn_zone->topology_node_id, sizeof(spawn_zone->topology_node_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "spawn_zone", spawn_zone_index, "bounds");
        if (!mk_mosul_required_rect(entries, key, &spawn_zone->bounds_m)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "spawn_zone", spawn_zone_index, "capacity");
        if (!mk_mosul_required_int(entries, key, &spawn_zone->capacity) || spawn_zone->capacity < 0) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "spawn_zone", spawn_zone_index, "active");
        if (!mk_mosul_optional_bool(entries, key, true, &spawn_zone->active)) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    return MK_OK;
}

static mk_result_t mk_mosul_load_unit_templates(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario
) {
    size_t template_count = 0;
    size_t template_index;

    if (!mk_mosul_optional_count(entries, "unit_template.count", MK_MAX_SCENARIO_UNIT_TEMPLATES, &template_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    scenario->unit_template_count = template_count;
    for (template_index = 0; template_index < template_count; ++template_index) {
        char key[128];
        mk_unit_template_t *unit_template = &scenario->unit_templates[template_index];

        memset(unit_template, 0, sizeof(*unit_template));
        unit_template->id = (uint32_t)(template_index + 1);

        mk_mosul_make_indexed_key(key, sizeof(key), "unit_template", template_index, "id");
        if (!mk_mosul_required_text(entries, key, unit_template->scenario_id, sizeof(unit_template->scenario_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit_template", template_index, "name");
        if (!mk_mosul_required_text(entries, key, unit_template->name, sizeof(unit_template->name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit_template", template_index, "role");
        if (!mk_mosul_required_text(entries, key, unit_template->role, sizeof(unit_template->role))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit_template", template_index, "side");
        if (!mk_mosul_required_side(entries, key, &unit_template->side)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit_template", template_index, "training");
        if (!mk_mosul_required_training(entries, key, &unit_template->training)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit_template", template_index, "default_spawn_zone_id");
        if (!mk_mosul_optional_text(
                entries,
                key,
                "",
                unit_template->default_spawn_zone_id,
                sizeof(unit_template->default_spawn_zone_id)
            )) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit_template", template_index, "expected_soldiers");
        if (!mk_mosul_required_int(entries, key, &unit_template->expected_soldiers)
            || unit_template->expected_soldiers < 0
            || unit_template->expected_soldiers > MK_MAX_SOLDIERS_PER_UNIT) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    return MK_OK;
}

static mk_result_t mk_mosul_load_civilian_archetypes(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario
) {
    size_t archetype_count = 0;
    size_t archetype_index;

    if (!mk_mosul_optional_count(
            entries,
            "civilian_archetype.count",
            MK_MAX_SCENARIO_CIVILIAN_ARCHETYPES,
            &archetype_count
        )) {
        return MK_ERROR_INVALID_DATA;
    }

    scenario->civilian_archetype_count = archetype_count;
    for (archetype_index = 0; archetype_index < archetype_count; ++archetype_index) {
        char key[128];
        mk_civilian_archetype_t *archetype = &scenario->civilian_archetypes[archetype_index];

        memset(archetype, 0, sizeof(*archetype));
        archetype->id = (uint32_t)(archetype_index + 1);

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_archetype", archetype_index, "id");
        if (!mk_mosul_required_text(entries, key, archetype->scenario_id, sizeof(archetype->scenario_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_archetype", archetype_index, "name");
        if (!mk_mosul_required_text(entries, key, archetype->name, sizeof(archetype->name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_archetype", archetype_index, "sprite_id");
        if (!mk_mosul_required_text(entries, key, archetype->sprite_id, sizeof(archetype->sprite_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_archetype", archetype_index, "baseline_stress");
        if (!mk_mosul_required_int(entries, key, &archetype->baseline_stress) || archetype->baseline_stress < 0) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_archetype", archetype_index, "baseline_risk");
        if (!mk_mosul_required_int(entries, key, &archetype->baseline_risk) || archetype->baseline_risk < 0) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_archetype", archetype_index, "compliance");
        if (!mk_mosul_required_int(entries, key, &archetype->compliance)
            || archetype->compliance < 0
            || archetype->compliance > 100) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_archetype", archetype_index, "protected_noncombatant");
        if (!mk_mosul_required_bool(entries, key, &archetype->protected_noncombatant)) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    return MK_OK;
}

static mk_result_t mk_mosul_load_civilian_groups(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario
) {
    size_t group_count = 0;
    size_t group_index;

    if (!mk_mosul_optional_count(entries, "civilian_group.count", MK_MAX_SCENARIO_CIVILIAN_GROUPS, &group_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    scenario->civilian_group_count = group_count;
    for (group_index = 0; group_index < group_count; ++group_index) {
        char key[128];
        mk_civilian_group_t *group = &scenario->civilian_groups[group_index];

        memset(group, 0, sizeof(*group));
        group->id = (uint32_t)(group_index + 1);

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_group", group_index, "id");
        if (!mk_mosul_required_text(entries, key, group->scenario_id, sizeof(group->scenario_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_group", group_index, "name");
        if (!mk_mosul_required_text(entries, key, group->name, sizeof(group->name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_group", group_index, "archetype_id");
        if (!mk_mosul_required_text(entries, key, group->archetype_id, sizeof(group->archetype_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_group", group_index, "spawn_zone_id");
        if (!mk_mosul_required_text(entries, key, group->spawn_zone_id, sizeof(group->spawn_zone_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_group", group_index, "level_id");
        if (!mk_mosul_optional_text(entries, key, "", group->level_id, sizeof(group->level_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_group", group_index, "topology_node_id");
        if (!mk_mosul_optional_text(entries, key, "", group->topology_node_id, sizeof(group->topology_node_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_group", group_index, "expected_count");
        if (!mk_mosul_required_int(entries, key, &group->expected_count)
            || group->expected_count < 0
            || group->expected_count > MK_MAX_CIVILIANS) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_group", group_index, "baseline_stress");
        if (!mk_mosul_required_int(entries, key, &group->baseline_stress) || group->baseline_stress < 0) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_group", group_index, "compliance");
        if (!mk_mosul_required_int(entries, key, &group->compliance)
            || group->compliance < 0
            || group->compliance > 100) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian_group", group_index, "protected_noncombatants");
        if (!mk_mosul_required_bool(entries, key, &group->protected_noncombatants)) {
            return MK_ERROR_INVALID_DATA;
        }
    }

    return MK_OK;
}

static mk_result_t mk_mosul_load_objectives(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario
) {
    size_t objective_count = 0;
    size_t objective_index;

    if (!mk_mosul_required_count(entries, "objective.count", MK_MAX_OBJECTIVES, &objective_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    for (objective_index = 0; objective_index < objective_count; ++objective_index) {
        char key[128];
        char name[MK_NAME_CAPACITY];
        char label[MK_NAME_CAPACITY];
        mk_objective_kind_t kind;
        mk_vec2_t position;
        float radius_m = 0.0f;
        int value = 0;
        mk_objective_t objective;
        mk_result_t result;

        mk_mosul_make_indexed_key(key, sizeof(key), "objective", objective_index, "name");
        if (!mk_mosul_required_text(entries, key, name, sizeof(name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "objective", objective_index, "kind");
        if (!mk_mosul_required_objective_kind(entries, key, &kind)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "objective", objective_index, "position");
        if (!mk_mosul_required_vec2(entries, key, &position)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "objective", objective_index, "radius_m");
        if (!mk_mosul_required_float(entries, key, &radius_m)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "objective", objective_index, "value");
        if (!mk_mosul_required_int(entries, key, &value)) {
            return MK_ERROR_INVALID_DATA;
        }

        objective = mk_make_objective(name, kind, position, radius_m, value);
        mk_mosul_make_indexed_key(key, sizeof(key), "objective", objective_index, "label");
        if (!mk_mosul_optional_text(entries, key, objective.label, label, sizeof(label))) {
            return MK_ERROR_INVALID_DATA;
        }
        mk_mosul_copy_text(objective.label, sizeof(objective.label), label);
        result = mk_scenario_add_objective(scenario, &objective, NULL);
        if (result != MK_OK) {
            return result;
        }
    }

    return MK_OK;
}

static mk_result_t mk_mosul_load_weapons(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_weapon_profile_t *weapons,
    size_t *out_weapon_count
) {
    size_t weapon_count = 0;
    size_t weapon_index;

    if (!mk_mosul_required_count(entries, "weapon.count", MK_MOSUL_SCENARIO_MAX_WEAPONS, &weapon_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    for (weapon_index = 0; weapon_index < weapon_count; ++weapon_index) {
        char key[128];
        char name[MK_NAME_CAPACITY];
        int effective_range_m = 0;
        int shots_per_action = 0;
        int damage = 0;
        int suppression = 0;

        mk_mosul_make_indexed_key(key, sizeof(key), "weapon", weapon_index, "name");
        if (!mk_mosul_required_text(entries, key, name, sizeof(name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "weapon", weapon_index, "effective_range_m");
        if (!mk_mosul_required_int(entries, key, &effective_range_m)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "weapon", weapon_index, "shots_per_action");
        if (!mk_mosul_required_int(entries, key, &shots_per_action)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "weapon", weapon_index, "damage");
        if (!mk_mosul_required_int(entries, key, &damage)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "weapon", weapon_index, "suppression");
        if (!mk_mosul_required_int(entries, key, &suppression)) {
            return MK_ERROR_INVALID_DATA;
        }

        weapons[weapon_index] = mk_make_weapon(name, effective_range_m, shots_per_action, damage, suppression);
    }

    *out_weapon_count = weapon_count;
    return MK_OK;
}

static const mk_civilian_archetype_t *mk_mosul_find_loaded_civilian_archetype(
    const mk_scenario_definition_t *scenario,
    const char *archetype_id
) {
    size_t index;

    if (scenario == NULL || archetype_id == NULL || archetype_id[0] == '\0') {
        return NULL;
    }

    for (index = 0; index < scenario->civilian_archetype_count; ++index) {
        if (strcmp(scenario->civilian_archetypes[index].scenario_id, archetype_id) == 0) {
            return &scenario->civilian_archetypes[index];
        }
    }

    return NULL;
}

static mk_result_t mk_mosul_load_civilians(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario,
    const uint32_t *faction_ids,
    size_t faction_count
) {
    size_t civilian_count = 0;
    size_t civilian_index;

    if (!mk_mosul_required_count(entries, "civilian.count", MK_MAX_CIVILIANS, &civilian_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    for (civilian_index = 0; civilian_index < civilian_count; ++civilian_index) {
        char key[128];
        char name[MK_NAME_CAPACITY];
        char archetype_id[MK_NAME_CAPACITY];
        char group_id[MK_NAME_CAPACITY];
        char spawn_zone_id[MK_NAME_CAPACITY];
        char level_id[MK_NAME_CAPACITY];
        char topology_node_id[MK_NAME_CAPACITY];
        char sprite_id[MK_NAME_CAPACITY];
        size_t faction_index = 0;
        mk_vec2_t position;
        mk_vec2_t destination;
        mk_civilian_state_t state;
        mk_civilian_intent_t intent = MK_CIVILIAN_INTENT_NONE;
        int stress = 0;
        int risk = 0;
        int compliance = 50;
        float speed_m_per_tick = 0.0f;
        bool has_destination = false;
        bool protected_noncombatant = true;
        const mk_civilian_archetype_t *archetype = NULL;
        mk_civilian_t civilian;
        mk_result_t result;

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "name");
        if (!mk_mosul_required_text(entries, key, name, sizeof(name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "archetype_id");
        if (!mk_mosul_optional_text(entries, key, "", archetype_id, sizeof(archetype_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "group_id");
        if (!mk_mosul_optional_text(entries, key, "", group_id, sizeof(group_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "spawn_zone_id");
        if (!mk_mosul_optional_text(entries, key, "", spawn_zone_id, sizeof(spawn_zone_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "level_id");
        if (!mk_mosul_optional_text(entries, key, "", level_id, sizeof(level_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "topology_node_id");
        if (!mk_mosul_optional_text(entries, key, "", topology_node_id, sizeof(topology_node_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        archetype = mk_mosul_find_loaded_civilian_archetype(scenario, archetype_id);
        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "sprite_id");
        if (!mk_mosul_optional_text(
                entries,
                key,
                archetype != NULL ? archetype->sprite_id : "",
                sprite_id,
                sizeof(sprite_id)
            )) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "faction_index");
        if (!mk_mosul_required_index(entries, key, faction_count, &faction_index)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "position");
        if (!mk_mosul_required_vec2(entries, key, &position)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "state");
        if (!mk_mosul_required_civilian_state(entries, key, &state)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "intent");
        if (!mk_mosul_optional_civilian_intent(entries, key, MK_CIVILIAN_INTENT_NONE, &intent)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "destination");
        if (!mk_mosul_optional_vec2(entries, key, &destination, &has_destination)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "stress");
        if (!mk_mosul_required_int(entries, key, &stress)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "risk");
        if (!mk_mosul_required_int(entries, key, &risk)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "protected_noncombatant");
        if (!mk_mosul_required_bool(entries, key, &protected_noncombatant)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "compliance");
        if (!mk_mosul_optional_int(entries, key, archetype != NULL ? archetype->compliance : 50, &compliance)
            || compliance < 0
            || compliance > 100) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "civilian", civilian_index, "speed_m_per_tick");
        if (!mk_mosul_optional_float(entries, key, MK_DEFAULT_CIVILIAN_SPEED_M_PER_TICK, &speed_m_per_tick)
            || speed_m_per_tick < 0.0f) {
            return MK_ERROR_INVALID_DATA;
        }

        civilian = mk_make_civilian(name, faction_ids[faction_index], position);
        mk_mosul_copy_text(civilian.archetype_id, sizeof(civilian.archetype_id), archetype_id);
        mk_mosul_copy_text(civilian.group_id, sizeof(civilian.group_id), group_id);
        mk_mosul_copy_text(civilian.spawn_zone_id, sizeof(civilian.spawn_zone_id), spawn_zone_id);
        mk_mosul_copy_text(civilian.level_id, sizeof(civilian.level_id), level_id);
        mk_mosul_copy_text(civilian.topology_node_id, sizeof(civilian.topology_node_id), topology_node_id);
        mk_mosul_copy_text(civilian.sprite_id, sizeof(civilian.sprite_id), sprite_id);
        civilian.state = state;
        civilian.intent = intent;
        civilian.destination_m = destination;
        civilian.has_destination = has_destination;
        civilian.speed_m_per_tick = speed_m_per_tick;
        if (has_destination) {
            mk_mosul_copy_text(civilian.destination_level_id, sizeof(civilian.destination_level_id), level_id);
        }
        civilian.stress = stress;
        civilian.risk = risk;
        civilian.compliance = compliance;
        civilian.protected_noncombatant = protected_noncombatant;
        result = mk_scenario_add_civilian(scenario, &civilian, NULL);
        if (result != MK_OK) {
            return result;
        }
    }

    return MK_OK;
}

static mk_result_t mk_mosul_load_traffic_vehicles(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario
) {
    size_t traffic_vehicle_count = 0;
    size_t traffic_vehicle_index;

    if (!mk_mosul_optional_count(entries, "traffic_vehicle.count", MK_MAX_TRAFFIC_VEHICLES, &traffic_vehicle_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    for (traffic_vehicle_index = 0; traffic_vehicle_index < traffic_vehicle_count; ++traffic_vehicle_index) {
        char key[128];
        char scenario_id[MK_NAME_CAPACITY];
        char name[MK_NAME_CAPACITY];
        char sprite_id[MK_NAME_CAPACITY];
        char level_id[MK_NAME_CAPACITY];
        char topology_node_id[MK_NAME_CAPACITY];
        char destination_level_id[MK_NAME_CAPACITY];
        mk_traffic_vehicle_kind_t kind;
        mk_traffic_boarding_mode_t boarding_mode;
        mk_vec2_t position;
        mk_vec2_t destination;
        float speed_m_per_tick = MK_DEFAULT_TRAFFIC_VEHICLE_SPEED_M_PER_TICK;
        float facing_degrees = 0.0f;
        int seat_capacity = 0;
        int occupied_seats = 0;
        bool has_destination = false;
        bool active = true;
        bool blocks_movement = true;
        mk_traffic_vehicle_t vehicle;
        mk_result_t result;

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "id");
        if (!mk_mosul_required_text(entries, key, scenario_id, sizeof(scenario_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "name");
        if (!mk_mosul_required_text(entries, key, name, sizeof(name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "kind");
        if (!mk_mosul_required_traffic_vehicle_kind(entries, key, &kind)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "position");
        if (!mk_mosul_required_vec2(entries, key, &position)) {
            return MK_ERROR_INVALID_DATA;
        }

        vehicle = mk_make_traffic_vehicle(scenario_id, name, kind, position);

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "sprite_id");
        if (!mk_mosul_required_text(entries, key, sprite_id, sizeof(sprite_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "level_id");
        if (!mk_mosul_optional_text(entries, key, "", level_id, sizeof(level_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "topology_node_id");
        if (!mk_mosul_optional_text(entries, key, "", topology_node_id, sizeof(topology_node_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "destination");
        if (!mk_mosul_optional_vec2(entries, key, &destination, &has_destination)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "destination_level_id");
        if (!mk_mosul_optional_text(entries, key, level_id, destination_level_id, sizeof(destination_level_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "speed_m_per_tick");
        if (!mk_mosul_optional_float(entries, key, vehicle.speed_m_per_tick, &speed_m_per_tick)
            || speed_m_per_tick < 0.0f) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "facing_degrees");
        if (!mk_mosul_optional_float(entries, key, vehicle.facing_degrees, &facing_degrees)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "seat_capacity");
        if (!mk_mosul_optional_int(entries, key, vehicle.seat_capacity, &seat_capacity)
            || seat_capacity < 0
            || seat_capacity > MK_MAX_TRAFFIC_VEHICLE_OCCUPANTS) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "occupied_seats");
        if (!mk_mosul_optional_int(entries, key, 0, &occupied_seats)
            || occupied_seats < 0
            || occupied_seats > seat_capacity) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "boarding_mode");
        if (!mk_mosul_optional_traffic_boarding_mode(entries, key, vehicle.boarding_mode, &boarding_mode)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "active");
        if (!mk_mosul_optional_bool(entries, key, true, &active)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "traffic_vehicle", traffic_vehicle_index, "blocks_movement");
        if (!mk_mosul_optional_bool(entries, key, true, &blocks_movement)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_copy_text(vehicle.sprite_id, sizeof(vehicle.sprite_id), sprite_id);
        mk_mosul_copy_text(vehicle.level_id, sizeof(vehicle.level_id), level_id);
        mk_mosul_copy_text(vehicle.topology_node_id, sizeof(vehicle.topology_node_id), topology_node_id);
        mk_mosul_copy_text(vehicle.destination_level_id, sizeof(vehicle.destination_level_id), destination_level_id);
        vehicle.destination_m = has_destination ? destination : position;
        vehicle.has_destination = has_destination;
        vehicle.speed_m_per_tick = speed_m_per_tick;
        vehicle.facing_degrees = facing_degrees;
        vehicle.seat_capacity = seat_capacity;
        vehicle.occupied_seats = occupied_seats;
        vehicle.boarding_mode = boarding_mode;
        vehicle.active = active;
        vehicle.blocks_movement = blocks_movement;

        result = mk_scenario_add_traffic_vehicle(scenario, &vehicle, NULL);
        if (result != MK_OK) {
            return result;
        }
    }

    return MK_OK;
}

static mk_result_t mk_mosul_load_units(
    const mk_mosul_scenario_entry_list_t *entries,
    mk_scenario_definition_t *scenario,
    const uint32_t *controller_ids,
    size_t controller_count,
    const uint32_t *faction_ids,
    size_t faction_count,
    const uint32_t *force_ids,
    size_t force_count,
    const mk_weapon_profile_t *weapons,
    size_t weapon_count
) {
    size_t unit_count = 0;
    size_t unit_index;

    if (!mk_mosul_required_count(entries, "unit.count", MK_MAX_UNITS, &unit_count)) {
        return MK_ERROR_INVALID_DATA;
    }

    for (unit_index = 0; unit_index < unit_count; ++unit_index) {
        char key[128];
        char name[MK_NAME_CAPACITY];
        char command_name[MK_NAME_CAPACITY];
        char callsign[MK_NAME_CAPACITY];
        char template_id[MK_NAME_CAPACITY];
        char group_id[MK_NAME_CAPACITY];
        char spawn_zone_id[MK_NAME_CAPACITY];
        char topology_node_id[MK_NAME_CAPACITY];
        char level_id[MK_NAME_CAPACITY];
        mk_side_t side;
        mk_training_t training;
        mk_vec2_t position;
        size_t faction_index = 0;
        size_t force_index = 0;
        size_t controller_index = 0;
        size_t soldier_count = 0;
        size_t soldier_index;
        bool hidden = false;
        bool revealed = false;
        int concealment = 0;
        mk_unit_t unit;
        mk_result_t result;

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "name");
        if (!mk_mosul_required_text(entries, key, name, sizeof(name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "side");
        if (!mk_mosul_required_side(entries, key, &side)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "training");
        if (!mk_mosul_required_training(entries, key, &training)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "position");
        if (!mk_mosul_required_vec2(entries, key, &position)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "template_id");
        if (!mk_mosul_optional_text(entries, key, "", template_id, sizeof(template_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "group_id");
        if (!mk_mosul_optional_text(entries, key, "", group_id, sizeof(group_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "spawn_zone_id");
        if (!mk_mosul_optional_text(entries, key, "", spawn_zone_id, sizeof(spawn_zone_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "topology_node_id");
        if (!mk_mosul_optional_text(entries, key, "", topology_node_id, sizeof(topology_node_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "level_id");
        if (!mk_mosul_optional_text(entries, key, "", level_id, sizeof(level_id))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "faction_index");
        if (!mk_mosul_required_index(entries, key, faction_count, &faction_index)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "force_index");
        if (!mk_mosul_required_index(entries, key, force_count, &force_index)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "controller_index");
        if (!mk_mosul_required_index(entries, key, controller_count, &controller_index)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "command_name");
        if (!mk_mosul_required_text(entries, key, command_name, sizeof(command_name))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "callsign");
        if (!mk_mosul_required_text(entries, key, callsign, sizeof(callsign))) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "soldier_count");
        if (!mk_mosul_required_count(entries, key, MK_MAX_SOLDIERS_PER_UNIT, &soldier_count)) {
            return MK_ERROR_INVALID_DATA;
        }

        unit = mk_make_unit(name, side, training, position);
        unit.faction_id = faction_ids[faction_index];
        unit.force_id = force_ids[force_index];
        unit.controller_id = controller_ids[controller_index];
        unit.command = mk_make_command_identity(command_name, callsign, side);
        mk_mosul_copy_text(unit.template_id, sizeof(unit.template_id), template_id);
        mk_mosul_copy_text(unit.group_id, sizeof(unit.group_id), group_id);
        mk_mosul_copy_text(unit.spawn_zone_id, sizeof(unit.spawn_zone_id), spawn_zone_id);
        mk_mosul_copy_text(unit.topology_node_id, sizeof(unit.topology_node_id), topology_node_id);
        mk_mosul_copy_text(unit.level_id, sizeof(unit.level_id), level_id);

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "hidden");
        if (!mk_mosul_optional_bool(entries, key, false, &hidden)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "revealed");
        if (!mk_mosul_optional_bool(entries, key, false, &revealed)) {
            return MK_ERROR_INVALID_DATA;
        }

        mk_mosul_make_indexed_key(key, sizeof(key), "unit", unit_index, "concealment");
        if (!mk_mosul_optional_int(entries, key, 0, &concealment)) {
            return MK_ERROR_INVALID_DATA;
        }

        if (concealment < 0) {
            return MK_ERROR_INVALID_DATA;
        }

        unit.hidden = hidden;
        unit.revealed = revealed;
        unit.concealment = concealment;

        for (soldier_index = 0; soldier_index < soldier_count; ++soldier_index) {
            char soldier_name[MK_NAME_CAPACITY];
            mk_soldier_role_t role;
            mk_vec2_t offset;
            size_t weapon_index = 0;
            mk_soldier_t soldier;

            mk_mosul_make_soldier_key(key, sizeof(key), unit_index, soldier_index, "name");
            if (!mk_mosul_required_text(entries, key, soldier_name, sizeof(soldier_name))) {
                return MK_ERROR_INVALID_DATA;
            }

            mk_mosul_make_soldier_key(key, sizeof(key), unit_index, soldier_index, "role");
            if (!mk_mosul_required_soldier_role(entries, key, &role)) {
                return MK_ERROR_INVALID_DATA;
            }

            mk_mosul_make_soldier_key(key, sizeof(key), unit_index, soldier_index, "weapon_index");
            if (!mk_mosul_required_index(entries, key, weapon_count, &weapon_index)) {
                return MK_ERROR_INVALID_DATA;
            }

            mk_mosul_make_soldier_key(key, sizeof(key), unit_index, soldier_index, "offset");
            if (!mk_mosul_required_vec2(entries, key, &offset)) {
                return MK_ERROR_INVALID_DATA;
            }

            soldier = mk_make_soldier(soldier_name, role, weapons[weapon_index]);
            soldier.offset_m = offset;

            mk_mosul_make_soldier_key(key, sizeof(key), unit_index, soldier_index, "ammo");
            if (!mk_mosul_optional_int(entries, key, soldier.ammo, &soldier.ammo)) {
                return MK_ERROR_INVALID_DATA;
            }

            result = mk_unit_add_soldier(&unit, &soldier, NULL);
            if (result != MK_OK) {
                return result;
            }
        }

        result = mk_scenario_add_unit(scenario, &unit, NULL);
        if (result != MK_OK) {
            return result;
        }
    }

    return MK_OK;
}

mk_result_t mk_mosul_load_scenario_file(
    const char *scenario_path,
    const char *project_root,
    mk_scenario_definition_t *out_scenario
) {
    mk_mosul_scenario_entry_list_t entries;
    mk_scenario_definition_t *scenario = NULL;
    mk_weapon_profile_t weapons[MK_MOSUL_SCENARIO_MAX_WEAPONS];
    uint32_t controller_ids[MK_MAX_CONTROLLERS];
    uint32_t faction_ids[MK_MAX_FACTIONS];
    uint32_t force_ids[MK_MAX_FORCES];
    size_t controller_count = 0;
    size_t faction_count = 0;
    size_t force_count = 0;
    size_t weapon_count = 0;
    char format[64];
    mk_game_t *validation_game = NULL;
    mk_result_t result;

    if (scenario_path == NULL || out_scenario == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    scenario = (mk_scenario_definition_t *)calloc(1, sizeof(*scenario));
    validation_game = (mk_game_t *)calloc(1, sizeof(*validation_game));
    if (scenario == NULL || validation_game == NULL) {
        result = MK_ERROR_CAPACITY;
        goto cleanup;
    }

    result = mk_mosul_read_entries(scenario_path, &entries);
    if (result != MK_OK) {
        goto cleanup;
    }

    if (!mk_mosul_required_text(&entries, "format", format, sizeof(format))
        || strcmp(format, "modernerKrieg.scenario.v1") != 0) {
        result = MK_ERROR_INVALID_DATA;
        goto cleanup;
    }

    memset(weapons, 0, sizeof(weapons));
    memset(controller_ids, 0, sizeof(controller_ids));
    memset(faction_ids, 0, sizeof(faction_ids));
    memset(force_ids, 0, sizeof(force_ids));

    if (!mk_mosul_required_text(&entries, "name", scenario->name, sizeof(scenario->name))
        || !mk_mosul_required_u64(&entries, "seed", &scenario->seed)) {
        result = MK_ERROR_INVALID_DATA;
        goto cleanup;
    }

    result = mk_mosul_load_briefing_and_scoring(&entries, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_map(&entries, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_validate_asset_references(&entries, project_root, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_tile_ranges(&entries, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_terrain(&entries, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_controllers(&entries, scenario, controller_ids, &controller_count);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_factions(&entries, scenario, faction_ids, &faction_count);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_forces(
        &entries,
        scenario,
        controller_ids,
        controller_count,
        faction_ids,
        faction_count,
        force_ids,
        &force_count
    );
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_spawn_zones(&entries, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_unit_templates(&entries, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_civilian_archetypes(&entries, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_civilian_groups(&entries, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_objectives(&entries, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_weapons(&entries, weapons, &weapon_count);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_civilians(&entries, scenario, faction_ids, faction_count);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_traffic_vehicles(&entries, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_mosul_load_units(
        &entries,
        scenario,
        controller_ids,
        controller_count,
        faction_ids,
        faction_count,
        force_ids,
        force_count,
        weapons,
        weapon_count
    );
    if (result != MK_OK) {
        goto cleanup;
    }

    result = mk_game_load_scenario(validation_game, scenario);
    if (result != MK_OK) {
        goto cleanup;
    }

    *out_scenario = *scenario;
    result = MK_OK;

cleanup:
    free(validation_game);
    free(scenario);
    return result;
}
