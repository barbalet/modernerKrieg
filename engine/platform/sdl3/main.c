#include "mk_asset_manifest.h"
#include "mk_ai.h"
#include "mk_core.h"
#include "mk_board_view.h"
#include "mk_log.h"
#include "mk_mosul_demo.h"

#include <SDL3/SDL.h>
#ifdef MK_HAS_SDL3_IMAGE
#include <SDL3_image/SDL_image.h>
#endif
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MK_PROJECT_SOURCE_DIR
#define MK_PROJECT_SOURCE_DIR "."
#endif

#define MK_SDL_MAP_MANIFEST_PATH "assets/mosul/manifests/market_commercial_streets_2003.mapmanifest"
#define MK_SDL_SPRITE_MANIFEST_PATH "assets/mosul/manifests/mosul_2003_sprites.spritemanifest"
#define MK_SDL_MARKER_MANIFEST_PATH "assets/mosul/manifests/mosul_2003_markers.markermanifest"
#define MK_SDL_TEXT_SCALE 2.0f

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

typedef struct {
    const char *project_root;
    const char *scenario_path;
    uint32_t smoke_frames;
    bool ai_only;
} mk_sdl_config_t;

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

static bool mk_sdl_make_project_path(
    const char *project_root,
    const char *relative_path,
    char *out_path,
    size_t capacity
) {
    const char *root = project_root == NULL || project_root[0] == '\0' ? "." : project_root;
    int written;

    if (relative_path == NULL || out_path == NULL || capacity == 0) {
        return false;
    }

    if (relative_path[0] == '/') {
        size_t length = strlen(relative_path);

        if (length >= capacity) {
            return false;
        }

        memcpy(out_path, relative_path, length + 1);
        return true;
    }

    written = snprintf(out_path, capacity, "%s/%s", root, relative_path);
    return written > 0 && (size_t)written < capacity;
}

static bool mk_sdl_parse_u32(const char *text, uint32_t *out_value) {
    char *end = NULL;
    unsigned long parsed;

    if (text == NULL || out_value == NULL || text[0] == '\0') {
        return false;
    }

    errno = 0;
    parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed > UINT32_MAX) {
        return false;
    }

    *out_value = (uint32_t)parsed;
    return true;
}

