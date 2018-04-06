#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "SDL.h"
#include "glm/glm.hpp"
#include "glm/vec3.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <cstdint>
#include <fstream>
#include <string>

/** TODO: 
 * 
 * Should we actually use windows.h rather than SDL?
 * If we'd do that, we could use some useful windows API calls.
 * Maybe still use SDL, but only use those calls for useful stuff?
 * How would I go about portability in a situation like that?
 *
**/

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

typedef glm::vec2 v2;
typedef glm::vec3 v3;
typedef glm::vec4 v4;

typedef glm::mat2 m2;
typedef glm::mat3 m3;
typedef glm::mat4 m4;

internal const f32 PI = glm::pi<f32>();

const i32 SCREEN_WIDTH = 512;
const i32 SCREEN_HEIGHT = 512;

struct Position {
    i32 x, y;
};

struct Grid {
    i32 grid_size, max_cell_count;
    i32 cell_width, cell_height;
};

enum INPUT_ENUM_FLAGS {
    INPUT_UP    = 0x0001,
    INPUT_DOWN  = 0x0002,
    INPUT_LEFT  = 0x0004,
    INPUT_RIGHT = 0x0008,
};

struct Game_Data {
    u32 input_flags;
    Position* positions;
    i32 snake_cell_count;
};

internal void
render_grid(SDL_Renderer *renderer, const Grid &grid) {
    const i32 k = 64;
    SDL_SetRenderDrawColor(renderer, k, k, k, 128);
    for(i32 i = grid.cell_width; i < SCREEN_WIDTH; i+=grid.cell_width) {
        SDL_RenderDrawLine(renderer, i, 0, i, SCREEN_HEIGHT);
        SDL_RenderDrawLine(renderer, i-1, 0, i-1, SCREEN_HEIGHT);
    }
    for(i32 i = grid.cell_width; i < SCREEN_HEIGHT; i+=grid.cell_width) {
        SDL_RenderDrawLine(renderer, 0, i, SCREEN_WIDTH, i);
        SDL_RenderDrawLine(renderer, 0, i-1, SCREEN_WIDTH, i-1);
    }
}

internal void
game_loop(Game_Data &data, const Grid &grid) {
    data.positions[0].x = (++data.positions[0].x) % grid.grid_size;
    data.input_flags = 0;
}

i32
main(i32 argc, char **argv) {
    // Init SDL stuff
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Error: %s\n", SDL_GetError());
        return -1;
    }
    atexit(SDL_Quit);

    SDL_Window *window = SDL_CreateWindow(
        "Snake A*",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        0);

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED);

    if (!window || !renderer) {
        printf("SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    const i32 grid_size = 32;

    const i32 cell_width = SCREEN_WIDTH / grid_size;
    const i32 cell_height = SCREEN_HEIGHT / grid_size;

    const i32 max_cell_count = grid_size * grid_size;

    Grid grid;
    grid.grid_size = grid_size;
    grid.cell_width = cell_width;
    grid.cell_height = cell_height;
    grid.max_cell_count = max_cell_count;

    SDL_Texture* grid_texture;
    {
    SDL_Surface* grid_surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    SDL_Renderer* grid_renderer = SDL_CreateSoftwareRenderer(grid_surface);
    render_grid(grid_renderer, grid);
    grid_texture = SDL_CreateTextureFromSurface(renderer, grid_surface);
    SDL_DestroyRenderer(grid_renderer);
    SDL_FreeSurface(grid_surface);
    }

    SDL_Rect rects[max_cell_count];
    Position positions[max_cell_count];

    //Init cells
    for(i32 i = 0; i < max_cell_count; i++) {
        SDL_Rect *rect = &rects[i];
        Position *pos = &positions[i];
        rect->w = cell_width;
        rect->h = cell_height;
        rect->x = pos->x = 0;
        rect->y = pos->y = 0;
    }

    Game_Data game_data;
    game_data.positions = positions;
    game_data.input_flags = 0;
    game_data.snake_cell_count = 1;

    b32 running = true;
    f64 frame_time = 1.f;
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
                            game_data.input_flags = game_data.input_flags | INPUT_UP;
                        }
                        break;

                        case SDLK_DOWN: {
                            game_data.input_flags = game_data.input_flags | INPUT_DOWN;
                        }
                        break;

                        case SDLK_LEFT: {
                            game_data.input_flags = game_data.input_flags | INPUT_LEFT;
                        }
                        break;

                        case SDLK_RIGHT: {
                            game_data.input_flags = game_data.input_flags | INPUT_RIGHT;
                        }
                        break;
                    }
                } break;
            }
        }

        //Update game logic
        if(current_time >= (last_update_time + frame_time)) {
            last_update_time = current_time;
            game_loop(game_data, grid);
        }

        //Update positions for rendering
        for(i32 i = 0; i < game_data.snake_cell_count; i++) {
            auto rect = &rects[i];
            auto position = &positions[i];
            rect->x = positions->x * cell_width;
            rect->y = positions->y * cell_height;
        }


        //Rendering
        //TODO: Render with OpenGL to get Vsync, so we don't hog all the cpu resources?
        // We could still use SDL rendering functions for the actual drawing and just send the texture to OpenGL
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, grid_texture, 0, 0);

        {
        const i32 k = 255;
        SDL_SetRenderDrawColor(renderer, k, k, k, 255);
        SDL_RenderFillRects(renderer, rects, game_data.snake_cell_count);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyWindow(window);
    return 0;
}
