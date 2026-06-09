#include "spawner.h"
#include <cmath>

Spawner::Spawner(int poolSize)
    : spawnTimer(0.0f), spawnInterval(2.0f),
      bossTimer(0.0f), bossInterval(30.0f), elapsed(0.0f), bossEventLine(-1)
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
        pos = map.FindFreeSpot(pos, 16.0f);

        EnemyType t = ENEMY_GRUNT;
        int roll = GetRandomValue(0, 99);
        if (elapsed > 60.0f && roll < 20) t = ENEMY_TANK;
        else if (roll < 35) t = ENEMY_FAST;

        e->Spawn(pos, t);
    }
}

void Spawner::SpawnBoss(Vector2 center, const TileMap& map)
{
    Enemy* e = GetInactive();
    if (!e) return;
    Vector2 pos = { center.x + 600.0f, center.y };
    pos = map.FindFreeSpot(pos, 38.0f);
    e->Spawn(pos, ENEMY_BOSS);
    bossEventLine = GetRandomValue(0, 1);  // выбираем реплику босса
}

void Spawner::Update(float deltaTime, Player& player, const TileMap& map)
{
    elapsed += deltaTime;

    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval)
    {
        spawnTimer = 0.0f;
        SpawnWave(player.position, map);
    }

    bossTimer += deltaTime;
    if (bossTimer >= bossInterval)
    {
        bossTimer = 0.0f;
        SpawnBoss(player.position, map);
    }

    for (auto& e : enemies)
    {
        e.Update(deltaTime, player.position, map);
        if (e.active && CheckCollisionRecs(e.GetRect(), player.GetRect()))
            player.TakeDamage(e.damage);
    }
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
