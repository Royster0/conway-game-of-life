#include "raylib.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 720
#define TARGET_FPS 60
#define CELL_SIZE 20
#define GRID_WIDTH (WINDOW_HEIGHT / CELL_SIZE)
#define GRID_HEIGHT (WINDOW_HEIGHT / CELL_SIZE)

// Find how many neighbors are alive
int countNeighbors(int grid[GRID_HEIGHT][GRID_WIDTH], int row, int col) {
    int alive = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;

            int newRow = row + i;
            int newCol = col + j;

            if (newRow >= 0 && newRow < GRID_HEIGHT &&
                newCol >= 0 && newCol < GRID_WIDTH) {
                    alive += grid[newRow][newCol];
                }
        }
    }

    return alive;
}

// Calculate the next generation of cells
void updateGrid(int grid[GRID_HEIGHT][GRID_WIDTH], int newGrid[GRID_HEIGHT][GRID_WIDTH]) {
    for (int row = 0; row < GRID_HEIGHT; row++) {
        for (int col = 0; col < GRID_WIDTH; col++) {
            int neighbors = countNeighbors(grid, row, col);

            if (grid[row][col]) {
                newGrid[row][col] = (neighbors == 2 || neighbors == 3);
            } else {
                newGrid[row][col] = (neighbors == 3);
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
    int grid[GRID_HEIGHT][GRID_WIDTH] = {0};
    int newGrid[GRID_HEIGHT][GRID_WIDTH] = {0};

    Camera2D camera = {
        .offset = (Vector2){WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2},
        .target = (Vector2){0, 0},
        .rotation = 0.0f,
        .zoom = 1.0f
    };

    // Gamestate
    bool paused = true;
    float updateTime = 0.0f;
    float updateInterval = 0.1f;
    bool isDragging = false;

    while (!WindowShouldClose()) {
        /* INPUT */
        // Pause
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
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

        // Mouse Left and Right Click to Toggle Cells
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            Vector2 gridPos = getGridPosition(GetMousePosition(), camera);
            int gridX = (int)gridPos.x;
            int gridY = (int)gridPos.y;

            if (gridX >= 0 && gridX < GRID_WIDTH &&
                gridY >= 0 && gridY < GRID_HEIGHT) {
                    grid[gridY][gridX] = IsMouseButtonDown(MOUSE_LEFT_BUTTON) ? 1 : 0;
            }
        }

        // UPDATING
        if (!paused) {
            updateTime += GetFrameTime();
            
            if (updateTime >= updateInterval) {
                updateGrid(grid, newGrid);
                memcpy(grid, newGrid, sizeof(grid));
                updateTime = 0.0f;
            }
        }

        // DRAWING
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode2D(camera);

        // Draw Grid
        for (int row = 0; row < GRID_HEIGHT; row++) {
            for (int col = 0; col < GRID_WIDTH; col++) {
                Rectangle cellRect = {col * CELL_SIZE, row * CELL_SIZE, CELL_SIZE, CELL_SIZE};

                // Empty cell
                DrawRectangleLines(cellRect.x, cellRect.y, cellRect.width, cellRect.height, LIGHTGRAY);

                // Alive cell
                if (grid[row][col]) {
                    DrawRectangle(cellRect.x, cellRect.y, cellRect.width, cellRect.height, BLACK);
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

        EndDrawing();
    }

    CloseWindow();
    return 0;
}