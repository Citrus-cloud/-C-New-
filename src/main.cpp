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
#include "effects.h"
#include "abilities.h"
#include "characters.h"
#include "bosses.h"

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

    TextureManager textures;

    GameSave save = LoadGameSave();
    if (audio.ready) SetMasterVolume(save.volume / 100.0f);

    TileMap map;
    map.LoadArt(textures);
    Player player(map.GetSpawnPoint());
    player.LoadSprites(textures);
    Spawner spawner(200);
    spawner.LoadArt(textures);
    Weapon weapon(300);
    ExpOrbs orbs(500);
    Traps traps;
    traps.Generate(map, 16, 777u);
    LootDrops loot(200);

    Effects effects;
    effects.LoadArt(textures);

    // Способности игрока (Шаг 23) и выбранный класс (Шаг 24).
    Abilities abilities;
    int selectedCharacter = CHAR_WARRIOR;

    float survivalTime = 0.0f;
    float subtitleTimer = 0.0f;
    int subtitleLine = -1;
    bool newRecord = false;

    GameState state = MENU;

    Camera2D camera = { 0 };
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    camera.target = player.position;
    Vector2 baseCamOffset = camera.offset;

    auto startNewGame = [&]() {
        map.Generate(96, 72, (unsigned)GetRandomValue(1, 1000000));
        map.LoadArt(textures);
        player = Player(map.GetSpawnPoint());
        player.LoadSprites(textures);
        spawner = Spawner(200);
        spawner.LoadArt(textures);
        weapon = Weapon(300);
        orbs = ExpOrbs(500);
        traps = Traps();
        traps.Generate(map, 16, (unsigned)GetRandomValue(1, 1000000));
        loot = LootDrops(200);
        effects.Clear();
        abilities.Reset();
        ResetUpgrades();

        // Применяем выбранный класс (Шаг 24).
        const CharacterClass& cc = GetCharacter(selectedCharacter);
        player.maxHealth += cc.bonusHealth;
        player.health = player.maxHealth;
        player.speed *= cc.moveSpeedMul;
        weapon.fireInterval *= cc.fireRateMul;
        weapon.damage += cc.bonusDamage;
        if (cc.startWithOrbit) abilities.UnlockOrbit();

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
        effects.Update(dt);

        if (state == MENU)
        {
            audio.PlayMenuMusic();

            if (IsKeyPressed(KEY_TAB))
                selectedCharacter = (selectedCharacter + 1) % CHAR_COUNT;

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
            if (player.gotHit)
            {
                audio.Hit();
                effects.Shake(7.0f, 0.25f);
                effects.Flash(Color{ 255, 60, 60, 255 }, 0.45f);
                effects.SpawnBlood(player.position, 8);
            }
            if (spawner.bossEventLine >= 0)
            {
                subtitleLine = spawner.bossEventLine;
                subtitleTimer = 4.0f;
                audio.PlayBossVoice(subtitleLine);
                spawner.bossEventLine = -1;
            }
            weapon.Update(dt, player.position, spawner, orbs, loot, effects);
            abilities.Update(dt, player.position, spawner, orbs, loot, effects);
            if (weapon.firedThisFrame) audio.Shoot();
            orbs.Update(dt, player);
            if (orbs.collectedThisFrame > 0) audio.Pickup();
            loot.Update(dt, player, weapon);
            traps.Update(dt, player);
            camera.target = player.position;
            camera.offset = { baseCamOffset.x + effects.ShakeOffset().x,
                              baseCamOffset.y + effects.ShakeOffset().y };

            if (subtitleTimer > 0.0f) subtitleTimer -= dt;

            if (player.health <= 0)
            {
                state = GAME_OVER;
                newRecord = false;
                int t = (int)survivalTime;
                if (t > save.bestTime) { save.bestTime = t; newRecord = true; }
                if (player.level > save.bestLevel) { save.bestLevel = player.level; newRecord = true; }
                if (newRecord) SaveGameSave(save);
                effects.Shake(14.0f, 0.6f);
                effects.SetFade(0.65f, 1.5f);
            }
            else if (player.TryLevelUp())
            {
                state = LEVEL_UP;
                audio.LevelUp();
                effects.SpawnMagicCircle(player.position, 1.6f);
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
                ApplyUpgrade(choice, player, weapon, abilities);
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
                effects.SetFade(0.0f, 2.0f);
                audio.PlayMenuMusic();
            }
        }

        BeginDrawing();
            ClearBackground(BLACK);

            if (state == MENU)
            {
                hud.DrawMenu(save.bestTime, save.bestLevel, save.volume);
                const CharacterClass& cc = GetCharacter(selectedCharacter);
                hud.Text(TextFormat("Класс: %s — %s   (TAB — сменить)", cc.name, cc.description),
                         screenWidth / 2.0f - 300.0f, screenHeight - 70.0f, 24.0f, RAYWHITE);
            }
            else
            {
                BeginMode2D(camera);
                    map.Draw(camera);
                    traps.Draw();
                    loot.Draw();
                    orbs.Draw();
                    spawner.Draw();
                    weapon.Draw();
                    player.Draw();
                    abilities.Draw(player.position);
                    effects.DrawWorld();
                EndMode2D();

                effects.DrawScreen(screenWidth, screenHeight);

                hud.DrawGame(player, spawner, weapon, survivalTime);

                if (subtitleTimer > 0.0f && subtitleLine >= 0)
                {
                    float alpha = (subtitleTimer < 1.0f) ? subtitleTimer : 1.0f;
                    const BossDef& bd = GetBossDef(subtitleLine);
                    hud.DrawSubtitle(bd.name, bd.line, alpha);
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
