#include <stdlib.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include "SDL.h"

#include <assert.h>

#if 1
#include <windows.h>
#else
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

/** TODO:
  -Don't hog all the resources and run at 9001 FPS all the time.
*/

#include <stdint.h>
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

struct Vec2 {
    i32 x, y;
};

struct AstarScore {
    i32 G;
    i32 H;
};

struct AstarCell {
    Vec2 position;
    AstarScore score;
    AstarCell* previous;
};

inline Vec2
operator+(const Vec2& lhs, const Vec2& rhs) {
    return { lhs.x + rhs.x, lhs.y + lhs.y };
}

inline Vec2&
operator+=(Vec2& lhs, const Vec2& rhs) {
    Vec2 result;
    result.x = lhs.x + rhs.x;
    result.y = lhs.y + rhs.y;
    lhs = result;
    return lhs;
}

inline bool
operator==(const Vec2& lhs, const Vec2& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool
operator!=(const Vec2& lhs, const Vec2& rhs) {
    return !(lhs == rhs);
}

template<typename T> inline b32
contains(std::vector<T>* vec, T val) {
    return std::find(vec->begin(), vec->end(), val) != vec->end();
}

//TODO: Badly named? Maybe this returning the found_cell is bad?
static b32
contains_position(std::vector<AstarCell*> *vec, Vec2 pos, AstarCell** found_cell_ptr = 0) {
    b32 found = false;
    for(int i = 0; i < vec->size(); i++) {
        auto* cell = vec->at(i);
        if(cell->position == pos) {
            found = true;
            if(found_cell_ptr) {
                *found_cell_ptr = cell;
            }
            break;
        }
    }
    return found;
}

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
    Vec2* positions;
    Vec2* positions_last_frame;
    Vec2 fruit_pos;
    i32 direction;
    b32 collided;
    u32 flash_count;
    u32 flash_counter;
};

static f32
distance(Vec2 a, Vec2 b) {
    f32 diff_x = b.x - a.x;
    f32 diff_y = b.y - a.y;
    f32 result = sqrt(diff_x*diff_x + diff_y*diff_y);
    return result;
}

enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
};

static void
render_grid(SDL_Renderer *renderer, Rendering* rendering) {
    const i32 k = 64;
    SDL_SetRenderDrawColor(renderer, k, k, k, 128);
    for(i32 i = rendering->cell_width; i < rendering->screen_width; i+=rendering->cell_width) {
        SDL_RenderDrawLine(renderer, i, 0, i, rendering->screen_height);
        SDL_RenderDrawLine(renderer, i-1, 0, i-1, rendering->screen_height);
    }
    for(i32 i = rendering->cell_width; i < rendering->screen_height; i+=rendering->cell_width) {
        SDL_RenderDrawLine(renderer, 0, i, rendering->screen_width, i);
        SDL_RenderDrawLine(renderer, 0, i-1, rendering->screen_width, i-1);
    }
}

static void
render_circle(SDL_Renderer *renderer, i32 px, i32 py, i32 radius) {
    Vec2 cur_pos;
    Vec2 center = {px + radius, py + radius};

    i32 count = (radius*2) * (radius*2);
    i32 point_count = 0;
    SDL_Point* point_buffer = (SDL_Point*)malloc(sizeof(SDL_Point) * count * count);

    for(i32 x = 0; x < count; x++) {
        for(i32 y = 0; y < count; y++) {
            cur_pos.x = x + px;
            cur_pos.y = y + py;
            i32 dist = distance(cur_pos, center);
            if(dist <= radius) {
                auto* point = &point_buffer[point_count];
                point->x = cur_pos.x;
                point->y = cur_pos.y;
                ++point_count;
            }
        }
    }

    SDL_RenderDrawPoints(renderer, point_buffer, point_count);

    free(point_buffer);
}

inline i32
f_score(AstarScore score) {
    return score.G + score.H;
}

static i32
find_lowest_fscore_index(std::vector<AstarCell*>* cells, i32 count) {
    i32 best_index = 0;
    i32 lowest_F = f_score(cells->at(0)->score);

    for(i32 i = 1; i < count; i++) {
        i32 F = f_score(cells->at(i)->score);
        if(F < lowest_F) {
            lowest_F = F;
            best_index = i;
        }
    }

    return best_index;
}

inline i32
astar_heuristic(Vec2 pos, Vec2 goal) {
    return (i32)distance(pos, goal);
}

