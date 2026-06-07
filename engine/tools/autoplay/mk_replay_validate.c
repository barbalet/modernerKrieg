#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *path;
    const char *expect_result;
    const char *expect_outcome;
    uint32_t from_tick;
    uint32_t to_tick;
    bool has_from_tick;
    bool has_to_tick;
    bool playback;
    bool quiet;
} mk_replay_validate_config_t;

typedef struct {
    uint32_t expected_steps;
    uint32_t expected_units;
    uint32_t expected_civilians;
    uint32_t expected_objectives;
    uint32_t initial_contacts;
    uint32_t current_tick;
    uint32_t current_units;
    uint32_t current_civilians;
    uint32_t current_objectives;
    uint32_t current_scores;
    uint32_t current_contacts;
    uint32_t current_controlled;
    uint32_t current_contested;
    uint32_t last_contact_id;
    uint32_t total_contacts;
    uint32_t final_tick;
    int final_score;
    int current_score;
    int current_civilian_risk;
    char current_outcome[32];
    char final_result[32];
    char final_outcome[32];
    bool have_header;
    bool have_start;
    bool have_gameplay_area;
    bool have_tick;
    bool have_end;
} mk_replay_state_t;

static void mk_replay_print_usage(const char *program_name) {
    fprintf(
        stderr,
        "usage: %s [--quiet] [--playback] [--from-tick N] [--to-tick N] [--expect-result RESULT] [--expect-outcome OUTCOME] REPLAY_PATH\n"
        "\n"
        "Validates a modernerKrieg text replay/event file and can print a compact per-tick playback summary.\n",
        program_name
    );
}

static bool mk_replay_parse_u32(const char *text, uint32_t *out_value) {
    char *end = NULL;
    unsigned long parsed;

    if (text == NULL || out_value == NULL || text[0] == '\0') {
        return false;
    }

    errno = 0;
    parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed > UINT32_MAX) {
        return false;
    }

    *out_value = (uint32_t)parsed;
    return true;
}

static bool mk_replay_parse_i32(const char *text, int *out_value) {
    char *end = NULL;
    long parsed;

    if (text == NULL || out_value == NULL || text[0] == '\0') {
        return false;
    }

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed < INT32_MIN || parsed > INT32_MAX) {
        return false;
    }

    *out_value = (int)parsed;
    return true;
}

static bool mk_replay_parse_number(const char *text) {
    char *end = NULL;

    if (text == NULL || text[0] == '\0') {
        return false;
    }

    errno = 0;
    (void)strtod(text, &end);
    return errno == 0 && end != text && *end == '\0';
}

static bool mk_replay_copy_text(char *out_text, size_t capacity, const char *text) {
    size_t length;

    if (out_text == NULL || capacity == 0 || text == NULL) {
        return false;
    }

    length = strlen(text);
    if (length >= capacity) {
        return false;
    }

    memcpy(out_text, text, length + 1);
    return true;
}

static const char *mk_replay_find_field(const char *line, const char *field) {
    size_t field_length;
    const char *cursor;

    if (line == NULL || field == NULL) {
        return NULL;
    }

    field_length = strlen(field);
    cursor = line;
    while ((cursor = strstr(cursor, field)) != NULL) {
        if ((cursor == line || cursor[-1] == ' ') && cursor[field_length] == '=') {
            return cursor + field_length + 1;
        }

        cursor += 1;
    }

    return NULL;
}

static bool mk_replay_read_field_word(const char *line, const char *field, char *out_word, size_t capacity) {
    const char *value = mk_replay_find_field(line, field);
    size_t length = 0;

    if (value == NULL || out_word == NULL || capacity == 0) {
        return false;
    }

    while (value[length] != '\0' && value[length] != ' ' && value[length] != '\r' && value[length] != '\n') {
        length += 1;
    }

    if (length == 0 || length >= capacity) {
        return false;
    }

    memcpy(out_word, value, length);
    out_word[length] = '\0';
    return true;
}

static bool mk_replay_has_quoted_field(const char *line, const char *field) {
    const char *value = mk_replay_find_field(line, field);
    const char *end;

    if (value == NULL || value[0] != '"') {
        return false;
    }

    end = strchr(value + 1, '"');
    return end != NULL;
}

