#include "mk_asset_manifest.h"
#include "mk_core.h"
#include "mk_board_view.h"
#include "mk_log.h"
#include "mk_mosul_demo.h"

#include <SDL3/SDL.h>
#ifdef MK_HAS_SDL3_IMAGE
#include <SDL3_image/SDL_image.h>
#endif
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MK_SDL_MAP_MANIFEST_PATH "assets/mosul/manifests/market_commercial_streets_2003.mapmanifest"
#define MK_SDL_SPRITE_MANIFEST_PATH "assets/mosul/manifests/mosul_2003_sprites.spritemanifest"
#define MK_SDL_MARKER_MANIFEST_PATH "assets/mosul/manifests/mosul_2003_markers.markermanifest"

typedef struct {
    char id[MK_NAME_CAPACITY];
    SDL_Texture *texture;
} mk_sdl_sprite_sheet_texture_t;

typedef struct {
    mk_asset_sprite_manifest_t manifest;
    mk_sdl_sprite_sheet_texture_t sheets[MK_ASSET_MAX_SPRITE_SHEETS];
    size_t sheet_count;
    bool manifest_loaded;
    bool textures_loaded;
} mk_sdl_sprite_assets_t;

typedef struct {
    mk_asset_marker_manifest_t manifest;
    bool loaded;
} mk_sdl_marker_assets_t;

typedef struct {
    bool quit_requested;
    bool pan_left;
    bool pan_right;
    bool pan_up;
    bool pan_down;
    bool zoom_in;
    bool zoom_out;
    bool select_pressed;
    bool order_pressed;
    mk_vec2_t mouse_screen_position;
} mk_sdl_input_t;

static SDL_FRect mk_sdl_rect(mk_rect_t rect) {
    SDL_FRect output;

    output.x = rect.x;
    output.y = rect.y;
    output.w = rect.width;
    output.h = rect.height;

    return output;
}

static mk_vec2_t mk_screen_center(const mk_board_view_t *view) {
    mk_vec2_t center;

    center.x = view->screen_rect_px.x + view->screen_rect_px.width * 0.5f;
    center.y = view->screen_rect_px.y + view->screen_rect_px.height * 0.5f;

    return center;
}

static void mk_sdl_copy_name(char *destination, size_t capacity, const char *source) {
    const char *text = source == NULL ? "" : source;
    size_t index = 0;

    if (destination == NULL || capacity == 0) {
        return;
    }

    for (; index + 1 < capacity && text[index] != '\0'; ++index) {
        destination[index] = text[index];
    }

    destination[index] = '\0';
}

static void mk_sdl_input_begin_frame(mk_sdl_input_t *input) {
    if (input == NULL) {
        return;
    }

    memset(input, 0, sizeof(*input));
}

static void mk_sdl_input_handle_event(mk_sdl_input_t *input, const SDL_Event *event) {
    if (input == NULL || event == NULL) {
        return;
    }

    if (event->type == SDL_EVENT_QUIT) {
        input->quit_requested = true;
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        switch (event->key.key) {
            case SDLK_ESCAPE:
                input->quit_requested = true;
                break;
            case SDLK_RIGHT:
                input->pan_right = true;
                break;
            case SDLK_LEFT:
                input->pan_left = true;
                break;
            case SDLK_DOWN:
                input->pan_down = true;
                break;
            case SDLK_UP:
                input->pan_up = true;
                break;
            case SDLK_EQUALS:
                input->zoom_in = true;
                break;
            case SDLK_MINUS:
                input->zoom_out = true;
                break;
            default:
                break;
        }
    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        input->mouse_screen_position.x = event->motion.x;
        input->mouse_screen_position.y = event->motion.y;
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        input->mouse_screen_position.x = event->button.x;
        input->mouse_screen_position.y = event->button.y;

        if (event->button.button == SDL_BUTTON_LEFT) {
            input->select_pressed = true;
        } else if (event->button.button == SDL_BUTTON_RIGHT) {
            input->order_pressed = true;
        }
    }
}

