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
    bool debug_log;
    FILE *transcript;
    FILE *replay;
    size_t replay_last_contact_count;
} mk_headless_run_observer_t;

static const char *mk_headless_side_name(mk_side_t side) {
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

static bool mk_headless_parse_side(const char *text, mk_side_t *out_side) {
    if (text == NULL || out_side == NULL) {
        return false;
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

    if (strcmp(text, "neutral") == 0) {
        *out_side = MK_SIDE_NEUTRAL;
        return true;
    }

    return false;
}

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
        case MK_ORDER_WITHDRAW:
            return "withdraw";
        case MK_ORDER_INVESTIGATE:
            return "investigate";
        case MK_ORDER_NONE:
        default:
            return "none";
    }
}

static const char *mk_headless_contact_kind_name(mk_contact_report_kind_t kind) {
    switch (kind) {
        case MK_CONTACT_REPORT_FIRE:
            return "fire";
        case MK_CONTACT_REPORT_REVEAL:
            return "reveal";
        case MK_CONTACT_REPORT_CIVILIAN_RISK:
            return "civilian_risk";
        case MK_CONTACT_REPORT_SUSPECTED_DANGER:
            return "suspected_danger";
        case MK_CONTACT_REPORT_FALSE_CONTACT:
            return "false_contact";
        default:
            return "unknown";
    }
}

