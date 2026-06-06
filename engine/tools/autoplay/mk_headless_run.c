#include "mk_ai.h"
#include "mk_core.h"
#include "mk_log.h"
#include "mk_mosul_demo.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MK_PROJECT_SOURCE_DIR
#define MK_PROJECT_SOURCE_DIR "."
#endif

typedef struct {
    bool quiet;
    FILE *transcript;
} mk_headless_run_observer_t;

static const char *mk_headless_order_name(mk_order_t order) {
    switch (order) {
        case MK_ORDER_HOLD:
            return "hold";
        case MK_ORDER_MOVE:
            return "move";
        case MK_ORDER_ASSAULT_MOVE:
            return "assault_move";
        case MK_ORDER_FIRE:
            return "fire";
        case MK_ORDER_SUPPRESS:
            return "suppress";
        case MK_ORDER_OVERWATCH:
            return "overwatch";
        case MK_ORDER_BREACH:
            return "breach";
        case MK_ORDER_RALLY:
            return "rally";
        case MK_ORDER_NONE:
        default:
            return "none";
    }
}

static const char *mk_headless_status_name(mk_unit_status_t status) {
    switch (status) {
        case MK_UNIT_READY:
            return "ready";
        case MK_UNIT_SUPPRESSED:
            return "suppressed";
        case MK_UNIT_PINNED:
            return "pinned";
        case MK_UNIT_BROKEN:
            return "broken";
        default:
            return "unknown";
    }
}

static void mk_headless_print_tick(FILE *stream, const mk_game_t *game) {
    size_t unit_index;

    fprintf(stream, "tick %u", game->tick);

    for (unit_index = 0; unit_index < game->unit_count; ++unit_index) {
        const mk_unit_t *unit = &game->units[unit_index];

        fprintf(
            stream,
            " | unit %u \"%s\" pos=(%.2f,%.2f) order=%s status=%s",
            unit->id,
            unit->name,
            unit->position_m.x,
            unit->position_m.y,
            mk_headless_order_name(unit->order),
            mk_headless_status_name(unit->status)
        );
    }

    fprintf(stream, "\n");
}

static mk_result_t mk_headless_observe_tick(const mk_game_t *game, void *user_data) {
    mk_headless_run_observer_t *observer = (mk_headless_run_observer_t *)user_data;

    if (game == NULL || observer == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!observer->quiet) {
        mk_headless_print_tick(stdout, game);
    }

    if (observer->transcript != NULL) {
        mk_headless_print_tick(observer->transcript, game);
    }

    return MK_OK;
}

static bool mk_headless_parse_u32(const char *text, uint32_t *out_value) {
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

static bool mk_headless_parse_u64(const char *text, uint64_t *out_value) {
    char *end = NULL;
    unsigned long long parsed;

    if (text == NULL || out_value == NULL || text[0] == '\0') {
        return false;
    }

    errno = 0;
    parsed = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return false;
    }

    *out_value = (uint64_t)parsed;
    return true;
}

static void mk_headless_print_usage(const char *program_name) {
    fprintf(
        stderr,
        "usage: %s [--scenario PATH] [--project-root PATH] [--steps N|--max-ticks N] [--seed N] [--quiet] [--transcript PATH] [--ai-only]\n"
        "\n"
        "Runs a MOSUL scenario headlessly for deterministic smoke tests and AI-only runs.\n",
        program_name
    );
}

static bool mk_headless_make_project_path(
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
        size_t length = strlen(relative_path);

        if (length >= capacity) {
            return false;
        }

        memcpy(out_path, relative_path, length + 1);
        return true;
    }

    written = snprintf(out_path, capacity, "%s/%s", root, relative_path);
    return written > 0 && (size_t)written < capacity;
}

static mk_result_t mk_headless_load_scenario(
    const char *scenario_path,
    const char *project_root,
    mk_scenario_definition_t *out_scenario
) {
    mk_result_t result;
    char project_path[512];

    if (scenario_path == NULL) {
        return mk_mosul_make_market_2003_scenario(out_scenario);
    }

    result = mk_mosul_load_scenario_file(scenario_path, project_root, out_scenario);
    if (result != MK_ERROR_NOT_FOUND || scenario_path[0] == '/') {
        return result;
    }

    if (!mk_headless_make_project_path(project_root, scenario_path, project_path, sizeof(project_path))) {
        return MK_ERROR_INVALID_DATA;
    }

    return mk_mosul_load_scenario_file(project_path, project_root, out_scenario);
}

static void mk_headless_print_header(FILE *stream, const mk_game_t *game, uint32_t steps) {
    fprintf(
        stream,
        "modernerKrieg headless run: scenario=\"%s\" seed=%llu steps=%u\n",
        game->scenario_name,
        (unsigned long long)game->rng_state,
        steps
    );
}

