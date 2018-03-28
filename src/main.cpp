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

    SDL_Rect rects[max_cell_count];
    SDL_Rect grid_rects[max_cell_count];
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

    //Init grid
    for(i32 y = 0; y < grid_size; y++) {
        for(i32 x = 0; x < grid_size; x++) {
            SDL_Rect* rect = &grid_rects[y * grid_size + x];
            rect->x = x * cell_width;
            rect->y = y * cell_height;
            rect->w = cell_width;
            rect->h = cell_height;
        }
    }

    i32 snake_cell_count = 1;

    b32 running = true;
    f64 current_time = (f32)SDL_GetPerformanceCounter() /
                       (f32)SDL_GetPerformanceFrequency();
    f64 last_time = 0;
    f64 delta_time = 0;
    i32 frame_counter = 0;
    i32 last_frame_count = 0;
    f64 last_fps_time = 0;
    while (running) {
        last_time = current_time;
        current_time = (f64)SDL_GetPerformanceCounter() /
                       (f64)SDL_GetPerformanceFrequency();
        delta_time = (f64)(current_time - last_time);

        // Count frames for every second and pri32 it as the title of the window
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
                    }
                } break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        {
        const i32 k = 64;
        SDL_SetRenderDrawColor(renderer, k, k, k, 128);
        SDL_RenderDrawRects(renderer, grid_rects, max_cell_count);
        }

        {
        const i32 k = 255;
        SDL_SetRenderDrawColor(renderer, k, k, k, 255);
        SDL_RenderFillRects(renderer, rects, snake_cell_count);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyWindow(window);
    return 0;
}
