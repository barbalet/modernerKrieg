#include "mk_asset_manifest.h"
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

static bool file_exists(const char *path) {
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        return false;
    }

    fclose(file);
    return true;
}

static void write_project_file_with_replacement(
    const char *source_relative_path,
    const char *output_path,
    const char *needle,
    const char *replacement
) {
    char source_path[512];
    char text[65536];
    FILE *source;
    FILE *output;
    size_t read_count;
    char *found;

    make_project_path(source_path, sizeof(source_path), source_relative_path);
    source = fopen(source_path, "rb");
    MK_TEST_ASSERT(source != NULL);
    read_count = fread(text, 1, sizeof(text) - 1, source);
    MK_TEST_ASSERT(ferror(source) == 0);
    MK_TEST_ASSERT(fclose(source) == 0);
    MK_TEST_ASSERT(read_count < sizeof(text) - 1);
    text[read_count] = '\0';

    found = strstr(text, needle);
    MK_TEST_ASSERT(found != NULL);

    output = fopen(output_path, "wb");
    MK_TEST_ASSERT(output != NULL);
    MK_TEST_ASSERT(fwrite(text, 1, (size_t)(found - text), output) == (size_t)(found - text));
    MK_TEST_ASSERT(fputs(replacement, output) >= 0);
    MK_TEST_ASSERT(fputs(found + strlen(needle), output) >= 0);
    MK_TEST_ASSERT(fclose(output) == 0);
}

static void test_map_manifest_loads_market_layers(void) {
    char path[512];
    char runtime_path[512];
    mk_asset_map_manifest_t manifest;

    make_project_path(
        path,
        sizeof(path),
        "assets/mosul/manifests/market_commercial_streets_2003.mapmanifest"
    );

    MK_TEST_ASSERT(mk_asset_load_map_manifest(path, MK_TEST_PROJECT_ROOT, &manifest) == MK_OK);
    MK_TEST_ASSERT(strcmp(manifest.id, "market_commercial_streets_2003") == 0);
    MK_TEST_ASSERT(strcmp(manifest.name, "Market / Commercial Streets 2003") == 0);
    MK_TEST_ASSERT_CLOSE(manifest.world_width_m, 500.0f);
    MK_TEST_ASSERT_CLOSE(manifest.world_height_m, 500.0f);
    MK_TEST_ASSERT_CLOSE(manifest.pixels_per_meter, 14.0f);
    MK_TEST_ASSERT(strcmp(manifest.origin, "top_left") == 0);
    MK_TEST_ASSERT(manifest.layer_count == 5);
    MK_TEST_ASSERT(strcmp(manifest.layers[0].id, "ground") == 0);
    MK_TEST_ASSERT(strcmp(manifest.layers[0].kind, "base") == 0);
    MK_TEST_ASSERT(strcmp(manifest.layers[4].id, "multistorey_mask") == 0);
    MK_TEST_ASSERT(strcmp(manifest.layers[4].alpha, "mask") == 0);
    MK_TEST_ASSERT(strcmp(manifest.overview_path, "assets/mosul/source/maps/market_commercial_streets_demo_2003/imgs/market_commercial_streets_demo_7000/preview_1400.png") == 0);

    make_project_path(runtime_path, sizeof(runtime_path), manifest.runtime_overview_path);
    MK_TEST_ASSERT(file_exists(runtime_path));
}

