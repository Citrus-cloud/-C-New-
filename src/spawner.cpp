#include "spawner.h"
#include <cmath>

Spawner::Spawner(int poolSize) : spawnTimer(0.0f), spawnInterval(2.0f)
{
    enemies.resize(poolSize);
}

Enemy* Spawner::GetInactive()
{
    for (auto& e : enemies)
        if (!e.active) return &e;
    return nullptr;
}

void Spawner::SpawnWave(Vector2 center, const TileMap& map)
{
    int count = 8;
    for (int i = 0; i < count; i++)
    {
        Enemy* e = GetInactive();
        if (!e) break;
        float angle = (float)i / count * 2.0f * PI;
        float r = 500.0f;
        Vector2 pos = { center.x + cosf(angle) * r, center.y + sinf(angle) * r };
        pos = map.FindFreeSpot(pos, 16.0f);  // не спавним внутри стены
        e->Spawn(pos);
    }
}

void Spawner::Update(float deltaTime, Vector2 playerPos, const TileMap& map)
{
    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval)
    {
        spawnTimer = 0.0f;
        SpawnWave(playerPos, map);
    }
    for (auto& e : enemies)
        e.Update(deltaTime, playerPos, map);
}

void Spawner::Draw() const
{
    for (auto& e : enemies)
        e.Draw();
}

int Spawner::ActiveCount() const
{
    int c = 0;
    for (auto& e : enemies)
        if (e.active) c++;
    return c;
}
