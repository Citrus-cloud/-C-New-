#pragma once
#include "raylib.h"
#include "enemy.h"
#include "tilemap.h"
#include <vector>

class Spawner
{
public:
    std::vector<Enemy> enemies;
    float spawnTimer;
    float spawnInterval;

    Spawner(int poolSize);
    Enemy* GetInactive();
    void SpawnWave(Vector2 center, const TileMap& map);
    void Update(float deltaTime, Vector2 playerPos, const TileMap& map);
    void Draw() const;
    int ActiveCount() const;
};