static void test_building_level_manifest_loads_multistorey_stack(void) {
    char path[512];
    char level_path[512];
    mk_asset_building_level_manifest_t manifest;
    const mk_asset_building_level_t *level;
    const mk_asset_building_feature_t *wall;
    const mk_asset_building_feature_t *door;
    const mk_asset_building_feature_t *window;
    const mk_asset_building_region_t *region;

    make_project_path(
        path,
        sizeof(path),
        "assets/mosul/manifests/market_commercial_streets_2003_building_levels.json"
    );

    MK_TEST_ASSERT(mk_asset_load_building_level_manifest(path, MK_TEST_PROJECT_ROOT, &manifest) == MK_OK);
    MK_TEST_ASSERT(manifest.schema_version == 1);
    MK_TEST_ASSERT(strcmp(manifest.id, "market_commercial_streets_2003_building_levels") == 0);
    MK_TEST_ASSERT(strcmp(manifest.map_id, "market_commercial_streets_2003") == 0);
    MK_TEST_ASSERT_CLOSE(manifest.world_width_m, 500.0f);
    MK_TEST_ASSERT_CLOSE(manifest.world_height_m, 500.0f);
    MK_TEST_ASSERT(manifest.pixel_width == 7000);
    MK_TEST_ASSERT(manifest.pixel_height == 7000);
    MK_TEST_ASSERT_CLOSE(manifest.pixels_per_meter, 14.0f);
    MK_TEST_ASSERT(manifest.max_storeys == 4);
    MK_TEST_ASSERT(manifest.level_count == 4);
    MK_TEST_ASSERT(manifest.feature_count == 25);
    MK_TEST_ASSERT(manifest.region_count == 8);

    level = mk_asset_find_building_level(&manifest, "level_02_roofs_and_second_floor");
    MK_TEST_ASSERT(level != NULL);
    MK_TEST_ASSERT(strcmp(level->alpha, "overlay") == 0);
    MK_TEST_ASSERT(strcmp(level->png_path, "assets/mosul/runtime/maps/market_commercial_streets_2003/levels/level_02_roofs_and_second_floor.png") == 0);
    make_project_path(level_path, sizeof(level_path), level->png_path);
    MK_TEST_ASSERT(file_exists(level_path));

    wall = mk_asset_find_building_feature(&manifest, "souq_west_outer_wall_ground");
    MK_TEST_ASSERT(wall != NULL);
    MK_TEST_ASSERT(strcmp(wall->kind, "wall") == 0);
    MK_TEST_ASSERT(mk_asset_building_feature_contains_pixel(wall, 1390, 1300));
    MK_TEST_ASSERT(mk_asset_building_level_blocks_los_at_pixel(&manifest, "level_01_ground", 1390, 1300));
    MK_TEST_ASSERT(mk_asset_building_level_blocks_movement_at_pixel(&manifest, "level_01_ground", 1390, 1300));

    door = mk_asset_find_building_feature(&manifest, "souq_west_door_ground");
    MK_TEST_ASSERT(door != NULL);
    MK_TEST_ASSERT(strcmp(door->kind, "door") == 0);
    MK_TEST_ASSERT(!mk_asset_building_level_blocks_los_at_pixel(&manifest, "level_01_ground", 1390, 1560));
    MK_TEST_ASSERT(!mk_asset_building_level_blocks_movement_at_pixel(&manifest, "level_01_ground", 1390, 1560));

    window = mk_asset_find_building_feature(&manifest, "souq_east_window_ground");
    MK_TEST_ASSERT(window != NULL);
    MK_TEST_ASSERT(strcmp(window->kind, "window") == 0);
    MK_TEST_ASSERT(!mk_asset_building_level_blocks_los_at_pixel(&manifest, "level_01_ground", 2300, 1320));
    MK_TEST_ASSERT(mk_asset_building_level_blocks_movement_at_pixel(&manifest, "level_01_ground", 2300, 1320));
    MK_TEST_ASSERT(!mk_asset_building_level_blocks_los_at_pixel(&manifest, "level_01_ground", 100, 100));
    MK_TEST_ASSERT(!mk_asset_building_level_blocks_movement_at_pixel(&manifest, "level_01_ground", 100, 100));

    region = mk_asset_find_building_region(&manifest, "single_storey_shop_row_north");
    MK_TEST_ASSERT(region != NULL);
    MK_TEST_ASSERT(region->storeys == 1);
    MK_TEST_ASSERT(strcmp(region->roof_level_id, "level_02_roofs_and_second_floor") == 0);
}

