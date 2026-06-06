#include "mk_core.h"
#include "mk_log.h"
#include "mk_mosul_demo.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bool quiet;
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

static void mk_headless_print_tick(const mk_game_t *game) {
    size_t unit_index;

    printf("tick %u", game->tick);

    for (unit_index = 0; unit_index < game->unit_count; ++unit_index) {
        const mk_unit_t *unit = &game->units[unit_index];

        printf(
            " | unit %u \"%s\" pos=(%.2f,%.2f) order=%s status=%s",
            unit->id,
            unit->name,
            unit->position_m.x,
            unit->position_m.y,
            mk_headless_order_name(unit->order),
            mk_headless_status_name(unit->status)
        );
    }

    printf("\n");
}

static mk_result_t mk_headless_observe_tick(const mk_game_t *game, void *user_data) {
    mk_headless_run_observer_t *observer = (mk_headless_run_observer_t *)user_data;

    if (game == NULL || observer == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!observer->quiet) {
        mk_headless_print_tick(game);
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
        "usage: %s [--steps N] [--seed N] [--quiet]\n"
        "\n"
        "Runs the East Mosul block scenario headlessly for deterministic smoke tests.\n",
        program_name
    );
}

int main(int argc, char **argv) {
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_headless_run_observer_t observer;
    uint32_t steps = 10;
    uint64_t seed = 0;
    bool has_seed = false;
    int arg_index;
    mk_result_t result;

    observer.quiet = false;

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

        if (strcmp(argument, "--steps") == 0) {
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

        mk_headless_print_usage(argv[0]);
        return 2;
    }

    result = mk_mosul_make_east_block_scenario(&scenario);
    if (result != MK_OK) {
        fprintf(stderr, "failed to create Mosul scenario: %s\n", mk_result_name(result));
        return 1;
    }

    if (has_seed) {
        scenario.seed = seed;
    }

    result = mk_game_load_scenario(&game, &scenario);
    if (result != MK_OK) {
        fprintf(stderr, "failed to load Mosul scenario: %s\n", mk_result_name(result));
        return 1;
    }

    if (!observer.quiet) {
        printf(
            "modernerKrieg headless run: scenario=\"%s\" seed=%llu steps=%u\n",
            game.scenario_name,
            (unsigned long long)game.rng_state,
            steps
        );
    }

    result = mk_game_run_fixed_steps(&game, steps, mk_headless_observe_tick, &observer);
    if (result != MK_OK) {
        fprintf(stderr, "headless run stopped: %s\n", mk_result_name(result));
        return 1;
    }

    if (!observer.quiet) {
        printf("result: ok tick=%u units=%u\n", game.tick, (unsigned)game.unit_count);
    }

    return 0;
}