inline b32
check_walkable_cell(Vec2 pos, Vec2* positions, i32 positions_count) {
    for(i32 i = 0; i < positions_count; i++) {
        Vec2 test_pos = positions[i];
        if(pos.x == test_pos.x && pos.y == test_pos.y) {
            return false;
        }
    }
    return true;
}

static i32
find_walkable_adjacent_cells(Vec2 current_position, Vec2* result_buffer,
                             Vec2* positions, i32 positions_count, i32 grid_size)
{
    const i32 candidate_count = 4;
    Vec2 candidate_positions[candidate_count] = {
        //Top row
                    { 0,  1 },

        //Middle row
        { -1,  0 },            { 1,  0 },

        //Bottom row
                    { 0, -1 }
    };
    for(i32 i = 0; i < candidate_count; i++) {
        //Offset by current_position
        candidate_positions[i] += current_position;
    }
    b32 candidate_is_not_walkable[candidate_count] = {0};

    //Mark cells walkable or not
    for(i32 j = 0; j < candidate_count; j++) {
        auto candidate_pos = candidate_positions[j];
        if(candidate_pos.y == grid_size || candidate_pos.x == grid_size ||
           candidate_pos.y < 0 || candidate_pos.x < 0)
        {
            candidate_is_not_walkable[j] = true;
            continue;
        }
        for(i32 i = 0; i < positions_count; i++) {
            auto compare_pos = positions[i];
            if(candidate_pos == compare_pos) {
                candidate_is_not_walkable[j] = true;
                break;
            }
        }
    }

    i32 found_count = 0;
    for(i32 i = 0; i < candidate_count; i++) {
        if(!candidate_is_not_walkable[i]) {
            result_buffer[found_count] = candidate_positions[i];
            found_count++;
        }
    }

    return found_count;
}

static std::vector<Vec2>
astar_reconstruct_path(AstarCell* goal) {
    std::vector<Vec2> path;
    AstarCell* current = goal;
    while(current->previous != 0) {
        path.push_back(current->position);
        current = current->previous;
    }

    std::reverse(path.begin(), path.end()); 
    return path;
}

static std::vector<Vec2>
find_path_with_astar(Vec2 start, Vec2 goal,
      Vec2* positions, i32 positions_count,
      i32 grid_size)
{
    i32 max_count = grid_size*grid_size;
    auto open_set = std::vector<AstarCell*>();
    open_set.reserve(max_count);
    auto closed_set = std::vector<AstarCell*>();
    closed_set.reserve(max_count);
    //TODO: pass memory from outside. Don't realloc this array every frame boi
    auto* cells = (AstarCell*)calloc(max_count, sizeof(AstarCell));
    i32 cell_counter = 0;

    AstarCell* current_cell = &cells[cell_counter++];
    current_cell->position = start;
    current_cell->score.G = 0;
    current_cell->score.H = astar_heuristic(start, goal);

    open_set.push_back(current_cell);

    Vec2 adjacent_cells[4];

    do {
        i32 current_index = find_lowest_fscore_index(&open_set, open_set.size());
        current_cell = open_set[current_index];

        closed_set.push_back(open_set[current_index]);
        open_set.erase(open_set.begin() + current_index);

        if(contains_position(&closed_set, goal)) {
            //Found path
            break;
        }

        i32 adjacent_cells_count = find_walkable_adjacent_cells(current_cell->position, adjacent_cells,
                                                                positions, positions_count, grid_size);

        for(i32 i = 0; i < adjacent_cells_count; i++) {
            Vec2 pos = adjacent_cells[i];
            if(contains_position(&closed_set, pos)) {
                continue;
            }

            AstarCell* found_cell;
            if(!contains_position(&open_set, pos, &found_cell)) {
                AstarScore score;
                score.G = current_cell->score.G+1;
                score.H = astar_heuristic(pos, goal);
                AstarCell* cell = &cells[cell_counter++];
                cell->position = pos;
                cell->score = score;
                cell->previous = current_cell;
                open_set.push_back(cell);
            } else {
                if(found_cell->previous) {
                    if(f_score(current_cell->score) < f_score(found_cell->previous->score)) {
                        //Don't allow diagonals
                        if(current_cell->position.x == found_cell->position.x ||
                           current_cell->position.y == found_cell->position.y)
                        {
                            found_cell->previous = current_cell;
                        }
                    }
                }
            }
        }

    } while(!open_set.empty());


    std::vector<Vec2> path;
    if(!closed_set.empty()) {
        path = astar_reconstruct_path(current_cell);
    }

    free(cells);
    return path;
}

