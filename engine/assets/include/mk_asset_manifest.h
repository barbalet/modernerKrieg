#ifndef MODERNER_KRIEG_ASSET_MANIFEST_H
#define MODERNER_KRIEG_ASSET_MANIFEST_H

#include "mk_core.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MK_ASSET_PATH_CAPACITY 256
#define MK_ASSET_KIND_CAPACITY 32
#define MK_ASSET_MAX_MAP_LAYERS 16
#define MK_ASSET_MAX_SPRITE_SHEETS 16
#define MK_ASSET_MAX_SPRITE_FRAMES 128
#define MK_ASSET_MAX_SPRITE_RENDER_ENTRIES 2048
#define MK_ASSET_MAX_MARKERS 64
#define MK_ASSET_MAX_BUILDING_LEVELS 8
#define MK_ASSET_MAX_BUILDING_FEATURES 128
#define MK_ASSET_MAX_BUILDING_REGIONS 64

typedef struct {
    char id[MK_NAME_CAPACITY];
    char kind[MK_ASSET_KIND_CAPACITY];
    char path[MK_ASSET_PATH_CAPACITY];
    int z;
    char alpha[MK_ASSET_KIND_CAPACITY];
} mk_asset_map_layer_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    char name[MK_NAME_CAPACITY];
    float world_width_m;
    float world_height_m;
    float pixels_per_meter;
    char origin[MK_ASSET_KIND_CAPACITY];
    char source_root[MK_ASSET_PATH_CAPACITY];
    char runtime_root[MK_ASSET_PATH_CAPACITY];
    char overview_path[MK_ASSET_PATH_CAPACITY];
    char runtime_overview_path[MK_ASSET_PATH_CAPACITY];
    char collision_output_path[MK_ASSET_PATH_CAPACITY];
    char navigation_output_path[MK_ASSET_PATH_CAPACITY];
    size_t layer_count;
    mk_asset_map_layer_t layers[MK_ASSET_MAX_MAP_LAYERS];
} mk_asset_map_manifest_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    char path[MK_ASSET_PATH_CAPACITY];
    int tile_width;
    int tile_height;
    int pivot_x;
    int pivot_y;
} mk_asset_sprite_sheet_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    char runtime_id[MK_NAME_CAPACITY];
    char sheet[MK_NAME_CAPACITY];
    char side[MK_ASSET_KIND_CAPACITY];
    char role[MK_ASSET_KIND_CAPACITY];
    char state[MK_ASSET_KIND_CAPACITY];
    char facing[MK_ASSET_KIND_CAPACITY];
    int x;
    int y;
    float scale_m;
} mk_asset_sprite_frame_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    char name[MK_NAME_CAPACITY];
    char fallback_runtime_id[MK_NAME_CAPACITY];
    char source_angle_root[MK_ASSET_PATH_CAPACITY];
    char runtime_rendered_root[MK_ASSET_PATH_CAPACITY];
    char runtime_pipeline_manifest[MK_ASSET_PATH_CAPACITY];
    char runtime_render_manifest[MK_ASSET_PATH_CAPACITY];
    size_t runtime_rendered_count;
    size_t runtime_infantry_count;
    size_t runtime_civilian_count;
    size_t runtime_weapon_count;
    size_t runtime_vehicle_count;
    size_t sheet_count;
    mk_asset_sprite_sheet_t sheets[MK_ASSET_MAX_SPRITE_SHEETS];
    size_t frame_count;
    mk_asset_sprite_frame_t frames[MK_ASSET_MAX_SPRITE_FRAMES];
} mk_asset_sprite_manifest_t;

typedef struct {
    char path[MK_ASSET_PATH_CAPACITY];
    char kind[MK_ASSET_KIND_CAPACITY];
    char item_id[MK_NAME_CAPACITY];
    char state[MK_ASSET_KIND_CAPACITY];
    char faction[MK_ASSET_KIND_CAPACITY];
    char angle[MK_ASSET_KIND_CAPACITY];
} mk_asset_sprite_render_entry_t;

