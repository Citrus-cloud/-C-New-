#include "spawner.h"
#include "textures.h"
#include "bosses.h"
#include <cmath>

Spawner::Spawner(int poolSize)
    : spawnTimer(0.0f), spawnInterval(2.0f),
      bossTimer(0.0f), bossInterval(30.0f), elapsed(0.0f),
      bossEventLine(-1), bossSpawnCount(0)
{
    enemies.resize(poolSize);
}

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
    if (tex.IsReal("assets/sprites/boss_spider.png"))
        artSpiderWalk = Animation(tex.Get("assets/sprites/boss_spider.png"), 6, 8.0f, true);
    if (tex.IsReal("assets/sprites/boss_knight.png"))
        artKnightWalk = Animation(tex.Get("assets/sprites/boss_knight.png"), 6, 8.0f, true);
    if (tex.IsReal("assets/sprites/enemy_death.png"))
        artEnemyDeath = Animation(tex.Get("assets/sprites/enemy_death.png"), 5, 12.0f, false);
    if (tex.IsReal("assets/sprites/boss_death.png"))
        artBossDeath = Animation(tex.Get("assets/sprites/boss_death.png"), 6, 10.0f, false);
}

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
    // Баланс (Шаг 29): со временем волны крупнее, от 6 до 12 врагов.
    int count = 6 + (int)(elapsed / 30.0f);
    if (count > 12) count = 12;
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
        AssignArt(e, t);
    }
}

void Spawner::SpawnBoss(Vector2 center, const TileMap& map)
{
    Enemy* e = GetInactive();
    if (!e) return;
    Vector2 pos = { center.x + 600.0f, center.y };
    pos = map.FindFreeSpot(pos, 38.0f);

    int bossId = bossSpawnCount % BOSS_COUNT;   // чередуем боссов
    bossSpawnCount++;

    e->Spawn(pos, ENEMY_BOSS);
    const BossDef& def = GetBossDef(bossId);
    e->ApplyBoss(def);
    e->bossId = bossId;

    AssignArt(e, ENEMY_BOSS);
    // Если есть отдельный спрайт босса — используем его.
    if (bossId == BOSS_SPIDER_QUEEN && artSpiderWalk.Valid()) e->walkAnim = artSpiderWalk;
    else if (bossId == BOSS_BLACK_KNIGHT && artKnightWalk.Valid()) e->walkAnim = artKnightWalk;

    bossEventLine = def.voiceLine;
}

void Spawner::Update(float deltaTime, Player& player, const TileMap& map)
{
    elapsed += deltaTime;

    // Баланс (Шаг 29): интервал спавна сокращается со временем (сложность растёт).
    spawnTimer += deltaTime;
    float curInterval = spawnInterval - elapsed * 0.01f;
    if (curInterval < 0.6f) curInterval = 0.6f;
    if (spawnTimer >= curInterval)
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

        // Призыв миньонов (Королева пауков, Шаг 21).
        if (e.active && !e.dying && e.canSummon)
        {
            e.summonTimer -= deltaTime;
            if (e.summonTimer <= 0.0f)
            {
                e.summonTimer = GetBossDef(e.bossId).summonInterval;
                for (int s = 0; s < 3; s++)
                {
                    Enemy* m = GetInactive();
                    if (!m) break;
                    float ang = (float)s / 3.0f * 2.0f * PI;
                    Vector2 mp = { e.position.x + cosf(ang) * 60.0f, e.position.y + sinf(ang) * 60.0f };
                    mp = map.FindFreeSpot(mp, 12.0f);
                    m->Spawn(mp, ENEMY_FAST);
                    AssignArt(m, ENEMY_FAST);
                }
            }
        }

        if (e.active && !e.dying && CheckCollisionRecs(e.GetRect(), player.GetRect()))
            player.TakeDamage(e.damage);
    }
}

void Spawner::Draw(Camera2D camera, int screenW, int screenH) const
{
    // Оптимизация (Шаг 28): рисуем только врагов в видимой области (culling).
    // При сотнях врагов это экономит вызовы отрисовки на тех, кого не видно.
    // (Дальнейший задел — упаковка спрайтов в один атлас текстур.)
    float halfW = (screenW * 0.5f) / camera.zoom + 96.0f;
    float halfH = (screenH * 0.5f) / camera.zoom + 96.0f;
    Vector2 c = camera.target;
    for (auto& e : enemies)
    {
        if (!e.active) continue;
        if (e.position.x < c.x - halfW || e.position.x > c.x + halfW ||
            e.position.y < c.y - halfH || e.position.y > c.y + halfH)
            continue;
        e.Draw();
    }
}

int Spawner::ActiveCount() const
{
    int cnt = 0;
    for (auto& e : enemies)
        if (e.active && !e.dying) cnt++;
    return cnt;
}