static void mk_sdl_apply_input(mk_game_t *game, mk_board_view_t *view, const mk_sdl_input_t *input) {
    const float pan_pixels = 48.0f;

    if (game == NULL || view == NULL || input == NULL) {
        return;
    }

    if (input->pan_right) {
        (void)mk_board_view_pan_pixels(view, &game->map, pan_pixels, 0.0f);
    }

    if (input->pan_left) {
        (void)mk_board_view_pan_pixels(view, &game->map, -pan_pixels, 0.0f);
    }

    if (input->pan_down) {
        (void)mk_board_view_pan_pixels(view, &game->map, 0.0f, pan_pixels);
    }

    if (input->pan_up) {
        (void)mk_board_view_pan_pixels(view, &game->map, 0.0f, -pan_pixels);
    }

    if (input->zoom_in) {
        (void)mk_board_view_zoom_at(view, &game->map, 1.25f, mk_screen_center(view));
    }

    if (input->zoom_out) {
        (void)mk_board_view_zoom_at(view, &game->map, 0.8f, mk_screen_center(view));
    }

    if (input->select_pressed || input->order_pressed) {
        mk_vec2_t map_position = mk_board_view_screen_to_map(view, input->mouse_screen_position);

        if (input->select_pressed) {
            uint32_t selected_unit_id = 0;
            if (mk_game_select_unit_at(game, map_position, MK_UNIT_PICK_RADIUS_M, &selected_unit_id) != MK_OK) {
                mk_game_clear_selection(game);
            }
        }

        if (input->order_pressed) {
            uint32_t contact_report_id = 0;

            if (mk_game_pick_contact_at(game, map_position, MK_UNIT_PICK_RADIUS_M * 1.5f, &contact_report_id) == MK_OK) {
                size_t report_index;
                mk_vec2_t investigate_position = map_position;

                for (report_index = 0; report_index < game->contact_report_count; ++report_index) {
                    if (game->contact_reports[report_index].id == contact_report_id) {
                        investigate_position = game->contact_reports[report_index].position_m;
                        break;
                    }
                }

                (void)mk_game_issue_selected_investigate_order(game, investigate_position);
            } else {
                (void)mk_game_issue_selected_move_order(game, map_position);
            }
        }
    }
}

static void mk_set_terrain_color(SDL_Renderer *renderer, mk_terrain_kind_t kind) {
    switch (kind) {
        case MK_TERRAIN_ROAD:
            SDL_SetRenderDrawColor(renderer, 64, 67, 65, 255);
            break;
        case MK_TERRAIN_BUILDING:
            SDL_SetRenderDrawColor(renderer, 92, 86, 75, 255);
            break;
        case MK_TERRAIN_RUBBLE:
            SDL_SetRenderDrawColor(renderer, 111, 103, 89, 255);
            break;
        case MK_TERRAIN_SUSPECTED_IED:
            SDL_SetRenderDrawColor(renderer, 116, 87, 62, 255);
            break;
        default:
            SDL_SetRenderDrawColor(renderer, 54, 60, 57, 255);
            break;
    }
}

static const char *mk_sdl_side_name(mk_side_t side) {
    switch (side) {
        case MK_SIDE_PLAYER:
            return "player";
        case MK_SIDE_OPFOR:
            return "opfor";
        case MK_SIDE_CIVILIAN:
            return "civilian";
        case MK_SIDE_NEUTRAL:
        default:
            return "neutral";
    }
}

static const char *mk_sdl_role_name(mk_soldier_role_t role) {
    switch (role) {
        case MK_ROLE_LEADER:
            return "leader";
        case MK_ROLE_MACHINE_GUNNER:
            return "machine_gunner";
        case MK_ROLE_RPG:
            return "rpg";
        case MK_ROLE_MARKSMAN:
            return "marksman";
        case MK_ROLE_ENGINEER:
            return "engineer";
        case MK_ROLE_MEDIC:
            return "medic";
        case MK_ROLE_DRONE_OPERATOR:
            return "drone_operator";
        case MK_ROLE_CIVILIAN:
            return "civilian";
        case MK_ROLE_RIFLEMAN:
        default:
            return "rifleman";
    }
}

static const char *mk_sdl_overlay_marker_id(mk_tactical_overlay_kind_t kind) {
    switch (kind) {
        case MK_TACTICAL_OVERLAY_SELECTION:
            return "selection_ring";
        case MK_TACTICAL_OVERLAY_MOVE_ROUTE:
            return "move_route";
        case MK_TACTICAL_OVERLAY_MOVE_TARGET:
            return "move_target";
        case MK_TACTICAL_OVERLAY_FIRE:
            return "fire_order";
        case MK_TACTICAL_OVERLAY_SUPPRESSION:
            return "suppression";
        case MK_TACTICAL_OVERLAY_CASUALTY:
            return "casualty";
        case MK_TACTICAL_OVERLAY_OBJECTIVE:
            return "objective";
        case MK_TACTICAL_OVERLAY_HIDDEN_CONTACT:
            return "hidden_contact";
        case MK_TACTICAL_OVERLAY_CIVILIAN_RISK:
            return "civilian_risk";
        case MK_TACTICAL_OVERLAY_SUSPECTED_CONTACT:
            return "hidden_contact";
        case MK_TACTICAL_OVERLAY_FALSE_CONTACT:
            return "civilian_risk";
        case MK_TACTICAL_OVERLAY_OBJECTIVE_CONTROL:
            return "objective";
        default:
            return "selection_ring";
    }
}