static void test_topology_manifest_loads_market_graph(void) {
    char building_path[512];
    char topology_path[512];
    mk_asset_building_level_manifest_t building_manifest;
    mk_asset_topology_manifest_t topology_manifest;
    const mk_asset_topology_node_t *node;
    const mk_asset_topology_portal_t *portal;
    const mk_asset_semantic_zone_t *zone;

    make_project_path(
        building_path,
        sizeof(building_path),
        "assets/mosul/manifests/market_commercial_streets_2003_building_levels.json"
    );
    make_project_path(
        topology_path,
        sizeof(topology_path),
        "assets/mosul/manifests/market_commercial_streets_2003_topology.json"
    );

    MK_TEST_ASSERT(mk_asset_load_building_level_manifest(building_path, MK_TEST_PROJECT_ROOT, &building_manifest) == MK_OK);
    MK_TEST_ASSERT(mk_asset_load_topology_manifest(topology_path, &building_manifest, &topology_manifest) == MK_OK);
    MK_TEST_ASSERT(topology_manifest.schema_version == 1);
    MK_TEST_ASSERT(strcmp(topology_manifest.id, "market_commercial_streets_2003_topology") == 0);
    MK_TEST_ASSERT(strcmp(topology_manifest.gameplay_area_id, building_manifest.id) == 0);
    MK_TEST_ASSERT(topology_manifest.node_count == 14);
    MK_TEST_ASSERT(topology_manifest.portal_count == 15);
    MK_TEST_ASSERT(topology_manifest.zone_count == 10);

    node = mk_asset_find_topology_node(&topology_manifest, "hotel_roof_access");
    MK_TEST_ASSERT(node != NULL);
    MK_TEST_ASSERT(strcmp(node->kind, "roof") == 0);
    MK_TEST_ASSERT(strcmp(node->level_id, "level_04_roof_access") == 0);
    MK_TEST_ASSERT(strcmp(node->region_id, "three_storey_hotel_east") == 0);
    MK_TEST_ASSERT(node->enterable);

    portal = mk_asset_find_topology_portal(&topology_manifest, "hotel_stair_ground_to_second");
    MK_TEST_ASSERT(portal != NULL);
    MK_TEST_ASSERT(strcmp(portal->kind, "stair") == 0);
    MK_TEST_ASSERT(portal->vertical);
    MK_TEST_ASSERT(portal->bidirectional);
    MK_TEST_ASSERT(strcmp(portal->feature_id, "hotel_stairwell_ground") == 0);

    zone = mk_asset_find_semantic_zone(&topology_manifest, "market_restricted_fire_lane");
    MK_TEST_ASSERT(zone != NULL);
    MK_TEST_ASSERT(strcmp(zone->kind, "restricted_fire_lane") == 0);
    MK_TEST_ASSERT(strcmp(zone->node_id, "street_market_junction") == 0);
    MK_TEST_ASSERT(zone->priority == 5);
}