static bool mk_replay_read_field_u32(const char *line, const char *field, uint32_t *out_value) {
    char word[64];

    return mk_replay_read_field_word(line, field, word, sizeof(word))
        && mk_replay_parse_u32(word, out_value);
}

static bool mk_replay_read_field_i32(const char *line, const char *field, int *out_value) {
    char word[64];

    return mk_replay_read_field_word(line, field, word, sizeof(word))
        && mk_replay_parse_i32(word, out_value);
}

static bool mk_replay_read_field_number(const char *line, const char *field) {
    char word[64];

    return mk_replay_read_field_word(line, field, word, sizeof(word))
        && mk_replay_parse_number(word);
}

static bool mk_replay_word_is_one_of(const char *word, const char *const *values, size_t value_count) {
    size_t index;

    if (word == NULL) {
        return false;
    }

    for (index = 0; index < value_count; ++index) {
        if (strcmp(word, values[index]) == 0) {
            return true;
        }
    }

    return false;
}

static bool mk_replay_read_enum(
    const char *line,
    const char *field,
    const char *const *values,
    size_t value_count,
    char *out_word,
    size_t capacity
) {
    if (!mk_replay_read_field_word(line, field, out_word, capacity)) {
        return false;
    }

    return mk_replay_word_is_one_of(out_word, values, value_count);
}

static bool mk_replay_validate_bool_field(const char *line, const char *field) {
    char word[8];

    if (!mk_replay_read_field_word(line, field, word, sizeof(word))) {
        return false;
    }

    return strcmp(word, "0") == 0 || strcmp(word, "1") == 0;
}

static bool mk_replay_fail(
    const mk_replay_validate_config_t *config,
    uint32_t line_number,
    const char *message
) {
    if (config == NULL || !config->quiet) {
        if (line_number > 0) {
            fprintf(stderr, "replay invalid at line %u: %s\n", line_number, message);
        } else {
            fprintf(stderr, "replay invalid: %s\n", message);
        }
    }

    return false;
}

static bool mk_replay_finalize_tick(
    const mk_replay_validate_config_t *config,
    const mk_replay_state_t *state,
    uint32_t line_number
) {
    if (state == NULL || !state->have_tick || state->current_tick == 0U) {
        return true;
    }

    if (state->current_units != state->expected_units) {
        return mk_replay_fail(config, line_number, "tick does not contain the expected unit records");
    }

    if (state->expected_civilians > 0U && state->current_civilians != state->expected_civilians) {
        return mk_replay_fail(config, line_number, "tick does not contain the expected civilian records");
    }

    if (state->current_objectives != state->expected_objectives) {
        return mk_replay_fail(config, line_number, "tick does not contain the expected objective records");
    }

    if (state->current_scores != 1U) {
        return mk_replay_fail(config, line_number, "tick does not contain exactly one score record");
    }

    if (config != NULL && config->playback && !config->quiet) {
        bool in_range = true;

        if (config->has_from_tick && state->current_tick < config->from_tick) {
            in_range = false;
        }
        if (config->has_to_tick && state->current_tick > config->to_tick) {
            in_range = false;
        }

        if (in_range) {
            printf(
                "replay tick=%u units=%u objectives=%u contacts=%u score=%d outcome=%s controlled=%u contested=%u civilian_risk=%d\n",
                state->current_tick,
                state->current_units,
                state->current_objectives,
                state->current_contacts,
                state->current_score,
                state->current_outcome[0] == '\0' ? "unknown" : state->current_outcome,
                state->current_controlled,
                state->current_contested,
                state->current_civilian_risk
            );
        }
    }

    return true;
}

static bool mk_replay_begin_tick(
    const mk_replay_validate_config_t *config,
    mk_replay_state_t *state,
    uint32_t tick,
    uint32_t line_number
) {
    if (state->have_tick) {
        if (tick < state->current_tick) {
            return mk_replay_fail(config, line_number, "event ticks moved backward");
        }

        if (tick == state->current_tick) {
            return true;
        }

        if (!mk_replay_finalize_tick(config, state, line_number)) {
            return false;
        }
    }

    state->current_tick = tick;
    state->current_units = 0;
    state->current_civilians = 0;
    state->current_objectives = 0;
    state->current_scores = 0;
    state->current_contacts = 0;
    state->current_controlled = 0;
    state->current_contested = 0;
    state->current_score = 0;
    state->current_civilian_risk = 0;
    state->current_outcome[0] = '\0';
    state->have_tick = true;
    return true;
}

