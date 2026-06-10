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

static mk_vec2_t test_rect_center(mk_rect_t rect) {
    return mk_test_vec2(rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f);
}

static void test_disable_protected_civilians(mk_game_t *game) {
    size_t index;

    MK_TEST_ASSERT(game != NULL);
    for (index = 0; index < game->civilian_count; ++index) {
        game->civilians[index].protected_noncombatant = false;
    }
}

static size_t test_find_suspected_terrain_index(const mk_game_t *game) {
    size_t index;

    MK_TEST_ASSERT(game != NULL);
    for (index = 0; index < game->map.terrain_count; ++index) {
        if (game->map.terrain[index].kind == MK_TERRAIN_SUSPECTED_IED) {
            return index;
        }
    }

    MK_TEST_ASSERT(false);
    return 0;
}

static size_t test_find_semantic_zone_kind_index(const mk_game_t *game, const char *kind) {
    size_t index;

    MK_TEST_ASSERT(game != NULL);
    MK_TEST_ASSERT(kind != NULL);
    for (index = 0; index < game->gameplay_area.semantic_zone_count; ++index) {
        if (strcmp(game->gameplay_area.semantic_zones[index].kind, kind) == 0) {
            return index;
        }
    }

    MK_TEST_ASSERT(false);
    return 0;
}

static size_t test_find_breachable_portal_index(const mk_game_t *game) {
    size_t index;

    MK_TEST_ASSERT(game != NULL);
    for (index = 0; index < game->gameplay_area.topology_portal_count; ++index) {
        const mk_gameplay_topology_portal_t *portal = &game->gameplay_area.topology_portals[index];

        if ((strcmp(portal->state, "closed") == 0 || strcmp(portal->state, "locked") == 0)
            && strcmp(portal->kind, "window") != 0
            && strcmp(portal->kind, "roof_edge") != 0) {
            return index;
        }
    }

    MK_TEST_ASSERT(false);
    return 0;
}

static void test_open_breachable_portals(mk_game_t *game) {
    size_t index;

    MK_TEST_ASSERT(game != NULL);
    for (index = 0; index < game->gameplay_area.topology_portal_count; ++index) {
        mk_gameplay_topology_portal_t *portal = &game->gameplay_area.topology_portals[index];

        if (strcmp(portal->state, "closed") == 0 || strcmp(portal->state, "locked") == 0) {
            strcpy(portal->state, "open");
        }
    }
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
    MK_TEST_ASSERT_CLOSE(game.units[1].target_position_m.x, 330.0f);
    MK_TEST_ASSERT_CLOSE(game.units[1].target_position_m.y, 238.0f);

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
    MK_TEST_ASSERT(first.units[0].order == MK_ORDER_INVESTIGATE);
    MK_TEST_ASSERT(first.units[1].order == second.units[1].order);
    MK_TEST_ASSERT(first.units[1].order == MK_ORDER_HOLD);
    MK_TEST_ASSERT(first.units[2].order == MK_ORDER_HOLD);
}

static void test_basic_ai_advances_expected_positions(void) {
    mk_game_t game = make_loaded_game();

    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    mk_game_step(&game);

    MK_TEST_ASSERT_CLOSE(game.units[0].position_m.x, 86.00f);
    MK_TEST_ASSERT_CLOSE(game.units[0].position_m.y, 245.81f);
    MK_TEST_ASSERT_CLOSE(game.units[1].position_m.x, 344.43f);
    MK_TEST_ASSERT_CLOSE(game.units[1].position_m.y, 232.23f);
    MK_TEST_ASSERT_CLOSE(game.units[2].position_m.x, 252.0f);
    MK_TEST_ASSERT_CLOSE(game.units[2].position_m.y, 206.0f);
}

static void test_basic_ai_suppresses_at_close_range(void) {
    mk_game_t game = make_loaded_game();

    game.units[0].position_m = mk_test_vec2(250.0f, 230.0f);
    game.units[1].position_m = mk_test_vec2(350.0f, 230.0f);
    game.units[1].hidden = false;
    game.units[1].revealed = true;
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
    game.units[1].hidden = false;
    game.units[1].revealed = true;
    game.civilians[0].position_m = mk_test_vec2(300.0f, 230.0f);
    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.units[0].order == MK_ORDER_HOLD);
    MK_TEST_ASSERT(game.units[0].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(!game.units[0].has_move_target);
}

