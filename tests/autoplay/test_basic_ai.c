#include "mk_ai.h"
#include "mk_mosul_demo.h"
#include "mk_test.h"

#include <stdio.h>
#include <string.h>

static mk_game_t make_loaded_game(void) {
    mk_scenario_definition_t scenario;
    mk_game_t game;

    MK_TEST_ASSERT(mk_mosul_make_market_2003_scenario(&scenario) == MK_OK);
    MK_TEST_ASSERT(mk_game_load_scenario(&game, &scenario) == MK_OK);

    return game;
}

static void test_basic_ai_emits_orders_for_both_sides(void) {
    mk_game_t game = make_loaded_game();

    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);

    MK_TEST_ASSERT(game.units[0].side == MK_SIDE_PLAYER);
    MK_TEST_ASSERT(game.units[0].order == MK_ORDER_MOVE);
    MK_TEST_ASSERT(game.units[0].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(game.units[0].has_move_target);
    MK_TEST_ASSERT_CLOSE(game.units[0].target_position_m.x, 330.0f);
    MK_TEST_ASSERT_CLOSE(game.units[0].target_position_m.y, 238.0f);

    MK_TEST_ASSERT(game.units[1].side == MK_SIDE_OPFOR);
    MK_TEST_ASSERT(game.units[1].order == MK_ORDER_MOVE);
    MK_TEST_ASSERT(game.units[1].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(game.units[1].has_move_target);
    MK_TEST_ASSERT_CLOSE(game.units[1].target_position_m.x, 80.0f);
    MK_TEST_ASSERT_CLOSE(game.units[1].target_position_m.y, 246.0f);

    MK_TEST_ASSERT(game.units[2].side == MK_SIDE_CIVILIAN);
    MK_TEST_ASSERT(game.units[2].order == MK_ORDER_HOLD);
    MK_TEST_ASSERT(!game.units[2].has_move_target);
}

static void test_basic_ai_run_is_deterministic(void) {
    mk_game_t first = make_loaded_game();
    mk_game_t second = make_loaded_game();
    int step;

    for (step = 0; step < 3; ++step) {
        MK_TEST_ASSERT(mk_ai_issue_basic_orders(&first) == MK_OK);
        MK_TEST_ASSERT(mk_ai_issue_basic_orders(&second) == MK_OK);
        mk_game_step(&first);
        mk_game_step(&second);
    }

    MK_TEST_ASSERT(first.tick == 3);
    MK_TEST_ASSERT(second.tick == 3);
    MK_TEST_ASSERT_CLOSE(first.units[0].position_m.x, second.units[0].position_m.x);
    MK_TEST_ASSERT_CLOSE(first.units[0].position_m.y, second.units[0].position_m.y);
    MK_TEST_ASSERT_CLOSE(first.units[1].position_m.x, second.units[1].position_m.x);
    MK_TEST_ASSERT_CLOSE(first.units[1].position_m.y, second.units[1].position_m.y);
    MK_TEST_ASSERT(first.units[0].order == MK_ORDER_MOVE);
    MK_TEST_ASSERT(first.units[1].order == MK_ORDER_MOVE);
    MK_TEST_ASSERT(first.units[2].order == MK_ORDER_HOLD);
}

static void test_basic_ai_advances_expected_positions(void) {
    mk_game_t game = make_loaded_game();

    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    mk_game_step(&game);

    MK_TEST_ASSERT_CLOSE(game.units[0].position_m.x, 86.00f);
    MK_TEST_ASSERT_CLOSE(game.units[0].position_m.y, 245.81f);
    MK_TEST_ASSERT_CLOSE(game.units[1].position_m.x, 344.01f);
    MK_TEST_ASSERT_CLOSE(game.units[1].position_m.y, 230.36f);
    MK_TEST_ASSERT_CLOSE(game.units[2].position_m.x, 252.0f);
    MK_TEST_ASSERT_CLOSE(game.units[2].position_m.y, 206.0f);
}

static void test_basic_ai_suppresses_at_close_range(void) {
    mk_game_t game = make_loaded_game();

    game.units[0].position_m = mk_test_vec2(250.0f, 230.0f);
    game.units[1].position_m = mk_test_vec2(350.0f, 230.0f);
    game.civilians[0].position_m = mk_test_vec2(252.0f, 180.0f);
    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.units[0].order == MK_ORDER_SUPPRESS);
    MK_TEST_ASSERT(game.units[1].order == MK_ORDER_SUPPRESS);
}

static void test_basic_ai_player_holds_near_civilian(void) {
    mk_game_t game = make_loaded_game();

    game.units[0].position_m = mk_test_vec2(252.0f, 220.0f);
    game.units[1].position_m = mk_test_vec2(350.0f, 230.0f);
    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.units[0].order == MK_ORDER_HOLD);
    MK_TEST_ASSERT(game.units[0].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(!game.units[0].has_move_target);
}

