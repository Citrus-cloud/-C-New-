#include "spawner.h"
#include "textures.h"
#include <cmath>

Spawner::Spawner(int poolSize)
    : spawnTimer(0.0f), spawnInterval(2.0f),
      bossTimer(0.0f), bossInterval(30.0f), elapsed(0.0f), bossEventLine(-1)
{
    enemies.resize(poolSize);
}

// Загружает прототипы анимаций врагов и боссов (Шаги 6-8).
// Если файла нет — анимация остаётся пустой, и враг рисуется квадратом.
void Spawner::LoadArt(TextureManager& tex)
{
    if (tex.IsReal("assets/sprites/enemy_grunt.png"))
        artGruntWalk = Animation(tex.Get("assets/sprites/enemy_grunt.png"), 4, 8.0f, true);
    if (tex.IsReal("assets/sprites/enemy_fast.png"))
        artFastWalk = Animation(tex.Get("assets/sprites/enemy_fast.png"), 4, 10.0f, true);
    if (tex.IsReal("assets/sprites/enemy_tank.png"))
        artTankWalk = Animation(tex.Get("assets/sprites/enemy_tank.png"), 4, 6.0f, true);
    if (tex.IsReal("assets/sprites/boss.png"))
        artBossWalk = Animation(tex.Get("assets/sprites/boss.png"), 6, 8.0f, true);
    if (tex.IsReal("assets/sprites/enemy_death.png"))
        artEnemyDeath = Animation(tex.Get("assets/sprites/enemy_death.png"), 5, 12.0f, false);
    if (tex.IsReal("assets/sprites/boss_death.png"))
        artBossDeath = Animation(tex.Get("assets/sprites/boss_death.png"), 6, 10.0f, false);
}

// Назначает врагу анимации по типу (копия прототипа, таймер с нуля).
void Spawner::AssignArt(Enemy* e, EnemyType t)
{
    switch (t)
    {
        case ENEMY_FAST: e->walkAnim = artFastWalk; break;
        case ENEMY_TANK: e->walkAnim = artTankWalk; break;
        case ENEMY_BOSS: e->walkAnim = artBossWalk; break;
        case ENEMY_GRUNT:
        default:         e->walkAnim = artGruntWalk; break;
    }
    e->deathAnim = (t == ENEMY_BOSS) ? artBossDeath : artEnemyDeath;
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
        AssignArt(e, t);  // выдаём спрайты под тип
    }
}

void Spawner::SpawnBoss(Vector2 center, const TileMap& map)
{
    Enemy* e = GetInactive();
    if (!e) return;
    Vector2 pos = { center.x + 600.0f, center.y };
    pos = map.FindFreeSpot(pos, 38.0f);
    e->Spawn(pos, ENEMY_BOSS);
    AssignArt(e, ENEMY_BOSS);
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
        // Умирающие враги больше не наносят урон.
        if (e.active && !e.dying && CheckCollisionRecs(e.GetRect(), player.GetRect()))
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
        if (e.active && !e.dying) c++;
    return c;
}
