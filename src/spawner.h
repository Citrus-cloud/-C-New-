#pragma once
#include "raylib.h"
#include "enemy.h"
#include "tilemap.h"
#include "player.h"
#include "animation.h"
#include <vector>

class TextureManager;
class TelegraphSystem;   // система телеграфов (Фаза 2)
class RangedSystem;      // система дальних атак / снарядов (Фаза 3)
class Effects;           // визуальные эффекты (Фаза 4: телепорт, приземление)
class HazardSystem;      // система опасных зон/луж (Фаза 5, ядовитый след)
class Weapon;            // оружие игрока (Фаза 6, Шаг 33): враги читают его снаряды для уклонения

class Spawner
{
public:
    std::vector<Enemy> enemies;
    float spawnTimer;
    float spawnInterval;
    float bossTimer;
    float bossInterval;
    float elapsed;
    int bossEventLine;
    int bossSpawnCount;   // сколько боссов уже вызвано (для чередования)
    float telegraphTimer; // таймер демо-телеграфа босса (Фаза 2)

    // --- Дальние атаки (Фаза 3) ---
    float shooterTimer;     // общий таймер выстрелов врагов-стрелков
    float bossRangedTimer;  // таймер дальних атак босса (лазер/залп)
    int   bossRangedPattern; // чередование паттернов босса (0=лазер, 1=залп)

    Spawner(int poolSize);
    void LoadArt(TextureManager& textures);
    void SetTelegraphs(TelegraphSystem* t);   // привязка системы телеграфов (Фаза 2)
    void SetRanged(RangedSystem* r);           // привязка системы снарядов (Фаза 3)
    void SetEffects(Effects* e);               // привязка системы эффектов (Фаза 4)
    void SetHazards(HazardSystem* h);           // привязка системы опасных зон (Фаза 5)
    void SetWeapon(Weapon* w);                  // привязка оружия игрока для уклонения от снарядов (Шаг 33)
    Enemy* GetInactive();
    void SpawnWave(Vector2 center, const TileMap& map);
    void SpawnBoss(Vector2 center, const TileMap& map);
    void SpawnMobilityTest(Vector2 center, const TileMap& map);   // отладка приёмов мобильности (F6, Шаг 22)
    void SpawnSpecialTest(Vector2 center, const TileMap& map);     // отладка особых способностей (F7, Шаг 28)
    void Update(float deltaTime, Player& player, const TileMap& map);
    void Draw(Camera2D camera, int screenW, int screenH) const;   // culling (Шаг 28)
    int ActiveCount() const;

private:
    Animation artGruntWalk, artFastWalk, artTankWalk, artBossWalk;
    Animation artSpiderWalk, artKnightWalk;   // спрайты конкретных боссов
    Animation artEnemyDeath, artBossDeath;
    TelegraphSystem* telegraphs;   // куда враги «заказывают» зоны (может быть nullptr)
    RangedSystem* ranged;          // куда враги выпускают снаряды (может быть nullptr)
    Effects* effects;              // визуальные эффекты приёмов мобильности (может быть nullptr)
    HazardSystem* hazards;         // куда враги роняют ядовитые лужи (может быть nullptr)
    Weapon* weapon;                // оружие игрока: источник снарядов для уклонения (может быть nullptr)
    void AssignArt(Enemy* e, EnemyType t);
    void MaybeAssignMobility(Enemy* e);   // назначить приём мобильности при спавне (Фаза 4)
    void MaybeAssignSpecial(Enemy* e);    // назначить особую способность при спавне (Фаза 5)
    void MaybeAssignElite(Enemy* e);      // назначить элитный модификатор при спавне (Фаза 6, Шаг 32)
    void MaybeAssignAI(Enemy* e);         // назначить улучшенный ИИ (уклонение от снарядов) при спавне (Шаг 33)
    void ApplySeparation(const TileMap& map, float dt);      // групповое расталкивание врагов (Шаг 33)
    void ApplyProjectileDodge(const TileMap& map, float dt);  // уклонение врагов от снарядов игрока (Шаг 33)
};