static bool mk_replay_validate_header(
    const mk_replay_validate_config_t *config,
    mk_replay_state_t *state,
    const char *line,
    uint32_t line_number
) {
    char version[16];

    if (strncmp(line, "mk_replay ", 10) != 0) {
        return mk_replay_fail(config, line_number, "first line must be a replay header");
    }

    if (!mk_replay_read_field_word(line, "version", version, sizeof(version)) || strcmp(version, "1") != 0) {
        return mk_replay_fail(config, line_number, "unsupported replay version");
    }

    if (!mk_replay_has_quoted_field(line, "scenario")
        || !mk_replay_read_field_u32(line, "steps", &state->expected_steps)
        || !mk_replay_validate_bool_field(line, "ai_only")) {
        return mk_replay_fail(config, line_number, "replay header is missing required fields");
    }

    state->have_header = true;
    return true;
}

static bool mk_replay_validate_start_event(
    const mk_replay_validate_config_t *config,
    mk_replay_state_t *state,
    const char *line,
    uint32_t tick,
    uint32_t line_number
) {
    if (state->have_start) {
        return mk_replay_fail(config, line_number, "replay contains multiple start events");
    }

    if (tick != 0U) {
        return mk_replay_fail(config, line_number, "start event must be at tick 0");
    }

    if (!mk_replay_read_field_u32(line, "units", &state->expected_units)
        || !mk_replay_read_field_u32(line, "civilians", &state->expected_civilians)
        || !mk_replay_read_field_u32(line, "objectives", &state->expected_objectives)
        || !mk_replay_read_field_u32(line, "contacts", &state->initial_contacts)) {
        return mk_replay_fail(config, line_number, "start event is missing required counts");
    }

    state->total_contacts = state->initial_contacts;
    state->last_contact_id = state->initial_contacts;
    state->have_start = true;
    return true;
}

static bool mk_replay_validate_unit_event(
    const mk_replay_validate_config_t *config,
    mk_replay_state_t *state,
    const char *line,
    uint32_t line_number
) {
    static const char *const sides[] = {"neutral", "player", "opfor", "civilian"};
    static const char *const orders[] = {
        "none",
        "hold",
        "move",
        "assault_move",
        "fire",
        "suppress",
        "overwatch",
        "breach",
        "rally",
        "withdraw",
        "investigate"
    };
    static const char *const statuses[] = {"ready", "suppressed", "pinned", "broken"};
    char word[32];
    uint32_t id;

    if (!state->have_start) {
        return mk_replay_fail(config, line_number, "unit event appeared before start");
    }

    if (!mk_replay_read_field_u32(line, "id", &id) || id == 0U) {
        return mk_replay_fail(config, line_number, "unit event has an invalid id");
    }

    if (!mk_replay_read_enum(line, "side", sides, sizeof(sides) / sizeof(sides[0]), word, sizeof(word))
        || !mk_replay_read_enum(line, "order", orders, sizeof(orders) / sizeof(orders[0]), word, sizeof(word))
        || !mk_replay_read_enum(line, "status", statuses, sizeof(statuses) / sizeof(statuses[0]), word, sizeof(word))
        || !mk_replay_read_field_number(line, "x")
        || !mk_replay_read_field_number(line, "y")
        || !mk_replay_read_field_number(line, "target_x")
        || !mk_replay_read_field_number(line, "target_y")
        || !mk_replay_validate_bool_field(line, "has_target")
        || !mk_replay_validate_bool_field(line, "hidden")
        || !mk_replay_validate_bool_field(line, "revealed")) {
        return mk_replay_fail(config, line_number, "unit event is missing required fields");
    }

    state->current_units += 1;
    return true;
}