static void mk_sdl_print_usage(const char *program_name) {
    fprintf(
        stderr,
        "usage: %s [--project-root PATH] [--scenario PATH] [--ai-only] [--smoke-frames N]\n"
        "\n"
        "Runs the SDL3 MOSUL demo shell. --ai-only watches both tactical sides, and --smoke-frames exits after N rendered frames.\n",
        program_name
    );
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
        case MK_TERRAIN_BREACH_POINT:
            SDL_SetRenderDrawColor(renderer, 92, 112, 87, 190);
            break;
        case MK_TERRAIN_ROOFTOP:
            SDL_SetRenderDrawColor(renderer, 78, 104, 116, 175);
            break;
        case MK_TERRAIN_SUSPECTED_IED:
            SDL_SetRenderDrawColor(renderer, 116, 87, 62, 190);
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

static const char *mk_sdl_order_marker_id(mk_order_t order) {
    switch (order) {
        case MK_ORDER_INVESTIGATE:
            return "order_investigate";
        case MK_ORDER_OVERWATCH:
            return "order_overwatch";
        case MK_ORDER_BREACH:
            return "order_breach_search";
        case MK_ORDER_FIRE:
        case MK_ORDER_SUPPRESS:
            return "order_suppress";
        case MK_ORDER_WITHDRAW:
        case MK_ORDER_RALLY:
            return "order_withdraw";
        case MK_ORDER_MOVE:
        case MK_ORDER_ASSAULT_MOVE:
            return "move_target";
        case MK_ORDER_HOLD:
        case MK_ORDER_NONE:
        default:
            return "order_hold";
    }
}

static const char *mk_sdl_overlay_marker_id(const mk_tactical_overlay_t *overlay) {
    if (overlay == NULL) {
        return "selection_ring";
    }

    if (overlay->kind == MK_TACTICAL_OVERLAY_ORDER_STATUS) {
        return mk_sdl_order_marker_id(overlay->order);
    }

    switch (overlay->kind) {
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
        case MK_TACTICAL_OVERLAY_BREACH_SEARCH:
        case MK_TACTICAL_OVERLAY_SEARCH_CACHE:
            return "breach_search";
        case MK_TACTICAL_OVERLAY_ROOFTOP_ACCESS:
            return "rooftop_access";
        case MK_TACTICAL_OVERLAY_ORDER_STATUS:
            return mk_sdl_order_marker_id(overlay->order);
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

static void mk_sdl_set_side_status_color(SDL_Renderer *renderer, mk_side_t side, bool contested) {
    if (contested) {
        SDL_SetRenderDrawColor(renderer, 210, 156, 68, 235);
    } else if (side == MK_SIDE_PLAYER) {
        SDL_SetRenderDrawColor(renderer, 91, 143, 186, 235);
    } else if (side == MK_SIDE_OPFOR) {
        SDL_SetRenderDrawColor(renderer, 153, 83, 67, 235);
    } else if (side == MK_SIDE_CIVILIAN) {
        SDL_SetRenderDrawColor(renderer, 180, 166, 120, 235);
    } else {
        SDL_SetRenderDrawColor(renderer, 112, 122, 116, 235);
    }
}

static float mk_sdl_max_float(float first, float second) {
    return first > second ? first : second;
}

static float mk_sdl_clamp_float(float value, float minimum, float maximum) {
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

static int mk_sdl_total_civilian_risk(const mk_game_t *game) {
    int risk = 0;
    size_t index;

    if (game == NULL) {
        return 0;
    }

    for (index = 0; index < game->civilian_count; ++index) {
        risk += game->civilians[index].risk;
    }

    return risk;
}

static const mk_unit_t *mk_sdl_selected_unit(const mk_game_t *game) {
    size_t index;

    if (game == NULL || game->selected_unit_id == 0) {
        return NULL;
    }

    for (index = 0; index < game->unit_count; ++index) {
        if (game->units[index].id == game->selected_unit_id) {
            return &game->units[index];
        }
    }

    return NULL;
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
        "modernerKrieg Mosul Demo | score %d | %s %s | contested %u | risk %d",
        score.total_score,
        objective_label,
        objective_side,
        (unsigned)score.contested_objectives,
        score.civilian_risk
    );
    SDL_SetWindowTitle(window, title);
}

static void mk_render_hud_bar(
    SDL_Renderer *renderer,
    SDL_FRect frame,
    float fill_ratio,
    mk_color_t fill_color
) {
    SDL_FRect fill = frame;

    SDL_SetRenderDrawColor(renderer, 36, 42, 43, 235);
    SDL_RenderFillRect(renderer, &frame);
    SDL_SetRenderDrawColor(renderer, 96, 104, 101, 255);
    SDL_RenderRect(renderer, &frame);

    fill.w = mk_sdl_clamp_float(fill_ratio, 0.0f, 1.0f) * frame.w;
    if (fill.w > 0.0f) {
        SDL_SetRenderDrawColor(renderer, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
        SDL_RenderFillRect(renderer, &fill);
    }
}

static mk_color_t mk_sdl_outcome_color(mk_outcome_t outcome) {
    switch (outcome) {
        case MK_OUTCOME_PLAYER_SUCCESS:
            return mk_make_color(91, 143, 186, 245);
        case MK_OUTCOME_PLAYER_PARTIAL:
            return mk_make_color(210, 156, 68, 245);
        case MK_OUTCOME_PLAYER_FAILURE:
            return mk_make_color(153, 83, 67, 245);
        case MK_OUTCOME_IN_PROGRESS:
        default:
            return mk_make_color(154, 172, 142, 245);
    }
}

static char mk_sdl_upper_ascii(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        return (char)(ch - ('a' - 'A'));
    }

    return ch;
}

static const uint8_t *mk_sdl_glyph_rows(char ch) {
    static const uint8_t glyph_space[7] = {0, 0, 0, 0, 0, 0, 0};
    static const uint8_t glyph_a[7] = {14, 17, 17, 31, 17, 17, 17};
    static const uint8_t glyph_b[7] = {30, 17, 17, 30, 17, 17, 30};
    static const uint8_t glyph_c[7] = {15, 16, 16, 16, 16, 16, 15};
    static const uint8_t glyph_d[7] = {30, 17, 17, 17, 17, 17, 30};
    static const uint8_t glyph_e[7] = {31, 16, 16, 30, 16, 16, 31};
    static const uint8_t glyph_f[7] = {31, 16, 16, 30, 16, 16, 16};
    static const uint8_t glyph_g[7] = {15, 16, 16, 19, 17, 17, 15};
    static const uint8_t glyph_h[7] = {17, 17, 17, 31, 17, 17, 17};
    static const uint8_t glyph_i[7] = {31, 4, 4, 4, 4, 4, 31};
    static const uint8_t glyph_j[7] = {1, 1, 1, 1, 17, 17, 14};
    static const uint8_t glyph_k[7] = {17, 18, 20, 24, 20, 18, 17};
    static const uint8_t glyph_l[7] = {16, 16, 16, 16, 16, 16, 31};
    static const uint8_t glyph_m[7] = {17, 27, 21, 21, 17, 17, 17};
    static const uint8_t glyph_n[7] = {17, 25, 21, 19, 17, 17, 17};
    static const uint8_t glyph_o[7] = {14, 17, 17, 17, 17, 17, 14};
    static const uint8_t glyph_p[7] = {30, 17, 17, 30, 16, 16, 16};
    static const uint8_t glyph_q[7] = {14, 17, 17, 17, 21, 18, 13};
    static const uint8_t glyph_r[7] = {30, 17, 17, 30, 20, 18, 17};
    static const uint8_t glyph_s[7] = {15, 16, 16, 14, 1, 1, 30};
    static const uint8_t glyph_t[7] = {31, 4, 4, 4, 4, 4, 4};
    static const uint8_t glyph_u[7] = {17, 17, 17, 17, 17, 17, 14};
    static const uint8_t glyph_v[7] = {17, 17, 17, 17, 17, 10, 4};
    static const uint8_t glyph_w[7] = {17, 17, 17, 21, 21, 27, 17};
    static const uint8_t glyph_x[7] = {17, 10, 4, 4, 4, 10, 17};
    static const uint8_t glyph_y[7] = {17, 10, 4, 4, 4, 4, 4};
    static const uint8_t glyph_z[7] = {31, 1, 2, 4, 8, 16, 31};
    static const uint8_t glyph_0[7] = {14, 17, 19, 21, 25, 17, 14};
    static const uint8_t glyph_1[7] = {4, 12, 4, 4, 4, 4, 14};
    static const uint8_t glyph_2[7] = {14, 17, 1, 2, 4, 8, 31};
    static const uint8_t glyph_3[7] = {30, 1, 1, 14, 1, 1, 30};
    static const uint8_t glyph_4[7] = {2, 6, 10, 18, 31, 2, 2};
    static const uint8_t glyph_5[7] = {31, 16, 16, 30, 1, 1, 30};
    static const uint8_t glyph_6[7] = {14, 16, 16, 30, 17, 17, 14};
    static const uint8_t glyph_7[7] = {31, 1, 2, 4, 8, 8, 8};
    static const uint8_t glyph_8[7] = {14, 17, 17, 14, 17, 17, 14};
    static const uint8_t glyph_9[7] = {14, 17, 17, 15, 1, 1, 14};
    static const uint8_t glyph_period[7] = {0, 0, 0, 0, 0, 12, 12};
    static const uint8_t glyph_colon[7] = {0, 12, 12, 0, 12, 12, 0};
    static const uint8_t glyph_dash[7] = {0, 0, 0, 31, 0, 0, 0};
    static const uint8_t glyph_slash[7] = {1, 2, 2, 4, 8, 8, 16};
    static const uint8_t glyph_comma[7] = {0, 0, 0, 0, 0, 12, 8};
    static const uint8_t glyph_apostrophe[7] = {12, 12, 8, 0, 0, 0, 0};
    static const uint8_t glyph_paren_open[7] = {2, 4, 8, 8, 8, 4, 2};
    static const uint8_t glyph_paren_close[7] = {8, 4, 2, 2, 2, 4, 8};

    switch (mk_sdl_upper_ascii(ch)) {
        case 'A': return glyph_a;
        case 'B': return glyph_b;
        case 'C': return glyph_c;
        case 'D': return glyph_d;
        case 'E': return glyph_e;
        case 'F': return glyph_f;
        case 'G': return glyph_g;
        case 'H': return glyph_h;
        case 'I': return glyph_i;
        case 'J': return glyph_j;
        case 'K': return glyph_k;
        case 'L': return glyph_l;
        case 'M': return glyph_m;
        case 'N': return glyph_n;
        case 'O': return glyph_o;
        case 'P': return glyph_p;
        case 'Q': return glyph_q;
        case 'R': return glyph_r;
        case 'S': return glyph_s;
        case 'T': return glyph_t;
        case 'U': return glyph_u;
        case 'V': return glyph_v;
        case 'W': return glyph_w;
        case 'X': return glyph_x;
        case 'Y': return glyph_y;
        case 'Z': return glyph_z;
        case '0': return glyph_0;
        case '1': return glyph_1;
        case '2': return glyph_2;
        case '3': return glyph_3;
        case '4': return glyph_4;
        case '5': return glyph_5;
        case '6': return glyph_6;
        case '7': return glyph_7;
        case '8': return glyph_8;
        case '9': return glyph_9;
        case '.': return glyph_period;
        case ':': return glyph_colon;
        case '-': return glyph_dash;
        case '/': return glyph_slash;
        case ',': return glyph_comma;
        case '\'': return glyph_apostrophe;
        case '(': return glyph_paren_open;
        case ')': return glyph_paren_close;
        case ' ':
        default:
            return glyph_space;
    }
}

static void mk_sdl_render_glyph(
    SDL_Renderer *renderer,
    char ch,
    float x,
    float y,
    float scale,
    mk_color_t color
) {
    const uint8_t *rows = mk_sdl_glyph_rows(ch);
    int row;
    int column;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (row = 0; row < 7; ++row) {
        for (column = 0; column < 5; ++column) {
            if ((rows[row] & (uint8_t)(1U << (4 - column))) != 0U) {
                SDL_FRect pixel;

                pixel.x = x + (float)column * scale;
                pixel.y = y + (float)row * scale;
                pixel.w = scale;
                pixel.h = scale;
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
}

static void mk_sdl_render_wrapped_text(
    SDL_Renderer *renderer,
    const char *text,
    SDL_FRect bounds,
    float scale,
    mk_color_t color
) {
    const char *cursor = text != NULL ? text : "";
    float char_advance = 6.0f * scale;
    float line_advance = 9.0f * scale;
    float x = bounds.x;
    float y = bounds.y;
    uint32_t column = 0;
    uint32_t max_columns = bounds.w > char_advance ? (uint32_t)(bounds.w / char_advance) : 1U;

    while (*cursor != '\0' && y + 7.0f * scale <= bounds.y + bounds.h) {
        char ch = *cursor++;

        if (ch == '\n') {
            x = bounds.x;
            y += line_advance;
            column = 0;
            continue;
        }

        if (column >= max_columns) {
            x = bounds.x;
            y += line_advance;
            column = 0;
            if (ch == ' ') {
                continue;
            }
        }

        mk_sdl_render_glyph(renderer, ch, x, y, scale, color);
        x += char_advance;
        column += 1;
    }
}

static void mk_sdl_render_text_panel(
    SDL_Renderer *renderer,
    SDL_FRect panel,
    const char *title,
    const char *body
) {
    SDL_FRect title_bounds;
    SDL_FRect body_bounds;

    SDL_SetRenderDrawColor(renderer, 18, 22, 24, 218);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 86, 96, 92, 255);
    SDL_RenderRect(renderer, &panel);

    title_bounds.x = panel.x + 8.0f;
    title_bounds.y = panel.y + 8.0f;
    title_bounds.w = panel.w - 16.0f;
    title_bounds.h = 16.0f;
    body_bounds.x = panel.x + 8.0f;
    body_bounds.y = panel.y + 28.0f;
    body_bounds.w = panel.w - 16.0f;
    body_bounds.h = panel.h - 34.0f;

    mk_sdl_render_wrapped_text(renderer, title, title_bounds, MK_SDL_TEXT_SCALE, mk_make_color(216, 210, 145, 245));
    mk_sdl_render_wrapped_text(renderer, body, body_bounds, MK_SDL_TEXT_SCALE, mk_make_color(218, 224, 213, 235));
}

static void mk_render_briefing_and_aar(
    SDL_Renderer *renderer,
    const mk_game_t *game,
    const mk_board_view_t *view
) {
    mk_after_action_report_t report;
    SDL_FRect briefing_panel;
    SDL_FRect status_panel;
    char status_text[MK_AFTER_ACTION_SUMMARY_CAPACITY + MK_SCENARIO_TEXT_CAPACITY];

    if (renderer == NULL || game == NULL || view == NULL) {
        return;
    }

    briefing_panel.x = view->screen_rect_px.x + 12.0f;
    briefing_panel.y = view->screen_rect_px.y + 12.0f;
    briefing_panel.w = 330.0f;
    briefing_panel.h = 92.0f;
    mk_sdl_render_text_panel(
        renderer,
        briefing_panel,
        "MISSION",
        game->briefing[0] != '\0' ? game->briefing : "No briefing loaded."
    );

    memset(&report, 0, sizeof(report));
    if (mk_game_after_action_report(game, &report) == MK_OK) {
        if (report.score.outcome == MK_OUTCOME_PLAYER_SUCCESS || report.score.outcome == MK_OUTCOME_PLAYER_PARTIAL) {
            (void)snprintf(
                status_text,
                sizeof(status_text),
                "%s %s",
                report.summary,
                report.narrative
            );
            status_panel.x = view->screen_rect_px.x + 12.0f;
            status_panel.y = view->screen_rect_px.y + view->screen_rect_px.height - 104.0f;
            status_panel.w = 460.0f;
            status_panel.h = 92.0f;
            mk_sdl_render_text_panel(renderer, status_panel, "AFTER ACTION", status_text);
        } else {
            (void)snprintf(
                status_text,
                sizeof(status_text),
                "Score %d. Objectives %u. Contested %u. Civilian risk %d. Tick %u.",
                report.score.total_score,
                (unsigned)report.score.controlled_objectives,
                (unsigned)report.score.contested_objectives,
                report.score.civilian_risk,
                game->tick
            );
            status_panel.x = view->screen_rect_px.x + 12.0f;
            status_panel.y = view->screen_rect_px.y + view->screen_rect_px.height - 76.0f;
            status_panel.w = 420.0f;
            status_panel.h = 64.0f;
            mk_sdl_render_text_panel(renderer, status_panel, "CURRENT STATUS", status_text);
        }
    }
}

static void mk_render_order_glyph(SDL_Renderer *renderer, mk_vec2_t center, float radius, mk_order_t order) {
    SDL_FRect rect;

    rect.x = center.x - radius;
    rect.y = center.y - radius;
    rect.w = radius * 2.0f;
    rect.h = radius * 2.0f;

    if (order == MK_ORDER_INVESTIGATE) {
        SDL_RenderLine(renderer, center.x, rect.y, rect.x + rect.w, center.y);
        SDL_RenderLine(renderer, rect.x + rect.w, center.y, center.x, rect.y + rect.h);
        SDL_RenderLine(renderer, center.x, rect.y + rect.h, rect.x, center.y);
        SDL_RenderLine(renderer, rect.x, center.y, center.x, rect.y);
    } else if (order == MK_ORDER_OVERWATCH) {
        SDL_RenderLine(renderer, rect.x, rect.y + rect.h, center.x, rect.y);
        SDL_RenderLine(renderer, center.x, rect.y, rect.x + rect.w, rect.y + rect.h);
    } else if (order == MK_ORDER_WITHDRAW || order == MK_ORDER_RALLY) {
        SDL_RenderLine(renderer, rect.x + rect.w, rect.y, rect.x, center.y);
        SDL_RenderLine(renderer, rect.x, center.y, rect.x + rect.w, rect.y + rect.h);
    } else if (order == MK_ORDER_FIRE || order == MK_ORDER_SUPPRESS) {
        SDL_RenderLine(renderer, rect.x, center.y, rect.x + rect.w, center.y);
        SDL_RenderLine(renderer, center.x, rect.y, center.x, rect.y + rect.h);
    } else if (order == MK_ORDER_BREACH) {
        SDL_RenderRect(renderer, &rect);
        SDL_RenderLine(renderer, rect.x, center.y, rect.x + rect.w, center.y);
    } else {
        SDL_RenderRect(renderer, &rect);
    }
}

static void mk_render_status_hud(SDL_Renderer *renderer, const mk_game_t *game, const mk_board_view_t *view) {
    mk_score_t score;
    SDL_FRect panel;
    SDL_FRect score_bar;
    SDL_FRect risk_bar;
    SDL_FRect objective_rect;
    mk_color_t score_color;
    float score_ratio;
    float risk_ratio;
    size_t objective_index;
    int civilian_risk;
    const mk_unit_t *selected_unit;

    if (renderer == NULL || game == NULL || view == NULL) {
        return;
    }

    if (mk_game_score(game, &score) != MK_OK) {
        memset(&score, 0, sizeof(score));
    }

    civilian_risk = mk_sdl_total_civilian_risk(game);
    panel.x = view->screen_rect_px.x;
    panel.y = 12.0f;
    panel.w = view->screen_rect_px.width;
    panel.h = 24.0f;

    SDL_SetRenderDrawColor(renderer, 22, 27, 28, 230);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 76, 84, 81, 255);
    SDL_RenderRect(renderer, &panel);

    score_ratio = ((float)score.total_score + 100.0f) / 600.0f;
    score_color = mk_sdl_outcome_color(score.outcome);
    score_bar.x = panel.x + 10.0f;
    score_bar.y = panel.y + 8.0f;
    score_bar.w = 190.0f;
    score_bar.h = 8.0f;
    mk_render_hud_bar(renderer, score_bar, score_ratio, score_color);

    risk_ratio = (float)civilian_risk / 40.0f;
    risk_bar.x = score_bar.x + score_bar.w + 18.0f;
    risk_bar.y = score_bar.y;
    risk_bar.w = 120.0f;
    risk_bar.h = 8.0f;
    mk_render_hud_bar(renderer, risk_bar, risk_ratio, mk_make_color(206, 181, 109, 245));

    objective_rect.x = risk_bar.x + risk_bar.w + 18.0f;
    objective_rect.y = panel.y + 6.0f;
    objective_rect.w = 12.0f;
    objective_rect.h = 12.0f;
    for (objective_index = 0; objective_index < game->objective_count && objective_index < 16; ++objective_index) {
        const mk_objective_t *objective = &game->objectives[objective_index];
        bool contested = objective->controlling_side == MK_SIDE_NEUTRAL && score.contested_objectives > 0;

        mk_sdl_set_side_status_color(renderer, objective->controlling_side, contested);
        SDL_RenderFillRect(renderer, &objective_rect);
        SDL_SetRenderDrawColor(renderer, 18, 22, 24, 255);
        SDL_RenderRect(renderer, &objective_rect);
        objective_rect.x += 16.0f;
    }

    selected_unit = mk_sdl_selected_unit(game);
    if (selected_unit != NULL) {
        mk_vec2_t center;

        center.x = panel.x + panel.w - 22.0f;
        center.y = panel.y + panel.h * 0.5f;
        mk_sdl_set_side_status_color(renderer, selected_unit->side, false);
        mk_render_order_glyph(renderer, center, 6.0f, selected_unit->order);
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
    const char *marker_id = mk_sdl_overlay_marker_id(overlay);
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

    if (overlay->kind == MK_TACTICAL_OVERLAY_ORDER_STATUS) {
        mk_render_order_glyph(renderer, overlay->screen_position_px, radius_px, overlay->order);
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

    mk_render_status_hud(renderer, game, view);
    mk_render_briefing_and_aar(renderer, game, view);
    SDL_RenderPresent(renderer);
}

static SDL_Texture *mk_load_manifest_map_texture(SDL_Renderer *renderer, const char *project_root) {
    mk_asset_map_manifest_t manifest;
    char manifest_path[512];
    mk_result_t result;

    if (!mk_sdl_make_project_path(project_root, MK_SDL_MAP_MANIFEST_PATH, manifest_path, sizeof(manifest_path))) {
        SDL_Log("Map manifest path is too long.");
        return NULL;
    }

    result = mk_asset_load_map_manifest(manifest_path, project_root, &manifest);
    if (result != MK_OK) {
        SDL_Log("Map manifest unavailable: %s", mk_result_name(result));
        return NULL;
    }

#ifdef MK_HAS_SDL3_IMAGE
    {
        char texture_path[512];
        SDL_Texture *texture = NULL;

        if (mk_sdl_make_project_path(project_root, manifest.runtime_overview_path, texture_path, sizeof(texture_path))) {
            texture = IMG_LoadTexture(renderer, texture_path);
        }

        if (texture == NULL
            && mk_sdl_make_project_path(project_root, manifest.overview_path, texture_path, sizeof(texture_path))) {
            texture = IMG_LoadTexture(renderer, texture_path);
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

static void mk_sdl_load_sprite_assets(SDL_Renderer *renderer, const char *project_root, mk_sdl_sprite_assets_t *assets) {
    char manifest_path[512];
    mk_result_t result;
    size_t index;

    if (assets == NULL) {
        return;
    }

    memset(assets, 0, sizeof(*assets));
    if (!mk_sdl_make_project_path(project_root, MK_SDL_SPRITE_MANIFEST_PATH, manifest_path, sizeof(manifest_path))) {
        SDL_Log("Sprite manifest path is too long.");
        return;
    }

    result = mk_asset_load_sprite_manifest(manifest_path, project_root, &assets->manifest);
    if (result != MK_OK) {
        SDL_Log("Sprite manifest unavailable: %s", mk_result_name(result));
        return;
    }

    assets->manifest_loaded = true;
    assets->sheet_count = assets->manifest.sheet_count;

#ifdef MK_HAS_SDL3_IMAGE
    for (index = 0; index < assets->manifest.sheet_count; ++index) {
        const mk_asset_sprite_sheet_t *sheet = &assets->manifest.sheets[index];
        char sheet_path[512];

        mk_sdl_copy_name(assets->sheets[index].id, sizeof(assets->sheets[index].id), sheet->id);
        if (!mk_sdl_make_project_path(project_root, sheet->path, sheet_path, sizeof(sheet_path))) {
            SDL_Log("Sprite sheet path is too long: %s", sheet->path);
            assets->textures_loaded = false;
            return;
        }

        assets->sheets[index].texture = IMG_LoadTexture(renderer, sheet_path);
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

static void mk_sdl_load_marker_assets(const char *project_root, mk_sdl_marker_assets_t *assets) {
    char manifest_path[512];
    mk_result_t result;

    if (assets == NULL) {
        return;
    }

    memset(assets, 0, sizeof(*assets));
    if (!mk_sdl_make_project_path(project_root, MK_SDL_MARKER_MANIFEST_PATH, manifest_path, sizeof(manifest_path))) {
        SDL_Log("Marker manifest path is too long.");
        return;
    }

    result = mk_asset_load_marker_manifest(manifest_path, &assets->manifest);
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
    mk_sdl_config_t config;
    bool running = true;
    uint32_t rendered_frames = 0;
    int arg_index;
    mk_game_t game;
    mk_scenario_definition_t scenario;
    mk_board_view_t view;
    mk_result_t result;

    memset(&config, 0, sizeof(config));
    config.project_root = MK_PROJECT_SOURCE_DIR;

    for (arg_index = 1; arg_index < argc; ++arg_index) {
        const char *argument = argv[arg_index];

        if (strcmp(argument, "--help") == 0) {
            mk_sdl_print_usage(argv[0]);
            return 0;
        }

        if (strcmp(argument, "--project-root") == 0) {
            if (arg_index + 1 >= argc) {
                mk_sdl_print_usage(argv[0]);
                return 2;
            }

            config.project_root = argv[arg_index + 1];
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--scenario") == 0) {
            if (arg_index + 1 >= argc) {
                mk_sdl_print_usage(argv[0]);
                return 2;
            }

            config.scenario_path = argv[arg_index + 1];
            arg_index += 1;
            continue;
        }

        if (strcmp(argument, "--ai-only") == 0) {
            config.ai_only = true;
            continue;
        }

        if (strcmp(argument, "--smoke-frames") == 0) {
            if (arg_index + 1 >= argc || !mk_sdl_parse_u32(argv[arg_index + 1], &config.smoke_frames)) {
                mk_sdl_print_usage(argv[0]);
                return 2;
            }

            arg_index += 1;
            continue;
        }

        mk_sdl_print_usage(argv[0]);
        return 2;
    }

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

    {
        char scenario_path[512];
        const char *relative_scenario_path = config.scenario_path != NULL
            ? config.scenario_path
            : MK_MOSUL_DEFAULT_SCENARIO_PATH;

        if (!mk_sdl_make_project_path(config.project_root, relative_scenario_path, scenario_path, sizeof(scenario_path))) {
            SDL_Log("Scenario path is too long.");
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }

        result = mk_mosul_load_scenario_file(scenario_path, config.project_root, &scenario);
    }
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

    map_texture = mk_load_manifest_map_texture(renderer, config.project_root);
    mk_sdl_load_sprite_assets(renderer, config.project_root, &sprite_assets);
    mk_sdl_load_marker_assets(config.project_root, &marker_assets);

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
        if (config.ai_only) {
            (void)mk_ai_issue_basic_orders(&game);
        }
        (void)mk_game_run_fixed_steps(&game, 1, NULL, NULL);
        mk_sdl_update_window_title(window, &game);
        mk_render(renderer, &game, &view, map_texture, &sprite_assets, &marker_assets);
        rendered_frames += 1;
        if (config.smoke_frames > 0 && rendered_frames >= config.smoke_frames) {
            running = false;
        }
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