static void test_sprite_manifest_loads_first_frames(void) {
    char path[512];
    mk_asset_sprite_manifest_t manifest;
    const mk_asset_sprite_sheet_t *sheet;
    const mk_asset_sprite_frame_t *frame;

    make_project_path(path, sizeof(path), "assets/mosul/manifests/mosul_2003_sprites.spritemanifest");
    MK_TEST_ASSERT(mk_asset_load_sprite_manifest(path, MK_TEST_PROJECT_ROOT, &manifest) == MK_OK);
    MK_TEST_ASSERT(strcmp(manifest.id, "mosul_2003_sprites") == 0);
    MK_TEST_ASSERT(manifest.sheet_count == 4);
    MK_TEST_ASSERT(manifest.frame_count == 8);
    MK_TEST_ASSERT(strcmp(manifest.runtime_render_manifest, "assets/mosul/runtime/sprites/rendered/render_manifest.json") == 0);
    MK_TEST_ASSERT(manifest.runtime_rendered_count == 1088);
    MK_TEST_ASSERT(manifest.runtime_infantry_count == 640);
    MK_TEST_ASSERT(manifest.runtime_civilian_count == 168);
    MK_TEST_ASSERT(manifest.runtime_weapon_count == 64);
    MK_TEST_ASSERT(manifest.runtime_vehicle_count == 216);

    sheet = mk_asset_find_sprite_sheet(&manifest, "us_allied_troops_128");
    MK_TEST_ASSERT(sheet != NULL);
    MK_TEST_ASSERT(sheet->tile_width == 128);
    MK_TEST_ASSERT(sheet->pivot_x == 64);

    frame = mk_asset_find_sprite_frame(&manifest, "us_patrol_rifleman_128_n");
    MK_TEST_ASSERT(frame != NULL);
    MK_TEST_ASSERT(strcmp(frame->sheet, "us_allied_troops_128") == 0);
    MK_TEST_ASSERT(strcmp(frame->side, "player") == 0);
    MK_TEST_ASSERT(strcmp(frame->role, "rifleman") == 0);
    MK_TEST_ASSERT_CLOSE(frame->scale_m, 2.0f);

    frame = mk_asset_find_sprite_frame(&manifest, "fallback_unit_marker");
    MK_TEST_ASSERT(frame != NULL);
    MK_TEST_ASSERT(strcmp(frame->state, "fallback") == 0);

    frame = mk_asset_find_sprite_frame(&manifest, "traffic_city_bus_intact_north");
    MK_TEST_ASSERT(frame != NULL);
    MK_TEST_ASSERT(strcmp(frame->sheet, "vehicles_128") == 0);
    MK_TEST_ASSERT(strcmp(frame->role, "traffic_bus") == 0);
}

static void test_sprite_render_manifest_loads_all_runtime_facings(void) {
    char path[512];
    char render_path[512];
    mk_asset_sprite_manifest_t compact_manifest;
    static mk_asset_sprite_render_manifest_t render_manifest;
    const mk_asset_sprite_render_entry_t *entry;

    make_project_path(path, sizeof(path), "assets/mosul/manifests/mosul_2003_sprites.spritemanifest");
    MK_TEST_ASSERT(mk_asset_load_sprite_manifest(path, MK_TEST_PROJECT_ROOT, &compact_manifest) == MK_OK);
    make_project_path(render_path, sizeof(render_path), compact_manifest.runtime_render_manifest);

    MK_TEST_ASSERT(mk_asset_load_sprite_render_manifest(render_path, MK_TEST_PROJECT_ROOT, &render_manifest) == MK_OK);
    MK_TEST_ASSERT(render_manifest.schema_version == 1);
    MK_TEST_ASSERT(strcmp(render_manifest.source_manifest, "assets/mosul/runtime/sprites/manifest.json") == 0);
    MK_TEST_ASSERT(render_manifest.rendered_count == 1088);
    MK_TEST_ASSERT(render_manifest.missing_source_count == 0);
    MK_TEST_ASSERT(render_manifest.error_count == 0);
    MK_TEST_ASSERT(render_manifest.infantry_count == 640);
    MK_TEST_ASSERT(render_manifest.civilian_count == 168);
    MK_TEST_ASSERT(render_manifest.weapon_count == 64);
    MK_TEST_ASSERT(render_manifest.vehicle_count == 216);

    entry = mk_asset_find_sprite_render_entry(
        &render_manifest,
        "infantry",
        "us_army_rifleman",
        "standing",
        "allied",
        "north"
    );
    MK_TEST_ASSERT(entry != NULL);
    MK_TEST_ASSERT(strcmp(entry->path, "assets/mosul/runtime/sprites/rendered/infantry_128/allied/us_army_rifleman/standing/north.png") == 0);

    entry = mk_asset_find_sprite_render_entry(
        &render_manifest,
        "civilian",
        "teenage_girl",
        "wounded",
        "civilian",
        "south_east"
    );
    MK_TEST_ASSERT(entry != NULL);
    MK_TEST_ASSERT(strcmp(entry->path, "assets/mosul/runtime/sprites/rendered/civilians_128/civilian/teenage_girl/wounded/south_east.png") == 0);

    entry = mk_asset_find_sprite_render_entry(
        &render_manifest,
        "weapon",
        "m16_rifle",
        NULL,
        NULL,
        "south_west"
    );
    MK_TEST_ASSERT(entry != NULL);
    MK_TEST_ASSERT(strcmp(entry->path, "assets/mosul/runtime/sprites/rendered/weapons_128/m16_rifle/south_west.png") == 0);

    entry = mk_asset_find_sprite_render_entry(
        &render_manifest,
        "vehicle",
        "armed_sedan",
        "destroyed",
        "opposing",
        "south_west"
    );
    MK_TEST_ASSERT(entry != NULL);
    MK_TEST_ASSERT(strcmp(entry->path, "assets/mosul/runtime/sprites/rendered/vehicles_1024/opposing/armed_sedan/destroyed/south_west.png") == 0);
}

