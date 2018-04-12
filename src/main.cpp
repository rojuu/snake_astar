#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include "SDL.h"

#define internal static
#define global_variable static

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

global_variable const i32 SCREEN_WIDTH = 256;
global_variable const i32 SCREEN_HEIGHT = 256;

typedef struct {
    i32 x, y;
} 
v2;

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

global_variable const i32 grid_size = 8;

global_variable const i32 cell_width = SCREEN_WIDTH / grid_size;
global_variable const i32 cell_height = SCREEN_HEIGHT / grid_size;

global_variable const i32 max_cell_count = grid_size * grid_size;

global_variable const f64 start_frame_time = 0.2;
global_variable const f64 min_frame_time = 0.00015;
global_variable const f64 speed_up_rate = 0.95;

global_variable f64 frame_time;

global_variable i32 snake_cell_count;
global_variable i32 input;
global_variable v2 positions[max_cell_count];
global_variable v2 positions_last_frame[max_cell_count];
global_variable v2& snake_pos = positions[0];
global_variable SDL_Rect rects[max_cell_count];
global_variable i32 direction = DIRECTION_RIGHT;

internal void
render_grid(SDL_Renderer *renderer) {
    const i32 k = 64;
    SDL_SetRenderDrawColor(renderer, k, k, k, 128);
    for(i32 i = cell_width; i < SCREEN_WIDTH; i+=cell_width) {
        SDL_RenderDrawLine(renderer, i, 0, i, SCREEN_HEIGHT);
        SDL_RenderDrawLine(renderer, i-1, 0, i-1, SCREEN_HEIGHT);
    }
    for(i32 i = cell_width; i < SCREEN_HEIGHT; i+=cell_width) {
        SDL_RenderDrawLine(renderer, 0, i, SCREEN_WIDTH, i);
        SDL_RenderDrawLine(renderer, 0, i-1, SCREEN_WIDTH, i-1);
    }
}

global_variable i32 fruit_radius = (cell_width/2) -3;
global_variable v2 fruit_pos;

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

global_variable b32 increase_snake_cell_count = false;
global_variable b32 decrease_snake_cell_count = false;

global_variable b32 collided = false;

global_variable const u32 flash_count   = 5;
global_variable       u32 flash_counter = 0;

global_variable b32 draw_snake = true;

internal void
randomize_fruit_pos() {
    b32 success = false;
    v2 new_pos;
    while(!success) {
        success=true;
        new_pos.x = rand() % grid_size;
        new_pos.y = rand() % grid_size;
        for(i32 i = 0; i < snake_cell_count; i++) {
            if(new_pos.x == positions[i].x && new_pos.y == positions[i].y) {
                success=false;
            }
        }
    }
    fruit_pos = new_pos;
}

internal void
reset_state() {
    collided = false;

    snake_pos.x = rand() % grid_size;
    snake_pos.y = rand() % grid_size;

    randomize_fruit_pos();

    for(i32 i = 0; i < max_cell_count; i++) {
        auto& rect = rects[i];
        auto& pos = positions[i];

        rect.w = cell_width;
        rect.h = cell_height;
        pos.x = snake_pos.x;// snake_pos.x;
        pos.y = snake_pos.y;// snake_pos.y;

        positions_last_frame[i] = pos;
    }

    input = -1;
    snake_cell_count = 3;
    flash_counter = 0;
    draw_snake = true;
    frame_time = start_frame_time;
}

internal void
reset_routine() {
    frame_time = start_frame_time;
    if(flash_counter < flash_count) {
        if(flash_counter % 2 == 0) {
            draw_snake = false;
        } else {
            draw_snake = true;
        }

        ++flash_counter;
    } else {
        reset_state();
    }
}

