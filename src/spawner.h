#pragma once
#include "raylib.h"
#include "enemy.h"
#include "tilemap.h"
#include "player.h"
#include "animation.h"
#include <vector>

class TextureManager;
class TelegraphSystem;   // система телеграфов (Фаза 2)

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

    Spawner(int poolSize);
    void LoadArt(TextureManager& textures);
    void SetTelegraphs(TelegraphSystem* t);   // привязка системы телеграфов (Фаза 2)
    Enemy* GetInactive();
    void SpawnWave(Vector2 center, const TileMap& map);
    void SpawnBoss(Vector2 center, const TileMap& map);
    void Update(float deltaTime, Player& player, const TileMap& map);
    void Draw(Camera2D camera, int screenW, int screenH) const;   // culling (Шаг 28)
    int ActiveCount() const;

private:
    Animation artGruntWalk, artFastWalk, artTankWalk, artBossWalk;
    Animation artSpiderWalk, artKnightWalk;   // спрайты конкретных боссов
    Animation artEnemyDeath, artBossDeath;
    TelegraphSystem* telegraphs;   // куда враги «заказывают» зоны (может быть nullptr)
    void AssignArt(Enemy* e, EnemyType t);
};