static void
randomize_fruit_pos(Game* game) {
    b32 success = false;
    Vec2 new_pos;
    while(!success) {
        success=true;
        new_pos.x = rand() % game->grid_size;
        new_pos.y = rand() % game->grid_size;
        for(i32 i = 0; i < game->snake_cell_count; i++) {
            if(new_pos.x == game->positions[i].x && new_pos.y == game->positions[i].y) {
                success=false;
            }
        }
    }
    game->fruit_pos = new_pos;
}

static void
reset_state(Game* game, Rendering* rendering) {
    game->collided = false;

    auto* snake_pos = game->positions;
    snake_pos->x = rand() % game->grid_size;
    snake_pos->y = rand() % game->grid_size;

    randomize_fruit_pos(game);

    for(i32 i = 0; i < game->max_cell_count; i++) {
        auto* rect = &rendering->rects[i];
        auto* pos = &game->positions[i];

        rect->w = rendering->cell_width;
        rect->h = rendering->cell_height;
        pos->x = snake_pos->x;
        pos->y = snake_pos->y;

        game->positions_last_frame[i] = *pos;
    }

    game->input = -1;
    game->snake_cell_count = 3;
    game->flash_counter = 0;
    rendering->draw_snake = true;
    game->frame_time = game->start_frame_time;
}

static void
reset_routine(Game* game, Rendering* rendering) {
    game->frame_time = game->start_frame_time;
    if(game->flash_counter < game->flash_count) {
        if(game->flash_counter % 2 == 0) {
            rendering->draw_snake = false;
        } else {
            rendering->draw_snake = true;
        }

        ++game->flash_counter;
    } else {
        reset_state(game, rendering);
    }
}

static void
game_loop(Game* game) {
    auto* snake_pos = &game->positions[0];

#if 0
    //Player input
    switch(game->input) {
        case UP:
            if(game->direction == RIGHT || game->direction == LEFT) {
                game->direction = UP;
            }
        break;
        case DOWN:
            if(game->direction == RIGHT || game->direction == LEFT) {
                game->direction = DOWN;
            }
        break;
        case RIGHT:
            if(game->direction == UP || game->direction == DOWN) {
                game->direction = RIGHT;
            }
        break;
        case LEFT:
            if(game->direction == UP || game->direction == DOWN) {
                game->direction = LEFT;
            }
        break;
    }
#else
    //Astar
    auto path = find_path_with_astar(*snake_pos, game->fruit_pos, game->positions, game->snake_cell_count, game->grid_size);
    if(path.size() > 1) {
        Vec2 path_next_pos = path[1]; //First element is our own position
        if(path_next_pos.x == snake_pos->x) { //UP/DOWN
            if(path_next_pos.y > snake_pos->y) {
                game->direction = UP;
            } else if(path_next_pos.y < snake_pos->y) {
                game->direction = DOWN;
            } else {
                DebugBreak(); //Should never happen
            }
        } else if(path_next_pos.y == snake_pos->y) { //LEFT/RIGHT
            if(path_next_pos.x > snake_pos->x) {
                game->direction = RIGHT;
            } else if(path_next_pos.x < snake_pos->x) {
                game->direction = LEFT;
            } else {
                DebugBreak(); //Should never happen
            }
        } else {
            DebugBreak(); //Should never happen
        }

        int i = 0;
    }
#endif

    switch(game->direction) {
        case UP:
            snake_pos->y++;
        break;
        case DOWN:
            snake_pos->y--;
        break;
        case LEFT:
            snake_pos->x--;
        break;
        case RIGHT:
            snake_pos->x++;
        break;
    }

    for(i32 i = 1; i < game->snake_cell_count; i++) {
        if(snake_pos->x == game->positions[i].x && snake_pos->y == game->positions[i].y) {
            game->collided = true;
        }
    }

    b32 edge_collision = false;
    if(game->direction == RIGHT && snake_pos->x >= game->grid_size) {
        edge_collision = true;
        --snake_pos->x;
    }
    if (game->direction == LEFT && snake_pos->x < 0) {
        edge_collision = true;
        ++snake_pos->x;
    }
    if (game->direction == UP && snake_pos->y >= game->grid_size) {
        edge_collision = true;
        --snake_pos->y;
    }
    if (game->direction == DOWN && snake_pos->y < 0) {
        edge_collision = true;
        ++snake_pos->y;
    }

    if(edge_collision) {
        game->collided = true;
        return;
    }

    if(snake_pos->x == game->fruit_pos.x && snake_pos->y == game->fruit_pos.y) {
        game->frame_time = max(game->min_frame_time, game->frame_time * game->speed_up_rate);
        i32 increment = game->snake_cell_count + 1;
        game->snake_cell_count = min(increment, game->max_cell_count);
        randomize_fruit_pos(game);
    }

    for(i32 i = 1; i < game->snake_cell_count; i++) {
        game->positions[i].x = game->positions_last_frame[i-1].x;
        game->positions[i].y = game->positions_last_frame[i-1].y;
    }


    for(i32 i = 0; i < game->snake_cell_count; i++) {
        game->positions_last_frame[i].x = game->positions[i].x;
        game->positions_last_frame[i].y = game->positions[i].y;
    }
}