static bool mk_replay_validate_population_event(
    const mk_replay_validate_config_t *config,
    const mk_replay_state_t *state,
    const char *line,
    uint32_t tick,
    uint32_t line_number
) {
    uint32_t scratch_count;

    if (state == NULL || !state->have_start) {
        return mk_replay_fail(config, line_number, "population event appeared before start");
    }

    if (tick != 0U) {
        return mk_replay_fail(config, line_number, "population event must be at tick 0");
    }

    if (!mk_replay_read_field_u32(line, "spawn_zones", &scratch_count)
        || !mk_replay_read_field_u32(line, "unit_templates", &scratch_count)
        || !mk_replay_read_field_u32(line, "civilian_archetypes", &scratch_count)
        || !mk_replay_read_field_u32(line, "civilian_groups", &scratch_count)) {
        return mk_replay_fail(config, line_number, "population event is missing required counts");
    }

    return true;
}

static bool mk_replay_validate_civilian_event(
    const mk_replay_validate_config_t *config,
    mk_replay_state_t *state,
    const char *line,
    uint32_t line_number
) {
    static const char *const intents[] = {
        "none",
        "shelter",
        "flee",
        "evacuate",
        "follow_instructions",
        "freeze",
        "assist_group"
    };
    char word[32];
    uint32_t id;
    int scratch_int;

    if (state == NULL || !state->have_start) {
        return mk_replay_fail(config, line_number, "civilian event appeared before start");
    }

    if (!mk_replay_read_field_u32(line, "id", &id)
        || id == 0U
        || !mk_replay_has_quoted_field(line, "archetype")
        || !mk_replay_has_quoted_field(line, "group")
        || !mk_replay_has_quoted_field(line, "spawn")
        || !mk_replay_has_quoted_field(line, "node")
        || !mk_replay_has_quoted_field(line, "level")
        || !mk_replay_read_field_number(line, "x")
        || !mk_replay_read_field_number(line, "y")
        || !mk_replay_read_enum(line, "intent", intents, sizeof(intents) / sizeof(intents[0]), word, sizeof(word))
        || !mk_replay_read_field_number(line, "dest_x")
        || !mk_replay_read_field_number(line, "dest_y")
        || !mk_replay_validate_bool_field(line, "has_destination")
        || !mk_replay_validate_bool_field(line, "has_route")
        || !mk_replay_read_field_u32(line, "route_step", &id)
        || !mk_replay_read_field_u32(line, "route_steps", &id)
        || !mk_replay_read_field_i32(line, "route_cost", &scratch_int)
        || !mk_replay_read_field_u32(line, "route_failures", &id)
        || !mk_replay_has_quoted_field(line, "route_reason")
        || !mk_replay_read_field_i32(line, "stress", &scratch_int)
        || !mk_replay_read_field_i32(line, "risk", &scratch_int)
        || !mk_replay_read_field_i32(line, "compliance", &scratch_int)
        || !mk_replay_validate_bool_field(line, "protected")
        || !mk_replay_has_quoted_field(line, "sprite")) {
        return mk_replay_fail(config, line_number, "civilian event is missing required fields");
    }

    state->current_civilians += 1U;
    return true;
}

static bool mk_replay_validate_gameplay_area_event(
    const mk_replay_validate_config_t *config,
    mk_replay_state_t *state,
    const char *line,
    uint32_t tick,
    uint32_t line_number
) {
    uint32_t levels;
    uint32_t features;
    uint32_t regions;
    uint32_t pixel_width;
    uint32_t pixel_height;

    if (!state->have_start) {
        return mk_replay_fail(config, line_number, "gameplay area event appeared before start");
    }

    if (state->have_gameplay_area) {
        return mk_replay_fail(config, line_number, "replay contains multiple gameplay area events");
    }

    if (tick != 0U) {
        return mk_replay_fail(config, line_number, "gameplay area event must be at tick 0");
    }

    if (!mk_replay_has_quoted_field(line, "id")
        || !mk_replay_has_quoted_field(line, "map")
        || !mk_replay_read_field_u32(line, "levels", &levels)
        || !mk_replay_read_field_u32(line, "features", &features)
        || !mk_replay_read_field_u32(line, "regions", &regions)
        || !mk_replay_read_field_u32(line, "pixel_width", &pixel_width)
        || !mk_replay_read_field_u32(line, "pixel_height", &pixel_height)
        || !mk_replay_read_field_number(line, "ppm")
        || levels == 0U
        || pixel_width == 0U
        || pixel_height == 0U) {
        return mk_replay_fail(config, line_number, "gameplay area event is missing required fields");
    }

    (void)features;
    (void)regions;
    state->have_gameplay_area = true;
    return true;
}