static void mk_sdl_set_color(SDL_Renderer *renderer, mk_color_t color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

static void mk_sdl_set_side_color(SDL_Renderer *renderer, mk_side_t side, bool casualty) {
    if (casualty) {
        SDL_SetRenderDrawColor(renderer, 95, 46, 42, 255);
    } else if (side == MK_SIDE_PLAYER) {
        SDL_SetRenderDrawColor(renderer, 91, 143, 186, 255);
    } else if (side == MK_SIDE_OPFOR) {
        SDL_SetRenderDrawColor(renderer, 153, 83, 67, 255);
    } else if (side == MK_SIDE_CIVILIAN) {
        SDL_SetRenderDrawColor(renderer, 180, 166, 120, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    }
}

static float mk_sdl_max_float(float first, float second) {
    return first > second ? first : second;
}

static void mk_sdl_update_window_title(SDL_Window *window, const mk_game_t *game) {
    mk_score_t score;
    const char *objective_side = "none";
    const char *objective_label = "objective";
    char title[160];

    if (window == NULL || game == NULL) {
        return;
    }

    if (game->objective_count > 0) {
        objective_side = mk_sdl_side_name(game->objectives[0].controlling_side);
        objective_label = game->objectives[0].label[0] != '\0' ? game->objectives[0].label : game->objectives[0].name;
    }

    if (mk_game_score(game, &score) != MK_OK) {
        memset(&score, 0, sizeof(score));
    }

    (void)snprintf(
        title,
        sizeof(title),
        "modernerKrieg Mosul Demo | score %d | %s %s",
        score.total_score,
        objective_label,
        objective_side
    );
    SDL_SetWindowTitle(window, title);
}

static void mk_render_map_background(
    SDL_Renderer *renderer,
    const mk_board_view_t *view,
    SDL_Texture *map_texture
) {
    SDL_FRect board = mk_sdl_rect(view->screen_rect_px);

    if (map_texture != NULL) {
        SDL_RenderTexture(renderer, map_texture, NULL, &board);
        SDL_SetRenderDrawColor(renderer, 114, 124, 107, 255);
        SDL_RenderRect(renderer, &board);
        return;
    }

    SDL_SetRenderDrawColor(renderer, 45, 52, 54, 255);
    SDL_RenderFillRect(renderer, &board);

    SDL_SetRenderDrawColor(renderer, 114, 124, 107, 255);
    SDL_RenderRect(renderer, &board);
}

static const mk_asset_sprite_frame_t *mk_sdl_find_soldier_sprite_frame(
    const mk_sdl_sprite_assets_t *assets,
    const mk_soldier_marker_t *marker
) {
    const char *side;
    const char *role;
    const char *state;
    size_t index;

    if (assets == NULL || marker == NULL || !assets->manifest_loaded) {
        return NULL;
    }

    side = mk_sdl_side_name(marker->side);
    role = mk_sdl_role_name(marker->role);
    state = marker->side == MK_SIDE_CIVILIAN ? "sheltering" : "ready";

    for (index = 0; index < assets->manifest.frame_count; ++index) {
        const mk_asset_sprite_frame_t *frame = &assets->manifest.frames[index];

        if (strcmp(frame->side, side) == 0
            && strcmp(frame->role, role) == 0
            && strcmp(frame->state, state) == 0) {
            return frame;
        }
    }

    return mk_asset_find_sprite_frame(&assets->manifest, assets->manifest.fallback_runtime_id);
}

static SDL_Texture *mk_sdl_find_sheet_texture(
    const mk_sdl_sprite_assets_t *assets,
    const char *sheet_id
) {
    size_t index;

    if (assets == NULL || sheet_id == NULL || !assets->textures_loaded) {
        return NULL;
    }

    for (index = 0; index < assets->sheet_count; ++index) {
        if (strcmp(assets->sheets[index].id, sheet_id) == 0) {
            return assets->sheets[index].texture;
        }
    }

    return NULL;
}

static void mk_render_soldier_fallback(
    SDL_Renderer *renderer,
    const mk_soldier_marker_t *marker
) {
    SDL_FRect rect;

    rect.x = marker->screen_position_px.x - 5.0f;
    rect.y = marker->screen_position_px.y - 5.0f;
    rect.w = 10.0f;
    rect.h = 10.0f;

    mk_sdl_set_side_color(renderer, marker->side, marker->casualty);
    SDL_RenderFillRect(renderer, &rect);
}

static void mk_render_soldier_marker(
    SDL_Renderer *renderer,
    const mk_board_view_t *view,
    const mk_sdl_sprite_assets_t *sprite_assets,
    const mk_soldier_marker_t *marker
) {
    const mk_asset_sprite_frame_t *frame = mk_sdl_find_soldier_sprite_frame(sprite_assets, marker);
    SDL_Texture *texture = frame != NULL ? mk_sdl_find_sheet_texture(sprite_assets, frame->sheet) : NULL;

    if (frame != NULL && texture != NULL) {
        const mk_asset_sprite_sheet_t *sheet = mk_asset_find_sprite_sheet(&sprite_assets->manifest, frame->sheet);
        float size_px = mk_sdl_max_float(16.0f, frame->scale_m * view->scale_px_per_m * 8.0f);
        SDL_FRect source_rect;
        SDL_FRect target_rect;

        source_rect.x = (float)frame->x;
        source_rect.y = (float)frame->y;
        source_rect.w = sheet != NULL ? (float)sheet->tile_width : 128.0f;
        source_rect.h = sheet != NULL ? (float)sheet->tile_height : 128.0f;
        target_rect.x = marker->screen_position_px.x - size_px * 0.5f;
        target_rect.y = marker->screen_position_px.y - size_px * 0.5f;
        target_rect.w = size_px;
        target_rect.h = size_px;
        SDL_RenderTexture(renderer, texture, &source_rect, &target_rect);
        return;
    }

    mk_render_soldier_fallback(renderer, marker);
}

static void mk_render_overlay(
    SDL_Renderer *renderer,
    const mk_sdl_marker_assets_t *marker_assets,
    const mk_tactical_overlay_t *overlay
) {
    const char *marker_id = mk_sdl_overlay_marker_id(overlay->kind);
    const mk_asset_marker_t *marker = marker_assets != NULL && marker_assets->loaded
        ? mk_asset_find_marker(&marker_assets->manifest, marker_id)
        : NULL;
    float radius_px = overlay->screen_radius_px > 0.0f ? overlay->screen_radius_px : 6.0f;
    SDL_FRect rect;

    if (marker != NULL) {
        mk_sdl_set_color(renderer, marker->color);
        if (marker->radius_m > 0.0f && overlay->screen_radius_px <= 0.0f) {
            radius_px = marker->radius_m;
        }
    } else {
        SDL_SetRenderDrawColor(renderer, 220, 216, 156, 220);
    }

    if (overlay->kind == MK_TACTICAL_OVERLAY_MOVE_ROUTE || overlay->kind == MK_TACTICAL_OVERLAY_FIRE) {
        SDL_RenderLine(
            renderer,
            overlay->screen_position_px.x,
            overlay->screen_position_px.y,
            overlay->target_screen_position_px.x,
            overlay->target_screen_position_px.y
        );
        return;
    }

    rect.x = overlay->screen_position_px.x - radius_px;
    rect.y = overlay->screen_position_px.y - radius_px;
    rect.w = radius_px * 2.0f;
    rect.h = radius_px * 2.0f;

    if (overlay->kind == MK_TACTICAL_OVERLAY_MOVE_TARGET) {
        SDL_RenderLine(renderer, rect.x, overlay->screen_position_px.y, rect.x + rect.w, overlay->screen_position_px.y);
        SDL_RenderLine(renderer, overlay->screen_position_px.x, rect.y, overlay->screen_position_px.x, rect.y + rect.h);
    } else if (overlay->kind == MK_TACTICAL_OVERLAY_CASUALTY) {
        SDL_RenderLine(renderer, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
        SDL_RenderLine(renderer, rect.x + rect.w, rect.y, rect.x, rect.y + rect.h);
    } else {
        SDL_RenderRect(renderer, &rect);
    }
}

static void mk_render(
    SDL_Renderer *renderer,
    const mk_game_t *game,
    const mk_board_view_t *view,
    SDL_Texture *map_texture,
    const mk_sdl_sprite_assets_t *sprite_assets,
    const mk_sdl_marker_assets_t *marker_assets
) {
    mk_game_snapshot_t snapshot;
    mk_soldier_marker_t soldier_markers[MK_MAX_UNITS * MK_MAX_SOLDIERS_PER_UNIT];
    mk_tactical_overlay_t overlays[MK_BOARD_VIEW_MAX_TACTICAL_OVERLAYS];
    size_t soldier_marker_count = 0;
    size_t overlay_count = 0;
    size_t terrain_index;
    size_t index;

    SDL_SetRenderDrawColor(renderer, 18, 22, 24, 255);
    SDL_RenderClear(renderer);

    mk_render_map_background(renderer, view, map_texture);

    for (terrain_index = 0; terrain_index < game->map.terrain_count; ++terrain_index) {
        const mk_terrain_zone_t *terrain = &game->map.terrain[terrain_index];
        SDL_FRect terrain_rect = mk_sdl_rect(mk_board_view_map_rect_to_screen(view, terrain->bounds_m));

        mk_set_terrain_color(renderer, terrain->kind);
        SDL_RenderFillRect(renderer, &terrain_rect);

        if (terrain->blocks_line_of_sight) {
            SDL_SetRenderDrawColor(renderer, 142, 132, 108, 255);
            SDL_RenderRect(renderer, &terrain_rect);
        }
    }

    if (mk_game_snapshot(game, &snapshot) == MK_OK
        && mk_board_view_collect_tactical_overlays(
            view,
            &snapshot,
            overlays,
            sizeof(overlays) / sizeof(overlays[0]),
            &overlay_count
        ) == MK_OK
        && mk_board_view_collect_soldier_markers(
            view,
            &snapshot,
            soldier_markers,
            sizeof(soldier_markers) / sizeof(soldier_markers[0]),
            &soldier_marker_count
        ) == MK_OK) {
        for (index = 0; index < overlay_count; ++index) {
            if (overlays[index].kind != MK_TACTICAL_OVERLAY_SELECTION
                && overlays[index].kind != MK_TACTICAL_OVERLAY_CASUALTY) {
                mk_render_overlay(renderer, marker_assets, &overlays[index]);
            }
        }

        for (index = 0; index < soldier_marker_count; ++index) {
            mk_render_soldier_marker(renderer, view, sprite_assets, &soldier_markers[index]);
        }

        for (index = 0; index < overlay_count; ++index) {
            if (overlays[index].kind == MK_TACTICAL_OVERLAY_SELECTION
                || overlays[index].kind == MK_TACTICAL_OVERLAY_CASUALTY) {
                mk_render_overlay(renderer, marker_assets, &overlays[index]);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

static SDL_Texture *mk_load_manifest_map_texture(SDL_Renderer *renderer) {
    mk_asset_map_manifest_t manifest;
    mk_result_t result;

    result = mk_asset_load_map_manifest(MK_SDL_MAP_MANIFEST_PATH, ".", &manifest);
    if (result != MK_OK) {
        SDL_Log("Map manifest unavailable: %s", mk_result_name(result));
        return NULL;
    }

#ifdef MK_HAS_SDL3_IMAGE
    {
        SDL_Texture *texture = IMG_LoadTexture(renderer, manifest.runtime_overview_path);

        if (texture == NULL) {
            texture = IMG_LoadTexture(renderer, manifest.overview_path);
        }

        if (texture == NULL) {
            SDL_Log("Failed to load map PNG \"%s\": %s", manifest.overview_path, SDL_GetError());
            return NULL;
        }

        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
        SDL_Log("Loaded map PNG \"%s\" from manifest \"%s\".", manifest.overview_path, manifest.id);
        return texture;
    }
#else
    SDL_Log("Loaded map manifest \"%s\"; SDL3_image is unavailable, using fallback map renderer.", manifest.id);
    return NULL;
#endif
}

static void mk_sdl_load_sprite_assets(SDL_Renderer *renderer, mk_sdl_sprite_assets_t *assets) {
    mk_result_t result;
    size_t index;

    if (assets == NULL) {
        return;
    }

    memset(assets, 0, sizeof(*assets));
    result = mk_asset_load_sprite_manifest(MK_SDL_SPRITE_MANIFEST_PATH, ".", &assets->manifest);
    if (result != MK_OK) {
        SDL_Log("Sprite manifest unavailable: %s", mk_result_name(result));
        return;
    }

    assets->manifest_loaded = true;
    assets->sheet_count = assets->manifest.sheet_count;

#ifdef MK_HAS_SDL3_IMAGE
    for (index = 0; index < assets->manifest.sheet_count; ++index) {
        const mk_asset_sprite_sheet_t *sheet = &assets->manifest.sheets[index];

        mk_sdl_copy_name(assets->sheets[index].id, sizeof(assets->sheets[index].id), sheet->id);
        assets->sheets[index].texture = IMG_LoadTexture(renderer, sheet->path);
        if (assets->sheets[index].texture == NULL) {
            SDL_Log("Failed to load sprite sheet \"%s\": %s", sheet->path, SDL_GetError());
            assets->textures_loaded = false;
            return;
        }

        SDL_SetTextureScaleMode(assets->sheets[index].texture, SDL_SCALEMODE_NEAREST);
    }

    assets->textures_loaded = true;
    SDL_Log("Loaded sprite manifest \"%s\" with %u sheets.", assets->manifest.id, (unsigned)assets->sheet_count);
#else
    (void)renderer;
    for (index = 0; index < assets->manifest.sheet_count; ++index) {
        mk_sdl_copy_name(assets->sheets[index].id, sizeof(assets->sheets[index].id), assets->manifest.sheets[index].id);
    }
    SDL_Log("Loaded sprite manifest \"%s\"; SDL3_image is unavailable, using fallback unit markers.", assets->manifest.id);
#endif
}

static void mk_sdl_destroy_sprite_assets(mk_sdl_sprite_assets_t *assets) {
    size_t index;

    if (assets == NULL) {
        return;
    }

    for (index = 0; index < assets->sheet_count; ++index) {
        if (assets->sheets[index].texture != NULL) {
            SDL_DestroyTexture(assets->sheets[index].texture);
            assets->sheets[index].texture = NULL;
        }
    }
}

static void mk_sdl_load_marker_assets(mk_sdl_marker_assets_t *assets) {
    mk_result_t result;

    if (assets == NULL) {
        return;
    }

    memset(assets, 0, sizeof(*assets));
    result = mk_asset_load_marker_manifest(MK_SDL_MARKER_MANIFEST_PATH, &assets->manifest);
    if (result != MK_OK) {
        SDL_Log("Marker manifest unavailable: %s", mk_result_name(result));
        return;
    }

    assets->loaded = true;
    SDL_Log("Loaded marker manifest \"%s\" with %u markers.", assets->manifest.id, (unsigned)assets->manifest.marker_count);
}

int main(int argc, char **argv) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *map_texture = NULL;
    mk_sdl_sprite_assets_t sprite_assets;
    mk_sdl_marker_assets_t marker_assets;
    bool running = true;
    mk_game_t game;
    mk_scenario_definition_t scenario;
    mk_board_view_t view;
    mk_result_t result;

    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("modernerKrieg Mosul Demo", 960, 640, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    result = mk_mosul_make_market_2003_scenario(&scenario);
    if (result == MK_OK) {
        result = mk_game_load_scenario(&game, &scenario);
    }

    if (result != MK_OK) {
        SDL_Log("Failed to load Mosul demo scenario: %s", mk_result_name(result));
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    (void)mk_game_update_objective_control(&game);
    SDL_Log("Briefing: %s", game.briefing[0] != '\0' ? game.briefing : "(none)");
    mk_sdl_update_window_title(window, &game);

    result = mk_board_view_fit_map(&view, &game.map, 960.0f, 640.0f, MK_BOARD_VIEW_DEFAULT_MARGIN_PX);
    if (result != MK_OK) {
        SDL_Log("Failed to create board view: %s", mk_result_name(result));
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    map_texture = mk_load_manifest_map_texture(renderer);
    mk_sdl_load_sprite_assets(renderer, &sprite_assets);
    mk_sdl_load_marker_assets(&marker_assets);

    while (running) {
        SDL_Event event;
        mk_sdl_input_t input;

        mk_sdl_input_begin_frame(&input);
        while (SDL_PollEvent(&event)) {
            mk_sdl_input_handle_event(&input, &event);
        }

        if (input.quit_requested) {
            running = false;
            continue;
        }

        mk_sdl_apply_input(&game, &view, &input);
        (void)mk_game_run_fixed_steps(&game, 1, NULL, NULL);
        mk_sdl_update_window_title(window, &game);
        mk_render(renderer, &game, &view, map_texture, &sprite_assets, &marker_assets);
        SDL_Delay(16);
    }

    mk_sdl_destroy_sprite_assets(&sprite_assets);

    if (map_texture != NULL) {
        SDL_DestroyTexture(map_texture);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
