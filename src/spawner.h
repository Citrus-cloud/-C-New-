#pragma once
#include "raylib.h"
#include "enemy.h"
#include <vector>

// Спавнер с пулом объектов: заранее создаём N врагов и переиспользуем их.
class Spawner
{
public:
    std::vector<Enemy> pool;  // пул врагов
    float spawnTimer;
    float spawnInterval;      // раз в сколько секунд волна

    Spawner(int maxEnemies);

    void Update(float deltaTime, Vector2 playerPos);
    void Draw() const;
    void SpawnWave(int count, Vector2 center);
    Enemy* GetInactive();      // найти свободного врага в пуле
    int ActiveCount() const;   // сколько врагов сейчас активно
};