static void test_marker_manifest_loads_tactical_markers(void) {
    char path[512];
    mk_asset_marker_manifest_t manifest;
    const mk_asset_marker_t *marker;

    make_project_path(path, sizeof(path), "assets/mosul/manifests/mosul_2003_markers.markermanifest");
    MK_TEST_ASSERT(mk_asset_load_marker_manifest(path, &manifest) == MK_OK);
    MK_TEST_ASSERT(strcmp(manifest.id, "mosul_2003_markers") == 0);
    MK_TEST_ASSERT(manifest.marker_count == 18);

    marker = mk_asset_find_marker(&manifest, "selection_ring");
    MK_TEST_ASSERT(marker != NULL);
    MK_TEST_ASSERT(strcmp(marker->kind, "selection") == 0);
    MK_TEST_ASSERT(strcmp(marker->shape, "ring") == 0);
    MK_TEST_ASSERT(marker->color.r == 220);
    MK_TEST_ASSERT(marker->color.a == 255);
    MK_TEST_ASSERT_CLOSE(marker->radius_m, 8.0f);
    MK_TEST_ASSERT(marker->line_width_px == 2);

    marker = mk_asset_find_marker(&manifest, "civilian_risk");
    MK_TEST_ASSERT(marker != NULL);
    MK_TEST_ASSERT(strcmp(marker->kind, "civilian_risk") == 0);
    MK_TEST_ASSERT(strcmp(marker->shape, "halo") == 0);

    marker = mk_asset_find_marker(&manifest, "order_investigate");
    MK_TEST_ASSERT(marker != NULL);
    MK_TEST_ASSERT(strcmp(marker->kind, "order_investigate") == 0);
    MK_TEST_ASSERT(strcmp(marker->shape, "diamond") == 0);
    MK_TEST_ASSERT(marker->line_width_px == 2);
}

static void test_missing_map_layer_is_rejected(void) {
    char path[512];
    mk_asset_map_manifest_t manifest;

    make_binary_path(path, sizeof(path), "bad_missing_layer.mapmanifest");
    write_text_file(
        path,
        "manifest_type=map\n"
        "id=bad\n"
        "name=Bad Map\n"
        "world_width_m=500\n"
        "world_height_m=500\n"
        "pixels_per_meter=14\n"
        "origin=top_left\n"
        "source_root=assets/mosul/source/maps/market_commercial_streets_demo_2003\n"
        "runtime_root=assets/mosul/runtime/maps/bad\n"
        "layer_count=1\n"
        "layer.0.id=missing\n"
        "layer.0.kind=base\n"
        "layer.0.path=assets/mosul/source/maps/market_commercial_streets_demo_2003/missing.png\n"
        "layer.0.z=0\n"
        "layer.0.alpha=opaque\n"
        "overview_path=assets/mosul/source/maps/market_commercial_streets_demo_2003/imgs/market_commercial_streets_demo_7000/preview_1400.png\n"
        "runtime_overview_path=assets/mosul/runtime/maps/bad/overview.png\n"
        "collision_output_path=assets/mosul/maps/bad_collision.mask\n"
        "navigation_output_path=assets/mosul/maps/bad_navigation.grid\n"
    );

    MK_TEST_ASSERT(mk_asset_load_map_manifest(path, MK_TEST_PROJECT_ROOT, &manifest) == MK_ERROR_INVALID_DATA);
}

