#include "raylib.h"
#include "player.h"

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Dungeon Survivors: D20 - C++ edition");
    SetTargetFPS(60);

    // Создаём игрока в центре мира (0, 0)
    Player player({ 0.0f, 0.0f });

    // Камера 2D, которая будет следить за игроком
    Camera2D camera = { 0 };
    camera.target = player.position;
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        player.Update(deltaTime);
        camera.target = player.position;  // камера следует за игроком

        BeginDrawing();
            ClearBackground(BLACK);

            BeginMode2D(camera);
                // Сетка-ориентир, чтобы было видно движение
                for (int x = -1000; x <= 1000; x += 100)
                    DrawLine(x, -1000, x, 1000, DARKGRAY);
                for (int y = -1000; y <= 1000; y += 100)
                    DrawLine(-1000, y, 1000, y, DARKGRAY);

                player.Draw();
            EndMode2D();

            DrawText("WASD / Arrows / Gamepad - move", 10, 40, 20, RAYWHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
