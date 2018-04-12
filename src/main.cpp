#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include "SDL.h"

#define internal static

#define array_count(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef i8 b8;
typedef i16 b16;
typedef i32 b32;
typedef i64 b64;

struct v2 {
    i32 x, y;
};

struct Rendering {
    i32 screen_width;
    i32 screen_height;
    i32 cell_width;
    i32 cell_height;
    i32 fruit_radius;
    SDL_Rect* rects;
    b32 draw_snake;
    SDL_Texture* grid_texture;
    SDL_Texture* circle_texture;
};

struct Game {
    f64 start_frame_time;
    f64 min_frame_time;
    f64 speed_up_rate;
    i32 grid_size;
    f64 frame_time;
    i32 snake_cell_count;
    i32 max_cell_count;
    i32 input;
    v2* positions;
    v2* positions_last_frame;
    v2 fruit_pos;
    i32 direction;
    b32 collided;
    u32 flash_count;
    u32 flash_counter;
};

internal i32
distance(v2 a, v2 b) {
    i32 diff_x = b.x - a.x;
    i32 diff_y = b.y - a.y;
    i32 result = sqrt(diff_x*diff_x + diff_y*diff_y);
    return result;
}

inline i32
max(i32 a, i32 b) {
    i32 result = a > b ? a : b;
    return result;
}

inline f64
max(f64 a, f64 b) {
    f64 result = a > b ? a : b;
    return result;
}

inline i32
min(i32 a, i32 b) {
    i32 result = a < b ? a : b;
    return result;
}

inline f64
min(f64 a, f64 b) {
    f64 result = a < b ? a : b;
    return result;
}

enum INPUT_E {
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
};

enum DIRECTION_E {
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
};

internal void
render_grid(SDL_Renderer *renderer, Rendering& rendering) {
    const i32 k = 64;
    SDL_SetRenderDrawColor(renderer, k, k, k, 128);
    for(i32 i = rendering.cell_width; i < rendering.screen_width; i+=rendering.cell_width) {
        SDL_RenderDrawLine(renderer, i, 0, i, rendering.screen_height);
        SDL_RenderDrawLine(renderer, i-1, 0, i-1, rendering.screen_height);
    }
    for(i32 i = rendering.cell_width; i < rendering.screen_height; i+=rendering.cell_width) {
        SDL_RenderDrawLine(renderer, 0, i, rendering.screen_width, i);
        SDL_RenderDrawLine(renderer, 0, i-1, rendering.screen_width, i-1);
    }
}


internal void
render_circle(SDL_Renderer *renderer, i32 px, i32 py, i32 radius) {
    v2 cur_pos;
    v2 center = {px + radius, py + radius};

    i32 count = (radius*2) * (radius*2);
    i32 point_count = 0;
    SDL_Point* point_buffer = (SDL_Point*)malloc(sizeof(SDL_Point) * count * count);

    for(i32 x = 0; x < count; x++) {
        for(i32 y = 0; y < count; y++) {
            cur_pos.x = x + px;
            cur_pos.y = y + py;
            i32 dist = distance(cur_pos, center);
            if(dist <= radius) {
                auto& point = point_buffer[point_count];
                point.x = cur_pos.x;
                point.y = cur_pos.y;
                ++point_count;
            }
        }
    }

    SDL_RenderDrawPoints(renderer, point_buffer, point_count);

    free(point_buffer);
}

internal void
randomize_fruit_pos(Game& game) {
    b32 success = false;
    v2 new_pos;
    while(!success) {
        success=true;
        new_pos.x = rand() % game.grid_size;
        new_pos.y = rand() % game.grid_size;
        for(i32 i = 0; i < game.snake_cell_count; i++) {
            if(new_pos.x == game.positions[i].x && new_pos.y == game.positions[i].y) {
                success=false;
            }
        }
    }
    game.fruit_pos = new_pos;
}

internal void
reset_state(Game& game, Rendering& rendering) {
    game.collided = false;

    auto& snake_pos = game.positions[0];
    snake_pos.x = rand() % game.grid_size;
    snake_pos.y = rand() % game.grid_size;

    randomize_fruit_pos(game);

    for(i32 i = 0; i < game.max_cell_count; i++) {
        auto& rect = rendering.rects[i];
        auto& pos = game.positions[i];

        rect.w = rendering.cell_width;
        rect.h = rendering.cell_height;
        pos.x = snake_pos.x;// snake_pos.x;
        pos.y = snake_pos.y;// snake_pos.y;

        game.positions_last_frame[i] = pos;
    }

    game.input = -1;
    game.snake_cell_count = 3;
    game.flash_counter = 0;
    rendering.draw_snake = true;
    game.frame_time = game.start_frame_time;
}