internal void
game_loop() {
    switch(input) {
        case INPUT_UP:
            if(direction == DIRECTION_RIGHT || direction == DIRECTION_LEFT) {
                direction = DIRECTION_UP;
            }
        break;
        case INPUT_DOWN:
            if(direction == DIRECTION_RIGHT || direction == DIRECTION_LEFT) {
                direction = DIRECTION_DOWN;
            }
        break;
        case INPUT_RIGHT:
            if(direction == DIRECTION_UP || direction == DIRECTION_DOWN) {
                direction = DIRECTION_RIGHT;
            }
        break;
        case INPUT_LEFT:
            if(direction == DIRECTION_UP || direction == DIRECTION_DOWN) {
                direction = DIRECTION_LEFT;
            }
        break;
    }

    switch(direction) {
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

    for(i32 i = 1; i < snake_cell_count; i++) {
        if(snake_pos.x == positions[i].x && snake_pos.y == positions[i].y) {
            collided = true;
        }
    }

    b32 edge_collision = false;
    if(direction == DIRECTION_RIGHT && snake_pos.x >= grid_size) {
        edge_collision = true;
        --snake_pos.x;
    }
    if (direction == DIRECTION_LEFT && snake_pos.x < 0) {
        edge_collision = true;
        ++snake_pos.x;
    }
    if (direction == DIRECTION_UP && snake_pos.y >= grid_size) {
        edge_collision = true;
        --snake_pos.y;
    }
    if (direction == DIRECTION_DOWN && snake_pos.y < 0) {
        edge_collision = true;
        ++snake_pos.y;
    }

    if(edge_collision) {
        collided = true;
        return;
    }

    if(snake_pos.x == fruit_pos.x && snake_pos.y == fruit_pos.y) {
        frame_time = max(min_frame_time, frame_time * speed_up_rate);
        snake_cell_count = min(++snake_cell_count, max_cell_count);
        randomize_fruit_pos();
    }

    for(i32 i = 1; i < snake_cell_count; i++) {
        positions[i].x = positions_last_frame[i-1].x;
        positions[i].y = positions_last_frame[i-1].y;
    }


    for(i32 i = 0; i < snake_cell_count; i++) {
        positions_last_frame[i].x = positions[i].x;
        positions_last_frame[i].y = positions[i].y;
    }
}

global_variable SDL_Texture* grid_texture;
global_variable SDL_Texture* circle_texture;

void
render_loop(SDL_Renderer* renderer) {
    for(i32 i = 0; i < snake_cell_count; i++) {
        auto& rect = rects[i];
        auto& position = positions[i];
        rect.x = position.x * cell_width;
        rect.y = (grid_size - position.y - 1) * cell_height;
    }

    //TODO: Render with OpenGL to get Vsync, so we don't hog all the cpu resources?
    // We could still use SDL rendering functions for the actual drawing and just send the texture to OpenGL

    SDL_RenderCopy(renderer, grid_texture, 0, 0);

    {
        const i32 k = 255;
        SDL_SetRenderDrawColor(renderer, k, k, k, 255);
    }

    //Draw fruit
    {
        SDL_Rect circle_rect;
        circle_rect.w = fruit_radius*2;
        circle_rect.h = fruit_radius*2;
        circle_rect.x = (fruit_pos.x * cell_width)+3;
        circle_rect.y = ((grid_size-fruit_pos.y-1) * cell_height)+3;
        SDL_RenderCopy(renderer, circle_texture, 0, &circle_rect);
    }

    if (draw_snake) {
        SDL_RenderFillRects(renderer, rects, snake_cell_count);
    }

    SDL_RenderPresent(renderer);
}

void
init_renderer(SDL_Renderer* renderer) {
    SDL_Surface* grid_surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    SDL_Renderer* grid_renderer = SDL_CreateSoftwareRenderer(grid_surface);
    render_grid(grid_renderer);
    grid_texture = SDL_CreateTextureFromSurface(renderer, grid_surface);
    SDL_DestroyRenderer(grid_renderer);
    SDL_FreeSurface(grid_surface);

    SDL_Surface* circle_surface = SDL_CreateRGBSurface(0, fruit_radius*2+1, fruit_radius*2+1, 32, 0, 0, 0, 0);
    SDL_Renderer* circle_renderer = SDL_CreateSoftwareRenderer(circle_surface);
    SDL_SetRenderDrawColor(circle_renderer, 0, 0, 0, 0);
    SDL_RenderClear(circle_renderer);
    SDL_SetRenderDrawColor(circle_renderer, 255, 255, 255, 255);
    render_circle(circle_renderer, 0, 0, fruit_radius);
    circle_texture = SDL_CreateTextureFromSurface(renderer, circle_surface);
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

    init_renderer(renderer);

    srand(time(0));
    reset_state();

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
                            input = INPUT_UP;
                        }
                        break;

                        case SDLK_DOWN: {
                            input = INPUT_DOWN;
                        }
                        break;

                        case SDLK_LEFT: {
                            input = INPUT_LEFT;
                        }
                        break;

                        case SDLK_RIGHT: {
                            input = INPUT_RIGHT;
                        }
                        break;
                    }
                } break;
            }
        }

        if(current_time >= (last_update_time + frame_time)) {
            last_update_time = current_time;
            if(!collided) {
                game_loop();
                decrease_snake_cell_count = increase_snake_cell_count = false;
            } else {
                reset_routine();
            }
        }

        render_loop(renderer);
    }

    SDL_DestroyWindow(window);
    return 0;
}
