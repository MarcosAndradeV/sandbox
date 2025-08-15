#include <stdbool.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
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

typedef enum {
    CELL_TYPE_NONE,
    CELL_TYPE_SAND,
} Cell_Type;

typedef struct {
    Rectangle rect;
    Color color;

    bool selected;
    bool active;
    Cell_Type type;
} Cell;

typedef struct {
    Cell* items;
    size_t capacity, count;
} Grid;

void UpdateSand(Grid* grid, Cell*c, int col, int row);
void UpdateLife(Grid* grid, Cell*c, int col, int row);

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
                .color = ((row + col) % 2 == 0) ? LIGHTCOLOR : DARKCOLOR,
            };
            da_append(&grid, cell);
        }
    }

    while (!WindowShouldClose()) {
        da_foreach(Cell, it, &grid) {
            if(!it->active) continue;
            int col = it->rect.x / cell_size;
            int row = (it->rect.y - UI_OFFSET) / cell_size;
            switch (it->type) {
                case CELL_TYPE_SAND: {
                    UpdateSand(&grid, it, col, row);
                } break;
                default: break;
            }
        }

        da_foreach(Cell, it, &grid) {
            Vector2 mouse_pos = GetMousePosition();
            mouse_rect.x = mouse_pos.x;
            mouse_rect.y = mouse_pos.y;
            if(CheckCollisionRecs(it->rect, mouse_rect)) {
                if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    it->active = true;
                    it->type = curr_place_type;
                } else if(IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
                    it->active = false;
                } else {
                    it->selected = true;
                }
            } else {
                it->selected = false;
            }
        }

        BeginDrawing();

        ClearBackground(DARKGRAY);

        DrawText(legend, 10, 10, 20, WHITE);

        da_foreach(Cell, it, &grid) {
            Color c = it->color;
            if (it->selected) {
                c = ColorBrightness(RED, 0.3);
            } else if(it->active) {
                switch (it->type) {
                    case CELL_TYPE_SAND: {
                        c = BEIGE;
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
    int i = GetRandomValue(-1, 1);
    int next = (col+i) + (row+1) * GRID_SIZE;
    if(next < grid->count) {
        if(!grid->items[next].active) {
            c->active = false;
            grid->items[next].active = true;
            grid->items[next].type = CELL_TYPE_SAND;
        }
    }
}
