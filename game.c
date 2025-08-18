#include <stdbool.h>
#include <stddef.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#define da_foreach_rev(Type, it, da) for (Type *it = (da)->items+(da)->count-1; it > (da)->items; --it)
#include "raylib.h"
#define GRID_MULT 128
#define GRID_SIZE (1 * GRID_MULT)
#define CELL_SIZE (960 / GRID_MULT)
#define WINDOW_WIDTH (GRID_SIZE * CELL_SIZE)
#define WINDOW_HEIGHT (GRID_SIZE * CELL_SIZE + 40)
#define UI_OFFSET 30
#define LIGHTCOLOR RAYWHITE
#define DARKCOLOR LIGHTGRAY
#define GRIDLINECOLOR BLACK
#define GRIDLINETICKNESS 0.0f
#define MOUSEHITBOX 0.1f
#define SIMULATION_SPEED 1

typedef enum {
    CELL_TYPE_NONE,
    CELL_TYPE_SAND,
    CELL_TYPE_WATER,
    CELL_TYPE_ROCK,
} Cell_Type;

typedef struct {
    Rectangle rect;
    Color color;

    bool selected;
    bool updated;
    Cell_Type type;
} Cell;

typedef struct {
    Cell* items;
    size_t capacity, count;
} Grid;

void UpdateSand(Grid* grid, Cell*c, int col, int row);
void UpdateWater(Grid* grid, Cell*c, int col, int row);
bool UpdateCell(Grid* grid, Cell* c, Cell_Type old_type, Cell_Type new_type, Cell_Type next_type, size_t next_pos);

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SandBox");
    SetTargetFPS(60);

    Rectangle mouse_rect = (Rectangle) {
        .width = MOUSEHITBOX,
        .height = MOUSEHITBOX,
    };
    Grid grid = {0};
    char * legend = temp_sprintf("%dx%d Grid", GRID_SIZE, GRID_SIZE);
    const int cell_size = CELL_SIZE;
    Cell_Type curr_place_type = CELL_TYPE_SAND;
    size_t frameCounter = 0;

    for (int row = 0; row < GRID_SIZE; row++)
    {
        for (int col = 0; col < GRID_SIZE; col++)
        {
            Cell cell = (Cell) {
                .rect = (Rectangle) {
                    .x = col * cell_size,
                    .y = row * cell_size + UI_OFFSET,
                    .width = cell_size,
                    .height = cell_size,
                },
                .type = (row == 0 || col == 0) || (row == GRID_SIZE-1 || col == GRID_SIZE-1)  ? CELL_TYPE_ROCK : CELL_TYPE_NONE,
                .color = BLACK, // ((row + col) % 2 == 0) ? LIGHTCOLOR : DARKCOLOR,
            };
            da_append(&grid, cell);
        }
    }

    while (!WindowShouldClose()) {

        da_foreach(Cell, it, &grid) {
            Vector2 mouse_pos = GetMousePosition();
            mouse_rect.x = mouse_pos.x;
            mouse_rect.y = mouse_pos.y;
            if(CheckCollisionRecs(it->rect, mouse_rect)) {
                if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    if(it->type != CELL_TYPE_ROCK) it->type = curr_place_type;
                } else if(IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
                    if(it->type != CELL_TYPE_ROCK) it->type = CELL_TYPE_NONE;
                } else if(IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
                    if(it->type != CELL_TYPE_ROCK) it->type = CELL_TYPE_WATER;
                } else {
                    it->selected = true;
                }
            } else {
                it->selected = false;
            }
            it->updated = false;
        }

        frameCounter++;
        if (frameCounter >= SIMULATION_SPEED) {
            frameCounter = 0;
            da_foreach_rev(Cell, it, &grid) {
                if(it->type == CELL_TYPE_NONE) continue;
                if(it->type == CELL_TYPE_ROCK) continue;
                int col = it->rect.x / cell_size;
                int row = (it->rect.y - UI_OFFSET) / cell_size;
                switch (it->type) {
                    case CELL_TYPE_SAND: {
                        UpdateSand(&grid, it, col, row);
                    } break;
                    case CELL_TYPE_WATER: {
                        UpdateWater(&grid, it, col, row);
                    } break;
                    default: break;
                }
            }
        }

        BeginDrawing();

        ClearBackground(DARKGRAY);

        DrawText(legend, 10, 10, 20, WHITE);

        da_foreach(Cell, it, &grid) {
            Color c = it->color;
            if (it->selected) {
                c = ColorBrightness(RED, 0.3);
            } else {
                switch (it->type) {
                    case CELL_TYPE_SAND: {
                        c = BEIGE;
                    } break;
                    case CELL_TYPE_WATER: {
                        c = BLUE;
                    } break;
                    case CELL_TYPE_ROCK: {
                        c = DARKGRAY;
                    } break;
                    default: break;
                }
            }

            DrawRectangleRec(it->rect, c);

            DrawRectangleLinesEx(it->rect, GRIDLINETICKNESS, GRIDLINECOLOR);
        }

        DrawRectangleLines(0, UI_OFFSET, WINDOW_WIDTH, GRID_SIZE * CELL_SIZE, BLACK);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

