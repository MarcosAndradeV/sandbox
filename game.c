#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
// Extending nob_da_foreach idea
#define da_foreach_rev(Type, it, da) for (Type *it = (da)->items+(da)->count-1; it > (da)->items; --it)

#include "raylib.h"

// Display Config
#define GRID_MULT 32

#define GRID_SIZE (1 * GRID_MULT)
#define CELL_SIZE (960 / GRID_MULT)
#define WINDOW_WIDTH (GRID_SIZE * CELL_SIZE)
#define UI_OFFSET 30
#define WINDOW_HEIGHT (GRID_SIZE * CELL_SIZE + UI_OFFSET)
#define MOUSEHITBOX 1.0f

#define SIMULATION_SPEED_BASE 1

// Colors
#define BACKGROUND_COLOR DARKGRAY

typedef enum {
    CELL_TYPE_NONE,
    CELL_TYPE_SAND,
    CELL_TYPE_WATER,
    CELL_TYPE_LIFE,
    CELL_TYPE_ROCK,
    CELL_TYPE_BEDROCK,
} Cell_Type;

static_assert(CELL_TYPE_NONE  == 0, "Cell_Type has change");
static Color Cell_Type_color_table[] = {
    [CELL_TYPE_BEDROCK] = GRAY,
    [CELL_TYPE_ROCK] = GRAY,
    [CELL_TYPE_WATER] = BLUE,
    [CELL_TYPE_LIFE] = GREEN,
    [CELL_TYPE_SAND] = BEIGE,
    [CELL_TYPE_NONE] = BACKGROUND_COLOR,
};
static_assert(ARRAY_LEN(Cell_Type_color_table) == 6, "Cell_Type has change");

typedef struct {
    Rectangle rect;
    bool updated;
    Cell_Type type;
} Cell;

static inline Color Cell_get_color(Cell *c) {
    NOB_ASSERT((c->type >= CELL_TYPE_NONE && c->type < ARRAY_LEN(Cell_Type_color_table)) && "Cell_get_color");
    return Cell_Type_color_table[c->type];
}

typedef struct {
    Cell* items;
    size_t capacity, count;
} Grid;

void UpdateSand(Grid*grid, Cell*c, int col, int row);
void UpdateWater(Grid*grid, Cell*c, int col, int row);
void UpdateLife(Grid*grid, Cell*c, int col, int row);
bool SwapCell(Grid *grid, size_t src_pos, size_t dst_pos);
bool TryMoveCell(Grid *grid, size_t src_pos, size_t dst_pos, Cell_Type allowed_type);

typedef void (*CellUpdateFn)(Grid*, Cell*, int, int);
static CellUpdateFn update_table[] = {
    [CELL_TYPE_NONE] = NULL,
    [CELL_TYPE_SAND] = UpdateSand,
    [CELL_TYPE_WATER] = UpdateWater,
    [CELL_TYPE_LIFE] = UpdateLife,
    [CELL_TYPE_ROCK] = NULL,
    [CELL_TYPE_BEDROCK] = NULL,
};
static_assert(ARRAY_LEN(update_table) == 6, "update_table has change");

static inline void Update(Grid*grid, Cell*c, int col, int row) {
    if (update_table[c->type]) {
        update_table[c->type](grid, c, col, row);
    }
}

bool SwapCell(Grid *grid, size_t src_pos, size_t dst_pos) {
    if (dst_pos >= grid->count) return false;

    Cell *src = &grid->items[src_pos];
    Cell *dst = &grid->items[dst_pos];

    Cell_Type tmp = src->type;
    src->type = dst->type;
    dst->type = tmp;

    src->updated = true;
    dst->updated = true;
    return true;
}

bool TryMoveCell(Grid *grid, size_t src_pos, size_t dst_pos, Cell_Type allowed_type) {
    if (dst_pos >= grid->count) return false;

    Cell *src = &grid->items[src_pos];
    Cell *dst = &grid->items[dst_pos];

    if (dst->type == allowed_type) {
        dst->type = src->type;
        dst->updated = true;
        src->type = CELL_TYPE_NONE;
        src->updated = true;
        return true;
    }
    return false;
}