internal void
reset_routine(Game& game, Rendering& rendering) {
    game.frame_time = game.start_frame_time;
    if(game.flash_counter < game.flash_count) {
        if(game.flash_counter % 2 == 0) {
            rendering.draw_snake = false;
        } else {
            rendering.draw_snake = true;
        }

        ++game.flash_counter;
    } else {
        reset_state(game, rendering);
    }
}

internal void
game_loop(Game& game) {
    auto& snake_pos = game.positions[0];

    switch(game.input) {
        case INPUT_UP:
            if(game.direction == DIRECTION_RIGHT || game.direction == DIRECTION_LEFT) {
                game.direction = DIRECTION_UP;
            }
        break;
        case INPUT_DOWN:
            if(game.direction == DIRECTION_RIGHT || game.direction == DIRECTION_LEFT) {
                game.direction = DIRECTION_DOWN;
            }
        break;
        case INPUT_RIGHT:
            if(game.direction == DIRECTION_UP || game.direction == DIRECTION_DOWN) {
                game.direction = DIRECTION_RIGHT;
            }
        break;
        case INPUT_LEFT:
            if(game.direction == DIRECTION_UP || game.direction == DIRECTION_DOWN) {
                game.direction = DIRECTION_LEFT;
            }
        break;
    }

    switch(game.direction) {
        case DIRECTION_UP:
            ++snake_pos.y;
        break;
        case DIRECTION_DOWN:
            --snake_pos.y;
        break;
        case DIRECTION_LEFT:
            --snake_pos.x;
        break;
        case DIRECTION_RIGHT:
            ++snake_pos.x;
        break;
    }

    for(i32 i = 1; i < game.snake_cell_count; i++) {
        if(snake_pos.x == game.positions[i].x && snake_pos.y == game.positions[i].y) {
            game.collided = true;
        }
    }

    b32 edge_collision = false;
    if(game.direction == DIRECTION_RIGHT && snake_pos.x >= game.grid_size) {
        edge_collision = true;
        --snake_pos.x;
    }
    if (game.direction == DIRECTION_LEFT && snake_pos.x < 0) {
        edge_collision = true;
        ++snake_pos.x;
    }
    if (game.direction == DIRECTION_UP && snake_pos.y >= game.grid_size) {
        edge_collision = true;
        --snake_pos.y;
    }
    if (game.direction == DIRECTION_DOWN && snake_pos.y < 0) {
        edge_collision = true;
        ++snake_pos.y;
    }

    if(edge_collision) {
        game.collided = true;
        return;
    }

    if(snake_pos.x == game.fruit_pos.x && snake_pos.y == game.fruit_pos.y) {
        game.frame_time = max(game.min_frame_time, game.frame_time * game.speed_up_rate);
        game.snake_cell_count = min(++game.snake_cell_count, game.max_cell_count);
        randomize_fruit_pos(game);
    }

    for(i32 i = 1; i < game.snake_cell_count; i++) {
        game.positions[i].x = game.positions_last_frame[i-1].x;
        game.positions[i].y = game.positions_last_frame[i-1].y;
    }


    for(i32 i = 0; i < game.snake_cell_count; i++) {
        game.positions_last_frame[i].x = game.positions[i].x;
        game.positions_last_frame[i].y = game.positions[i].y;
    }
}

void
render_loop(SDL_Renderer* renderer, Rendering& rendering, Game& game) {
    for(i32 i = 0; i < game.snake_cell_count; i++) {
        auto& rect = rendering.rects[i];
        auto& position = game.positions[i];
        rect.x = position.x * rendering.cell_width;
        rect.y = (game.grid_size - position.y - 1) * rendering.cell_height;
    }

    //TODO: Render with OpenGL to get Vsync, so we don't hog all the cpu resources?
    // We could still use SDL rendering functions for the actual drawing and just send the texture to OpenGL

    SDL_RenderCopy(renderer, rendering.grid_texture, 0, 0);

    {
        const i32 k = 255;
        SDL_SetRenderDrawColor(renderer, k, k, k, 255);
    }

    //Draw fruit
    {
        SDL_Rect circle_rect;
        circle_rect.w = rendering.fruit_radius*2;
        circle_rect.h = rendering.fruit_radius*2;
        circle_rect.x = (game.fruit_pos.x * rendering.cell_width)+3;
        circle_rect.y = ((game.grid_size-game.fruit_pos.y-1) * rendering.cell_height)+3;
        SDL_RenderCopy(renderer, rendering.circle_texture, 0, &circle_rect);
    }

    if (rendering.draw_snake) {
        SDL_RenderFillRects(renderer, rendering.rects, game.snake_cell_count);
    }

    SDL_RenderPresent(renderer);
}

