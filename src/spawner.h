#pragma once
#include "raylib.h"
#include "enemy.h"
#include "tilemap.h"
#include "player.h"
#include "animation.h"
#include <vector>

class TextureManager;

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

    Spawner(int poolSize);
    void LoadArt(TextureManager& textures);
    Enemy* GetInactive();
    void SpawnWave(Vector2 center, const TileMap& map);
    void SpawnBoss(Vector2 center, const TileMap& map);
    void Update(float deltaTime, Player& player, const TileMap& map);
    void Draw() const;
    int ActiveCount() const;

private:
    Animation artGruntWalk, artFastWalk, artTankWalk, artBossWalk;
    Animation artSpiderWalk, artKnightWalk;   // спрайты конкретных боссов
    Animation artEnemyDeath, artBossDeath;
    void AssignArt(Enemy* e, EnemyType t);
};