static void test_sprite_manifest_rejects_missing_sheet_reference(void) {
    char path[512];
    mk_asset_sprite_manifest_t manifest;

    make_binary_path(path, sizeof(path), "bad_missing_sheet.spritemanifest");
    write_text_file(
        path,
        "manifest_type=sprites\n"
        "id=bad_sprites\n"
        "name=Bad Sprites\n"
        "fallback_runtime_id=missing_frame\n"
        "sheet_count=1\n"
        "sheet.0.id=real_sheet\n"
        "sheet.0.path=assets/mosul/source/sprite_sheets/12_us_ally_troops_topdown_128.png\n"
        "sheet.0.tile_width=128\n"
        "sheet.0.tile_height=128\n"
        "sheet.0.pivot_x=64\n"
        "sheet.0.pivot_y=64\n"
        "frame_count=1\n"
        "frame.0.id=bad_frame\n"
        "frame.0.runtime_id=missing_frame\n"
        "frame.0.sheet=no_such_sheet\n"
        "frame.0.side=player\n"
        "frame.0.role=rifleman\n"
        "frame.0.state=ready\n"
        "frame.0.facing=north\n"
        "frame.0.x=0\n"
        "frame.0.y=0\n"
        "frame.0.scale_m=2.0\n"
    );

    MK_TEST_ASSERT(mk_asset_load_sprite_manifest(path, MK_TEST_PROJECT_ROOT, &manifest) == MK_ERROR_INVALID_DATA);
}

static void test_marker_manifest_rejects_bad_color(void) {
    char path[512];
    mk_asset_marker_manifest_t manifest;

    make_binary_path(path, sizeof(path), "bad_marker_color.markermanifest");
    write_text_file(
        path,
        "manifest_type=markers\n"
        "id=bad_markers\n"
        "name=Bad Markers\n"
        "marker_count=1\n"
        "marker.0.id=bad\n"
        "marker.0.kind=selection\n"
        "marker.0.shape=ring\n"
        "marker.0.color=999,216,156,255\n"
        "marker.0.radius_m=8\n"
        "marker.0.line_width_px=2\n"
    );

    MK_TEST_ASSERT(mk_asset_load_marker_manifest(path, &manifest) == MK_ERROR_INVALID_DATA);
}

static void test_sprite_render_manifest_rejects_missing_runtime_png(void) {
    char path[512];
    static mk_asset_sprite_render_manifest_t manifest;

    make_binary_path(path, sizeof(path), "bad_missing_runtime_sprite.json");
    write_text_file(
        path,
        "{\n"
        "  \"schema_version\": 1,\n"
        "  \"source_manifest\": \"assets/mosul/runtime/sprites/manifest.json\",\n"
        "  \"rendered_count\": 1,\n"
        "  \"missing_source_count\": 0,\n"
        "  \"error_count\": 0,\n"
        "  \"rendered\": [\n"
        "    {\n"
        "      \"path\": \"assets/mosul/runtime/sprites/rendered/infantry_128/allied/us_army_rifleman/standing/missing.png\",\n"
        "      \"kind\": \"infantry\",\n"
        "      \"item_id\": \"us_army_rifleman\",\n"
        "      \"state\": \"standing\",\n"
        "      \"faction\": \"allied\",\n"
        "      \"angle\": \"north\"\n"
        "    }\n"
        "  ]\n"
        "}\n"
    );

    MK_TEST_ASSERT(mk_asset_load_sprite_render_manifest(path, MK_TEST_PROJECT_ROOT, &manifest) == MK_ERROR_INVALID_DATA);
}

