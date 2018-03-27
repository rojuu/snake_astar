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

//#define arrayCount(arr) (sizeof(arr)/sizeof(*arr))
#define array_count(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#include "types.cpp"
#include "math.cpp"

internal const f32 PI = glm::pi<f32>();

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

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
        0);//SDL_WINDOW_OPENGL);

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED);

    if (!window || !renderer) {
        printf("SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    const int snake_cell_width = 124;
    const int snake_cell_height = 124;
    SDL_Rect rects[] {
        {
            SCREEN_WIDTH/2,
            SCREEN_HEIGHT/2,
            snake_cell_width,
            snake_cell_height
        }
    };

    const int rect_count = array_count(rects);

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
                    }
                } break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRects(renderer, rects, rect_count);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyWindow(window);
    return 0;
}
