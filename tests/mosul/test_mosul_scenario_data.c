#include "mk_mosul_demo.h"
#include "mk_test.h"

#include <stdio.h>
#include <string.h>

#ifndef MK_TEST_PROJECT_ROOT
#define MK_TEST_PROJECT_ROOT "."
#endif

#ifndef MK_TEST_BINARY_DIR
#define MK_TEST_BINARY_DIR "."
#endif

static void make_project_path(char *out_path, size_t capacity, const char *relative_path) {
    int written = snprintf(out_path, capacity, "%s/%s", MK_TEST_PROJECT_ROOT, relative_path);

    MK_TEST_ASSERT(written > 0);
    MK_TEST_ASSERT((size_t)written < capacity);
}

static void make_binary_path(char *out_path, size_t capacity, const char *name) {
    int written = snprintf(out_path, capacity, "%s/%s", MK_TEST_BINARY_DIR, name);

    MK_TEST_ASSERT(written > 0);
    MK_TEST_ASSERT((size_t)written < capacity);
}

static void write_text_file(const char *path, const char *text) {
    FILE *file = fopen(path, "w");

    MK_TEST_ASSERT(file != NULL);
    MK_TEST_ASSERT(fputs(text, file) >= 0);
    MK_TEST_ASSERT(fclose(file) == 0);
}

static void load_default_data_scenario(mk_scenario_definition_t *out_scenario) {
    char path[512];

    make_project_path(path, sizeof(path), MK_MOSUL_DEFAULT_SCENARIO_PATH);
    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, out_scenario) == MK_OK);
}

static void test_default_scenario_data_matches_fixture_shape(void) {
    mk_scenario_definition_t loaded;
    mk_scenario_definition_t fixture;

    load_default_data_scenario(&loaded);
    MK_TEST_ASSERT(mk_mosul_make_market_2003_fixture_scenario(&fixture) == MK_OK);

    MK_TEST_ASSERT(strcmp(loaded.name, fixture.name) == 0);
    MK_TEST_ASSERT(loaded.seed == fixture.seed);
    MK_TEST_ASSERT(strcmp(loaded.map.name, fixture.map.name) == 0);
    MK_TEST_ASSERT_CLOSE(loaded.map.width_m, fixture.map.width_m);
    MK_TEST_ASSERT_CLOSE(loaded.map.height_m, fixture.map.height_m);
    MK_TEST_ASSERT(loaded.map.tile_count == fixture.map.tile_count);
    MK_TEST_ASSERT(loaded.map.tiles[46].kind == fixture.map.tiles[46].kind);
    MK_TEST_ASSERT(loaded.controller_count == fixture.controller_count);
    MK_TEST_ASSERT(loaded.faction_count == fixture.faction_count);
    MK_TEST_ASSERT(loaded.force_count == fixture.force_count);
    MK_TEST_ASSERT(loaded.objective_count == fixture.objective_count);
    MK_TEST_ASSERT(loaded.civilian_count == fixture.civilian_count);
    MK_TEST_ASSERT(loaded.unit_count == fixture.unit_count);
    MK_TEST_ASSERT(strcmp(loaded.forces[0].command.callsign, fixture.forces[0].command.callsign) == 0);
    MK_TEST_ASSERT(strcmp(loaded.units[0].name, fixture.units[0].name) == 0);
    MK_TEST_ASSERT(strcmp(loaded.units[0].command.callsign, fixture.units[0].command.callsign) == 0);
    MK_TEST_ASSERT_CLOSE(loaded.units[0].position_m.x, fixture.units[0].position_m.x);
    MK_TEST_ASSERT_CLOSE(loaded.units[0].position_m.y, fixture.units[0].position_m.y);
    MK_TEST_ASSERT(loaded.units[0].soldier_count == fixture.units[0].soldier_count);
    MK_TEST_ASSERT(loaded.units[1].soldiers[1].role == fixture.units[1].soldiers[1].role);
    MK_TEST_ASSERT(loaded.units[1].hidden == fixture.units[1].hidden);
    MK_TEST_ASSERT(loaded.units[1].revealed == fixture.units[1].revealed);
    MK_TEST_ASSERT(loaded.units[1].concealment == fixture.units[1].concealment);
    MK_TEST_ASSERT(loaded.units[2].soldiers[0].ammo == 0);
}

static void test_public_default_scenario_uses_data_file(void) {
    mk_scenario_definition_t scenario;
    mk_game_t game;

    MK_TEST_ASSERT(mk_mosul_make_market_2003_scenario(&scenario) == MK_OK);
    MK_TEST_ASSERT(mk_game_load_scenario(&game, &scenario) == MK_OK);
    MK_TEST_ASSERT(strcmp(game.scenario_name, "Market Commercial Streets 2003") == 0);
    MK_TEST_ASSERT(game.unit_count == 3);
}

