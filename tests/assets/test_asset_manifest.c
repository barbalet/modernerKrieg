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

static void test_map_manifest_loads_market_layers(void) {
    char path[512];
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
    MK_TEST_ASSERT(manifest.frame_count == 5);

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

int main(void) {
    test_map_manifest_loads_market_layers();
    test_sprite_manifest_loads_first_frames();
    test_missing_map_layer_is_rejected();
    test_sprite_manifest_rejects_missing_sheet_reference();

    puts("mk_asset_manifest_tests: ok");
    return 0;
}