static void test_basic_ai_withdraws_when_pinned(void) {
    mk_game_t game = make_loaded_game();

    game.units[1].hidden = false;
    game.units[1].revealed = true;
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

static void test_basic_ai_investigates_suspected_contact(void) {
    mk_game_t game = make_loaded_game();

    game.contact_report_count = 1;
    memset(&game.contact_reports[0], 0, sizeof(game.contact_reports[0]));
    game.contact_reports[0].id = 1;
    game.contact_reports[0].kind = MK_CONTACT_REPORT_SUSPECTED_DANGER;
    game.contact_reports[0].side = MK_SIDE_OPFOR;
    game.contact_reports[0].attacker_unit_id = game.units[0].id;
    game.contact_reports[0].target_unit_id = game.units[1].id;
    game.contact_reports[0].position_m = mk_test_vec2(220.0f, 246.0f);
    game.contact_reports[0].target_position_m = game.contact_reports[0].position_m;
    game.contact_reports[0].confidence = 55;
    game.contact_reports[0].visible = true;

    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.units[0].order == MK_ORDER_INVESTIGATE);
    MK_TEST_ASSERT(game.units[0].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(game.units[0].has_move_target);
    MK_TEST_ASSERT_CLOSE(game.units[0].target_position_m.x, 220.0f);
    MK_TEST_ASSERT_CLOSE(game.units[0].target_position_m.y, 246.0f);
}

static void test_basic_ai_overwatches_at_investigation_distance(void) {
    mk_game_t game = make_loaded_game();

    game.contact_report_count = 1;
    memset(&game.contact_reports[0], 0, sizeof(game.contact_reports[0]));
    game.contact_reports[0].id = 1;
    game.contact_reports[0].kind = MK_CONTACT_REPORT_SUSPECTED_DANGER;
    game.contact_reports[0].side = MK_SIDE_OPFOR;
    game.contact_reports[0].attacker_unit_id = game.units[0].id;
    game.contact_reports[0].target_unit_id = game.units[1].id;
    game.contact_reports[0].position_m = mk_test_vec2(92.0f, 246.0f);
    game.contact_reports[0].target_position_m = game.contact_reports[0].position_m;
    game.contact_reports[0].confidence = 55;
    game.contact_reports[0].visible = true;

    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.units[0].order == MK_ORDER_OVERWATCH);
    MK_TEST_ASSERT(game.units[0].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(!game.units[0].has_move_target);
}

static void test_basic_ai_searches_nearby_false_contact_terrain(void) {
    mk_game_t game = make_loaded_game();
    size_t terrain_index = test_find_suspected_terrain_index(&game);
    mk_terrain_zone_t *terrain = &game.map.terrain[terrain_index];
    mk_vec2_t position = test_rect_center(terrain->bounds_m);

    test_disable_protected_civilians(&game);
    game.units[0].position_m = position;
    game.contact_report_count = 1;
    memset(&game.contact_reports[0], 0, sizeof(game.contact_reports[0]));
    game.contact_reports[0].id = 1;
    game.contact_reports[0].kind = MK_CONTACT_REPORT_FALSE_CONTACT;
    game.contact_reports[0].side = MK_SIDE_NEUTRAL;
    game.contact_reports[0].attacker_unit_id = game.units[0].id;
    game.contact_reports[0].terrain_id = terrain->id;
    game.contact_reports[0].position_m = position;
    game.contact_reports[0].target_position_m = position;
    game.contact_reports[0].confidence = 45;
    game.contact_reports[0].visible = true;

    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.map.terrain[terrain_index].searched);
    MK_TEST_ASSERT(game.units[0].order == MK_ORDER_INVESTIGATE);
    MK_TEST_ASSERT(game.units[0].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(game.contact_report_count == 2);
    MK_TEST_ASSERT(game.contact_reports[1].kind == MK_CONTACT_REPORT_SEARCH);
}

static void test_basic_ai_searches_nearby_semantic_cache(void) {
    mk_game_t game = make_loaded_game();
    size_t zone_index = test_find_semantic_zone_kind_index(&game, "cache");
    mk_gameplay_semantic_zone_t *zone = &game.gameplay_area.semantic_zones[zone_index];

    test_disable_protected_civilians(&game);
    test_open_breachable_portals(&game);
    game.units[1].hidden = false;
    game.units[1].revealed = true;
    game.units[0].position_m = test_rect_center(zone->bounds_m);
    strcpy(game.units[0].level_id, zone->level_id);
    strcpy(game.units[0].topology_node_id, zone->node_id);

    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.gameplay_area.semantic_zones[zone_index].searched);
    MK_TEST_ASSERT(game.gameplay_area.semantic_zones[zone_index].last_search_outcome == MK_SEARCH_OUTCOME_CACHE_FOUND);
    MK_TEST_ASSERT(game.units[0].order == MK_ORDER_INVESTIGATE);
    MK_TEST_ASSERT(game.units[0].order_source == MK_ORDER_SOURCE_AI);
}

