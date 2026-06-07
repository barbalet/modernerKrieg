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
    mk_vec2_t map_position = make_vec2(80.0f, 246.0f);
    mk_vec2_t screen_position;
    mk_vec2_t round_trip;

    assert(mk_board_view_fit_map(&view, &snapshot.map, 960.0f, 640.0f, MK_BOARD_VIEW_DEFAULT_MARGIN_PX) == MK_OK);
    assert_close(view.screen_rect_px.x, 48.0f);
    assert_close(view.screen_rect_px.y, 48.0f);
    assert_close(view.screen_rect_px.width, 864.0f);
    assert_close(view.screen_rect_px.height, 544.0f);
    assert_close(view.scale_px_per_m, 1.088f);

    screen_position = mk_board_view_map_to_screen(&view, map_position);
    assert_close(screen_position.x, 135.04f);
    assert_close(screen_position.y, 315.65f);

    round_trip = mk_board_view_screen_to_map(&view, screen_position);
    assert_close(round_trip.x, map_position.x);
    assert_close(round_trip.y, map_position.y);
}

static void test_zoom_and_pan(void) {
    mk_game_snapshot_t snapshot = make_snapshot();
    mk_board_view_t view;
    mk_vec2_t anchor_screen = make_vec2(120.0f, 120.0f);
    mk_vec2_t anchor_before;
    mk_vec2_t anchor_after;
    mk_rect_t visible_bounds;

    assert(mk_board_view_fit_map(&view, &snapshot.map, 960.0f, 640.0f, 48.0f) == MK_OK);
    anchor_before = mk_board_view_screen_to_map(&view, anchor_screen);
    assert(mk_board_view_zoom_at(&view, &snapshot.map, 2.0f, anchor_screen) == MK_OK);
    anchor_after = mk_board_view_screen_to_map(&view, anchor_screen);

    assert_close(view.scale_px_per_m, 2.176f);
    assert_close(anchor_before.x, anchor_after.x);
    assert_close(anchor_before.y, anchor_after.y);

    assert(mk_board_view_pan_pixels(&view, &snapshot.map, 68.0f, 34.0f) == MK_OK);
    visible_bounds = mk_board_view_visible_map_bounds(&view);
    assert_close(visible_bounds.x, 64.34f);
    assert_close(visible_bounds.y, 48.71f);
    assert_close(visible_bounds.width, 397.06f);
    assert_close(visible_bounds.height, 250.0f);
}

static void test_soldier_markers_from_snapshot(void) {
    mk_game_snapshot_t snapshot = make_snapshot();
    mk_board_view_t view;
    mk_soldier_marker_t markers[MK_MAX_UNITS * MK_MAX_SOLDIERS_PER_UNIT];
    size_t marker_count = 0;
    size_t needed_count = 0;

    assert(mk_board_view_fit_map(&view, &snapshot.map, 960.0f, 640.0f, 48.0f) == MK_OK);
    assert(mk_board_view_collect_soldier_markers(&view, &snapshot, NULL, 0, &needed_count) == MK_OK);
    assert(needed_count == 4);
    assert(mk_board_view_collect_soldier_markers(&view, &snapshot, markers, 2, &marker_count) == MK_ERROR_CAPACITY);
    assert(marker_count == 4);
    assert(mk_board_view_collect_soldier_markers(&view, &snapshot, markers, sizeof(markers) / sizeof(markers[0]), &marker_count) == MK_OK);
    assert(marker_count == 4);

    assert(markers[0].unit_id == 1);
    assert(markers[0].soldier_id == 1);
    assert(markers[0].role == MK_ROLE_LEADER);
    assert(markers[0].selected_unit);
    assert_close(markers[0].position_m.x, 76.0f);
    assert_close(markers[0].position_m.y, 243.0f);

    assert(markers[3].unit_id == 3);
    assert(markers[3].role == MK_ROLE_CIVILIAN);
    assert(!markers[3].selected_unit);
    assert(markers[3].side == MK_SIDE_CIVILIAN);
}

