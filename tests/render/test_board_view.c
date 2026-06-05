#include "mk_board_view.h"
#include "mk_mosul_demo.h"

#include <assert.h>
#include <stdio.h>

static float abs_float(float value) {
    return value < 0.0f ? -value : value;
}

static void assert_close(float actual, float expected) {
    assert(abs_float(actual - expected) < 0.01f);
}

static mk_vec2_t make_vec2(float x, float y) {
    mk_vec2_t value;

    value.x = x;
    value.y = y;

    return value;
}

static mk_game_snapshot_t make_snapshot(void) {
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_game_snapshot_t snapshot;

    assert(mk_mosul_make_east_block_scenario(&scenario) == MK_OK);
    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(mk_game_select_unit(&game, 1) == MK_OK);
    assert(mk_game_snapshot(&game, &snapshot) == MK_OK);

    return snapshot;
}

static void test_fit_and_round_trip(void) {
    mk_game_snapshot_t snapshot = make_snapshot();
    mk_board_view_t view;
    mk_vec2_t map_position = make_vec2(34.0f, 82.0f);
    mk_vec2_t screen_position;
    mk_vec2_t round_trip;

    assert(mk_board_view_fit_map(&view, &snapshot.map, 960.0f, 640.0f, MK_BOARD_VIEW_DEFAULT_MARGIN_PX) == MK_OK);
    assert_close(view.screen_rect_px.x, 48.0f);
    assert_close(view.screen_rect_px.y, 48.0f);
    assert_close(view.screen_rect_px.width, 864.0f);
    assert_close(view.screen_rect_px.height, 544.0f);
    assert_close(view.scale_px_per_m, 3.4f);

    screen_position = mk_board_view_map_to_screen(&view, map_position);
    assert_close(screen_position.x, 163.6f);
    assert_close(screen_position.y, 326.8f);

    round_trip = mk_board_view_screen_to_map(&view, screen_position);
    assert_close(round_trip.x, map_position.x);
    assert_close(round_trip.y, map_position.y);
}

static void test_zoom_and_pan(void) {
    mk_game_snapshot_t snapshot = make_snapshot();
    mk_board_view_t view;
    mk_vec2_t anchor_screen = make_vec2(400.0f, 300.0f);
    mk_vec2_t anchor_before;
    mk_vec2_t anchor_after;
    mk_rect_t visible_bounds;

    assert(mk_board_view_fit_map(&view, &snapshot.map, 960.0f, 640.0f, 48.0f) == MK_OK);
    anchor_before = mk_board_view_screen_to_map(&view, anchor_screen);
    assert(mk_board_view_zoom_at(&view, &snapshot.map, 2.0f, anchor_screen) == MK_OK);
    anchor_after = mk_board_view_screen_to_map(&view, anchor_screen);

    assert_close(view.scale_px_per_m, 6.8f);
    assert_close(anchor_before.x, anchor_after.x);
    assert_close(anchor_before.y, anchor_after.y);

    assert(mk_board_view_pan_pixels(&view, &snapshot.map, 68.0f, 34.0f) == MK_OK);
    visible_bounds = mk_board_view_visible_map_bounds(&view);
    assert_close(visible_bounds.x, 61.76f);
    assert_close(visible_bounds.y, 42.06f);
    assert_close(visible_bounds.width, 127.06f);
    assert_close(visible_bounds.height, 80.0f);
}

static void test_soldier_markers_from_snapshot(void) {
    mk_game_snapshot_t snapshot = make_snapshot();
    mk_board_view_t view;
    mk_soldier_marker_t markers[MK_MAX_UNITS * MK_MAX_SOLDIERS_PER_UNIT];
    size_t marker_count = 0;
    size_t needed_count = 0;

    assert(mk_board_view_fit_map(&view, &snapshot.map, 960.0f, 640.0f, 48.0f) == MK_OK);
    assert(mk_board_view_collect_soldier_markers(&view, &snapshot, NULL, 0, &needed_count) == MK_OK);
    assert(needed_count == 6);
    assert(mk_board_view_collect_soldier_markers(&view, &snapshot, markers, 2, &marker_count) == MK_ERROR_CAPACITY);
    assert(marker_count == 6);
    assert(mk_board_view_collect_soldier_markers(&view, &snapshot, markers, sizeof(markers) / sizeof(markers[0]), &marker_count) == MK_OK);
    assert(marker_count == 6);

    assert(markers[0].unit_id == 1);
    assert(markers[0].soldier_id == 1);
    assert(markers[0].role == MK_ROLE_LEADER);
    assert(markers[0].selected_unit);
    assert_close(markers[0].position_m.x, 32.0f);
    assert_close(markers[0].position_m.y, 80.0f);

    assert(markers[3].unit_id == 2);
    assert(markers[3].role == MK_ROLE_RIFLEMAN);
    assert(!markers[3].selected_unit);
    assert(markers[5].side == MK_SIDE_CIVILIAN);
}

int main(void) {
    test_fit_and_round_trip();
    test_zoom_and_pan();
    test_soldier_markers_from_snapshot();

    puts("mk_board_view_tests: ok");
    return 0;
}