void UpdateSand(Grid*grid, Cell*c, int col, int row) {
    c->updated = true;
    size_t pos = col + row * GRID_SIZE;

    size_t down = col + (row+1) * GRID_SIZE;
    if (TryMoveCell(grid, pos, down, CELL_TYPE_NONE)) return;
    int side = GetRandomValue(0, 1) ? -1 : 1;
    size_t down_side = (col+(side)) + (row+1) * GRID_SIZE;
    if (TryMoveCell(grid, pos, down_side, CELL_TYPE_NONE)) return;

    if (row < GRID_SIZE - 1) {
        size_t down = col + (row+1) * GRID_SIZE;
        if (grid->items[down].type == CELL_TYPE_WATER && SwapCell(grid, pos, down)) return;
    }
}

void UpdateWater(Grid*grid, Cell*c, int col, int row) {
    c->updated = true;
    size_t pos = col + row * GRID_SIZE;
    size_t down = col + (row+1) * GRID_SIZE;
    size_t top = col + (row-1) * GRID_SIZE;

    if (TryMoveCell(grid, pos, down, CELL_TYPE_NONE)) return;

    int dir = GetRandomValue(0, 1) == 0 ? -1 : 1;
    size_t side = (col+dir) + row * GRID_SIZE;
    if (TryMoveCell(grid, pos, side, CELL_TYPE_NONE)) return;

    dir = GetRandomValue(0, 1) == 0 ? -1 : 1;
    size_t down_side = (col+dir) + (row+1) * GRID_SIZE;
    if (TryMoveCell(grid, pos, down_side,  CELL_TYPE_NONE)) return;

    if (grid->items[top].type == CELL_TYPE_SAND && SwapCell(grid, pos, top)) return;
}

int CountNeighbors(Grid *grid, int col, int row, Cell_Type type) {
    int count = 0;
    if (row > 0) { size_t top = col + (row-1) * GRID_SIZE; if(grid->items[top].type == type) count++; }
    if (col > 0) { size_t left = (col-1) + row * GRID_SIZE; if(grid->items[left].type == type) count++; }
    if (col > 0 &&
        row < GRID_SIZE - 1) { size_t down_left = (col+1) + (row-1) * GRID_SIZE; if(grid->items[down_left].type == type) count++; }
    if (row < GRID_SIZE - 1) { size_t down = col + (row+1) * GRID_SIZE; if(grid->items[down].type == type) count++; }
    if (col < GRID_SIZE - 1) { size_t right = (col+1) + row * GRID_SIZE; if(grid->items[right].type == type) count++; }
    if (col < GRID_SIZE - 1) { size_t down_right = (col+1) + (row+1) * GRID_SIZE; if(grid->items[down_right].type == type) count++; }
    return count;
}

bool HasNeighbor(Grid *grid, int col, int row, Cell_Type type) {
    if (row > 0)               { size_t top    = col     + (row-1) * GRID_SIZE; if (grid->items[top].type == type) return true; }
    if (row < GRID_SIZE - 1)   { size_t down   = col     + (row+1) * GRID_SIZE; if (grid->items[down].type == type) return true; }
    if (col > 0)               { size_t left   = (col-1) + row * GRID_SIZE;     if (grid->items[left].type == type) return true; }
    if (col < GRID_SIZE - 1)   { size_t right  = (col+1) + row * GRID_SIZE;     if (grid->items[right].type == type) return true; }
    return false;
}

void UpdateLife(Grid*grid, Cell*c, int col, int row) {
    c->updated = true;

    size_t candidates[4];
    int count = 0;

    if (row > 0) {
        size_t top = col + (row-1) * GRID_SIZE;
        if (grid->items[top].type == CELL_TYPE_NONE && HasNeighbor(grid, col, row-1, CELL_TYPE_WATER)) {
            candidates[count++] = top;
        }
    }
    if (row < GRID_SIZE - 1) {
        size_t down = col + (row+1) * GRID_SIZE;
        if (grid->items[down].type == CELL_TYPE_NONE && HasNeighbor(grid, col, row+1, CELL_TYPE_WATER)) {
            // candidates[count++] = down;
            grid->items[down].type = CELL_TYPE_LIFE;
            grid->items[down].updated = true;
            return;
        }
    }
    if (col > 0) {
        size_t left = (col-1) + row * GRID_SIZE;
        if (grid->items[left].type == CELL_TYPE_NONE && HasNeighbor(grid, col-1, row, CELL_TYPE_WATER)) {
            candidates[count++] = left;
        }
    }
    if (col < GRID_SIZE - 1) {
        size_t right = (col+1) + row * GRID_SIZE;
        if (grid->items[right].type == CELL_TYPE_NONE && HasNeighbor(grid, col+1, row, CELL_TYPE_WATER)) {
            candidates[count++] = right;
        }
    }

    if (count == 0) return;

    int choice = GetRandomValue(0, count-1);
    size_t chosen = candidates[choice];

    grid->items[chosen].type = CELL_TYPE_LIFE;
    grid->items[chosen].updated = true;
}

