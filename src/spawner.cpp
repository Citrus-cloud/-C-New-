#include "spawner.h"
#include <cmath>

Spawner::Spawner(int maxEnemies)
    : spawnTimer(0.0f), spawnInterval(2.0f)
{
    pool.resize(maxEnemies);  // создаём весь пул сразу (все неактивны)
}

Enemy* Spawner::GetInactive()
{
    // Ищем первого свободного врага и возвращаем указатель на него
    for (Enemy& e : pool)
    {
        if (!e.active) return &e;
    }
    return nullptr;  // свободных нет
}

void Spawner::SpawnWave(int count, Vector2 center)
{
    for (int i = 0; i < count; i++)
    {
        Enemy* e = GetInactive();
        if (e == nullptr) break;  // пул заполнен - выходим

        // Спавним по кругу вокруг игрока
        float angle = (float)GetRandomValue(0, 359) * DEG2RAD;
        float dist = (float)GetRandomValue(400, 700);
        Vector2 pos = {
            center.x + cosf(angle) * dist,
            center.y + sinf(angle) * dist
        };
        e->Spawn(pos);  // -> у указателя вызываем метод
    }
}

void Spawner::Update(float deltaTime, Vector2 playerPos)
{
    // Копим время; как только набрали spawnInterval - новая волна
    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval)
    {
        spawnTimer = 0.0f;
        SpawnWave(5, playerPos);
    }

    for (Enemy& e : pool)
        e.Update(deltaTime, playerPos);
}

void Spawner::Draw() const
{
    for (const Enemy& e : pool)
        e.Draw();
}

int Spawner::ActiveCount() const
{
    int count = 0;
    for (const Enemy& e : pool)
        if (e.active) count++;
    return count;
}