typedef struct {
    int schema_version;
    char source_manifest[MK_ASSET_PATH_CAPACITY];
    size_t rendered_count;
    size_t missing_source_count;
    size_t error_count;
    size_t infantry_count;
    size_t civilian_count;
    size_t weapon_count;
    size_t vehicle_count;
    mk_asset_sprite_render_entry_t rendered[MK_ASSET_MAX_SPRITE_RENDER_ENTRIES];
} mk_asset_sprite_render_manifest_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    char kind[MK_ASSET_KIND_CAPACITY];
    char shape[MK_ASSET_KIND_CAPACITY];
    mk_color_t color;
    float radius_m;
    int line_width_px;
} mk_asset_marker_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    char name[MK_NAME_CAPACITY];
    size_t marker_count;
    mk_asset_marker_t markers[MK_ASSET_MAX_MARKERS];
} mk_asset_marker_manifest_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    int index;
    float elevation_m;
    char png_path[MK_ASSET_PATH_CAPACITY];
    char alpha[MK_ASSET_KIND_CAPACITY];
    bool blocks_los_default;
    bool blocks_movement_default;
} mk_asset_building_level_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    char level_id[MK_NAME_CAPACITY];
    char kind[MK_ASSET_KIND_CAPACITY];
    int x;
    int y;
    int width;
    int height;
    bool blocks_los;
    bool blocks_movement;
    bool allows_los;
    bool allows_movement;
} mk_asset_building_feature_t;

typedef struct {
    char id[MK_NAME_CAPACITY];
    int storeys;
    int x;
    int y;
    int width;
    int height;
    char roof_level_id[MK_NAME_CAPACITY];
} mk_asset_building_region_t;

typedef struct {
    int schema_version;
    char id[MK_NAME_CAPACITY];
    char map_id[MK_NAME_CAPACITY];
    char name[MK_NAME_CAPACITY];
    float world_width_m;
    float world_height_m;
    int pixel_width;
    int pixel_height;
    float pixels_per_meter;
    char origin[MK_ASSET_KIND_CAPACITY];
    char art_style[MK_ASSET_PATH_CAPACITY];
    int max_storeys;
    size_t level_count;
    mk_asset_building_level_t levels[MK_ASSET_MAX_BUILDING_LEVELS];
    size_t feature_count;
    mk_asset_building_feature_t features[MK_ASSET_MAX_BUILDING_FEATURES];
    size_t region_count;
    mk_asset_building_region_t regions[MK_ASSET_MAX_BUILDING_REGIONS];
} mk_asset_building_level_manifest_t;

mk_result_t mk_asset_load_map_manifest(
    const char *manifest_path,
    const char *project_root,
    mk_asset_map_manifest_t *out_manifest
);

mk_result_t mk_asset_load_sprite_manifest(
    const char *manifest_path,
    const char *project_root,
    mk_asset_sprite_manifest_t *out_manifest
);

mk_result_t mk_asset_load_sprite_render_manifest(
    const char *manifest_path,
    const char *project_root,
    mk_asset_sprite_render_manifest_t *out_manifest
);

mk_result_t mk_asset_load_marker_manifest(
    const char *manifest_path,
    mk_asset_marker_manifest_t *out_manifest
);

mk_result_t mk_asset_load_building_level_manifest(
    const char *manifest_path,
    const char *project_root,
    mk_asset_building_level_manifest_t *out_manifest
);

const mk_asset_sprite_sheet_t *mk_asset_find_sprite_sheet(
    const mk_asset_sprite_manifest_t *manifest,
    const char *sheet_id
);

const mk_asset_sprite_frame_t *mk_asset_find_sprite_frame(
    const mk_asset_sprite_manifest_t *manifest,
    const char *runtime_id
);

const mk_asset_sprite_render_entry_t *mk_asset_find_sprite_render_entry(
    const mk_asset_sprite_render_manifest_t *manifest,
    const char *kind,
    const char *item_id,
    const char *state,
    const char *faction,
    const char *angle
);

const mk_asset_marker_t *mk_asset_find_marker(
    const mk_asset_marker_manifest_t *manifest,
    const char *marker_id
);

const mk_asset_building_level_t *mk_asset_find_building_level(
    const mk_asset_building_level_manifest_t *manifest,
    const char *level_id
);

const mk_asset_building_feature_t *mk_asset_find_building_feature(
    const mk_asset_building_level_manifest_t *manifest,
    const char *feature_id
);

const mk_asset_building_region_t *mk_asset_find_building_region(
    const mk_asset_building_level_manifest_t *manifest,
    const char *region_id
);

bool mk_asset_building_feature_contains_pixel(
    const mk_asset_building_feature_t *feature,
    int x,
    int y
);

bool mk_asset_building_level_blocks_los_at_pixel(
    const mk_asset_building_level_manifest_t *manifest,
    const char *level_id,
    int x,
    int y
);

bool mk_asset_building_level_blocks_movement_at_pixel(
    const mk_asset_building_level_manifest_t *manifest,
    const char *level_id,
    int x,
    int y
);

#ifdef __cplusplus
}
#endif

#endif
