#include "raylib.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 720
#define TARGET_FPS 60
#define CELL_SIZE 20
#define MAX_GRID_SIZE 1500

typedef struct {
    int width;
    int height;
    int** cells;
} Grid;

Grid createGrid(int width, int height) {
    Grid grid = {width, height, (int**)malloc(height * sizeof(int*))};
    for (int i = 0; i < height; i++) {
        grid.cells[i] = (int*)calloc(width, sizeof(int));
    }

    return grid;
}

void freeGrid(Grid* grid) {
    for (int i = 0; i < grid->height; i++) {
        free(grid->cells[i]);
    }

    free(grid->cells);
}

void clearGrid(Grid* grid) {
    for (int i = 0; i < grid->height; i++) {
        memset(grid->cells[i], 0, grid->width * sizeof(int));
    }
}

void resizeGrid(Grid* grid, Grid* newGrid, int newWidth, int newHeight) {
    for (int y = 0; y < grid->height && y < newHeight; y++) {
        for (int x = 0; x < grid->width && x < newWidth; x++) {
            newGrid->cells[y][x] = grid->cells[y][x];
        }
    }
}

// Find how many neighbors are alive
int countNeighbors(Grid grid, int row, int col) {
    int alive = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;

            int newRow = row + i;
            int newCol = col + j;

            if (newRow >= 0 && newRow < grid.height &&
                newCol >= 0 && newCol < grid.width) {
                    alive += grid.cells[newRow][newCol];
                }
        }
    }

    return alive;
}

// Calculate the next generation of cells
void updateGrid(Grid grid, Grid* newGrid) {
    for (int row = 0; row < grid.height; row++) {
        for (int col = 0; col < grid.width; col++) {
            int neighbors = countNeighbors(grid, row, col);

            if (grid.cells[row][col]) {
                newGrid->cells[row][col] = (neighbors == 2 || neighbors == 3);
            } else {
                newGrid->cells[row][col] = (neighbors == 3);
            }
        }
    }
}