static void mk_headless_print_result(FILE *stream, const mk_game_t *game) {
    fprintf(stream, "result: ok tick=%u units=%u\n", game->tick, (unsigned)game->unit_count);
}

static mk_result_t mk_headless_run_steps(
    mk_game_t *game,
    uint32_t steps,
    bool ai_only,
    mk_headless_run_observer_t *observer
) {
    uint32_t index;

    if (!ai_only) {
        return mk_game_run_fixed_steps(game, steps, mk_headless_observe_tick, observer);
    }

    for (index = 0; index < steps; ++index) {
        mk_result_t result = mk_ai_issue_basic_orders(game);

        if (result != MK_OK) {
            return result;
        }

        result = mk_game_run_fixed_steps(game, 1, mk_headless_observe_tick, observer);
        if (result != MK_OK) {
            return result;
        }
    }

    return MK_OK;
}

int main(int argc, char **argv) {
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_headless_run_observer_t observer;
    const char *scenario_path = NULL;
    const char *project_root = MK_PROJECT_SOURCE_DIR;
    const char *transcript_path = NULL;
    uint32_t steps = 10;
    uint64_t seed = 0;
    bool has_seed = false;
    bool ai_only = false;
    int arg_index;
    mk_result_t result;

    observer.quiet = false;
    observer.transcript = NULL;

    for (arg_index = 1; arg_index < argc; ++arg_index) {
        const char *argument = argv[arg_index];

        if (strcmp(argument, "--help") == 0) {
            mk_headless_print_usage(argv[0]);
            return 0;
        }

        if (strcmp(argument, "--quiet") == 0) {
            observer.quiet = true;
            continue;
        }

        if (strcmp(argument, "--ai-only") == 0) {
            ai_only = true;
            continue;
        }

        if (strcmp(argument, "--steps") == 0 || strcmp(argument, "--max-ticks") == 0) {
            if (arg_index + 1 >= argc || !mk_headless_parse_u32(argv[arg_index + 1], &steps)) {
                mk_headless_print_usage(argv[0]);
                return 2;
            }

            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--seed") == 0) {
            if (arg_index + 1 >= argc || !mk_headless_parse_u64(argv[arg_index + 1], &seed)) {
                mk_headless_print_usage(argv[0]);
                return 2;
            }

            has_seed = true;
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--scenario") == 0) {
            if (arg_index + 1 >= argc) {
                mk_headless_print_usage(argv[0]);
                return 2;
            }

            scenario_path = argv[arg_index + 1];
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--project-root") == 0) {
            if (arg_index + 1 >= argc) {
                mk_headless_print_usage(argv[0]);
                return 2;
            }

            project_root = argv[arg_index + 1];
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--transcript") == 0) {
            if (arg_index + 1 >= argc) {
                mk_headless_print_usage(argv[0]);
                return 2;
            }

            transcript_path = argv[arg_index + 1];
            arg_index += 1;
            continue;
        }

        mk_headless_print_usage(argv[0]);
        return 2;
    }

    if (transcript_path != NULL) {
        observer.transcript = fopen(transcript_path, "w");
        if (observer.transcript == NULL) {
            fprintf(stderr, "failed to open transcript \"%s\"\n", transcript_path);
            return 1;
        }
    }

    result = mk_headless_load_scenario(scenario_path, project_root, &scenario);
    if (result != MK_OK) {
        fprintf(stderr, "failed to load Mosul scenario: %s\n", mk_result_name(result));
        if (observer.transcript != NULL) {
            fclose(observer.transcript);
        }
        return 1;
    }

    if (has_seed) {
        scenario.seed = seed;
    }

    result = mk_game_load_scenario(&game, &scenario);
    if (result != MK_OK) {
        fprintf(stderr, "failed to load Mosul scenario: %s\n", mk_result_name(result));
        if (observer.transcript != NULL) {
            fclose(observer.transcript);
        }
        return 1;
    }

    if (!observer.quiet) {
        mk_headless_print_header(stdout, &game, steps);
    }

    if (observer.transcript != NULL) {
        mk_headless_print_header(observer.transcript, &game, steps);
    }

    result = mk_headless_run_steps(&game, steps, ai_only, &observer);
    if (result != MK_OK) {
        fprintf(stderr, "headless run stopped: %s\n", mk_result_name(result));
        if (observer.transcript != NULL) {
            fclose(observer.transcript);
        }
        return 1;
    }

    if (!observer.quiet) {
        mk_headless_print_result(stdout, &game);
    }

    if (observer.transcript != NULL) {
        mk_headless_print_result(observer.transcript, &game);
        fclose(observer.transcript);
    }

    return 0;
}
