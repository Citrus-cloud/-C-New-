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
            if (IsKeyPressed(KEY_ESC