void
render_loop(SDL_Renderer* renderer, Rendering* rendering, Game* game) {
    for(i32 i = 0; i < game->snake_cell_count; i++) {
        auto* rect = &rendering->rects[i];
        auto* position = &game->positions[i];
        rect->x = position->x * rendering->cell_width;
        rect->y = (game->grid_size - position->y - 1) * rendering->cell_height;
    }

    SDL_RenderCopy(renderer, rendering->grid_texture, 0, 0);

    {
        const i32 k = 255;
        SDL_SetRenderDrawColor(renderer, k, k, k, 255);
    }

    //Draw fruit
    {
        SDL_Rect circle_rect;
        circle_rect.w = rendering->fruit_radius*2;
        circle_rect.h = rendering->fruit_radius*2;
        circle_rect.x = (game->fruit_pos.x * rendering->cell_width)+3;
        circle_rect.y = ((game->grid_size-game->fruit_pos.y-1) * rendering->cell_height)+3;
        SDL_RenderCopy(renderer, rendering->circle_texture, 0, &circle_rect);
    }

    if (rendering->draw_snake) {
        SDL_RenderFillRects(renderer, rendering->rects, game->snake_cell_count);
    }

    SDL_RenderPresent(renderer);
}

void
init_renderer(SDL_Renderer* renderer, Rendering* rendering) {
    SDL_Surface* grid_surface = SDL_CreateRGBSurface(0, rendering->screen_width, rendering->screen_height, 32, 0, 0, 0, 0);
    SDL_Renderer* grid_renderer = SDL_CreateSoftwareRenderer(grid_surface);
    render_grid(grid_renderer, rendering);
    rendering->grid_texture = SDL_CreateTextureFromSurface(renderer, grid_surface);
    SDL_DestroyRenderer(grid_renderer);
    SDL_FreeSurface(grid_surface);

    SDL_Surface* circle_surface =
        SDL_CreateRGBSurface(0, rendering->fruit_radius*2+1, rendering->fruit_radius*2+1, 32, 0, 0, 0, 0);
    SDL_Renderer* circle_renderer = SDL_CreateSoftwareRenderer(circle_surface);
    SDL_SetRenderDrawColor(circle_renderer, 0, 0, 0, 0);
    SDL_RenderClear(circle_renderer);
    SDL_SetRenderDrawColor(circle_renderer, 255, 255, 255, 255);
    render_circle(circle_renderer, 0, 0, rendering->fruit_radius);
    rendering->circle_texture = SDL_CreateTextureFromSurface(renderer, circle_surface);
    SDL_DestroyRenderer(circle_renderer);
    SDL_FreeSurface(circle_surface);
}

i32
main(i32 argc, char **argv) {
    // Init SDL stuff
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        const char* error = SDL_GetError();
        assert("SDL_Error" == error);
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
    game.positions = (Vec2*)malloc(sizeof(Vec2) * game.max_cell_count);
    game.positions_last_frame = (Vec2*)malloc(sizeof(Vec2) * game.max_cell_count);
    game.direction = RIGHT;
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
        const char* error = SDL_GetError();
        assert("SDL_Error" == error);
        return -1;
    }

    init_renderer(renderer, &rendering);

    srand(time(0));
    reset_state(&game, &rendering);

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
                            game.input = UP;
                        }
                        break;

                        case SDLK_DOWN: {
                            game.input = DOWN;
                        }
                        break;

                        case SDLK_LEFT: {
                            game.input = LEFT;
                        }
                        break;

                        case SDLK_RIGHT: {
                            game.input = RIGHT;
                        }
                        break;
                    }
                }
                break;
            }
        }

        if(current_time >= (last_update_time + game.frame_time)) {
            last_update_time = current_time;
            if(!game.collided) {
                game_loop(&game);
            } else {
                reset_routine(&game, &rendering);
            }
        }

        render_loop(renderer, &rendering, &game);
    }

    SDL_DestroyWindow(window);
    return 0;
}
