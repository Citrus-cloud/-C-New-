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
#include "tuning.h"     // единый конфиг боя (Фаза 1)
#include "director.h"   // менеджер волн и доступности приёмов (Фаза 1)
#include "telegraph.h"  // система телеграфов-предупреждений (Фаза 2)
#include "ranged.h"     // система дальних атак / снарядов врагов (Фаза 3)
#include "hazards.h"    // система опасных зон/луж (Фаза 5)

// Состояния игры. MENU/SELECT/META/SETTINGS — экраны лагеря (Фаза 6).
enum GameState { MENU, SELECT, META, SETTINGS, PLAYING, LEVEL_UP, PAUSED, GAME_OVER };

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Dungeon Survivors: D20 - C++ edition v0.3");
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
    WaveDirector director;   // менеджер волн и доступности приёмов (Фаза 1)
    TelegraphSystem telegraphs(64);   // система предупреждающих зон (Фаза 2)
    RangedSystem ranged(256);         // пул снарядов врагов (Фаза 3)
    spawner.SetTelegraphs(&telegraphs);   // враги «заказывают» зоны через спавнер
    spawner.SetRanged(&ranged);           // и выпускают снаряды через него (Фаза 3)
    HazardSystem hazards(Tuning::kHazardPoolSize);   // пул опасных луж (Фаза 5)
    spawner.SetHazards(&hazards);          // ядовитый след врагов (Фаза 5)
    Weapon weapon(300);
    spawner.SetWeapon(&weapon);            // враги-«уклонисты» читают снаряды игрока (Фаза 6, Шаг 33)
    ExpOrbs orbs(500);
    Traps traps;
    traps.Generate(map, 16, 777u);
    LootDrops loot(200);

    Effects effects;
    effects.LoadArt(textures);
    spawner.SetEffects(&effects);   // эффекты для приёмов мобильности (Фаза 4)

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
    bool showDebug = false;     // отладочный показ правил конфига (клавиша F3, Фаза 1)
    bool showOverlay = false;   // справочный оверлей: сводка забега и управление (клавиша F1, Шаг 35)
    bool showTune = false;      // панель живой настройки баланса (клавиша F8, Шаг 36)
    int  tuneRow = 0;           // выбранная строка панели настройки баланса (Шаг 36)

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
        director.Reset();   // сброс времени забега и кулдаунов (Фаза 1)
        telegraphs.Clear();                 // очистка предупреждающих зон (Фаза 2)
        ranged.Clear();                     // очистка снарядов врагов (Фаза 3)
        hazards.Clear();                    // очистка опасных луж (Фаза 5)
        spawner.SetTelegraphs(&telegraphs); // спавнер пересоздан — заново привязываем телеграфы
        spawner.SetRanged(&ranged);         // и систему снарядов (Фаза 3)
        spawner.SetEffects(&effects);       // и систему эффектов (Фаза 4)
        spawner.SetHazards(&hazards);       // и систему опасных зон (Фаза 5)
        spawner.SetWeapon(&weapon);         // и оружие игрока для уклонения (Фаза 6, Шаг 33)
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
            if (IsKeyPressed(KEY_F1)) showOverlay = !showOverlay;   // справочный оверлей (Шаг 35)
            if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;   // отладка: показать правила конфига
            if (IsKeyPressed(KEY_F4))   // тест: вручную заказать круговую зону под игроком (Фаза 2)
                telegraphs.SpawnCircle(player.position, 120.0f, 15, Tuning::kTelegraphDefaultFill, ORANGE);
            if (IsKeyPressed(KEY_F5))   // тест: залп снарядов по игроку из точки выше (Фаза 3)
                ranged.FireVolley(Vector2{ player.position.x, player.position.y - 300.0f },
                                  player.position, Tuning::kVolleyCount,
                                  Tuning::kVolleySpread, Tuning::kVolleyDamage, PURPLE);
            if (IsKeyPressed(KEY_F6))   // тест: спавн врагов со всеми приёмами мобильности (Фаза 4)
                spawner.SpawnMobilityTest(player.position, map);
            if (IsKeyPressed(KEY_F7))   // тест: спавн врагов со всеми особыми способностями (Фаза 5)
                spawner.SpawnSpecialTest(player.position, map);
            if (IsKeyPressed(KEY_F8)) showTune = !showTune;   // панель живой настройки баланса (Шаг 36)
            if (showTune)   // [ / ] — изменить значение, PgUp/PgDn — выбрать строку
            {
                if (IsKeyPressed(KEY_PAGE_DOWN)) tuneRow = (tuneRow + 1) % 6;
                if (IsKeyPressed(KEY_PAGE_UP))   tuneRow = (tuneRow + 5) % 6;
                int tdir = 0;
                if (IsKeyPressed(KEY_RIGHT_BRACKET)) tdir = 1;
                if (IsKeyPressed(KEY_LEFT_BRACKET))  tdir = -1;
                if (tdir != 0)
                {
                    if (tuneRow == 0) { weapon.damage += tdir; if (weapon.damage < 1) weapon.damage = 1; }
                    else if (tuneRow == 1) { weapon.fireInterval += tdir * 0.02f; if (weapon.fireInterval < 0.05f) weapon.fireInterval = 0.05f; }
                    else if (tuneRow == 2) { weapon.projectileCount += tdir; if (weapon.projectileCount < 1) weapon.projectileCount = 1; }
                    else if (tuneRow == 3) { weapon.pierce += tdir; if (weapon.pierce < 0) weapon.pierce = 0; }
                    else if (tuneRow == 4) { player.speed += tdir * 10.0f; if (player.speed < 10.0f) player.speed = 10.0f; }
                    else if (tuneRow == 5) { player.maxHealth += tdir * 10.0f; if (player.maxHealth < 10.0f) player.maxHealth = 10.0f; if (player.health > player.maxHealth) player.health = player.maxHealth; }
                }
            }
            survivalTime += dt;
            director.Update(dt);   // продвигаем время забега (Фаза 1)
            player.Update(dt, map);
            spawner.Update(dt, player, map);
            telegraphs.Update(dt, player, effects);   // продвигаем зоны и наносим урон (Фаза 2)
            ranged.Update(dt, player, effects);       // движение снарядов и урон (Фаза 3)
            hazards.Update(dt, player);               // тик урона от ядовитых луж (Фаза 5)
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
                const char* t = "\u0412\u042b\u0411\u041e\u0420 \u0413\u0415\u0420\u041e\u042f \u0418 \u041a\u0410\u0420\u0422\u042b";
                hud.Text(t, screenWidth / 2.0f - hud.TextWidth(t, 40) / 2.0f, 110, 40, GOLD);

                const CharacterClass& cc = GetCharacter(selectedCharacter);
                const char* ct = TextFormat("\u041a\u043b\u0430\u0441\u0441:  < %s >", cc.name);
                hud.Text(ct, screenWidth / 2.0f - hud.TextWidth(ct, 30) / 2.0f, 230, 30, RAYWHITE);
                hud.Text(cc.description, screenWidth / 2.0f - hud.TextWidth(cc.description, 20) / 2.0f, 270, 20, LIGHTGRAY);

                const MapDef& md = GetMap(selectedMap);
                const char* mt = TextFormat("\u041a\u0430\u0440\u0442\u0430:  %s", md.name);
                hud.Text(mt, screenWidth / 2.0f - hud.TextWidth(mt, 30) / 2.0f, 340, 30, RAYWHITE);
                const char* mdesc = TextFormat("%s  (%dx%d)", md.description, md.width, md.height);
                hud.Text(mdesc, screenWidth / 2.0f - hud.TextWidth(mdesc, 20) / 2.0f, 380, 20, LIGHTGRAY);

                const char* h1 = "\u0412\u043b\u0435\u0432\u043e/\u0412\u043f\u0440\u0430\u0432\u043e \u2014 \u043a\u043b\u0430\u0441\u0441,   \u0412\u0432\u0435\u0440\u0445/\u0412\u043d\u0438\u0437 \u2014 \u043a\u0430\u0440\u0442\u0430";
                hud.Text(h1, screenWidth / 2.0f - hud.TextWidth(h1, 20) / 2.0f, 470, 20, GRAY);
                const char* h2 = "ENTER / ESC \u2014 \u043d\u0430\u0437\u0430\u0434 \u0432 \u043b\u0430\u0433\u0435\u0440\u044c";
                hud.Text(h2, screenWidth / 2.0f - hud.TextWidth(h2, 20) / 2.0f, 502, 20, GRAY);
            }
            else if (state == META)
            {
                ClearBackground(Color{ 15, 12, 20, 255 });
                const char* t = "\u041b\u0410\u0413\u0415\u0420\u042c: \u041c\u0415\u0422\u0410-\u041f\u0420\u041e\u041a\u0410\u0427\u041a\u0410";
                hud.Text(t, screenWidth / 2.0f - hud.TextWidth(t, 38) / 2.0f, 100, 38, GOLD);
                const char* co = TextFormat("\u041c\u043e\u043d\u0435\u0442\u044b: %d", save.coins);
                hud.Text(co, screenWidth / 2.0f - hud.TextWidth(co, 26) / 2.0f, 165, 26, YELLOW);

                float x = screenWidth / 2.0f - 230.0f;
                int costH = 20 + save.upgHealth * 15;
                int costD = 20 + save.upgDamage * 15;
                int costS = 20 + save.upgSpeed * 15;
                hud.Text(TextFormat("1)  +\u0417\u0434\u043e\u0440\u043e\u0432\u044c\u0435    \u0443\u0440. %d    \u0446\u0435\u043d\u0430 %d", save.upgHealth, costH), x, 250, 24, RAYWHITE);
                hud.Text(TextFormat("2)  +\u0423\u0440\u043e\u043d        \u0443\u0440. %d    \u0446\u0435\u043d\u0430 %d", save.upgDamage, costD), x, 295, 24, RAYWHITE);
                hud.Text(TextFormat("3)  +\u0421\u043a\u043e\u0440\u043e\u0441\u0442\u044c    \u0443\u0440. %d    \u0446\u0435\u043d\u0430 %d", save.upgSpeed, costS), x, 340, 24, RAYWHITE);

                const char* hint = "\u041f\u043e\u043a\u0443\u043f\u0430\u0439 \u043a\u043b\u0430\u0432\u0438\u0448\u0430\u043c\u0438 1 / 2 / 3.   ESC \u2014 \u043d\u0430\u0437\u0430\u0434";
                hud.Text(hint, screenWidth / 2.0f - hud.TextWidth(hint, 20) / 2.0f, 430, 20, GRAY);
            }
            else if (state == SETTINGS)
            {
                ClearBackground(Color{ 15, 12, 20, 255 });
                const char* t = "\u041d\u0410\u0421\u0422\u0420\u041e\u0419\u041a\u0418";
                hud.Text(t, screenWidth / 2.0f - hud.TextWidth(t, 40) / 2.0f, 110, 40, GOLD);

                float x = screenWidth / 2.0f - 230.0f;
                Color c0 = (settingsRow == 0) ? GOLD : RAYWHITE;
                Color c1 = (settingsRow == 1) ? GOLD : RAYWHITE;
                Color c2 = (settingsRow == 2) ? GOLD : RAYWHITE;
                hud.Text(TextFormat("\u041c\u0443\u0437\u044b\u043a\u0430:         %d%%", save.musicVolume), x, 240, 26, c0);
                hud.Text(TextFormat("\u0417\u0432\u0443\u043a\u0438:          %d%%", save.sfxVolume), x, 290, 26, c1);
                hud.Text(TextFormat("\u041f\u043e\u043b\u043d\u044b\u0439 \u044d\u043a\u0440\u0430\u043d:   %s", save.fullscreen ? "\u0414\u0410" : "\u041d\u0415\u0422"), x, 340, 26, c2);

                const char* hint = "\u0412\u0432\u0435\u0440\u0445/\u0412\u043d\u0438\u0437 \u2014 \u0432\u044b\u0431\u043e\u0440,  \u0412\u043b\u0435\u0432\u043e/\u0412\u043f\u0440\u0430\u0432\u043e \u2014 \u0438\u0437\u043c\u0435\u043d\u0438\u0442\u044c,  ESC \u2014 \u043d\u0430\u0437\u0430\u0434";
                hud.Text(hint, screenWidth / 2.0f - hud.TextWidth(hint, 18) / 2.0f, 430, 18, GRAY);
            }
            else
            {
                BeginMode2D(camera);
                    map.Draw(camera);
                    traps.Draw();
                    telegraphs.Draw();   // предупреждающие зоны — по земле, под сущностями (Фаза 2)
                    hazards.Draw();      // ядовитые лужи — по земле, под сущностями (Фаза 5)
                    loot.Draw();
                    orbs.Draw();
                    spawner.Draw(camera, screenWidth, screenHeight);
                    ranged.Draw();       // снаряды врагов поверх врагов (Фаза 3)
                    weapon.Draw();
                    player.Draw();
                    abilities.Draw(player.position);
                    effects.DrawWorld();
                    if (showDebug) telegraphs.DrawDebug();   // отладка: контуры зон (Фаза 2)
                    if (showDebug) ranged.DrawDebug();       // отладка: радиусы снарядов (Фаза 3)
                    if (showDebug) hazards.DrawDebug();       // отладка: радиусы луж (Фаза 5)
                EndMode2D();

                effects.DrawScreen(screenWidth, screenHeight);

                hud.DrawGame(player, spawner, weapon, survivalTime);

                // Справочный оверлей (F1, Шаг 35): сводка забега и управление в одной панели.
                // Кириллический текст — отрисовывается игровым шрифтом (Шаг 37).
                if (showOverlay)
                {
                    float ox = screenWidth - 380.0f, oy = 80.0f;
                    float ow = 360.0f, oh = 344.0f;
                    DrawRectangle((int)ox, (int)oy, (int)ow, (int)oh, Color{ 10, 10, 16, 210 });
                    DrawRectangleLines((int)ox, (int)oy, (int)ow, (int)oh, Color{ 120, 200, 255, 255 });
                    float tx = ox + 18.0f, ty = oy + 14.0f;
                    hud.Text("\u0418\u041d\u0424\u041e  (F1 \u2014 \u0437\u0430\u043a\u0440\u044b\u0442\u044c)", tx, ty, 22, Color{ 120, 200, 255, 255 }); ty += 34.0f;
                    hud.Text(TextFormat("\u0412\u0440\u0435\u043c\u044f:   %.1f \u0441", survivalTime), tx, ty, 18, RAYWHITE); ty += 24.0f;
                    hud.Text(TextFormat("\u0423\u0440\u043e\u0432\u0435\u043d\u044c: %d", (int)player.level), tx, ty, 18, RAYWHITE); ty += 24.0f;
                    hud.Text(TextFormat("HP:      %d / %d", (int)player.health, (int)player.maxHealth), tx, ty, 18, RAYWHITE); ty += 24.0f;
                    hud.Text(TextFormat("\u041e\u0440\u0443\u0436\u0438\u0435:  \u0443\u0440.%d  \u0443\u0440\u043e\u043d %d  x%d  \u043f\u0440\u043e\u0431\u043e\u0439 %d%s",
                        weapon.level, weapon.damage, weapon.projectileCount, weapon.pierce,
                        weapon.evolved ? "  [\u042d\u0412\u041e]" : ""), tx, ty, 18, RAYWHITE); ty += 24.0f;
                    hud.Text(TextFormat("\u041c\u043e\u043d\u0435\u0442\u044b: %d", save.coins), tx, ty, 18, GOLD); ty += 30.0f;
                    hud.Text("\u0423\u043f\u0440\u0430\u0432\u043b\u0435\u043d\u0438\u0435:", tx, ty, 18, Color{ 120, 200, 255, 255 }); ty += 24.0f;
                    hud.Text("WASD / \u0441\u0442\u0440\u0435\u043b\u043a\u0438 \u2014 \u0434\u0432\u0438\u0436\u0435\u043d\u0438\u0435", tx, ty, 16, LIGHTGRAY); ty += 21.0f;
                    hud.Text("\u041f\u0440\u043e\u0431\u0435\u043b / Shift \u2014 \u0440\u044b\u0432\u043e\u043a", tx, ty, 16, LIGHTGRAY); ty += 21.0f;
                    hud.Text("ESC \u2014 \u043f\u0430\u0443\u0437\u0430", tx, ty, 16, LIGHTGRAY); ty += 21.0f;
                    hud.Text("F1 \u2014 \u044d\u0442\u043e\u0442 \u043e\u0432\u0435\u0440\u043b\u0435\u0439", tx, ty, 16, LIGHTGRAY); ty += 21.0f;
                    hud.Text("F3 \u2014 \u043e\u0442\u043b\u0430\u0434\u043a\u0430", tx, ty, 16, LIGHTGRAY); ty += 21.0f;
                    hud.Text("F8 \u2014 \u043d\u0430\u0441\u0442\u0440\u043e\u0439\u043a\u0430 \u0431\u0430\u043b\u0430\u043d\u0441\u0430", tx, ty, 16, LIGHTGRAY);
                }

                // Панель живой настройки баланса (F8, Шаг 36): меняем ключевые параметры
                // прямо в бою и сразу видим эффект. Кириллица — игровым шрифтом (Шаг 37).
                if (showTune)
                {
                    float px = 16.0f, py = screenHeight - 252.0f;
                    float pw = 380.0f, ph = 234.0f;
                    DrawRectangle((int)px, (int)py, (int)pw, (int)ph, Color{ 16, 10, 10, 215 });
                    DrawRectangleLines((int)px, (int)py, (int)pw, (int)ph, Color{ 255, 180, 90, 255 });
                    float tx = px + 16.0f, ty = py + 12.0f;
                    hud.Text("\u041d\u0410\u0421\u0422\u0420\u041e\u0419\u041a\u0410 (F8)   [ ] \u2014 \u0438\u0437\u043c\u0435\u043d\u0438\u0442\u044c   PgUp/Dn \u2014 \u0441\u0442\u0440\u043e\u043a\u0430", tx, ty, 16, Color{ 255, 180, 90, 255 }); ty += 30.0f;
                    hud.Text(TextFormat("%c \u0423\u0440\u043e\u043d \u043e\u0440\u0443\u0436\u0438\u044f:  %d", tuneRow == 0 ? '>' : ' ', weapon.damage), tx, ty, 18, tuneRow == 0 ? GOLD : RAYWHITE); ty += 26.0f;
                    hud.Text(TextFormat("%c \u0418\u043d\u0442\u0435\u0440\u0432\u0430\u043b \u043e\u0433\u043d\u044f: %.2f", tuneRow == 1 ? '>' : ' ', weapon.fireInterval), tx, ty, 18, tuneRow == 1 ? GOLD : RAYWHITE); ty += 26.0f;
                    hud.Text(TextFormat("%c \u0421\u043d\u0430\u0440\u044f\u0434\u043e\u0432:     %d", tuneRow == 2 ? '>' : ' ', weapon.projectileCount), tx, ty, 18, tuneRow == 2 ? GOLD : RAYWHITE); ty += 26.0f;
                    hud.Text(TextFormat("%c \u041f\u0440\u043e\u0431\u043e\u0439:       %d", tuneRow == 3 ? '>' : ' ', weapon.pierce), tx, ty, 18, tuneRow == 3 ? GOLD : RAYWHITE); ty += 26.0f;
                    hud.Text(TextFormat("%c \u0421\u043a\u043e\u0440\u043e\u0441\u0442\u044c:     %.0f", tuneRow == 4 ? '>' : ' ', player.speed), tx, ty, 18, tuneRow == 4 ? GOLD : RAYWHITE); ty += 26.0f;
                    hud.Text(TextFormat("%c \u041c\u0430\u043a\u0441. HP:     %.0f", tuneRow == 5 ? '>' : ' ', player.maxHealth), tx, ty, 18, tuneRow == 5 ? GOLD : RAYWHITE); ty += 26.0f;
                }

                // Отладочный оверлей (F3): текущие правила конфига и статус приёмов (Шаг 5).
                // Статусы кириллицей (Шаг 37); имена приёмов — внутренние, латиницей.
                if (showDebug)
                {
                    float dx = 16.0f, dy = 90.0f;
                    hud.Text(TextFormat("\u041e\u0422\u041b\u0410\u0414\u041a\u0410 F3  t=%.1f  \u0432\u043e\u043b\u043d\u0430=%d  \u0441\u043f\u0430\u0432\u043d=%.2f  TG=%d  PROJ=%d",
                        director.elapsed, director.WaveCount(), director.SpawnInterval(),
                        telegraphs.ActiveCount(), ranged.ActiveCount()), dx, dy, 18, LIME);
                    dy += 26.0f;
                    for (int i = 0; i < Tuning::ABILITY_COUNT; i++)
                    {
                        Tuning::AbilityId id = (Tuning::AbilityId)i;
                        const Tuning::AbilityRule& r = Tuning::GetRule(id);
                        const char* status;
                        Color col;
                        if (!director.IsUnlocked(id)) { status = TextFormat("\u0417\u0410\u041a\u0420\u042b\u0422 \u043e\u0442\u043a\u0440=%.0f\u0441", r.unlockTime); col = GRAY; }
                        else if (director.CanUse(id)) { status = "\u0413\u041e\u0422\u041e\u0412"; col = LIME; }
                        else { status = TextFormat("\u041a\u0414 %.1f\u0441", director.CooldownLeft(id)); col = ORANGE; }
                        hud.Text(TextFormat("%-9s %s", r.name, status), dx, dy, 16, col);
                        dy += 19.0f;
                    }
                }

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
                    const char* ce = TextFormat("\u0417\u0430\u0440\u0430\u0431\u043e\u0442\u0430\u043d\u043e \u043c\u043e\u043d\u0435\u0442: %d   (\u0432\u0441\u0435\u0433\u043e: %d)", lastEarnedCoins, save.coins);
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
