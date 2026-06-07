#include "mk_demo.h"
#include "mk_test.h"

#include <stdio.h>
#include <string.h>

static mk_demo_session_t *make_loaded_session(bool ai_only) {
    mk_demo_session_config_t config;
    mk_demo_session_t *session = NULL;

    memset(&config, 0, sizeof(config));
    config.screen_width_px = 960.0f;
    config.screen_height_px = 640.0f;
    config.margin_px = 48.0f;
    config.ai_only = ai_only;
    MK_TEST_ASSERT(mk_demo_session_create(&config, &session) == MK_OK);
    MK_TEST_ASSERT(session != NULL);
    MK_TEST_ASSERT(mk_demo_session_load_default(session) == MK_OK);
    MK_TEST_ASSERT(mk_demo_session_fit_board(session, 960.0f, 640.0f, 48.0f) == MK_OK);
    return session;
}

static const mk_demo_draw_command_t *find_draw_command(
    const mk_demo_draw_command_t *commands,
    size_t command_count,
    mk_demo_draw_kind_t kind
) {
    size_t index;

    for (index = 0; index < command_count; ++index) {
        if (commands[index].kind == kind) {
            return &commands[index];
        }
    }

    return NULL;
}

static void test_session_lifecycle_and_summary(void) {
    mk_demo_session_t *session = make_loaded_session(false);
    mk_demo_summary_t summary;
    mk_game_snapshot_t snapshot;

    MK_TEST_ASSERT(mk_demo_session_summary(session, &summary) == MK_OK);
    MK_TEST_ASSERT(summary.tick == 0);
    MK_TEST_ASSERT(summary.unit_count >= 3);
    MK_TEST_ASSERT(summary.civilian_count > 0);
    MK_TEST_ASSERT(summary.objective_count > 0);
    MK_TEST_ASSERT(summary.map_width_m > 0.0f);
    MK_TEST_ASSERT(strcmp(summary.scenario_name, "Market Commercial Streets 2003") == 0);
    MK_TEST_ASSERT(summary.score.outcome == MK_OUTCOME_PLAYER_FAILURE);

    MK_TEST_ASSERT(mk_demo_session_snapshot(session, &snapshot) == MK_OK);
    MK_TEST_ASSERT(snapshot.unit_count == summary.unit_count);
    MK_TEST_ASSERT(snapshot.gameplay_area.level_count > 0);

    mk_demo_session_destroy(session);
}

static void test_session_draw_commands_and_picking(void) {
    mk_demo_session_t *session = make_loaded_session(false);
    mk_demo_draw_command_t commands[MK_DEMO_MAX_DRAW_COMMANDS];
    const mk_demo_draw_command_t *unit_command;
    const mk_demo_draw_command_t *level_command;
    const mk_demo_draw_command_t *civilian_command;
    const mk_demo_draw_command_t *objective_command;
    mk_demo_pick_result_t pick;
    size_t command_count = 0;

    MK_TEST_ASSERT(mk_demo_session_collect_draw_commands(session, NULL, 0, &command_count) == MK_OK);
    MK_TEST_ASSERT(command_count > 0);
    MK_TEST_ASSERT(command_count <= MK_DEMO_MAX_DRAW_COMMANDS);
    MK_TEST_ASSERT(mk_demo_session_collect_draw_commands(
        session,
        commands,
        sizeof(commands) / sizeof(commands[0]),
        &command_count
    ) == MK_OK);

    unit_command = find_draw_command(commands, command_count, MK_DEMO_DRAW_UNIT);
    level_command = find_draw_command(commands, command_count, MK_DEMO_DRAW_LEVEL);
    civilian_command = find_draw_command(commands, command_count, MK_DEMO_DRAW_CIVILIAN);
    objective_command = find_draw_command(commands, command_count, MK_DEMO_DRAW_OBJECTIVE);
    MK_TEST_ASSERT(unit_command != NULL);
    MK_TEST_ASSERT(level_command != NULL);
    MK_TEST_ASSERT(civilian_command != NULL);
    MK_TEST_ASSERT(objective_command != NULL);
    MK_TEST_ASSERT(level_command->asset_path[0] != '\0');

    MK_TEST_ASSERT(mk_demo_session_pick_screen(
        session,
        unit_command->screen_position_px,
        18.0f,
        &pick
    ) == MK_OK);
    MK_TEST_ASSERT(pick.kind == MK_DEMO_PICK_UNIT);
    MK_TEST_ASSERT(pick.id == unit_command->id);

    MK_TEST_ASSERT(mk_demo_session_select_screen(
        session,
        unit_command->screen_position_px,
        18.0f,
        &pick
    ) == MK_OK);
    MK_TEST_ASSERT(pick.kind == MK_DEMO_PICK_UNIT);
    MK_TEST_ASSERT(mk_demo_session_issue_selected_move_screen(
        session,
        objective_command->screen_position_px
    ) == MK_OK);

    mk_demo_session_destroy(session);
}

