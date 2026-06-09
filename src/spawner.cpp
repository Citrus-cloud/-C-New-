#include "spawner.h"
#include "textures.h"
#include "bosses.h"
#include "tuning.h"      // все числовые параметры спавна берём отсюда (Фаза 1)
#include "telegraph.h"  // система телеграфов (Фаза 2)
#include <cmath>

Spawner::Spawner(int poolSize)
    : spawnTimer(0.0f), spawnInterval(Tuning::kSpawnBaseInterval),
      bossTimer(0.0f), bossInterval(Tuning::kBossInterval), elapsed(0.0f),
      bossEventLine(-1), bossSpawnCount(0), telegraphTimer(0.0f),
      telegraphs(nullptr)
{
    enemies.resize(poolSize);
}

void Spawner::SetTelegraphs(TelegraphSystem* t)
{
    telegraphs = t;
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
    // Размер волны берём из конфига (растёт со временем до потолка).
    int count = Tuning::CurrentWaveCount(elapsed);
    for (int i = 0; i < count; i++)
    {
        Enemy* e = GetInactive();
        if (!e) break;
        float angle = (float)i / count * 2.0f * PI;
        float r = 500.0f;
        Vector2 pos = { center.x + cosf(angle) * r, center.y + sinf(angle) * r };
        pos = map.FindFreeSpot(pos, 16.0f);

        // Состав волны тоже из конфига: танки после kTankUnlockTime, шансы — kTankChance/kFastChance.
        EnemyType t = ENEMY_GRUNT;
        int roll = GetRandomValue(0, 99);
        if (elapsed > Tuning::kTankUnlockTime && roll < Tuning::kTankChance) t = ENEMY_TANK;
        else if (roll < Tuning::kFastChance) t = ENEMY_FAST;

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

    // Темп волн — из конфига (интервал сокращается со временем, см. Tuning).
    spawnTimer += deltaTime;
    float curInterval = Tuning::CurrentSpawnInterval(elapsed);
    if (spawnTimer >= curInterval)
    {
        spawnTimer = 0.0f;
        SpawnWave(player.position, map);
    }

    bossTimer += deltaTime;
    if (Tuning::kBossEnabled && bossTimer >= Tuning::kBossInterval)
    {
        bossTimer = 0.0f;
        SpawnBoss(player.position, map);
    }

    bool bossActive = false;   // есть ли сейчас живой босс (для демо-телеграфа)
    for (auto& e : enemies)
    {
        e.Update(deltaTime, player.position, map);
        if (e.active && !e.dying && e.type == ENEMY_BOSS) bossActive = true;

        // Призыв миньонов (Королева пауков). Интервал — из BossDef, кол-во — из Tuning.
        if (e.active && !e.dying && e.canSummon)
        {
            e.summonTimer -= deltaTime;
            if (e.summonTimer <= 0.0f)
            {
                e.summonTimer = GetBossDef(e.bossId).summonInterval;
                for (int s = 0; s < Tuning::kSummonMinionCount; s++)
                {
                    Enemy* m = GetInactive();
                    if (!m) break;
                    float ang = (float)s / Tuning::kSummonMinionCount * 2.0f * PI;
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

    // Тест конвейера телеграфов (Шаг 9): активный босс периодически «заказывает»
    // предупреждающую зону под игроком. Все числа — из конфига.
    if (telegraphs && bossActive)
    {
        telegraphTimer += deltaTime;
        if (telegraphTimer >= Tuning::kBossTelegraphInterval)
        {
            telegraphTimer = 0.0f;
            telegraphs->SpawnCircle(player.position,
                                    Tuning::kBossTelegraphRadius,
                                    Tuning::kBossTelegraphDamage,
                                    Tuning::kTelegraphDefaultFill,
                                    Color{ 230, 60, 60, 255 });
        }
    }
    else
    {
        telegraphTimer = 0.0f;   // нет босса — сбрасываем накопление
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
