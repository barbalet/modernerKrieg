#include "mk_core.h"
#include "mk_board_view.h"
#include "mk_mosul_demo.h"

#include <SDL3/SDL.h>
#include <stdbool.h>

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

static void mk_render(SDL_Renderer *renderer, const mk_game_t *game, const mk_board_view_t *view) {
    SDL_FRect board = mk_sdl_rect(view->screen_rect_px);
    size_t terrain_index;
    size_t objective_index;
    size_t unit_index;

    SDL_SetRenderDrawColor(renderer, 18, 22, 24, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 45, 52, 54, 255);
    SDL_RenderFillRect(renderer, &board);

    SDL_SetRenderDrawColor(renderer, 114, 124, 107, 255);
    SDL_RenderRect(renderer, &board);

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

int main(int argc, char **argv) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    bool running = true;
    mk_game_t game;
    mk_scenario_definition_t scenario;
    mk_board_view_t view;

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

    if (mk_mosul_make_east_block_scenario(&scenario) != MK_OK
        || mk_game_load_scenario(&game, &scenario) != MK_OK) {
        SDL_Log("Failed to load Mosul demo scenario.");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (mk_board_view_fit_map(&view, &game.map, 960.0f, 640.0f, MK_BOARD_VIEW_DEFAULT_MARGIN_PX) != MK_OK) {
        SDL_Log("Failed to create board view.");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_RIGHT) {
                (void)mk_board_view_pan_pixels(&view, &game.map, 48.0f, 0.0f);
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_LEFT) {
                (void)mk_board_view_pan_pixels(&view, &game.map, -48.0f, 0.0f);
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_DOWN) {
                (void)mk_board_view_pan_pixels(&view, &game.map, 0.0f, 48.0f);
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_UP) {
                (void)mk_board_view_pan_pixels(&view, &game.map, 0.0f, -48.0f);
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_EQUALS) {
                (void)mk_board_view_zoom_at(&view, &game.map, 1.25f, mk_screen_center(&view));
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_MINUS) {
                (void)mk_board_view_zoom_at(&view, &game.map, 0.8f, mk_screen_center(&view));
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                mk_vec2_t screen_position;
                mk_vec2_t map_position;

                screen_position.x = event.button.x;
                screen_position.y = event.button.y;
                map_position = mk_board_view_screen_to_map(&view, screen_position);

                if (event.button.button == SDL_BUTTON_LEFT) {
                    uint32_t selected_unit_id = 0;
                    if (mk_game_select_unit_at(&game, map_position, MK_UNIT_PICK_RADIUS_M, &selected_unit_id) != MK_OK) {
                        mk_game_clear_selection(&game);
                    }
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    (void)mk_game_issue_selected_move_order(&game, map_position);
                }
            }
        }

        mk_game_step(&game);
        mk_render(renderer, &game, &view);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
