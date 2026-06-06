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
#include <string.h>

#define MK_SDL_MAP_MANIFEST_PATH "assets/mosul/manifests/market_commercial_streets_2003.mapmanifest"

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
            (void)mk_game_issue_selected_move_order(game, map_position);
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

static void mk_render(
    SDL_Renderer *renderer,
    const mk_game_t *game,
    const mk_board_view_t *view,
    SDL_Texture *map_texture
) {
    size_t terrain_index;
    size_t objective_index;
    size_t unit_index;

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

    for (objective_index = 0; objective_index < game->objective_count; ++objective_index) {
        const mk_objective_t *objective = &game->objectives[objective_index];
        mk_vec2_t objective_center = mk_board_view_map_to_screen(view, objective->position_m);
        SDL_FRect objective_rect;

        objective_rect.x = objective_center.x - objective->radius_m * view->scale_px_per_m;
        objective_rect.y = objective_center.y - objective->radius_m * view->scale_px_per_m;
        objective_rect.w = objective->radius_m * 2.0f * view->scale_px_per_m;
        objective_rect.h = objective->radius_m * 2.0f * view->scale_px_per_m;

        SDL_SetRenderDrawColor(renderer, 139, 128, 74, 255);
        SDL_RenderRect(renderer, &objective_rect);
    }

    for (unit_index = 0; unit_index < game->unit_count; ++unit_index) {
        const mk_unit_t *unit = &game->units[unit_index];
        mk_vec2_t unit_position = mk_board_view_map_to_screen(view, unit->position_m);
        SDL_FRect unit_rect;

        unit_rect.x = unit_position.x - 12.0f;
        unit_rect.y = unit_position.y - 12.0f;
        unit_rect.w = 24.0f;
        unit_rect.h = 24.0f;

        if (unit->side == MK_SIDE_PLAYER) {
            SDL_SetRenderDrawColor(renderer, 91, 143, 186, 255);
        } else if (unit->side == MK_SIDE_OPFOR) {
            SDL_SetRenderDrawColor(renderer, 153, 83, 67, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 180, 166, 120, 255);
        }

        SDL_RenderFillRect(renderer, &unit_rect);

        if (unit->id == game->selected_unit_id) {
            SDL_FRect selection_rect = unit_rect;

            selection_rect.x -= 4.0f;
            selection_rect.y -= 4.0f;
            selection_rect.w += 8.0f;
            selection_rect.h += 8.0f;

            SDL_SetRenderDrawColor(renderer, 220, 216, 156, 255);
            SDL_RenderRect(renderer, &selection_rect);
        }

        if (unit->has_move_target) {
            mk_vec2_t target_position = mk_board_view_map_to_screen(view, unit->target_position_m);
            float start_x = unit_position.x;
            float start_y = unit_position.y;
            float target_x = target_position.x;
            float target_y = target_position.y;
            SDL_FRect target_rect = { target_x - 5.0f, target_y - 5.0f, 10.0f, 10.0f };

            SDL_SetRenderDrawColor(renderer, 220, 216, 156, 255);
            SDL_RenderLine(renderer, start_x, start_y, target_x, target_y);
            SDL_RenderRect(renderer, &target_rect);
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
        SDL_Texture *texture = IMG_LoadTexture(renderer, manifest.overview_path);

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

int main(int argc, char **argv) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *map_texture = NULL;
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

    result = mk_board_view_fit_map(&view, &game.map, 960.0f, 640.0f, MK_BOARD_VIEW_DEFAULT_MARGIN_PX);
    if (result != MK_OK) {
        SDL_Log("Failed to create board view: %s", mk_result_name(result));
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    map_texture = mk_load_manifest_map_texture(renderer);

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
        mk_render(renderer, &game, &view, map_texture);
        SDL_Delay(16);
    }

    if (map_texture != NULL) {
        SDL_DestroyTexture(map_texture);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