void UpdateSand(Grid*grid, Cell*c, int col, int row) {
    if(c->updated) return;
    c->updated = true;

    size_t down = col + (row+1) * GRID_SIZE;
    if(UpdateCell(grid, c, CELL_TYPE_NONE,CELL_TYPE_NONE, CELL_TYPE_SAND, down)) return;

    size_t down_left = (col-1) + (row+1) * GRID_SIZE;
    if(UpdateCell(grid, c, CELL_TYPE_NONE,CELL_TYPE_NONE, CELL_TYPE_SAND, down_left)) return;

    size_t down_right = (col+1) + (row+1) * GRID_SIZE;
    if(UpdateCell(grid, c, CELL_TYPE_NONE,CELL_TYPE_NONE, CELL_TYPE_SAND, down_right)) return;
}

bool UpdateCell(Grid* grid, Cell* c, Cell_Type old_type, Cell_Type new_type, Cell_Type next_type, size_t next_pos) {
    if(next_pos < grid->count) {
        if(grid->items[next_pos].type == old_type) {
            grid->items[next_pos].type = next_type;
            grid->items[next_pos].updated = true;
            c->type = new_type;
            return true;
        }
    }
    return false;
}

void UpdateWater(Grid*grid, Cell*c, int col, int row) {
    if(c->updated) return;
    c->updated = true;
    size_t down = col + (row+1) * GRID_SIZE;
    size_t down_left = (col-1) + (row+1) * GRID_SIZE;
    size_t down_right = (col+1) + (row+1) * GRID_SIZE;
    size_t left = (col-1) + row * GRID_SIZE;
    size_t right = (col+1) + row * GRID_SIZE;
    size_t top = col + (row-1) * GRID_SIZE;

    if(UpdateCell(grid, c, CELL_TYPE_SAND, CELL_TYPE_SAND, CELL_TYPE_WATER, top)) return;
    if(UpdateCell(grid, c, CELL_TYPE_NONE, CELL_TYPE_NONE, CELL_TYPE_WATER, down)) return;
    if(UpdateCell(grid, c, CELL_TYPE_NONE, CELL_TYPE_NONE, CELL_TYPE_WATER, down_left)) return;
    if(UpdateCell(grid, c, CELL_TYPE_NONE, CELL_TYPE_NONE, CELL_TYPE_WATER, down_right)) return;
    if(UpdateCell(grid, c, CELL_TYPE_NONE, CELL_TYPE_NONE, CELL_TYPE_WATER, left)) return;
    if(UpdateCell(grid, c, CELL_TYPE_NONE, CELL_TYPE_NONE, CELL_TYPE_WATER, right)) return;
}
