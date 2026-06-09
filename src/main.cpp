#include "raylib.h"
#include "player.h"
#include "tilemap.h"
#include "spawner.h"
#include "combat.h"
#include "pickups.h"
#include "upgrades.h"
#include "traps.h"

// Состояния игры
enum GameState { PLAYING, LEVEL_UP };

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Dungeon Survivors: D20 - C++ edition");
    SetTargetFPS(60);

    TileMap map;
    Player player(map.GetSpawnPoint());
    Spawner spawner(200);
    Weapon weapon(300);
    ExpOrbs orbs(500);
    Traps traps;
    traps.Generate(map, 16, 777u);

    GameState state = PLAYING;

    Camera2D camera = { 0 };
    camera.target = player.position;
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        if (state == PLAYING)
        {
            player.Update(deltaTime, map);
            spawner.Update(deltaTime, player.position, map);
            weapon.Update(deltaTime, player.position, spawner, orbs);
            orbs.Update(deltaTime, player);
            traps.Update(deltaTime, player);
            camera.target = player.position;

            if (player.TryLevelUp())
                state = LEVEL_UP;
        }
        else if (state == LEVEL_UP)
        {
            int choice = 0;
            if (IsKeyPressed(KEY_ONE))   choice = 1;
            if (IsKeyPressed(KEY_TWO))   choice = 2;
            if (IsKeyPressed(KEY_THREE)) choice = 3;

            if (choice != 0)
            {
                ApplyUpgrade(choice, player, weapon);
                if (!player.TryLevelUp())
                    state = PLAYING;
            }
        }

        BeginDrawing();
            ClearBackground(BLACK);

            BeginMode2D(camera);
                map.Draw();
                traps.Draw();
                orbs.Draw();
                spawner.Draw();
                weapon.Draw();
                player.Draw();
            EndMode2D();

            DrawText("WASD / Arrows / Gamepad - move", 10, 40, 20, RAYWHITE);
            DrawText(TextFormat("HP: %d", player.health), 10, 65, 20, (player.health > 30) ? GREEN : RED);
            DrawText(TextFormat("Enemies: %d", spawner.ActiveCount()), 10, 90, 20, RAYWHITE);
            DrawText(TextFormat("Level: %d   XP: %d / %d", player.level, player.xp, player.xpToNext), 10, 115, 20, RAYWHITE);
            DrawFPS(10, 10);

            if (state == LEVEL_UP)
            {
                DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 160 });
                DrawText("LEVEL UP!", screenWidth / 2 - 110, 220, 50, YELLOW);
                DrawText("Vyberi uluchshenie (1 / 2 / 3):", screenWidth / 2 - 220, 300, 26, RAYWHITE);
                DrawText(GetUpgradeText(1), screenWidth / 2 - 200, 360, 24, RAYWHITE);
                DrawText(GetUpgradeText(2), screenWidth / 2 - 200, 400, 24, RAYWHITE);
                DrawText(GetUpgradeText(3), screenWidth / 2 - 200, 440, 24, RAYWHITE);
            }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