static const char *mk_headless_outcome_name(mk_outcome_t outcome) {
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

static bool mk_headless_parse_outcome(const char *text, mk_outcome_t *out_outcome) {
    if (text == NULL || out_outcome == NULL) {
        return false;
    }

    if (strcmp(text, "success") == 0) {
        *out_outcome = MK_OUTCOME_PLAYER_SUCCESS;
        return true;
    }

    if (strcmp(text, "partial") == 0) {
        *out_outcome = MK_OUTCOME_PLAYER_PARTIAL;
        return true;
    }

    if (strcmp(text, "failure") == 0) {
        *out_outcome = MK_OUTCOME_PLAYER_FAILURE;
        return true;
    }

    if (strcmp(text, "in_progress") == 0) {
        *out_outcome = MK_OUTCOME_IN_PROGRESS;
        return true;
    }

    return false;
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
    size_t civilian_index;
    int civilian_risk = 0;

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

    for (civilian_index = 0; civilian_index < game->civilian_count; ++civilian_index) {
        civilian_risk += game->civilians[civilian_index].risk;
    }

    fprintf(stream, " | contacts=%u civilian_risk=%d", (unsigned)game->contact_report_count, civilian_risk);
    fprintf(stream, "\n");
}

static void mk_headless_print_debug_tick(FILE *stream, const mk_game_t *game) {
    mk_score_t score;
    size_t objective_index;

    if (mk_game_score(game, &score) != MK_OK) {
        memset(&score, 0, sizeof(score));
    }

    fprintf(
        stream,
        "debug tick=%u rng=%llu score=%d controlled=%u contested=%u",
        game->tick,
        (unsigned long long)game->rng_state,
        score.total_score,
        (unsigned)score.controlled_objectives,
        (unsigned)score.contested_objectives
    );

    for (objective_index = 0; objective_index < game->objective_count; ++objective_index) {
        const mk_objective_t *objective = &game->objectives[objective_index];

        fprintf(
            stream,
            " objective.%u=%s",
            objective->id,
            mk_headless_side_name(objective->controlling_side)
        );
    }

    if (game->contact_report_count > 0) {
        const mk_contact_report_t *report = &game->contact_reports[game->contact_report_count - 1];

        fprintf(
            stream,
            " last_contact=(id=%u kind=%s target=%u terrain=%u confidence=%d resolved=%d)",
            report->id,
            mk_headless_contact_kind_name(report->kind),
            report->target_unit_id,
            report->terrain_id,
            report->confidence,
            report->resolved ? 1 : 0
        );
    } else {
        fprintf(stream, " last_contact=none");
    }

    fprintf(stream, "\n");
}

static void mk_headless_print_replay_header(FILE *stream, const mk_game_t *game, uint32_t steps, bool ai_only) {
    if (stream == NULL || game == NULL) {
        return;
    }

    fprintf(
        stream,
        "mk_replay version=1 scenario=\"%s\" seed=%llu steps=%u ai_only=%d\n",
        game->scenario_name,
        (unsigned long long)game->rng_state,
        steps,
        ai_only ? 1 : 0
    );
    fprintf(
        stream,
        "event tick=%u kind=start units=%u objectives=%u contacts=%u\n",
        game->tick,
        (unsigned)game->unit_count,
        (unsigned)game->objective_count,
        (unsigned)game->contact_report_count
    );
    if (mk_gameplay_area_is_loaded(&game->gameplay_area)) {
        fprintf(
            stream,
            "event tick=%u kind=gameplay_area id=\"%s\" map=\"%s\" levels=%u features=%u regions=%u topology_id=\"%s\" topology_nodes=%u topology_portals=%u semantic_zones=%u pixel_width=%d pixel_height=%d ppm=%.2f\n",
            game->tick,
            game->gameplay_area.id,
            game->gameplay_area.map_id,
            (unsigned)game->gameplay_area.level_count,
            (unsigned)game->gameplay_area.feature_count,
            (unsigned)game->gameplay_area.region_count,
            game->gameplay_area.topology_id,
            (unsigned)game->gameplay_area.topology_node_count,
            (unsigned)game->gameplay_area.topology_portal_count,
            (unsigned)game->gameplay_area.semantic_zone_count,
            game->gameplay_area.pixel_width,
            game->gameplay_area.pixel_height,
            game->gameplay_area.pixels_per_meter
        );
    }
}

static void mk_headless_print_replay_tick(
    FILE *stream,
    const mk_game_t *game,
    mk_headless_run_observer_t *observer
) {
    mk_score_t score;
    size_t unit_index;
    size_t objective_index;
    size_t contact_index;

    if (stream == NULL || game == NULL || observer == NULL) {
        return;
    }

    for (unit_index = 0; unit_index < game->unit_count; ++unit_index) {
        const mk_unit_t *unit = &game->units[unit_index];

        fprintf(
            stream,
            "event tick=%u kind=unit id=%u side=%s order=%s status=%s x=%.2f y=%.2f target_x=%.2f target_y=%.2f has_target=%d hidden=%d revealed=%d\n",
            game->tick,
            unit->id,
            mk_headless_side_name(unit->side),
            mk_headless_order_name(unit->order),
            mk_headless_status_name(unit->status),
            unit->position_m.x,
            unit->position_m.y,
            unit->target_position_m.x,
            unit->target_position_m.y,
            unit->has_move_target ? 1 : 0,
            unit->hidden ? 1 : 0,
            unit->revealed ? 1 : 0
        );
    }

    for (objective_index = 0; objective_index < game->objective_count; ++objective_index) {
        const mk_objective_t *objective = &game->objectives[objective_index];

        fprintf(
            stream,
            "event tick=%u kind=objective id=%u label=\"%s\" side=%s value=%d\n",
            game->tick,
            objective->id,
            objective->label,
            mk_headless_side_name(objective->controlling_side),
            objective->value
        );
    }

    if (mk_game_score(game, &score) != MK_OK) {
        memset(&score, 0, sizeof(score));
    }
    fprintf(
        stream,
        "event tick=%u kind=score total=%d outcome=%s controlled=%u contested=%u civilian_risk=%d\n",
        game->tick,
        score.total_score,
        mk_headless_outcome_name(score.outcome),
        (unsigned)score.controlled_objectives,
        (unsigned)score.contested_objectives,
        score.civilian_risk
    );

    for (contact_index = observer->replay_last_contact_count;
         contact_index < game->contact_report_count;
         ++contact_index) {
        const mk_contact_report_t *report = &game->contact_reports[contact_index];

        fprintf(
            stream,
            "event tick=%u kind=contact id=%u contact=%s side=%s observer=%u target=%u civilian=%u terrain=%u confidence=%d resolved=%d x=%.2f y=%.2f target_x=%.2f target_y=%.2f\n",
            game->tick,
            report->id,
            mk_headless_contact_kind_name(report->kind),
            mk_headless_side_name(report->side),
            report->attacker_unit_id,
            report->target_unit_id,
            report->civilian_id,
            report->terrain_id,
            report->confidence,
            report->resolved ? 1 : 0,
            report->position_m.x,
            report->position_m.y,
            report->target_position_m.x,
            report->target_position_m.y
        );
    }

    observer->replay_last_contact_count = game->contact_report_count;
}

static void mk_headless_print_replay_end(FILE *stream, const mk_game_t *game, mk_result_t result) {
    mk_score_t score;

    if (stream == NULL || game == NULL) {
        return;
    }

    if (mk_game_score(game, &score) != MK_OK) {
        memset(&score, 0, sizeof(score));
    }

    fprintf(
        stream,
        "event tick=%u kind=end result=%s score=%d outcome=%s\n",
        game->tick,
        mk_result_name(result),
        score.total_score,
        mk_headless_outcome_name(score.outcome)
    );
}

static mk_result_t mk_headless_observe_tick(const mk_game_t *game, void *user_data) {
    mk_headless_run_observer_t *observer = (mk_headless_run_observer_t *)user_data;

    if (game == NULL || observer == NULL) {
        return MK_ERROR_INVALID_ARGUMENT;
    }

    if (!observer->quiet) {
        mk_headless_print_tick(stdout, game);
        if (observer->debug_log) {
            mk_headless_print_debug_tick(stdout, game);
        }
    }

    if (observer->transcript != NULL) {
        mk_headless_print_tick(observer->transcript, game);
        if (observer->debug_log) {
            mk_headless_print_debug_tick(observer->transcript, game);
        }
    }

    if (observer->replay != NULL) {
        mk_headless_print_replay_tick(observer->replay, game, observer);
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

static bool mk_headless_parse_i32(const char *text, int *out_value) {
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

static void mk_headless_print_usage(const char *program_name) {
    fprintf(
        stderr,
        "usage: %s [--scenario PATH] [--project-root PATH] [--steps N|--max-ticks N] [--seed N] [--quiet] [--transcript PATH] [--replay PATH] [--ai-only] [--aar] [--briefing] [--debug-log] [--expect-objective SIDE] [--expect-outcome OUTCOME] [--expect-contested N] [--expect-min-civilian-risk N] [--expect-min-score N]\n"
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
    if (mk_gameplay_area_is_loaded(&game->gameplay_area)) {
        fprintf(
            stream,
            "gameplay_area: id=\"%s\" map=\"%s\" levels=%u features=%u regions=%u pixels=%dx%d ppm=%.2f\n",
            game->gameplay_area.id,
            game->gameplay_area.map_id,
            (unsigned)game->gameplay_area.level_count,
            (unsigned)game->gameplay_area.feature_count,
            (unsigned)game->gameplay_area.region_count,
            game->gameplay_area.pixel_width,
            game->gameplay_area.pixel_height,
            game->gameplay_area.pixels_per_meter
        );
        if (mk_gameplay_area_topology_is_loaded(&game->gameplay_area)) {
            fprintf(
                stream,
                "topology: id=\"%s\" nodes=%u portals=%u zones=%u\n",
                game->gameplay_area.topology_id,
                (unsigned)game->gameplay_area.topology_node_count,
                (unsigned)game->gameplay_area.topology_portal_count,
                (unsigned)game->gameplay_area.semantic_zone_count
            );
        }
    } else {
        fprintf(stream, "gameplay_area: none\n");
    }
}

static void mk_headless_print_briefing(FILE *stream, const mk_game_t *game) {
    fprintf(stream, "briefing: %s\n", game->briefing[0] != '\0' ? game->briefing : "(none)");
}

static void mk_headless_print_result(FILE *stream, const mk_game_t *game) {
    fprintf(stream, "result: ok tick=%u units=%u\n", game->tick, (unsigned)game->unit_count);
}

static void mk_headless_print_after_action(FILE *stream, const mk_game_t *game) {
    mk_after_action_report_t report;

    if (mk_game_after_action_report(game, &report) != MK_OK) {
        fprintf(stream, "after_action: unavailable\n");
        return;
    }

    fprintf(stream, "after_action: %s\n", report.summary);
    if (report.narrative[0] != '\0') {
        fprintf(stream, "after_action_text: %s\n", report.narrative);
    }
}

static bool mk_headless_objective_side_present(const mk_game_t *game, mk_side_t side) {
    size_t index;

    if (game == NULL) {
        return false;
    }

    for (index = 0; index < game->objective_count; ++index) {
        if (game->objectives[index].controlling_side == side) {
            return true;
        }
    }

    return false;
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

static void mk_headless_close_outputs(mk_headless_run_observer_t *observer) {
    if (observer == NULL) {
        return;
    }

    if (observer->transcript != NULL) {
        fclose(observer->transcript);
        observer->transcript = NULL;
    }

    if (observer->replay != NULL) {
        fclose(observer->replay);
        observer->replay = NULL;
    }
}

int main(int argc, char **argv) {
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_headless_run_observer_t observer;
    const char *scenario_path = NULL;
    const char *project_root = MK_PROJECT_SOURCE_DIR;
    const char *transcript_path = NULL;
    const char *replay_path = NULL;
    uint32_t steps = 10;
    uint64_t seed = 0;
    bool has_seed = false;
    bool ai_only = false;
    bool print_aar = false;
    bool print_briefing = false;
    bool expect_objective = false;
    bool expect_outcome = false;
    bool expect_contested = false;
    bool expect_min_civilian_risk = false;
    bool expect_min_score = false;
    mk_side_t expected_objective_side = MK_SIDE_NEUTRAL;
    mk_outcome_t expected_outcome = MK_OUTCOME_IN_PROGRESS;
    uint32_t expected_contested = 0;
    int expected_min_civilian_risk = 0;
    int expected_min_score = 0;
    int arg_index;
    mk_result_t result;

    observer.quiet = false;
    observer.debug_log = false;
    observer.transcript = NULL;
    observer.replay = NULL;
    observer.replay_last_contact_count = 0;

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

        if (strcmp(argument, "--aar") == 0) {
            print_aar = true;
            continue;
        }

        if (strcmp(argument, "--briefing") == 0) {
            print_briefing = true;
            continue;
        }

        if (strcmp(argument, "--debug-log") == 0) {
            observer.debug_log = true;
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

        if (strcmp(argument, "--replay") == 0) {
            if (arg_index + 1 >= argc) {
                mk_headless_print_usage(argv[0]);
                return 2;
            }

            replay_path = argv[arg_index + 1];
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--expect-objective") == 0) {
            if (arg_index + 1 >= argc || !mk_headless_parse_side(argv[arg_index + 1], &expected_objective_side)) {
                mk_headless_print_usage(argv[0]);
                return 2;
            }

            expect_objective = true;
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--expect-outcome") == 0) {
            if (arg_index + 1 >= argc || !mk_headless_parse_outcome(argv[arg_index + 1], &expected_outcome)) {
                mk_headless_print_usage(argv[0]);
                return 2;
            }

            expect_outcome = true;
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--expect-contested") == 0) {
            if (arg_index + 1 >= argc || !mk_headless_parse_u32(argv[arg_index + 1], &expected_contested)) {
                mk_headless_print_usage(argv[0]);
                return 2;
            }

            expect_contested = true;
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--expect-min-civilian-risk") == 0) {
            if (arg_index + 1 >= argc || !mk_headless_parse_i32(argv[arg_index + 1], &expected_min_civilian_risk)) {
                mk_headless_print_usage(argv[0]);
                return 2;
            }

            expect_min_civilian_risk = true;
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--expect-min-score") == 0) {
            if (arg_index + 1 >= argc || !mk_headless_parse_i32(argv[arg_index + 1], &expected_min_score)) {
                mk_headless_print_usage(argv[0]);
                return 2;
            }

            expect_min_score = true;
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

    if (replay_path != NULL) {
        observer.replay = fopen(replay_path, "w");
        if (observer.replay == NULL) {
            fprintf(stderr, "failed to open replay \"%s\"\n", replay_path);
            mk_headless_close_outputs(&observer);
            return 1;
        }
    }

    result = mk_headless_load_scenario(scenario_path, project_root, &scenario);
    if (result != MK_OK) {
        fprintf(stderr, "failed to load Mosul scenario: %s\n", mk_result_name(result));
        mk_headless_close_outputs(&observer);
        return 1;
    }

    if (has_seed) {
        scenario.seed = seed;
    }

    result = mk_game_load_scenario(&game, &scenario);
    if (result != MK_OK) {
        fprintf(stderr, "failed to load Mosul scenario: %s\n", mk_result_name(result));
        mk_headless_close_outputs(&observer);
        return 1;
    }

    if (!observer.quiet) {
        mk_headless_print_header(stdout, &game, steps);
        if (print_briefing) {
            mk_headless_print_briefing(stdout, &game);
        }
    }

    if (observer.transcript != NULL) {
        mk_headless_print_header(observer.transcript, &game, steps);
        if (print_briefing) {
            mk_headless_print_briefing(observer.transcript, &game);
        }
    }

    if (observer.replay != NULL) {
        mk_headless_print_replay_header(observer.replay, &game, steps, ai_only);
        observer.replay_last_contact_count = game.contact_report_count;
    }

    result = mk_headless_run_steps(&game, steps, ai_only, &observer);
    if (result != MK_OK) {
        fprintf(stderr, "headless run stopped: %s\n", mk_result_name(result));
        if (observer.replay != NULL) {
            mk_headless_print_replay_end(observer.replay, &game, result);
        }
        mk_headless_close_outputs(&observer);
        return 1;
    }

    if (expect_objective && !mk_headless_objective_side_present(&game, expected_objective_side)) {
        fprintf(stderr, "expected objective controlled by %s\n", mk_headless_side_name(expected_objective_side));
        if (observer.replay != NULL) {
            mk_headless_print_replay_end(observer.replay, &game, MK_ERROR_INVALID_DATA);
        }
        mk_headless_close_outputs(&observer);
        return 1;
    }

    if (expect_outcome || expect_contested || expect_min_civilian_risk || expect_min_score) {
        mk_score_t score;

        result = mk_game_score(&game, &score);
        if (result != MK_OK) {
            fprintf(stderr, "failed to score final state: %s\n", mk_result_name(result));
            if (observer.replay != NULL) {
                mk_headless_print_replay_end(observer.replay, &game, result);
            }
            mk_headless_close_outputs(&observer);
            return 1;
        }

        if (expect_outcome && score.outcome != expected_outcome) {
            fprintf(
                stderr,
                "expected outcome %s but saw %s\n",
                mk_headless_outcome_name(expected_outcome),
                mk_headless_outcome_name(score.outcome)
            );
            if (observer.replay != NULL) {
                mk_headless_print_replay_end(observer.replay, &game, MK_ERROR_INVALID_DATA);
            }
            mk_headless_close_outputs(&observer);
            return 1;
        }

        if (expect_contested && score.contested_objectives != expected_contested) {
            fprintf(stderr, "expected contested objectives == %u\n", (unsigned)expected_contested);
            if (observer.replay != NULL) {
                mk_headless_print_replay_end(observer.replay, &game, MK_ERROR_INVALID_DATA);
            }
            mk_headless_close_outputs(&observer);
            return 1;
        }

        if (expect_min_civilian_risk && score.civilian_risk < expected_min_civilian_risk) {
            fprintf(stderr, "expected civilian risk >= %d\n", expected_min_civilian_risk);
            if (observer.replay != NULL) {
                mk_headless_print_replay_end(observer.replay, &game, MK_ERROR_INVALID_DATA);
            }
            mk_headless_close_outputs(&observer);
            return 1;
        }

        if (expect_min_score && score.total_score < expected_min_score) {
            fprintf(stderr, "expected score >= %d\n", expected_min_score);
            if (observer.replay != NULL) {
                mk_headless_print_replay_end(observer.replay, &game, MK_ERROR_INVALID_DATA);
            }
            mk_headless_close_outputs(&observer);
            return 1;
        }
    }

    if (!observer.quiet) {
        mk_headless_print_result(stdout, &game);
        if (print_aar) {
            mk_headless_print_after_action(stdout, &game);
        }
    }

    if (observer.transcript != NULL) {
        mk_headless_print_result(observer.transcript, &game);
        if (print_aar) {
            mk_headless_print_after_action(observer.transcript, &game);
        }
    }

    if (observer.replay != NULL) {
        mk_headless_print_replay_end(observer.replay, &game, MK_OK);
    }

    mk_headless_close_outputs(&observer);
    return 0;
}