static void test_session_ai_only_step_and_counters(void) {
    mk_demo_session_t *session = make_loaded_session(true);
    mk_demo_summary_t before;
    mk_demo_summary_t after;
    mk_demo_performance_counters_t counters;

    MK_TEST_ASSERT(mk_demo_session_summary(session, &before) == MK_OK);
    MK_TEST_ASSERT(mk_demo_session_step(session, 2) == MK_OK);
    MK_TEST_ASSERT(mk_demo_session_summary(session, &after) == MK_OK);
    MK_TEST_ASSERT(after.tick == before.tick + 2U);
    MK_TEST_ASSERT(mk_demo_session_performance(session, &counters) == MK_OK);
    MK_TEST_ASSERT(counters.fixed_ticks == 2);
    MK_TEST_ASSERT(counters.ai_order_batches == 2);
    MK_TEST_ASSERT(counters.ai_units_considered > 0);
    MK_TEST_ASSERT(counters.draw_queries == 0);
    MK_TEST_ASSERT(mk_demo_session_reset_performance(session) == MK_OK);
    MK_TEST_ASSERT(mk_demo_session_performance(session, &counters) == MK_OK);
    MK_TEST_ASSERT(counters.fixed_ticks == 0);
    MK_TEST_ASSERT(counters.ai_order_batches == 0);
    MK_TEST_ASSERT(counters.ai_units_considered == 0);

    mk_demo_session_destroy(session);
}

static void test_session_audit_and_debug_exports(void) {
    mk_demo_session_t *session = make_loaded_session(true);
    mk_demo_audit_report_t audit;
    mk_after_action_report_t aar;
    char debug_text[2048];
    char topology_text[4096];

    MK_TEST_ASSERT(mk_demo_session_step(session, 1) == MK_OK);
    MK_TEST_ASSERT(mk_demo_session_audit(session, &audit) == MK_OK);
    MK_TEST_ASSERT(audit.level_count >= 4);
    MK_TEST_ASSERT(audit.feature_count > 0);
    MK_TEST_ASSERT(audit.region_count > 0);
    MK_TEST_ASSERT(audit.topology_node_count > 0);
    MK_TEST_ASSERT(audit.topology_portal_count > 0);
    MK_TEST_ASSERT(audit.semantic_zone_count > 0);
    MK_TEST_ASSERT(audit.objective_count > 0);
    MK_TEST_ASSERT(audit.unit_count >= 3);
    MK_TEST_ASSERT(audit.civilian_count > 0);
    MK_TEST_ASSERT(audit.missing_level_image_paths == 0);
    MK_TEST_ASSERT(audit.empty_topology_node_ids == 0);
    MK_TEST_ASSERT(audit.warnings == 0);
    MK_TEST_ASSERT(strstr(audit.summary, "warnings=0") != NULL);

    MK_TEST_ASSERT(mk_demo_session_after_action(session, &aar) == MK_OK);
    MK_TEST_ASSERT(aar.summary[0] != '\0');
    MK_TEST_ASSERT(aar.narrative[0] != '\0');
    MK_TEST_ASSERT(strstr(aar.summary, "score=") != NULL);

    MK_TEST_ASSERT(mk_demo_session_debug_text(session, debug_text, sizeof(debug_text)) == MK_OK);
    MK_TEST_ASSERT(strstr(debug_text, "scenario=\"Market Commercial Streets 2003\"") != NULL);
    MK_TEST_ASSERT(strstr(debug_text, "tick=1") != NULL);
    MK_TEST_ASSERT(strstr(debug_text, "performance") != NULL);
    MK_TEST_ASSERT(strstr(debug_text, "after_action") != NULL);
    MK_TEST_ASSERT(strstr(debug_text, "civilian_sample") != NULL);

    MK_TEST_ASSERT(mk_demo_session_topology_debug_text(session, topology_text, sizeof(topology_text)) == MK_OK);
    MK_TEST_ASSERT(strstr(topology_text, "topology") != NULL);
    MK_TEST_ASSERT(strstr(topology_text, "nodes=") != NULL);

    mk_demo_session_destroy(session);
}

static void test_session_load_specific_scenario(void) {
    mk_demo_session_config_t config;
    mk_demo_session_t *session = NULL;
    mk_demo_summary_t summary;

    memset(&config, 0, sizeof(config));
    config.screen_width_px = 800.0f;
    config.screen_height_px = 600.0f;
    config.margin_px = 32.0f;
    MK_TEST_ASSERT(mk_demo_session_create(&config, &session) == MK_OK);
    MK_TEST_ASSERT(mk_demo_session_load_scenario(
        session,
        "game/mosul/scenarios/market_cache_search_smoke_2003.mkscenario"
    ) == MK_OK);
    MK_TEST_ASSERT(mk_demo_session_summary(session, &summary) == MK_OK);
    MK_TEST_ASSERT(strcmp(summary.scenario_name, "Market Cache Search Smoke 2003") == 0);
    MK_TEST_ASSERT(summary.unit_count > 0);

    mk_demo_session_destroy(session);
}

int main(void) {
    test_session_lifecycle_and_summary();
    test_session_draw_commands_and_picking();
    test_session_ai_only_step_and_counters();
    test_session_audit_and_debug_exports();
    test_session_load_specific_scenario();

    puts("mk_demo_session_tests: ok");
    return 0;
}