// Finds position of mouse and translates to position on grid
Vector2 getGridPosition(Vector2 mousePos, Camera2D camera) {
    Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
    Vector2 gridPos = {
        floorf(worldPos.x / CELL_SIZE),
        floorf(worldPos.y / CELL_SIZE)
    };

    return gridPos;
}

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Conway's Game of Life");
    SetTargetFPS(TARGET_FPS);

    // Initiate grid
    int initialWidth = (GetScreenWidth() / CELL_SIZE) * 2;
    int initialHeight = (GetScreenHeight() / CELL_SIZE) * 2; 

    // One grid for current, one for next gen/zoom
    Grid grid = createGrid(initialWidth, initialHeight);
    Grid newGrid = createGrid(initialWidth, initialHeight);

    Camera2D camera = {
        .offset = (Vector2){GetScreenWidth() / 2, GetScreenHeight() / 2},
        .target = (Vector2){0, 0},
        .rotation = 0.0f,
        .zoom = 1.0f
    };

    // Gamestate
    bool paused = true;
    float updateTime = 0.0f;
    float updateInterval = 0.1f;
    bool isDragging = false;

    // Track visible area
    Rectangle viewRect = {0};

    while (!WindowShouldClose()) {
        /* INPUT */
        // Pause
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }

        // Clear
        if (IsKeyPressed(KEY_C)) {
            clearGrid(&grid);
        }

        // Camera Panning
        if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) isDragging = true;
        if (IsMouseButtonReleased(MOUSE_MIDDLE_BUTTON)) isDragging = false;

        if (isDragging) {
            Vector2 delta = GetMouseDelta();
            camera.target.x -= delta.x / camera.zoom;
            camera.target.y -= delta.y / camera.zoom;
        }

        // Zoom
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

            // Zoom step amount
            const float zoomStep = 0.125f;
            camera.zoom += (wheel * zoomStep * camera.zoom);

            // Set limits on zoom speed
            if (camera.zoom < 0.125f) camera.zoom = 0.125f;
            if (camera.zoom > 3.0f) camera.zoom = 3.0f;

            Vector2 newMouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
            camera.target.x += (mouseWorldPos.x - newMouseWorldPos.x);
            camera.target.y += (mouseWorldPos.y - newMouseWorldPos.y);
        }

        // Calculate visible area
        Vector2 topLeft = GetScreenToWorld2D((Vector2){0, 0}, camera);
        Vector2 bottomRight = GetScreenToWorld2D((Vector2){GetScreenWidth(), GetScreenHeight()}, camera);
        viewRect = (Rectangle) {topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y};

        // Check to see if resize is needed
        int resizeWidth = (int)((fabs(viewRect.width) / CELL_SIZE) * 1.5f);
        int resizeHeight = (int)((fabs(viewRect.height) / CELL_SIZE) * 1.5f);

        if (resizeWidth > grid.width || resizeHeight > grid.height) {
            int newWidth = (resizeWidth > grid.width) ? resizeWidth : grid.width;
            int newHeight = (resizeHeight > grid.height) ? resizeHeight : grid.height;

            // Limit grid size because this is C and idk what can happen
            newWidth = (newWidth > MAX_GRID_SIZE) ? MAX_GRID_SIZE : newWidth;
            newHeight = (newHeight > MAX_GRID_SIZE) ? MAX_GRID_SIZE : newHeight;

            Grid tempGrid = createGrid(newWidth, newHeight);
            resizeGrid(&grid, &tempGrid, newWidth, newHeight);
            freeGrid(&grid);
            grid = tempGrid;

            tempGrid = createGrid(newWidth, newHeight);
            freeGrid(&newGrid);
            newGrid = tempGrid;
        }

        // Mouse Left and Right Click to Toggle Cells
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            Vector2 gridPos = getGridPosition(GetMousePosition(), camera);
            int gridX = (int)gridPos.x;
            int gridY = (int)gridPos.y;

            if (gridX >= 0 && gridX < grid.width &&
                gridY >= 0 && gridY < grid.height) {
                    grid.cells[gridY][gridX] = IsMouseButtonDown(MOUSE_LEFT_BUTTON) ? 1 : 0;
            }
        }

        /* UPDATING */
        if (!paused) {
            updateTime += GetFrameTime();
            
            if (updateTime >= updateInterval) {
                updateGrid(grid, &newGrid);

                Grid temp = grid;
                grid = newGrid;
                newGrid = temp;
                
                updateTime = 0.0f;
            }
        }

        /* DRAWING */
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode2D(camera);

        // Only draw visible cells and grid lines +- 1
        int startX = (int)(topLeft.x / CELL_SIZE) - 1;
        int startY = (int)(topLeft.y / CELL_SIZE) - 1;
        int endX = (int)(bottomRight.x / CELL_SIZE) + 1;
        int endY = (int)(bottomRight.y / CELL_SIZE) + 1;

        // Clamp to grid bounds
        startX = (startX < 0) ? 0 : startX;
        startY = (startY < 0) ? 0 : startY;
        endX = (endX > grid.width) ? grid.width : endX;
        endY = (endY > grid.height) ? grid.height : endY;

        // Draw Grid
        for (int row = startY; row < endY; row++) {
            for (int col = startX; col < endX; col++) {
                Rectangle cellRect = {col * CELL_SIZE, row * CELL_SIZE, CELL_SIZE, CELL_SIZE};

                // Empty cell
                DrawRectangleLines(cellRect.x, cellRect.y, cellRect.width, cellRect.height, LIGHTGRAY);

                // Alive cell
                if (grid.cells[row][col]) {
                    DrawRectangleRec(cellRect, BLACK);
                }
            }
        }

        EndMode2D();

        // Draw Instructions
        DrawText(paused ? "PAUSED" : "RUNNING", 10, 10, 20, paused ? RED : GREEN);
        DrawText("Middle Mouse - Pan", 10, 40, 15, DARKGRAY);
        DrawText("Mouse Wheel - Zoom", 10, 70, 15, DARKGRAY);
        DrawText("Left Click - Fill Cell", 10, 100, 15, DARKGRAY);
        DrawText("Right Click - Delete Cell", 10, 130, 15, DARKGRAY);
        DrawText("Space - Pause/Unpause", 10, 160, 15, DARKGRAY);
        DrawText("C - Clear", 10, 160, 15, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}