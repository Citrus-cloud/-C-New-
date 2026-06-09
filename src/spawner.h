#pragma once
#include "raylib.h"
#include "enemy.h"
#include "tilemap.h"
#include "player.h"
#include <vector>

class Spawner
{
public:
    std::vector<Enemy> enemies;
    float spawnTimer;
    float spawnInterval;
    float bossTimer;
    float bossInterval;
    float elapsed;
    int bossEventLine;  // -1 = ничего; иначе номер реплики появившегося босса

    Spawner(int poolSize);
    Enemy* GetInactive();
    void SpawnWave(Vector2 center, const TileMap& map);
    void SpawnBoss(Vector2 center, const TileMap& map);
    void Update(float deltaTime, Player& player, const TileMap& map);
    void Draw() const;
    int ActiveCount() const;
};