static bool mk_replay_validate_objective_event(
    const mk_replay_validate_config_t *config,
    mk_replay_state_t *state,
    const char *line,
    uint32_t line_number
) {
    static const char *const sides[] = {"neutral", "player", "opfor", "civilian"};
    char side[32];
    uint32_t id;
    int value;

    if (!state->have_start) {
        return mk_replay_fail(config, line_number, "objective event appeared before start");
    }

    if (!mk_replay_read_field_u32(line, "id", &id)
        || id == 0U
        || !mk_replay_has_quoted_field(line, "label")
        || !mk_replay_read_enum(line, "side", sides, sizeof(sides) / sizeof(sides[0]), side, sizeof(side))
        || !mk_replay_read_field_i32(line, "value", &value)) {
        return mk_replay_fail(config, line_number, "objective event is missing required fields");
    }

    state->current_objectives += 1;
    return true;
}

static bool mk_replay_validate_score_event(
    const mk_replay_validate_config_t *config,
    mk_replay_state_t *state,
    const char *line,
    uint32_t line_number
) {
    static const char *const outcomes[] = {"in_progress", "success", "partial", "failure"};
    char outcome[32];
    int total;
    uint32_t controlled;
    uint32_t contested;
    int civilian_risk;

    if (!state->have_start) {
        return mk_replay_fail(config, line_number, "score event appeared before start");
    }

    if (state->current_scores > 0U) {
        return mk_replay_fail(config, line_number, "tick contains multiple score events");
    }

    if (!mk_replay_read_field_i32(line, "total", &total)
        || !mk_replay_read_enum(line, "outcome", outcomes, sizeof(outcomes) / sizeof(outcomes[0]), outcome, sizeof(outcome))
        || !mk_replay_read_field_u32(line, "controlled", &controlled)
        || !mk_replay_read_field_u32(line, "contested", &contested)
        || !mk_replay_read_field_i32(line, "civilian_risk", &civilian_risk)
        || civilian_risk < 0) {
        return mk_replay_fail(config, line_number, "score event is missing required fields");
    }

    if (!mk_replay_copy_text(state->current_outcome, sizeof(state->current_outcome), outcome)) {
        return mk_replay_fail(config, line_number, "score outcome is too long");
    }

    state->current_score = total;
    state->current_controlled = controlled;
    state->current_contested = contested;
    state->current_civilian_risk = civilian_risk;
    state->current_scores += 1;
    return true;
}

static bool mk_replay_validate_contact_event(
    const mk_replay_validate_config_t *config,
    mk_replay_state_t *state,
    const char *line,
    uint32_t line_number
) {
    static const char *const sides[] = {"neutral", "player", "opfor", "civilian"};
    static const char *const contacts[] = {
        "fire",
        "reveal",
        "civilian_risk",
        "suspected_danger",
        "false_contact",
        "search"
    };
    char word[32];
    uint32_t id;
    uint32_t scratch_id;
    int confidence;

    if (!state->have_start) {
        return mk_replay_fail(config, line_number, "contact event appeared before start");
    }

    if (!mk_replay_read_field_u32(line, "id", &id) || id == 0U || id <= state->last_contact_id) {
        return mk_replay_fail(config, line_number, "contact event ids must increase");
    }

    if (!mk_replay_read_enum(line, "contact", contacts, sizeof(contacts) / sizeof(contacts[0]), word, sizeof(word))
        || !mk_replay_read_enum(line, "side", sides, sizeof(sides) / sizeof(sides[0]), word, sizeof(word))
        || !mk_replay_read_field_u32(line, "observer", &scratch_id)
        || !mk_replay_read_field_u32(line, "target", &scratch_id)
        || !mk_replay_read_field_u32(line, "civilian", &scratch_id)
        || !mk_replay_read_field_u32(line, "terrain", &scratch_id)
        || !mk_replay_read_field_i32(line, "confidence", &confidence)
        || !mk_replay_validate_bool_field(line, "resolved")
        || !mk_replay_read_field_number(line, "x")
        || !mk_replay_read_field_number(line, "y")
        || !mk_replay_read_field_number(line, "target_x")
        || !mk_replay_read_field_number(line, "target_y")) {
        return mk_replay_fail(config, line_number, "contact event is missing required fields");
    }

    state->last_contact_id = id;
    state->total_contacts += 1;
    state->current_contacts += 1;
    return true;
}