static void test_basic_ai_player_holds_risky_fire_lane(void) {
    mk_game_t game = make_loaded_game();

    game.units[0].position_m = mk_test_vec2(250.0f, 230.0f);
    game.units[1].position_m = mk_test_vec2(350.0f, 230.0f);
    game.civilians[0].position_m = mk_test_vec2(300.0f, 230.0f);
    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.units[0].order == MK_ORDER_HOLD);
    MK_TEST_ASSERT(game.units[0].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(!game.units[0].has_move_target);
}

static void test_basic_ai_withdraws_when_pinned(void) {
    mk_game_t game = make_loaded_game();

    game.units[0].suppression = 20;
    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.units[0].order == MK_ORDER_WITHDRAW);
    MK_TEST_ASSERT(game.units[0].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(game.units[0].has_move_target);
    MK_TEST_ASSERT(game.units[0].target_position_m.x < game.units[0].position_m.x);
}

static void test_basic_ai_opfor_withdraws_after_reveal(void) {
    mk_game_t game = make_loaded_game();

    game.units[1].revealed = true;
    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.units[1].order == MK_ORDER_WITHDRAW);
    MK_TEST_ASSERT(game.units[1].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(game.units[1].has_move_target);
    MK_TEST_ASSERT(game.units[1].target_position_m.x > game.units[1].position_m.x);
}

static void test_basic_ai_opfor_withdraws_after_taking_fire(void) {
    mk_game_t game = make_loaded_game();

    game.tick = 4;
    game.contact_report_count = 1;
    game.contact_reports[0].kind = MK_CONTACT_REPORT_FIRE;
    game.contact_reports[0].tick = 3;
    game.contact_reports[0].target_unit_id = game.units[1].id;
    game.contact_reports[0].resolved = true;

    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.units[1].order == MK_ORDER_WITHDRAW);
    MK_TEST_ASSERT(game.units[1].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(game.units[1].has_move_target);
    MK_TEST_ASSERT(game.units[1].target_position_m.x > game.units[1].position_m.x);
}

static const char *test_order_name(mk_order_t order) {
    switch (order) {
        case MK_ORDER_MOVE:
            return "move";
        case MK_ORDER_SUPPRESS:
            return "suppress";
        case MK_ORDER_WITHDRAW:
            return "withdraw";
        case MK_ORDER_HOLD:
            return "hold";
        default:
            return "other";
    }
}

static void append_ai_transcript(mk_test_transcript_t *transcript, mk_game_t *game, int steps) {
    int step;

    for (step = 0; step < steps; ++step) {
        MK_TEST_ASSERT(mk_ai_issue_basic_orders(game) == MK_OK);
        mk_game_step(game);
        mk_test_transcript_append(
            transcript,
            "tick=%u player=(%.2f,%.2f) player_order=%s opfor=(%.2f,%.2f) opfor_order=%s contacts=%u risk=%d\n",
            game->tick,
            game->units[0].position_m.x,
            game->units[0].position_m.y,
            test_order_name(game->units[0].order),
            game->units[1].position_m.x,
            game->units[1].position_m.y,
            test_order_name(game->units[1].order),
            (unsigned)game->contact_report_count,
            game->civilians[0].risk
        );
    }
}

static void test_basic_ai_transcript_is_deterministic(void) {
    mk_game_t first = make_loaded_game();
    mk_game_t second = make_loaded_game();
    mk_test_transcript_t first_transcript;
    mk_test_transcript_t second_transcript;

    mk_test_transcript_init(&first_transcript);
    mk_test_transcript_init(&second_transcript);
    append_ai_transcript(&first_transcript, &first, 3);
    append_ai_transcript(&second_transcript, &second, 3);

    MK_TEST_ASSERT(strcmp(first_transcript.text, second_transcript.text) == 0);
    MK_TEST_ASSERT(mk_test_transcript_contains(&first_transcript, "tick=3"));
    MK_TEST_ASSERT(mk_test_transcript_contains(&first_transcript, "player_order=move"));
    MK_TEST_ASSERT(mk_test_transcript_contains(&first_transcript, "opfor_order=move"));
}

int main(void) {
    test_basic_ai_emits_orders_for_both_sides();
    test_basic_ai_run_is_deterministic();
    test_basic_ai_advances_expected_positions();
    test_basic_ai_suppresses_at_close_range();
    test_basic_ai_player_holds_near_civilian();
    test_basic_ai_player_holds_risky_fire_lane();
    test_basic_ai_withdraws_when_pinned();
    test_basic_ai_opfor_withdraws_after_reveal();
    test_basic_ai_opfor_withdraws_after_taking_fire();
    test_basic_ai_transcript_is_deterministic();

    puts("mk_basic_ai_tests: ok");
    return 0;
}