static Cell_Type curr_place_type;

static Rectangle mouse_rect = (Rectangle) {
    .width = MOUSEHITBOX,
    .height = MOUSEHITBOX,
};

void UpdateMouseRect(Grid* grid) {
    int col = mouse_rect.x / (int)CELL_SIZE;
    int row = (mouse_rect.y - UI_OFFSET) / (int)CELL_SIZE;
    Vector2 mouse_pos = GetMousePosition();
    mouse_rect.x = mouse_pos.x;
    mouse_rect.y = mouse_pos.y;

    size_t pos = col + row * GRID_SIZE;
    if(pos >= grid->count) return;
    Cell* it = &grid->items[pos];
    if(it->type == CELL_TYPE_BEDROCK) return;
    if(CheckCollisionRecs(it->rect, mouse_rect)) it->type = curr_place_type;
}

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SandBox");
    SetTargetFPS(60);

    Grid grid = {0};
    char * legend = temp_sprintf("%dx%d Grid", GRID_SIZE, GRID_SIZE);
    size_t frameCounter = 0;
    size_t simulationSpeed = SIMULATION_SPEED_BASE;
    bool simulationPaused = false;

    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            Cell cell = (Cell) {
                .rect = (Rectangle) {
                    .x = col * (int)CELL_SIZE,
                    .y = row * (int)CELL_SIZE + UI_OFFSET,
                    .width = (int)CELL_SIZE,
                    .height = (int)CELL_SIZE,
                },
                .type = (row == 0 || col == 0) || (row == GRID_SIZE-1 || col == GRID_SIZE-1)  ? CELL_TYPE_BEDROCK : CELL_TYPE_NONE,
            };
            da_append(&grid, cell);
        }
    }

    while (!WindowShouldClose()) {
        if(IsKeyDown(KEY_Q)) curr_place_type = CELL_TYPE_NONE;
        if(IsKeyDown(KEY_W)) curr_place_type = CELL_TYPE_WATER;
        if(IsKeyDown(KEY_S)) curr_place_type = CELL_TYPE_SAND;
        if(IsKeyDown(KEY_E)) curr_place_type = CELL_TYPE_ROCK;
        if(IsKeyDown(KEY_R)) curr_place_type = CELL_TYPE_LIFE;
        if(IsKeyPressed(KEY_P)) simulationPaused = !simulationPaused;
        if(IsKeyPressed(KEY_EQUAL)) simulationSpeed = (simulationSpeed > SIMULATION_SPEED_BASE ? simulationSpeed - 1 : SIMULATION_SPEED_BASE);
        if(IsKeyPressed(KEY_MINUS)) simulationSpeed++;

        if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) UpdateMouseRect(&grid);

        if(!simulationPaused) frameCounter++;
        if (frameCounter >= simulationSpeed) {
            frameCounter = 0;
            da_foreach_rev(Cell, c, &grid) {
                if(c->updated) continue;
                int col = c->rect.x / (int)CELL_SIZE;
                int row = (c->rect.y - UI_OFFSET) / (int)CELL_SIZE;
                Update(&grid, c, col, row);
            }
        }

        BeginDrawing();

        ClearBackground(BACKGROUND_COLOR);

        DrawText(legend, 10, 10, 20, WHITE);

        da_foreach(Cell, it, &grid) {
            it->updated = false;

            Color c = Cell_get_color(it);

            DrawRectangleRec(it->rect, c);
        }

        DrawRectangleLines(0, UI_OFFSET, WINDOW_WIDTH, GRID_SIZE * CELL_SIZE, BLACK);

        EndDrawing();
    }

    da_free(grid);

    CloseWindow();

    return 0;
}