static bool mk_replay_validate_end_event(
    const mk_replay_validate_config_t *config,
    mk_replay_state_t *state,
    const char *line,
    uint32_t tick,
    uint32_t line_number
) {
    static const char *const outcomes[] = {"in_progress", "success", "partial", "failure"};
    char outcome[32];
    char result[32];
    int score;

    if (state->have_end) {
        return mk_replay_fail(config, line_number, "replay contains multiple end events");
    }

    if (!state->have_start) {
        return mk_replay_fail(config, line_number, "end event appeared before start");
    }

    if (!mk_replay_read_field_word(line, "result", result, sizeof(result))
        || !mk_replay_read_field_i32(line, "score", &score)
        || !mk_replay_read_enum(line, "outcome", outcomes, sizeof(outcomes) / sizeof(outcomes[0]), outcome, sizeof(outcome))) {
        return mk_replay_fail(config, line_number, "end event is missing required fields");
    }

    if (!mk_replay_copy_text(state->final_result, sizeof(state->final_result), result)
        || !mk_replay_copy_text(state->final_outcome, sizeof(state->final_outcome), outcome)) {
        return mk_replay_fail(config, line_number, "end event fields are too long");
    }

    state->final_tick = tick;
    state->final_score = score;
    state->have_end = true;
    return true;
}

static bool mk_replay_validate_event(
    const mk_replay_validate_config_t *config,
    mk_replay_state_t *state,
    const char *line,
    uint32_t line_number
) {
    char kind[32];
    uint32_t tick;

    if (!state->have_header) {
        return mk_replay_fail(config, line_number, "event appeared before header");
    }

    if (strncmp(line, "event ", 6) != 0) {
        return mk_replay_fail(config, line_number, "expected an event line");
    }

    if (state->have_end) {
        return mk_replay_fail(config, line_number, "event appeared after end");
    }

    if (!mk_replay_read_field_u32(line, "tick", &tick)
        || !mk_replay_read_field_word(line, "kind", kind, sizeof(kind))) {
        return mk_replay_fail(config, line_number, "event is missing tick or kind");
    }

    if (!mk_replay_begin_tick(config, state, tick, line_number)) {
        return false;
    }

    if (strcmp(kind, "start") == 0) {
        return mk_replay_validate_start_event(config, state, line, tick, line_number);
    }

    if (strcmp(kind, "unit") == 0) {
        return mk_replay_validate_unit_event(config, state, line, line_number);
    }

    if (strcmp(kind, "population") == 0) {
        return mk_replay_validate_population_event(config, state, line, tick, line_number);
    }

    if (strcmp(kind, "civilian") == 0) {
        return mk_replay_validate_civilian_event(config, state, line, line_number);
    }

    if (strcmp(kind, "gameplay_area") == 0) {
        return mk_replay_validate_gameplay_area_event(config, state, line, tick, line_number);
    }

    if (strcmp(kind, "objective") == 0) {
        return mk_replay_validate_objective_event(config, state, line, line_number);
    }

    if (strcmp(kind, "score") == 0) {
        return mk_replay_validate_score_event(config, state, line, line_number);
    }

    if (strcmp(kind, "contact") == 0) {
        return mk_replay_validate_contact_event(config, state, line, line_number);
    }

    if (strcmp(kind, "end") == 0) {
        return mk_replay_validate_end_event(config, state, line, tick, line_number);
    }

    return mk_replay_fail(config, line_number, "unknown event kind");
}

static bool mk_replay_strip_newline(char *line) {
    size_t length;

    if (line == NULL) {
        return false;
    }

    length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        line[length - 1] = '\0';
        length -= 1;
    }

    return true;
}

