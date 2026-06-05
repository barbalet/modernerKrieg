#include "mk_core.h"

#include <SDL3/SDL.h>
#include <stdbool.h>

static void mk_render(SDL_Renderer *renderer, const mk_game_t *game) {
    SDL_FRect board = { 48.0f, 48.0f, 864.0f, 544.0f };
    SDL_FRect objective = { 676.0f, 244.0f, 96.0f, 96.0f };
    size_t unit_index;

    SDL_SetRenderDrawColor(renderer, 18, 22, 24, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 45, 52, 54, 255);
    SDL_RenderFillRect(renderer, &board);

    SDL_SetRenderDrawColor(renderer, 114, 124, 107, 255);
    SDL_RenderRect(renderer, &board);

    SDL_SetRenderDrawColor(renderer, 85, 85, 80, 255);
    SDL_RenderFillRect(renderer, &objective);

    for (unit_index = 0; unit_index < game->unit_count; ++unit_index) {
        const mk_unit_t *unit = &game->units[unit_index];
        SDL_FRect unit_rect = {
            48.0f + unit->position_m.x,
            48.0f + unit->position_m.y,
            24.0f,
            24.0f
        };

        if (unit->side == MK_SIDE_PLAYER) {
            SDL_SetRenderDrawColor(renderer, 91, 143, 186, 255);
        } else if (unit->side == MK_SIDE_OPFOR) {
            SDL_SetRenderDrawColor(renderer, 153, 83, 67, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 180, 166, 120, 255);
        }

        SDL_RenderFillRect(renderer, &unit_rect);
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    bool running = true;
    mk_game_t game;
    mk_weapon_profile_t m4;
    mk_weapon_profile_t akm;
    mk_unit_t cts;
    mk_unit_t defender;
    mk_soldier_t cts_lead;
    mk_soldier_t cts_rifleman;
    mk_soldier_t defensive_rifleman;
    mk_vec2_t cts_position;
    mk_vec2_t defender_position;

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

    mk_game_init(&game, UINT64_C(0x4D4F53554C));

    m4 = mk_make_weapon("M4", 300, 2, 35, 8);
    akm = mk_make_weapon("AKM", 250, 2, 30, 7);
    cts_lead = mk_make_soldier("CTS Lead", MK_ROLE_LEADER, m4);
    cts_rifleman = mk_make_soldier("CTS Rifleman", MK_ROLE_RIFLEMAN, m4);
    defensive_rifleman = mk_make_soldier("Defender", MK_ROLE_RIFLEMAN, akm);
    cts_position.x = 160.0f;
    cts_position.y = 292.0f;
    defender_position.x = 646.0f;
    defender_position.y = 292.0f;

    cts = mk_make_unit("CTS Assault Element", MK_SIDE_PLAYER, MK_TRAINING_ELITE, cts_position);
    mk_unit_add_soldier(&cts, &cts_lead, NULL);
    mk_unit_add_soldier(&cts, &cts_rifleman, NULL);
    mk_game_add_unit(&game, &cts, NULL);

    defender = mk_make_unit("Hidden Defensive Cell", MK_SIDE_OPFOR, MK_TRAINING_REGULAR, defender_position);
    mk_unit_add_soldier(&defender, &defensive_rifleman, NULL);
    mk_game_add_unit(&game, &defender, NULL);

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        mk_game_step(&game);
        mk_render(renderer, &game);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
