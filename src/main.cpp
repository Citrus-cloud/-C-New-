#include "raylib.h"

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    // Создаём окно и задаём целевой FPS
    InitWindow(screenWidth, screenHeight, "Dungeon Survivors: D20 - C++ edition");
    SetTargetFPS(60);

    // Главный игровой цикл: работает, пока окно не закрыли
    while (!WindowShouldClose())
    {
        // deltaTime — время между кадрами (понадобится для движения)
        float deltaTime = GetFrameTime();
        (void)deltaTime;

        BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Dungeon Survivors: D20", 380, 300, 40, RAYWHITE);
            DrawText("C++ edition (raylib) - karkas gotov!", 360, 360, 24, GRAY);
            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