static bool mk_replay_validate_file(const mk_replay_validate_config_t *config, mk_replay_state_t *out_state) {
    FILE *file;
    mk_replay_state_t state;
    char line[2048];
    uint32_t line_number = 0;

    if (config == NULL || config->path == NULL || out_state == NULL) {
        return false;
    }

    file = fopen(config->path, "r");
    if (file == NULL) {
        return mk_replay_fail(config, 0, "could not open replay file");
    }

    memset(&state, 0, sizeof(state));
    while (fgets(line, sizeof(line), file) != NULL) {
        line_number += 1;
        mk_replay_strip_newline(line);

        if (line[0] == '\0') {
            continue;
        }

        if (!state.have_header) {
            if (!mk_replay_validate_header(config, &state, line, line_number)) {
                fclose(file);
                return false;
            }
            continue;
        }

        if (!mk_replay_validate_event(config, &state, line, line_number)) {
            fclose(file);
            return false;
        }
    }

    if (ferror(file) != 0) {
        fclose(file);
        return mk_replay_fail(config, 0, "could not read replay file");
    }

    fclose(file);

    if (!state.have_header) {
        return mk_replay_fail(config, 0, "replay file is empty");
    }

    if (!state.have_start) {
        return mk_replay_fail(config, 0, "replay does not contain a start event");
    }

    if (!state.have_end) {
        return mk_replay_fail(config, 0, "replay does not contain an end event");
    }

    if (!mk_replay_finalize_tick(config, &state, line_number)) {
        return false;
    }

    if (state.final_tick != state.expected_steps) {
        return mk_replay_fail(config, 0, "final tick does not match replay steps");
    }

    if (config->expect_result != NULL && strcmp(state.final_result, config->expect_result) != 0) {
        return mk_replay_fail(config, 0, "final result did not match expectation");
    }

    if (config->expect_outcome != NULL && strcmp(state.final_outcome, config->expect_outcome) != 0) {
        return mk_replay_fail(config, 0, "final outcome did not match expectation");
    }

    *out_state = state;
    return true;
}

static bool mk_replay_parse_arguments(int argc, char **argv, mk_replay_validate_config_t *config) {
    int arg_index;

    if (config == NULL) {
        return false;
    }

    memset(config, 0, sizeof(*config));
    for (arg_index = 1; arg_index < argc; ++arg_index) {
        const char *argument = argv[arg_index];

        if (strcmp(argument, "--help") == 0) {
            mk_replay_print_usage(argv[0]);
            exit(0);
        }

        if (strcmp(argument, "--quiet") == 0) {
            config->quiet = true;
            continue;
        }

        if (strcmp(argument, "--playback") == 0) {
            config->playback = true;
            continue;
        }

        if (strcmp(argument, "--from-tick") == 0) {
            if (arg_index + 1 >= argc || !mk_replay_parse_u32(argv[arg_index + 1], &config->from_tick)) {
                return false;
            }

            config->has_from_tick = true;
            config->playback = true;
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--to-tick") == 0) {
            if (arg_index + 1 >= argc || !mk_replay_parse_u32(argv[arg_index + 1], &config->to_tick)) {
                return false;
            }

            config->has_to_tick = true;
            config->playback = true;
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--expect-result") == 0) {
            if (arg_index + 1 >= argc) {
                return false;
            }

            config->expect_result = argv[arg_index + 1];
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--expect-outcome") == 0) {
            if (arg_index + 1 >= argc) {
                return false;
            }

            config->expect_outcome = argv[arg_index + 1];
            arg_index += 1;
            continue;
        }

        if (argument[0] == '-' || config->path != NULL) {
            return false;
        }

        config->path = argument;
    }

    if (config->has_from_tick && config->has_to_tick && config->from_tick > config->to_tick) {
        return false;
    }

    return config->path != NULL;
}

int main(int argc, char **argv) {
    mk_replay_validate_config_t config;
    mk_replay_state_t state;

    if (!mk_replay_parse_arguments(argc, argv, &config)) {
        mk_replay_print_usage(argv[0]);
        return 2;
    }

    if (!mk_replay_validate_file(&config, &state)) {
        return 1;
    }

    if (!config.quiet) {
        printf(
            "replay ok path=\"%s\" ticks=%u units=%u objectives=%u contacts=%u score=%d outcome=%s result=%s\n",
            config.path,
            state.final_tick,
            state.expected_units,
            state.expected_objectives,
            state.total_contacts,
            state.final_score,
            state.final_outcome,
            state.final_result
        );
    }

    return 0;
}
