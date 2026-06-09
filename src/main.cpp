#include "raylib.h"
#include "player.h"
#include "tilemap.h"
#include "spawner.h"
#include "combat.h"
#include "pickups.h"
#include "upgrades.h"
#include "traps.h"
#include "loot.h"
#include "audio.h"
#include "hud.h"
#include "savegame.h"
#include "textures.h"

// Состояния игры
enum GameState { MENU, PLAYING, LEVEL_UP, PAUSED, GAME_OVER };

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Dungeon Survivors: D20 - C++ edition v0.1");
    SetTargetFPS(60);

    AudioManager audio;
    audio.Init();
    HUD hud;
    hud.Init(screenWidth, screenHeight);

    // Менеджер текстур: грузит и кэширует спрайты (PNG). Один на всю игру.
    TextureManager textures;

    GameSave save = LoadGameSave();
    if (audio.ready) SetMasterVolume(save.volume / 100.0f);

    TileMap map;
    Player player(map.GetSpawnPoint());
    player.LoadSprites(textures);
    Spawner spawner(200);
    spawner.LoadArt(textures);
    Weapon weapon(300);
    ExpOrbs orbs(500);
    Traps traps;
    traps.Generate(map, 16, 777u);
    LootDrops loot(200);

    float survivalTime = 0.0f;
    float subtitleTimer = 0.0f;
    int subtitleLine = -1;
    bool newRecord = false;
    const char* bossSpeakers[2] = { "Королева пауков", "Чёрный рыцарь" };
    const char* bossLines[2] = { "Ты будешь кормом моих детей.", "Жалкая пародия." };

    GameState state = MENU;

    Camera2D camera = { 0 };
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    camera.target = player.position;

    auto startNewGame = [&]() {
        map.Generate(64, 44, (unsigned)GetRandomValue(1, 1000000));
        player = Player(map.GetSpawnPoint());
        player.LoadSprites(textures);
        spawner = Spawner(200);
        spawner.LoadArt(textures);
        weapon = Weapon(300);
        orbs = ExpOrbs(500);
        traps = Traps();
        traps.Generate(map, 16, (unsigned)GetRandomValue(1, 1000000));
        loot = LootDrops(200);
        survivalTime = 0.0f;
        subtitleTimer = 0.0f;
        subtitleLine = -1;
        newRecord = false;
        camera.target = player.position;
    };

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        audio.Update();

        if (state == MENU)
        {
            audio.PlayMenuMusic();

            // Настройка громкости
            if (IsKeyPressed(KEY_LEFT))
            {
                save.volume -= 5; if (save.volume < 0) save.volume = 0;
                if (audio.ready) SetMasterVolume(save.volume / 100.0f);
                SaveGameSave(save);
            }
            if (IsKeyPressed(KEY_RIGHT))
            {
                save.volume += 5; if (save.volume > 100) save.volume = 100;
                if (audio.ready) SetMasterVolume(save.volume / 100.0f);
                SaveGameSave(save);
            }

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            {
                startNewGame();
                state = PLAYING;
                audio.PlayGameMusic();
            }
        }
        else if (state == PLAYING)
        {
            survivalTime += dt;
            player.Update(dt, map);
            spawner.Update(dt, player, map);
            if (player.gotHit) audio.Hit();
            if (spawner.bossEventLine >= 0)
            {
                subtitleLine = spawner.bossEventLine;
                subtitleTimer = 4.0f;
                audio.PlayBossVoice(subtitleLine);
                spawner.bossEventLine = -1;
            }
            weapon.Update(dt, player.position, spawner, orbs, loot);
            if (weapon.firedThisFrame) audio.Shoot();
            orbs.Update(dt, player);
            if (orbs.collectedThisFrame > 0) audio.Pickup();
            loot.Update(dt, player, weapon);
            traps.Update(dt, player);
            camera.target = player.position;

            if (subtitleTimer > 0.0f) subtitleTimer -= dt;

            if (player.health <= 0)
            {
                state = GAME_OVER;
                newRecord = false;
                int t = (int)survivalTime;
                if (t > save.bestTime) { save.bestTime = t; newRecord = true; }
                if (player.level > save.bestLevel) { save.bestLevel = player.level; newRecord = true; }
                if (newRecord) SaveGameSave(save);
            }
            else if (player.TryLevelUp())
            {
                state = LEVEL_UP;
                audio.LevelUp();
            }

            if (IsKeyPressed(KEY_ESCAPE)) state = PAUSED;
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
                if (!player.TryLevelUp()) state = PLAYING;
            }
        }
        else if (state == PAUSED)
        {
            if (IsKeyPressed(KEY_ESCAPE)) state = PLAYING;
        }
        else if (state == GAME_OVER)
        {
            if (IsKeyPressed(KEY_ENTER))
            {
                state = MENU;
                audio.PlayMenuMusic();
            }
        }

        BeginDrawing();
            ClearBackground(BLACK);

            if (state == MENU)
            {
                hud.DrawMenu(save.bestTime, save.bestLevel, save.volume);
            }
            else
            {
                BeginMode2D(camera);
                    map.Draw();
                    traps.Draw();
                    loot.Draw();
                    orbs.Draw();
                    spawner.Draw();
                    weapon.Draw();
                    player.Draw();
                EndMode2D();

                hud.DrawGame(player, spawner, weapon, survivalTime);

                if (subtitleTimer > 0.0f && subtitleLine >= 0)
                {
                    float alpha = (subtitleTimer < 1.0f) ? subtitleTimer : 1.0f;
                    hud.DrawSubtitle(bossSpeakers[subtitleLine], bossLines[subtitleLine], alpha);
                }

                if (state == LEVEL_UP)
                    hud.DrawLevelUp(GetUpgradeText(1), GetUpgradeText(2), GetUpgradeText(3));
                else if (state == PAUSED)
                    hud.DrawPause();
                else if (state == GAME_OVER)
                    hud.DrawGameOver(survivalTime, player.level, save.bestTime, save.bestLevel, newRecord);
            }

            DrawFPS(screenWidth - 90, screenHeight - 28);
        EndDrawing();
    }

    audio.Unload();
    hud.Unload();
    textures.UnloadAll();
    CloseWindow();
    return 0;
}