static void test_missing_asset_manifest_is_rejected(void) {
    char path[512];
    mk_scenario_definition_t scenario;

    make_binary_path(path, sizeof(path), "bad_missing_asset.mkscenario");
    write_text_file(
        path,
        "format=modernerKrieg.scenario.v1\n"
        "name=Bad Missing Asset\n"
        "seed=1\n"
        "asset.map_manifest=assets/mosul/manifests/missing.mapmanifest\n"
        "asset.sprite_manifest=assets/mosul/manifests/mosul_2003_sprites.spritemanifest\n"
        "map.name=Market / Commercial Streets\n"
        "map.width_m=500\n"
        "map.height_m=500\n"
        "map.tile_columns=10\n"
        "map.tile_rows=10\n"
        "map.tile_width_m=50\n"
        "map.tile_height_m=50\n"
        "map.default_terrain=open\n"
    );

    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_ERROR_INVALID_DATA);
}

static void test_invalid_force_reference_is_rejected(void) {
    char path[512];
    mk_scenario_definition_t scenario;

    make_binary_path(path, sizeof(path), "bad_force_reference.mkscenario");
    write_text_file(
        path,
        "format=modernerKrieg.scenario.v1\n"
        "name=Bad Force Reference\n"
        "seed=1\n"
        "asset.map_manifest=assets/mosul/manifests/market_commercial_streets_2003.mapmanifest\n"
        "asset.sprite_manifest=assets/mosul/manifests/mosul_2003_sprites.spritemanifest\n"
        "map.name=Market / Commercial Streets\n"
        "map.width_m=500\n"
        "map.height_m=500\n"
        "map.tile_columns=10\n"
        "map.tile_rows=10\n"
        "map.tile_width_m=50\n"
        "map.tile_height_m=50\n"
        "map.default_terrain=open\n"
        "tile_range.count=0\n"
        "terrain.count=0\n"
        "controller.count=1\n"
        "controller.0.name=AI\n"
        "controller.0.side=player\n"
        "controller.0.kind=tactical_ai\n"
        "faction.count=1\n"
        "faction.0.name=Faction\n"
        "faction.0.side=player\n"
        "faction.0.color=255,255,255,255\n"
        "force.count=1\n"
        "force.0.name=Broken Force\n"
        "force.0.side=player\n"
        "force.0.faction_index=99\n"
        "force.0.controller_index=0\n"
        "force.0.command_name=Broken\n"
        "force.0.callsign=BAD\n"
    );

    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_ERROR_INVALID_DATA);
}

static void test_impossible_objective_bounds_are_rejected(void) {
    char path[512];
    mk_scenario_definition_t scenario;

    make_binary_path(path, sizeof(path), "bad_objective_bounds.mkscenario");
    write_text_file(
        path,
        "format=modernerKrieg.scenario.v1\n"
        "name=Bad Objective Bounds\n"
        "seed=1\n"
        "asset.map_manifest=assets/mosul/manifests/market_commercial_streets_2003.mapmanifest\n"
        "asset.sprite_manifest=assets/mosul/manifests/mosul_2003_sprites.spritemanifest\n"
        "map.name=Market / Commercial Streets\n"
        "map.width_m=500\n"
        "map.height_m=500\n"
        "map.tile_columns=10\n"
        "map.tile_rows=10\n"
        "map.tile_width_m=50\n"
        "map.tile_height_m=50\n"
        "map.default_terrain=open\n"
        "tile_range.count=0\n"
        "terrain.count=0\n"
        "controller.count=1\n"
        "controller.0.name=AI\n"
        "controller.0.side=player\n"
        "controller.0.kind=tactical_ai\n"
        "faction.count=1\n"
        "faction.0.name=Faction\n"
        "faction.0.side=player\n"
        "faction.0.color=255,255,255,255\n"
        "force.count=1\n"
        "force.0.name=Force\n"
        "force.0.side=player\n"
        "force.0.faction_index=0\n"
        "force.0.controller_index=0\n"
        "force.0.command_name=Command\n"
        "force.0.callsign=OK\n"
        "objective.count=1\n"
        "objective.0.name=Impossible\n"
        "objective.0.kind=control\n"
        "objective.0.position=999,238\n"
        "objective.0.radius_m=28\n"
        "objective.0.value=1\n"
        "weapon.count=0\n"
        "civilian.count=0\n"
        "unit.count=0\n"
    );

    MK_TEST_ASSERT(mk_mosul_load_scenario_file(path, MK_TEST_PROJECT_ROOT, &scenario) == MK_ERROR_INVALID_DATA);
}

int main(void) {
    test_default_scenario_data_matches_fixture_shape();
    test_public_default_scenario_uses_data_file();
    test_missing_asset_manifest_is_rejected();
    test_invalid_force_reference_is_rejected();
    test_impossible_objective_bounds_are_rejected();

    puts("mk_mosul_scenario_data_tests: ok");
    return 0;
}
