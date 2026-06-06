#include "mk_ai.h"
#include "mk_mosul_demo.h"
#include "mk_test.h"

#include <stdio.h>

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

int main(void) {
    test_basic_ai_emits_orders_for_both_sides();
    test_basic_ai_run_is_deterministic();
    test_basic_ai_advances_expected_positions();

    puts("mk_basic_ai_tests: ok");
    return 0;
}
