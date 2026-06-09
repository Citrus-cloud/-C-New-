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
#include "maps.h"

// Состояния игры. MENU/SELECT/META/SETTINGS — экраны лагеря (Фаза 6).
enum GameState { MENU, SELECT, META, SETTINGS, PLAYING, LEVEL_UP, PAUSED, GAME_OVER };

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Dungeon Survivors: D20 - C++ edition v0.2");
    SetTargetFPS(60);

    AudioManager audio;
    audio.Init();
    HUD hud;
    hud.Init(screenWidth, screenHeight);

    TextureManager textures;

    GameSave save = LoadGameSave();
    audio.SetMusicLevel(save.musicVolume);
    audio.SetSfxLevel(save.sfxVolume);
    if (save.fullscreen) ToggleFullscreen();

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

    // Способности игрока (Шаг 23), выбранный класс и карта (Шаг 24-25).
    Abilities abilities;
    int selectedCharacter = CHAR_WARRIOR;
    int selectedMap = MAP_DUNGEON;
    int settingsRow = 0;        // выбранная строка в настройках (Шаг 27)
    int lastEarnedCoins = 0;    // монеты за последний забег (Шаг 26)

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
        const MapDef& md = GetMap(selectedMap);
        map.Generate(md.width, md.height, (unsigned)GetRandomValue(1, 1000000));
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
        player.speed *= cc.moveSpeedMul;
        weapon.fireInterval *= cc.fireRateMul;
        weapon.damage += cc.bonusDamage;
        if (cc.startWithOrbit) abilities.UnlockOrbit();

        // Применяем мета-прокачку из лагеря (Шаг 26).
        player.maxHealth += save.upgHealth * 20;
        player.speed += save.upgSpeed * 15.0f;
        weapon.damage += save.upgDamage * 3;

        player.health = player.maxHealth;

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
            if (IsKeyPressed(KEY_C)) state = SELECT;
            else if (IsKeyPressed(KEY_U)) state = META;
            else if (IsKeyPressed(KEY_O)) state = SETTINGS;
            else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            {
                startNewGame();
                state = PLAYING;
                audio.PlayGameMusic();
            }
        }
        else if (state == SELECT)
        {
            if (IsKeyPressed(KEY_LEFT))  selectedCharacter = (selectedCharacter + CHAR_COUNT - 1) % CHAR_COUNT;
            if (IsKeyPressed(KEY_RIGHT)) selectedCharacter = (selectedCharacter + 1) % CHAR_COUNT;
            if (IsKeyPressed(KEY_UP))    selectedMap = (selectedMap + MAP_COUNT - 1) % MAP_COUNT;
            if (IsKeyPressed(KEY_DOWN))  selectedMap = (selectedMap + 1) % MAP_COUNT;
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) state = MENU;
        }
        else if (state == META)
        {
            int costH = 20 + save.upgHealth * 15;
            int costD = 20 + save.upgDamage * 15;
            int costS = 20 + save.upgSpeed * 15;
            if (IsKeyPressed(KEY_ONE) && save.coins >= costH) { save.coins -= costH; save.upgHealth++; SaveGameSave(save); }
            if (IsKeyPressed(KEY_TWO) && save.coins >= costD) { save.coins -= costD; save.upgDamage++; SaveGameSave(save); }
            if (IsKeyPressed(KEY_THREE) && save.coins >= costS) { save.coins -= costS; save.upgSpeed++; SaveGameSave(save); }
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) state = MENU;
        }
        else if (state == SETTINGS)
        {
            if (IsKeyPressed(KEY_UP))   settingsRow = (settingsRow + 2) % 3;
            if (IsKeyPressed(KEY_DOWN)) settingsRow = (settingsRow + 1) % 3;
            int delta = 0;
            if (IsKeyPressed(KEY_RIGHT)) delta = 5;
            if (IsKeyPressed(KEY_LEFT))  delta = -5;
            if (settingsRow == 0 && delta != 0)
            {
                save.musicVolume += delta;
                if (save.musicVolume < 0) save.musicVolume = 0;
                if (save.musicVolume > 100) save.musicVolume = 100;
                audio.SetMusicLevel(save.musicVolume);
                SaveGameSave(save);
            }
            else if (settingsRow == 1 && delta != 0)
            {
                save.sfxVolume += delta;
                if (save.sfxVolume < 0) save.sfxVolume = 0;
                if (save.sfxVolume > 100) save.sfxVolume = 100;
                audio.SetSfxLevel(save.sfxVolume);
                SaveGameSave(save);
            }
            else if (settingsRow == 2 && (delta != 0 || IsKeyPressed(KEY_ENTER)))
            {
                save.fullscreen = save.fullscreen ? 0 : 1;
                ToggleFullscreen();
                SaveGameSave(save);
            }
            if (IsKeyPressed(KEY_ESCAPE)) state = MENU;
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
                // Монеты за забег (Шаг 26): за уровень и время.
                lastEarnedCoins = player.level * 5 + (int)(survivalTime / 5.0f);
                save.coins += lastEarnedCoins;
                SaveGameSave(save);
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
                hud.DrawMenu(save.bestTime, save.bestLevel, save.coins);
            }
            else if (state == SELECT)
            {
                ClearBackground(Color{ 15, 12, 20, 255 });
                const char* t = "ВЫБОР ГЕРОЯ И КАРТЫ";
                hud.Text(t, screenWidth / 2.0f - hud.TextWidth(t, 40) / 2.0f, 110, 40, GOLD);

                const CharacterClass& cc = GetCharacter(selectedCharacter);
                const char* ct = TextFormat("Класс:  < %s >", cc.name);
                hud.Text(ct, screenWidth / 2.0f - hud.TextWidth(ct, 30) / 2.0f, 230, 30, RAYWHITE);
                hud.Text(cc.description, screenWidth / 2.0f - hud.TextWidth(cc.description, 20) / 2.0f, 270, 20, LIGHTGRAY);

                const MapDef& md = GetMap(selectedMap);
                const char* mt = TextFormat("Карта:  %s", md.name);
                hud.Text(mt, screenWidth / 2.0f - hud.TextWidth(mt, 30) / 2.0f, 340, 30, RAYWHITE);
                const char* mdesc = TextFormat("%s  (%dx%d)", md.description, md.width, md.height);
                hud.Text(mdesc, screenWidth / 2.0f - hud.TextWidth(mdesc, 20) / 2.0f, 380, 20, LIGHTGRAY);

                const char* h1 = "Влево/Вправо — класс,   Вверх/Вниз — карта";
                hud.Text(h1, screenWidth / 2.0f - hud.TextWidth(h1, 20) / 2.0f, 470, 20, GRAY);
                const char* h2 = "ENTER / ESC — назад в лагерь";
                hud.Text(h2, screenWidth / 2.0f - hud.TextWidth(h2, 20) / 2.0f, 502, 20, GRAY);
            }
            else if (state == META)
            {
                ClearBackground(Color{ 15, 12, 20, 255 });
                const char* t = "ЛАГЕРЬ: МЕТА-ПРОКАЧКА";
                hud.Text(t, screenWidth / 2.0f - hud.TextWidth(t, 38) / 2.0f, 100, 38, GOLD);
                const char* co = TextFormat("Монеты: %d", save.coins);
                hud.Text(co, screenWidth / 2.0f - hud.TextWidth(co, 26) / 2.0f, 165, 26, YELLOW);

                float x = screenWidth / 2.0f - 230.0f;
                int costH = 20 + save.upgHealth * 15;
                int costD = 20 + save.upgDamage * 15;
                int costS = 20 + save.upgSpeed * 15;
                hud.Text(TextFormat("1)  +Здоровье    ур. %d    цена %d", save.upgHealth, costH), x, 250, 24, RAYWHITE);
                hud.Text(TextFormat("2)  +Урон        ур. %d    цена %d", save.upgDamage, costD), x, 295, 24, RAYWHITE);
                hud.Text(TextFormat("3)  +Скорость    ур. %d    цена %d", save.upgSpeed, costS), x, 340, 24, RAYWHITE);

                const char* hint = "Покупай клавишами 1 / 2 / 3.   ESC — назад";
                hud.Text(hint, screenWidth / 2.0f - hud.TextWidth(hint, 20) / 2.0f, 430, 20, GRAY);
            }
            else if (state == SETTINGS)
            {
                ClearBackground(Color{ 15, 12, 20, 255 });
                const char* t = "НАСТРОЙКИ";
                hud.Text(t, screenWidth / 2.0f - hud.TextWidth(t, 40) / 2.0f, 110, 40, GOLD);

                float x = screenWidth / 2.0f - 230.0f;
                Color c0 = (settingsRow == 0) ? GOLD : RAYWHITE;
                Color c1 = (settingsRow == 1) ? GOLD : RAYWHITE;
                Color c2 = (settingsRow == 2) ? GOLD : RAYWHITE;
                hud.Text(TextFormat("Музыка:         %d%%", save.musicVolume), x, 240, 26, c0);
                hud.Text(TextFormat("Звуки:          %d%%", save.sfxVolume), x, 290, 26, c1);
                hud.Text(TextFormat("Полный экран:   %s", save.fullscreen ? "ДА" : "НЕТ"), x, 340, 26, c2);

                const char* hint = "Вверх/Вниз — выбор,  Влево/Вправо — изменить,  ESC — назад";
                hud.Text(hint, screenWidth / 2.0f - hud.TextWidth(hint, 18) / 2.0f, 430, 18, GRAY);
            }
            else
            {
                BeginMode2D(camera);
                    map.Draw(camera);
                    traps.Draw();
                    loot.Draw();
                    orbs.Draw();
                    spawner.Draw(camera, screenWidth, screenHeight);
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
                {
                    hud.DrawGameOver(survivalTime, player.level, save.bestTime, save.bestLevel, newRecord);
                    const char* ce = TextFormat("Заработано монет: %d   (всего: %d)", lastEarnedCoins, save.coins);
                    hud.Text(ce, screenWidth / 2.0f - hud.TextWidth(ce, 22) / 2.0f, 410, 22, GOLD);
                }
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
