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
    const char *scenario_path;
    const char *project_root;
    uint32_t battles;
    uint32_t max_ticks;
    uint32_t summary_every;
    uint32_t watchdog_ticks;
    uint64_t seed;
    bool has_seed;
    bool forever;
    bool quiet;
    bool verbose;
    bool fail_on_stall;
} mk_ai_battle_config_t;

typedef struct {
    uint32_t battles_run;
    uint32_t stalled_battles;
    uint32_t failed_battles;
    int best_score;
    int worst_score;
} mk_ai_battle_totals_t;

static const char *mk_ai_battle_side_name(mk_side_t side) {
    switch (side) {
        case MK_SIDE_PLAYER:
            return "player";
        case MK_SIDE_OPFOR:
            return "opfor";
        case MK_SIDE_CIVILIAN:
            return "civilian";
        case MK_SIDE_NEUTRAL:
        default:
            return "neutral";
    }
}

static const char *mk_ai_battle_order_name(mk_order_t order) {
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
        case MK_ORDER_WITHDRAW:
            return "withdraw";
        case MK_ORDER_INVESTIGATE:
            return "investigate";
        case MK_ORDER_NONE:
        default:
            return "none";
    }
}

static const char *mk_ai_battle_outcome_name(mk_outcome_t outcome) {
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

static const char *mk_ai_battle_status_name(mk_unit_status_t status) {
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

static bool mk_ai_battle_parse_u32(const char *text, uint32_t *out_value) {
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

static bool mk_ai_battle_parse_u64(const char *text, uint64_t *out_value) {
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

static void mk_ai_battle_print_usage(const char *program_name) {
    fprintf(
        stderr,
        "usage: %s [--scenario PATH] [--project-root PATH] [--battles N|--forever] [--ticks N|--max-ticks N] [--seed N] [--summary-every N] [--watchdog N] [--fail-on-stall] [--quiet] [--verbose]\n"
        "\n"
        "Runs deterministic MOSUL AI-vs-AI battles with both tactical sides controlled by AI.\n",
        program_name
    );
}

static bool mk_ai_battle_make_project_path(
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

static mk_result_t mk_ai_battle_load_scenario(
    const mk_ai_battle_config_t *config,
    mk_scenario_definition_t *out_scenario
) {
    char project_path[512];
    const char *scenario_path;
    mk_result_t result;

    if (config == NULL || out_scenario == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    scenario_path = config->scenario_path != NULL ? config->scenario_path : MK_MOSUL_DEFAULT_SCENARIO_PATH;
    result = mk_mosul_load_scenario_file(scenario_path, config->project_root, out_scenario);
    if (result != MK_ERROR_NOT_FOUND || scenario_path[0] == '/') {
        return result;
    }

    if (!mk_ai_battle_make_project_path(config->project_root, scenario_path, project_path, sizeof(project_path))) {
        return MK_ERROR_INVALID_DATA;
    }

    return mk_mosul_load_scenario_file(project_path, config->project_root, out_scenario);
}

static void mk_ai_battle_force_tactical_ai(mk_scenario_definition_t *scenario) {
    size_t index;

    if (scenario == NULL) {
        return;
    }

    for (index = 0; index < scenario->controller_count; ++index) {
        mk_controller_slot_t *controller = &scenario->controllers[index];

        if (controller->side == MK_SIDE_PLAYER || controller->side == MK_SIDE_OPFOR) {
            controller->kind = MK_CONTROLLER_TACTICAL_AI;
        }
    }
}

static uint64_t mk_ai_battle_hash_u64(uint64_t hash, uint64_t value) {
    hash ^= value;
    hash *= UINT64_C(1099511628211);
    return hash;
}

static uint64_t mk_ai_battle_hash_i32(uint64_t hash, int value) {
    return mk_ai_battle_hash_u64(hash, (uint64_t)(uint32_t)value);
}

static int mk_ai_battle_quantize_position(float value) {
    return (int)(value * 100.0f);
}

static uint64_t mk_ai_battle_progress_signature(const mk_game_t *game, const mk_score_t *score) {
    uint64_t hash = UINT64_C(1469598103934665603);
    size_t index;

    if (game == NULL || score == NULL) {
        return 0;
    }

    hash = mk_ai_battle_hash_i32(hash, score->total_score);
    hash = mk_ai_battle_hash_u64(hash, game->contact_report_count);

    for (index = 0; index < game->objective_count; ++index) {
        const mk_objective_t *objective = &game->objectives[index];

        hash = mk_ai_battle_hash_u64(hash, objective->id);
        hash = mk_ai_battle_hash_u64(hash, (uint64_t)objective->controlling_side);
    }

    for (index = 0; index < game->unit_count; ++index) {
        const mk_unit_t *unit = &game->units[index];

        if (unit->side == MK_SIDE_CIVILIAN) {
            continue;
        }

        hash = mk_ai_battle_hash_u64(hash, unit->id);
        hash = mk_ai_battle_hash_u64(hash, (uint64_t)unit->order);
        hash = mk_ai_battle_hash_u64(hash, (uint64_t)unit->status);
        hash = mk_ai_battle_hash_i32(hash, mk_ai_battle_quantize_position(unit->position_m.x));
        hash = mk_ai_battle_hash_i32(hash, mk_ai_battle_quantize_position(unit->position_m.y));
        hash = mk_ai_battle_hash_i32(hash, mk_ai_battle_quantize_position(unit->target_position_m.x));
        hash = mk_ai_battle_hash_i32(hash, mk_ai_battle_quantize_position(unit->target_position_m.y));
        hash = mk_ai_battle_hash_u64(hash, unit->has_move_target ? 1U : 0U);
        hash = mk_ai_battle_hash_u64(hash, unit->revealed ? 1U : 0U);
    }

    return hash;
}

static void mk_ai_battle_count_objectives(
    const mk_game_t *game,
    uint32_t *out_player,
    uint32_t *out_opfor,
    uint32_t *out_neutral
) {
    uint32_t player = 0;
    uint32_t opfor = 0;
    uint32_t neutral = 0;
    size_t index;

    if (game != NULL) {
        for (index = 0; index < game->objective_count; ++index) {
            if (game->objectives[index].controlling_side == MK_SIDE_PLAYER) {
                player += 1;
            } else if (game->objectives[index].controlling_side == MK_SIDE_OPFOR) {
                opfor += 1;
            } else {
                neutral += 1;
            }
        }
    }

    if (out_player != NULL) {
        *out_player = player;
    }
    if (out_opfor != NULL) {
        *out_opfor = opfor;
    }
    if (out_neutral != NULL) {
        *out_neutral = neutral;
    }
}

static void mk_ai_battle_print_unit_line(const mk_game_t *game) {
    size_t index;

    if (game == NULL) {
        return;
    }

    for (index = 0; index < game->unit_count; ++index) {
        const mk_unit_t *unit = &game->units[index];

        if (unit->side == MK_SIDE_CIVILIAN) {
            continue;
        }

        printf(
            "    unit=%u side=%s order=%s status=%s pos=(%.2f,%.2f) target=(%.2f,%.2f) contacts=%u\n",
            unit->id,
            mk_ai_battle_side_name(unit->side),
            mk_ai_battle_order_name(unit->order),
            mk_ai_battle_status_name(unit->status),
            unit->position_m.x,
            unit->position_m.y,
            unit->target_position_m.x,
            unit->target_position_m.y,
            (unsigned)game->contact_report_count
        );
    }
}

static void mk_ai_battle_print_summary(
    uint32_t battle_index,
    const mk_game_t *game,
    const mk_score_t *score,
    bool verbose
) {
    uint32_t player_objectives = 0;
    uint32_t opfor_objectives = 0;
    uint32_t neutral_objectives = 0;

    if (game == NULL || score == NULL) {
        return;
    }

    mk_ai_battle_count_objectives(game, &player_objectives, &opfor_objectives, &neutral_objectives);
    printf(
        "battle=%u tick=%u score=%d outcome=%s objectives(player=%u,opfor=%u,neutral=%u,contested=%u) contacts=%u risk=%d\n",
        battle_index,
        game->tick,
        score->total_score,
        mk_ai_battle_outcome_name(score->outcome),
        player_objectives,
        opfor_objectives,
        neutral_objectives,
        (unsigned)score->contested_objectives,
        (unsigned)game->contact_report_count,
        score->civilian_risk
    );

    if (verbose) {
        mk_ai_battle_print_unit_line(game);
    }
}

static mk_result_t mk_ai_battle_run_one(
    const mk_ai_battle_config_t *config,
    uint32_t battle_index,
    mk_ai_battle_totals_t *totals
) {
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_score_t score;
    mk_result_t result;
    uint64_t previous_signature = 0;
    uint32_t stagnant_ticks = 0;
    uint32_t tick_index;
    bool stalled = false;

    result = mk_ai_battle_load_scenario(config, &scenario);
    if (result != MK_OK) {
        fprintf(stderr, "battle=%u failed to load scenario: %s\n", battle_index, mk_result_name(result));
        return result;
    }

    if (config->has_seed) {
        scenario.seed = config->seed + (uint64_t)(battle_index - 1U);
    } else {
        scenario.seed += (uint64_t)(battle_index - 1U) * UINT64_C(0x9E3779B97F4A7C15);
    }

    mk_ai_battle_force_tactical_ai(&scenario);
    result = mk_game_load_scenario(&game, &scenario);
    if (result != MK_OK) {
        fprintf(stderr, "battle=%u failed to enter scenario: %s\n", battle_index, mk_result_name(result));
        return result;
    }

    if (!config->quiet) {
        printf(
            "battle=%u start scenario=\"%s\" seed=%llu max_ticks=%u\n",
            battle_index,
            game.scenario_name,
            (unsigned long long)game.rng_state,
            config->max_ticks
        );
    }

    if (mk_game_score(&game, &score) != MK_OK) {
        memset(&score, 0, sizeof(score));
    }
    previous_signature = mk_ai_battle_progress_signature(&game, &score);

    for (tick_index = 0; tick_index < config->max_ticks; ++tick_index) {
        uint64_t signature;

        result = mk_ai_issue_basic_orders(&game);
        if (result != MK_OK) {
            fprintf(stderr, "battle=%u tick=%u AI order failure: %s\n", battle_index, game.tick, mk_result_name(result));
            return result;
        }

        mk_game_step(&game);
        result = mk_game_score(&game, &score);
        if (result != MK_OK) {
            fprintf(stderr, "battle=%u tick=%u score failure: %s\n", battle_index, game.tick, mk_result_name(result));
            return result;
        }

        signature = mk_ai_battle_progress_signature(&game, &score);
        if (signature == previous_signature) {
            stagnant_ticks += 1;
        } else {
            stagnant_ticks = 0;
            previous_signature = signature;
        }

        if (!config->quiet
            && config->summary_every > 0
            && (game.tick == 1U || game.tick % config->summary_every == 0U)) {
            mk_ai_battle_print_summary(battle_index, &game, &score, config->verbose);
        }

        if (config->watchdog_ticks > 0 && stagnant_ticks >= config->watchdog_ticks) {
            stalled = true;
            printf(
                "battle=%u stalled tick=%u stagnant_ticks=%u last_score=%d contacts=%u\n",
                battle_index,
                game.tick,
                stagnant_ticks,
                score.total_score,
                (unsigned)game.contact_report_count
            );
            mk_ai_battle_print_unit_line(&game);
            break;
        }
    }

    if (!config->quiet) {
        mk_ai_battle_print_summary(battle_index, &game, &score, true);
    }

    if (totals != NULL) {
        totals->battles_run += 1;
        if (stalled) {
            totals->stalled_battles += 1;
        }
        if (totals->battles_run == 1 || score.total_score > totals->best_score) {
            totals->best_score = score.total_score;
        }
        if (totals->battles_run == 1 || score.total_score < totals->worst_score) {
            totals->worst_score = score.total_score;
        }
    }

    if (stalled && config->fail_on_stall) {
        return MK_ERROR_INVALID_DATA;
    }

    return MK_OK;
}

static bool mk_ai_battle_parse_arguments(int argc, char **argv, mk_ai_battle_config_t *config) {
    int arg_index;

    if (config == NULL) {
        return false;
    }

    memset(config, 0, sizeof(*config));
    config->project_root = MK_PROJECT_SOURCE_DIR;
    config->battles = 3;
    config->max_ticks = 120;
    config->summary_every = 10;
    config->watchdog_ticks = 40;

    for (arg_index = 1; arg_index < argc; ++arg_index) {
        const char *argument = argv[arg_index];

        if (strcmp(argument, "--help") == 0) {
            mk_ai_battle_print_usage(argv[0]);
            exit(0);
        }

        if (strcmp(argument, "--quiet") == 0) {
            config->quiet = true;
            continue;
        }

        if (strcmp(argument, "--verbose") == 0) {
            config->verbose = true;
            continue;
        }

        if (strcmp(argument, "--forever") == 0) {
            config->forever = true;
            continue;
        }

        if (strcmp(argument, "--fail-on-stall") == 0) {
            config->fail_on_stall = true;
            continue;
        }

        if (strcmp(argument, "--scenario") == 0) {
            if (arg_index + 1 >= argc) {
                return false;
            }
            config->scenario_path = argv[++arg_index];
            continue;
        }

        if (strcmp(argument, "--project-root") == 0) {
            if (arg_index + 1 >= argc) {
                return false;
            }
            config->project_root = argv[++arg_index];
            continue;
        }

        if (strcmp(argument, "--battles") == 0) {
            if (arg_index + 1 >= argc || !mk_ai_battle_parse_u32(argv[arg_index + 1], &config->battles)) {
                return false;
            }
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--ticks") == 0 || strcmp(argument, "--max-ticks") == 0) {
            if (arg_index + 1 >= argc || !mk_ai_battle_parse_u32(argv[arg_index + 1], &config->max_ticks)) {
                return false;
            }
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--summary-every") == 0) {
            if (arg_index + 1 >= argc || !mk_ai_battle_parse_u32(argv[arg_index + 1], &config->summary_every)) {
                return false;
            }
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--watchdog") == 0) {
            if (arg_index + 1 >= argc || !mk_ai_battle_parse_u32(argv[arg_index + 1], &config->watchdog_ticks)) {
                return false;
            }
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--seed") == 0) {
            if (arg_index + 1 >= argc || !mk_ai_battle_parse_u64(argv[arg_index + 1], &config->seed)) {
                return false;
            }
            config->has_seed = true;
            arg_index += 1;
            continue;
        }

        return false;
    }

    return config->battles > 0 && config->max_ticks > 0;
}

int main(int argc, char **argv) {
    mk_ai_battle_config_t config;
    mk_ai_battle_totals_t totals;
    uint32_t battle_index = 1;

    if (!mk_ai_battle_parse_arguments(argc, argv, &config)) {
        mk_ai_battle_print_usage(argv[0]);
        return 2;
    }

    memset(&totals, 0, sizeof(totals));

    while (config.forever || battle_index <= config.battles) {
        mk_result_t result = mk_ai_battle_run_one(&config, battle_index, &totals);

        if (result != MK_OK) {
            totals.failed_battles += 1;
            fprintf(stderr, "battle=%u failed: %s\n", battle_index, mk_result_name(result));
            return 1;
        }

        battle_index += 1;
    }

    if (!config.quiet) {
        printf(
            "ai_battle_totals battles=%u failed=%u stalled=%u best_score=%d worst_score=%d\n",
            totals.battles_run,
            totals.failed_battles,
            totals.stalled_battles,
            totals.best_score,
            totals.worst_score
        );
    }

    return totals.failed_battles == 0 ? 0 : 1;
}
