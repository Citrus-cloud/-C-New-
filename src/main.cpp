#include "raylib.h"
#include "player.h"
#include "tilemap.h"
#include "spawner.h"
#include "combat.h"
#include "pickups.h"

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Dungeon Survivors: D20 - C++ edition");
    SetTargetFPS(60);

    TileMap map;
    Player player({ 224.0f, 160.0f });
    Spawner spawner(200);   // пул врагов
    Weapon weapon(300);     // пул снарядов
    ExpOrbs orbs(500);      // пул сфер опыта

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
        weapon.Update(deltaTime, player.position, spawner, orbs);
        orbs.Update(deltaTime, player);
        camera.target = player.position;

        BeginDrawing();
            ClearBackground(BLACK);

            BeginMode2D(camera);
                map.Draw();
                orbs.Draw();
                spawner.Draw();
                weapon.Draw();
                player.Draw();
            EndMode2D();

            DrawText("WASD / Arrows / Gamepad - move", 10, 40, 20, RAYWHITE);
            DrawText(TextFormat("Enemies: %d", spawner.ActiveCount()), 10, 65, 20, RAYWHITE);
            DrawText(TextFormat("XP: %d", player.xp), 10, 90, 20, RAYWHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