static void test_tactical_overlays_from_snapshot(void) {
    mk_scenario_definition_t scenario;
    mk_game_t game;
    mk_game_snapshot_t snapshot;
    mk_board_view_t view;
    mk_tactical_overlay_t overlays[MK_BOARD_VIEW_MAX_TACTICAL_OVERLAYS];
    size_t overlay_count = 0;
    bool saw_selection = false;
    bool saw_route = false;
    bool saw_target = false;
    bool saw_fire = false;
    bool saw_suppression = false;
    bool saw_casualty = false;
    bool saw_objective = false;
    bool saw_objective_control = false;
    bool saw_order_status = false;
    bool saw_hidden_contact = false;
    bool saw_civilian_risk = false;
    bool saw_suspected_contact = false;
    bool saw_false_contact = false;
    bool saw_breach_search = false;
    bool saw_rooftop_access = false;
    bool saw_search_cache = false;
    size_t index;

    assert(mk_mosul_make_east_block_scenario(&scenario) == MK_OK);
    assert(mk_game_load_scenario(&game, &scenario) == MK_OK);
    assert(mk_game_select_unit(&game, 1) == MK_OK);
    assert(mk_game_issue_selected_move_order(&game, make_vec2(112.0f, 246.0f)) == MK_OK);
    game.units[1].suppression = 9;
    game.units[1].status = MK_UNIT_PINNED;
    game.units[1].soldiers[0].casualty = true;
    game.civilians[0].risk = 4;
    game.objectives[0].controlling_side = MK_SIDE_PLAYER;
    assert(mk_game_snapshot(&game, &snapshot) == MK_OK);
    snapshot.units[0].has_route = true;
    snapshot.units[0].route_step_count = 2;
    snapshot.units[0].route_step_index = 1;
    snapshot.units[0].route_waypoints_m[1] = make_vec2(96.0f, 240.0f);
    snapshot.contact_report_count = 3;
    snapshot.contact_reports[0].id = 1;
    snapshot.contact_reports[0].kind = MK_CONTACT_REPORT_FIRE;
    snapshot.contact_reports[0].attacker_unit_id = 1;
    snapshot.contact_reports[0].target_unit_id = 2;
    snapshot.contact_reports[0].side = MK_SIDE_PLAYER;
    snapshot.contact_reports[0].position_m = snapshot.units[0].position_m;
    snapshot.contact_reports[0].target_position_m = snapshot.units[1].position_m;
    snapshot.contact_reports[0].suppression_added = 8;
    snapshot.contact_reports[1].id = 2;
    snapshot.contact_reports[1].kind = MK_CONTACT_REPORT_SUSPECTED_DANGER;
    snapshot.contact_reports[1].attacker_unit_id = 1;
    snapshot.contact_reports[1].target_unit_id = 2;
    snapshot.contact_reports[1].side = MK_SIDE_OPFOR;
    snapshot.contact_reports[1].position_m = snapshot.units[1].position_m;
    snapshot.contact_reports[1].confidence = 44;
    snapshot.contact_reports[2].id = 3;
    snapshot.contact_reports[2].kind = MK_CONTACT_REPORT_FALSE_CONTACT;
    snapshot.contact_reports[2].attacker_unit_id = 1;
    snapshot.contact_reports[2].terrain_id = 3;
    snapshot.contact_reports[2].side = MK_SIDE_NEUTRAL;
    snapshot.contact_reports[2].position_m = make_vec2(218.0f, 330.0f);
    snapshot.contact_reports[2].confidence = 30;
    assert(mk_board_view_fit_map(&view, &snapshot.map, 960.0f, 640.0f, 48.0f) == MK_OK);

    assert(mk_board_view_collect_tactical_overlays(&view, &snapshot, NULL, 0, &overlay_count) == MK_OK);
    assert(overlay_count == 16);
    assert(mk_board_view_collect_tactical_overlays(&view, &snapshot, overlays, 3, &overlay_count) == MK_ERROR_CAPACITY);
    assert(overlay_count == 16);
    assert(mk_board_view_collect_tactical_overlays(
        &view,
        &snapshot,
        overlays,
        sizeof(overlays) / sizeof(overlays[0]),
        &overlay_count
    ) == MK_OK);
    assert(overlay_count == 16);

    for (index = 0; index < overlay_count; ++index) {
        const mk_tactical_overlay_t *overlay = &overlays[index];

        if (overlay->kind == MK_TACTICAL_OVERLAY_SELECTION) {
            saw_selection = true;
            assert(overlay->unit_id == 1);
            assert_close(overlay->screen_radius_px, MK_UNIT_PICK_RADIUS_M * view.scale_px_per_m);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_MOVE_ROUTE) {
            saw_route = true;
            assert(overlay->unit_id == 1);
            assert_close(overlay->target_position_m.x, 96.0f);
            assert_close(overlay->target_position_m.y, 240.0f);
            assert(overlay->intensity == 1);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_MOVE_TARGET) {
            saw_target = true;
            assert(overlay->unit_id == 1);
            assert_close(overlay->position_m.x, 112.0f);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_FIRE) {
            saw_fire = true;
            assert(overlay->unit_id == 1);
            assert_close(overlay->target_position_m.x, 350.0f);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_SUPPRESSION) {
            saw_suppression = true;
            assert(overlay->unit_id == 2);
            assert(overlay->intensity == 9);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_CASUALTY) {
            saw_casualty = true;
            assert(overlay->unit_id == 2);
            assert(overlay->soldier_id == 1);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_OBJECTIVE) {
            saw_objective = true;
            assert(overlay->objective_id == 1);
            assert_close(overlay->radius_m, 28.0f);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_OBJECTIVE_CONTROL) {
            saw_objective_control = true;
            assert(overlay->objective_id == 1);
            assert(overlay->side == MK_SIDE_PLAYER);
            assert_close(overlay->radius_m, 14.0f);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_ORDER_STATUS) {
            saw_order_status = true;
            assert(overlay->unit_id == 1);
            assert(overlay->order == MK_ORDER_MOVE);
            assert(overlay->side == MK_SIDE_PLAYER);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_HIDDEN_CONTACT) {
            saw_hidden_contact = true;
            assert(overlay->unit_id == 2);
            assert(overlay->intensity == 18);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_CIVILIAN_RISK) {
            saw_civilian_risk = true;
            assert(overlay->civilian_id == 1);
            assert(overlay->intensity == 4);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_SUSPECTED_CONTACT) {
            saw_suspected_contact = true;
            assert(overlay->unit_id == 2);
            assert(overlay->intensity == 44);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_FALSE_CONTACT) {
            saw_false_contact = true;
            assert(overlay->terrain_id == 3);
            assert(overlay->intensity == 30);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_BREACH_SEARCH) {
            saw_breach_search = true;
            assert(overlay->terrain_id == 4);
            assert_close(overlay->position_m.x, 295.0f);
            assert_close(overlay->position_m.y, 226.0f);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_ROOFTOP_ACCESS) {
            saw_rooftop_access = true;
            assert(overlay->terrain_id == 5);
            assert_close(overlay->position_m.x, 364.0f);
            assert_close(overlay->position_m.y, 166.0f);
        } else if (overlay->kind == MK_TACTICAL_OVERLAY_SEARCH_CACHE) {
            saw_search_cache = true;
            assert(overlay->terrain_id == 6);
            assert_close(overlay->position_m.x, 224.0f);
            assert_close(overlay->position_m.y, 328.0f);
        }
    }

    assert(saw_selection);
    assert(saw_route);
    assert(saw_target);
    assert(saw_fire);
    assert(saw_suppression);
    assert(saw_casualty);
    assert(saw_objective);
    assert(saw_objective_control);
    assert(saw_order_status);
    assert(saw_hidden_contact);
    assert(saw_civilian_risk);
    assert(saw_suspected_contact);
    assert(saw_false_contact);
    assert(saw_breach_search);
    assert(saw_rooftop_access);
    assert(saw_search_cache);
}

int main(void) {
    test_fit_and_round_trip();
    test_zoom_and_pan();
    test_soldier_markers_from_snapshot();
    test_tactical_overlays_from_snapshot();

    puts("mk_board_view_tests: ok");
    return 0;
}