static void test_building_level_manifest_rejects_missing_level_png(void) {
    char path[512];
    mk_asset_building_level_manifest_t manifest;

    make_binary_path(path, sizeof(path), "bad_missing_building_level_png.json");
    write_text_file(
        path,
        "{\n"
        "  \"schema_version\": 1,\n"
        "  \"id\": \"bad_building_levels\",\n"
        "  \"map_id\": \"market_commercial_streets_2003\",\n"
        "  \"name\": \"Bad Building Levels\",\n"
        "  \"world_width_m\": 500,\n"
        "  \"world_height_m\": 500,\n"
        "  \"pixel_width\": 7000,\n"
        "  \"pixel_height\": 7000,\n"
        "  \"pixels_per_meter\": 14,\n"
        "  \"origin\": \"top_left\",\n"
        "  \"art_style\": \"test\",\n"
        "  \"max_storeys\": 1,\n"
        "  \"level_count\": 1,\n"
        "  \"feature_count\": 1,\n"
        "  \"building_region_count\": 1,\n"
        "  \"levels\": [\n"
        "    {\n"
        "      \"id\": \"ground\",\n"
        "      \"index\": 1,\n"
        "      \"elevation_m\": 0,\n"
        "      \"png\": \"assets/mosul/runtime/maps/market_commercial_streets_2003/levels/missing.png\",\n"
        "      \"alpha\": \"opaque\",\n"
        "      \"blocks_los_default\": false,\n"
        "      \"blocks_movement_default\": false\n"
        "    }\n"
        "  ],\n"
        "  \"features\": [\n"
        "    {\n"
        "      \"id\": \"wall\",\n"
        "      \"level_id\": \"ground\",\n"
        "      \"kind\": \"wall\",\n"
        "      \"x\": 0,\n"
        "      \"y\": 0,\n"
        "      \"width\": 10,\n"
        "      \"height\": 10,\n"
        "      \"blocks_los\": true,\n"
        "      \"blocks_movement\": true,\n"
        "      \"allows_los\": false,\n"
        "      \"allows_movement\": false\n"
        "    }\n"
        "  ],\n"
        "  \"building_regions\": [\n"
        "    {\n"
        "      \"id\": \"region\",\n"
        "      \"storeys\": 1,\n"
        "      \"x\": 0,\n"
        "      \"y\": 0,\n"
        "      \"width\": 10,\n"
        "      \"height\": 10,\n"
        "      \"roof_level_id\": \"ground\"\n"
        "    }\n"
        "  ]\n"
        "}\n"
    );

    MK_TEST_ASSERT(mk_asset_load_building_level_manifest(path, MK_TEST_PROJECT_ROOT, &manifest) == MK_ERROR_INVALID_DATA);
}

static void test_topology_manifest_rejects_one_way_portal(void) {
    char building_path[512];
    char bad_topology_path[512];
    mk_asset_building_level_manifest_t building_manifest;
    mk_asset_topology_manifest_t topology_manifest;

    make_project_path(
        building_path,
        sizeof(building_path),
        "assets/mosul/manifests/market_commercial_streets_2003_building_levels.json"
    );
    make_binary_path(bad_topology_path, sizeof(bad_topology_path), "bad_one_way_topology.json");
    write_project_file_with_replacement(
        "assets/mosul/manifests/market_commercial_streets_2003_topology.json",
        bad_topology_path,
        "\"bidirectional\": true",
        "\"bidirectional\": false"
    );

    MK_TEST_ASSERT(mk_asset_load_building_level_manifest(building_path, MK_TEST_PROJECT_ROOT, &building_manifest) == MK_OK);
    MK_TEST_ASSERT(mk_asset_load_topology_manifest(bad_topology_path, &building_manifest, &topology_manifest) == MK_ERROR_INVALID_DATA);
}

int main(void) {
    test_map_manifest_loads_market_layers();
    test_building_level_manifest_loads_multistorey_stack();
    test_topology_manifest_loads_market_graph();
    test_sprite_manifest_loads_first_frames();
    test_sprite_render_manifest_loads_all_runtime_facings();
    test_marker_manifest_loads_tactical_markers();
    test_missing_map_layer_is_rejected();
    test_sprite_manifest_rejects_missing_sheet_reference();
    test_marker_manifest_rejects_bad_color();
    test_sprite_render_manifest_rejects_missing_runtime_png();
    test_building_level_manifest_rejects_missing_level_png();
    test_topology_manifest_rejects_one_way_portal();

    puts("mk_asset_manifest_tests: ok");
    return 0;
}
