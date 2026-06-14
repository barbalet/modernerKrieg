#include "mk_core.h"
#include "mk_mosul_demo.h"
#include "mk_test.h"

#include <stdio.h>

typedef struct {
    uint32_t calls;
    uint32_t fail_on_call;
    mk_test_transcript_t transcript;
} fixed_loop_observer_t;

static mk_result_t record_tick_observer(const mk_game_t *game, void *user_data) {
    fixed_loop_observer_t *observer = (fixed_loop_observer_t *)user_data;

    MK_TEST_ASSERT(game != NULL);
    MK_TEST_ASSERT(observer != NULL);

    observer->calls += 1;
    mk_test_transcript_append(
        &observer->transcript,
        "tick=%u units=%u selected=%u\n",
        game->tick,
        (unsigned)game->unit_count,
        game->selected_unit_id
    );

    if (observer->fail_on_call != 0 && observer->calls == observer->fail_on_call) {
        return MK_ERROR_INVALID_DATA;
    }

    return MK_OK;
}

static mk_game_t make_loaded_game(void) {
    mk_scenario_definition_t scenario;
    mk_game_t game;

    MK_TEST_ASSERT(mk_mosul_make_east_block_scenario(&scenario) == MK_OK);
    MK_TEST_ASSERT(mk_game_load_scenario(&game, &scenario) == MK_OK);

    return game;
}

static void test_fixed_loop_runs_requested_steps(void) {
    mk_game_t game = make_loaded_game();
    fixed_loop_observer_t observer;

    observer.calls = 0;
    observer.fail_on_call = 0;
    mk_test_transcript_init(&observer.transcript);

    MK_TEST_ASSERT(mk_game_run_fixed_steps(&game, 3, record_tick_observer, &observer) == MK_OK);
    MK_TEST_ASSERT(game.tick == 3);
    MK_TEST_ASSERT(observer.calls == 3);
    MK_TEST_ASSERT(mk_test_transcript_contains(&observer.transcript, "tick=1 units=6 selected=0"));
    MK_TEST_ASSERT(mk_test_transcript_contains(&observer.transcript, "tick=3 units=6 selected=0"));
}

static void test_fixed_loop_can_stop_on_observer_failure(void) {
    mk_game_t game = make_loaded_game();
    fixed_loop_observer_t observer;

    observer.calls = 0;
    observer.fail_on_call = 2;
    mk_test_transcript_init(&observer.transcript);

    MK_TEST_ASSERT(
        mk_game_run_fixed_steps(&game, 5, record_tick_observer, &observer) == MK_ERROR_INVALID_DATA
    );
    MK_TEST_ASSERT(game.tick == 2);
    MK_TEST_ASSERT(observer.calls == 2);
    MK_TEST_ASSERT(mk_test_transcript_contains(&observer.transcript, "tick=2 units=6 selected=0"));
}

static void test_fixed_loop_rejects_invalid_game(void) {
    MK_TEST_ASSERT(mk_game_run_fixed_steps(NULL, 1, NULL, NULL) == MK_ERROR_INVALID_ARGUMENT);
}

int main(void) {
    test_fixed_loop_runs_requested_steps();
    test_fixed_loop_can_stop_on_observer_failure();
    test_fixed_loop_rejects_invalid_game();

    puts("mk_fixed_loop_tests: ok");
    return 0;
}
