#include "raylib.h"
#include "player.h"
#include "tilemap.h"
#include "spawner.h"

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Dungeon Survivors: D20 - C++ edition");
    SetTargetFPS(60);

    TileMap map;
    Player player({ 224.0f, 160.0f });
    Spawner spawner(200);  // пул на 200 врагов

    Camera2D camera = { 0 };
    camera.target = player.position;
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        player.Update(deltaTime, map);
        spawner.Update(deltaTime, player.position);
        camera.target = player.position;

        BeginDrawing();
            ClearBackground(BLACK);

            BeginMode2D(camera);
                map.Draw();
                spawner.Draw();
                player.Draw();
            EndMode2D();

            DrawText("WASD / Arrows / Gamepad - move", 10, 40, 20, RAYWHITE);
            DrawText(TextFormat("Enemies: %d", spawner.ActiveCount()), 10, 65, 20, RAYWHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