void
init_renderer(SDL_Renderer* renderer, Rendering& rendering) {
    SDL_Surface* grid_surface = SDL_CreateRGBSurface(0, rendering.screen_width, rendering.screen_height, 32, 0, 0, 0, 0);
    SDL_Renderer* grid_renderer = SDL_CreateSoftwareRenderer(grid_surface);
    render_grid(grid_renderer, rendering);
    rendering.grid_texture = SDL_CreateTextureFromSurface(renderer, grid_surface);
    SDL_DestroyRenderer(grid_renderer);
    SDL_FreeSurface(grid_surface);

    SDL_Surface* circle_surface =
        SDL_CreateRGBSurface(0, rendering.fruit_radius*2+1, rendering.fruit_radius*2+1, 32, 0, 0, 0, 0);
    SDL_Renderer* circle_renderer = SDL_CreateSoftwareRenderer(circle_surface);
    SDL_SetRenderDrawColor(circle_renderer, 0, 0, 0, 0);
    SDL_RenderClear(circle_renderer);
    SDL_SetRenderDrawColor(circle_renderer, 255, 255, 255, 255);
    render_circle(circle_renderer, 0, 0, rendering.fruit_radius);
    rendering.circle_texture = SDL_CreateTextureFromSurface(renderer, circle_surface);
    SDL_DestroyRenderer(circle_renderer);
    SDL_FreeSurface(circle_surface);
}

i32
main(i32 argc, char **argv) {
    // Init SDL stuff
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Error: %s\n", SDL_GetError());
        return -1;
    }
    atexit(SDL_Quit);

    Game game;
    game.start_frame_time = 0.2;
    game.min_frame_time = 0.00015;
    game.speed_up_rate = 0.95;
    game.grid_size = 8;
    game.frame_time = 0;
    game.snake_cell_count = 0;
    game.max_cell_count = game.grid_size * game.grid_size;
    game.input = 0;
    game.positions = (v2*)malloc(sizeof(v2) * game.max_cell_count);
    game.positions_last_frame = (v2*)malloc(sizeof(v2) * game.max_cell_count);
    game.direction = DIRECTION_RIGHT;
    game.collided = false;
    game.flash_count   = 5;
    game.flash_counter = 0;
    game.fruit_pos = { 0 };

    Rendering rendering;
    rendering.screen_width = 256;
    rendering.screen_height = 256;
    rendering.cell_width = rendering.screen_width / game.grid_size;
    rendering.cell_height = rendering.screen_height / game.grid_size;
    rendering.fruit_radius = (rendering.cell_width/2) -3;
    rendering.rects = (SDL_Rect*)malloc(sizeof(SDL_Rect) * game.max_cell_count);
    rendering.draw_snake = true;


    SDL_Window *window = SDL_CreateWindow(
        "Snake A*",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        rendering.screen_width, rendering.screen_height,
        0);

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED);

    if (!window || !renderer) {
        printf("SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    init_renderer(renderer, rendering);

    srand(time(0));
    reset_state(game, rendering);

    b32 running = true;
    f64 current_time = (f32)SDL_GetPerformanceCounter() /
                       (f32)SDL_GetPerformanceFrequency();
    f64 last_time = 0;
    f64 delta_time = 0;
    i32 frame_counter = 0;
    i32 last_frame_count = 0;
    f64 last_fps_time = 0;
    f64 last_update_time = 0;
    while (running) {
        last_time = current_time;
        current_time = (f64)SDL_GetPerformanceCounter() /
                       (f64)SDL_GetPerformanceFrequency();
        delta_time = (f64)(current_time - last_time);

        // Count frames for every second and print it as the title of the window
        ++frame_counter;
        if (current_time >= (last_fps_time + 1.f)) {
            last_fps_time = current_time;
            i32 deltaFrames = frame_counter - last_frame_count;
            last_frame_count = frame_counter;
            char title[64];
            sprintf(title, "FPS: %d", deltaFrames);
            SDL_SetWindowTitle(window, title);
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    running = false;
                }
                break;

                case SDL_KEYDOWN: {
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE: {
                            running = false;
                        }
                        break;

                        case SDLK_UP: {
                            game.input = INPUT_UP;
                        }
                        break;

                        case SDLK_DOWN: {
                            game.input = INPUT_DOWN;
                        }
                        break;

                        case SDLK_LEFT: {
                            game.input = INPUT_LEFT;
                        }
                        break;

                        case SDLK_RIGHT: {
                            game.input = INPUT_RIGHT;
                        }
                        break;
                    }
                } break;
            }
        }

        if(current_time >= (last_update_time + game.frame_time)) {
            last_update_time = current_time;
            if(!game.collided) {
                game_loop(game);
            } else {
                reset_routine(game, rendering);
            }
        }

        render_loop(renderer, rendering, game);
    }

    SDL_DestroyWindow(window);
    return 0;
}
