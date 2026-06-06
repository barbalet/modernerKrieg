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
#define MK_ASSET_MAX_MARKERS 64

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
    size_t sheet_count;
    mk_asset_sprite_sheet_t sheets[MK_ASSET_MAX_SPRITE_SHEETS];
    size_t frame_count;
    mk_asset_sprite_frame_t frames[MK_ASSET_MAX_SPRITE_FRAMES];
} mk_asset_sprite_manifest_t;

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

mk_result_t mk_asset_load_marker_manifest(
    const char *manifest_path,
    mk_asset_marker_manifest_t *out_manifest
);

const mk_asset_sprite_sheet_t *mk_asset_find_sprite_sheet(
    const mk_asset_sprite_manifest_t *manifest,
    const char *sheet_id
);

const mk_asset_sprite_frame_t *mk_asset_find_sprite_frame(
    const mk_asset_sprite_manifest_t *manifest,
    const char *runtime_id
);

const mk_asset_marker_t *mk_asset_find_marker(
    const mk_asset_marker_manifest_t *manifest,
    const char *marker_id
);

#ifdef __cplusplus
}
#endif

#endif