static void test_basic_ai_breaches_nearby_closed_portal(void) {
    mk_game_t game = make_loaded_game();
    size_t portal_index = test_find_breachable_portal_index(&game);
    mk_gameplay_topology_portal_t *portal = &game.gameplay_area.topology_portals[portal_index];

    test_disable_protected_civilians(&game);
    game.units[1].hidden = false;
    game.units[1].revealed = true;
    game.units[0].position_m = test_rect_center(portal->bounds_m);
    strcpy(game.units[0].level_id, portal->level_id);
    strcpy(game.units[0].topology_node_id, portal->from_node_id);

    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(strcmp(game.gameplay_area.topology_portals[portal_index].state, "breached") == 0);
    MK_TEST_ASSERT(game.gameplay_area.topology_portals[portal_index].breached);
    MK_TEST_ASSERT(game.units[0].order == MK_ORDER_BREACH);
    MK_TEST_ASSERT(game.units[0].order_source == MK_ORDER_SOURCE_AI);
}

static void test_basic_ai_hidden_topology_opfor_holds_defensive_node(void) {
    mk_game_t game = make_loaded_game();

    MK_TEST_ASSERT(game.unit_count > 3);
    MK_TEST_ASSERT(game.units[3].side == MK_SIDE_OPFOR);
    MK_TEST_ASSERT(game.units[3].hidden);
    MK_TEST_ASSERT(game.units[3].topology_node_id[0] != '\0');

    MK_TEST_ASSERT(mk_ai_issue_basic_orders(&game) == MK_OK);
    MK_TEST_ASSERT(game.units[3].order == MK_ORDER_OVERWATCH);
    MK_TEST_ASSERT(game.units[3].order_source == MK_ORDER_SOURCE_AI);
    MK_TEST_ASSERT(!game.units[3].has_move_target);
}

static const char *test_order_name(mk_order_t order) {
    switch (order) {
        case MK_ORDER_MOVE:
            return "move";
        case MK_ORDER_SUPPRESS:
            return "suppress";
        case MK_ORDER_WITHDRAW:
            return "withdraw";
        case MK_ORDER_INVESTIGATE:
            return "investigate";
        case MK_ORDER_OVERWATCH:
            return "overwatch";
        case MK_ORDER_HOLD:
            return "hold";
        case MK_ORDER_BREACH:
            return "breach";
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
    MK_TEST_ASSERT(mk_test_transcript_contains(&first_transcript, "player_order=investigate"));
    MK_TEST_ASSERT(mk_test_transcript_contains(&first_transcript, "opfor_order=hold"));
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
    test_basic_ai_investigates_suspected_contact();
    test_basic_ai_overwatches_at_investigation_distance();
    test_basic_ai_searches_nearby_false_contact_terrain();
    test_basic_ai_searches_nearby_semantic_cache();
    test_basic_ai_breaches_nearby_closed_portal();
    test_basic_ai_hidden_topology_opfor_holds_defensive_node();
    test_basic_ai_transcript_is_deterministic();

    puts("mk_basic_ai_tests: ok");
    return 0;
}
