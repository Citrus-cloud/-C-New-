#include "spawner.h"
#include "textures.h"
#include "bosses.h"
#include "tuning.h"      // все числовые параметры спавна берём отсюда (Фаза 1)
#include "telegraph.h"  // система телеграфов (Фаза 2)
#include "ranged.h"     // система снарядов (Фаза 3)
#include <cmath>

Spawner::Spawner(int poolSize)
    : spawnTimer(0.0f), spawnInterval(Tuning::kSpawnBaseInterval),
      bossTimer(0.0f), bossInterval(Tuning::kBossInterval), elapsed(0.0f),
      bossEventLine(-1), bossSpawnCount(0), telegraphTimer(0.0f),
      shooterTimer(0.0f), bossRangedTimer(0.0f), bossRangedPattern(0),
      telegraphs(nullptr), ranged(nullptr), effects(nullptr)
{
    enemies.resize(poolSize);
}

void Spawner::SetTelegraphs(TelegraphSystem* t)
{
    telegraphs = t;
}

void Spawner::SetRanged(RangedSystem* r)
{
    ranged = r;
}

void Spawner::SetEffects(Effects* e)
{
    effects = e;
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

// Назначает обычному врагу приём мобильности (Фаза 4). С вероятностью kMobilityChance
// выбирает один из РАЗБЛОКИРОВАННЫХ к текущему времени приёмов по их весам из kAbilityRules.
void Spawner::MaybeAssignMobility(Enemy* e)
{
    if (!e || e->type == ENEMY_BOSS) return;
    if (GetRandomValue(0, 99) >= Tuning::kMobilityChance) return;

    const Tuning::AbilityId ids[5]  = { Tuning::ABIL_JUMP_SLAM, Tuning::ABIL_TELEPORT,
                                        Tuning::ABIL_BLINK, Tuning::ABIL_DASH, Tuning::ABIL_FLANK };
    const int               kinds[5] = { MOB_JUMP_SLAM, MOB_TELEPORT, MOB_BLINK, MOB_DASH, MOB_FLANK };

    // Суммарный вес разблокированных приёмов.
    float total = 0.0f;
    for (int i = 0; i < 5; i++)
        if (Tuning::IsUnlockedAt(ids[i], elapsed)) total += Tuning::GetRule(ids[i]).weight;
    if (total <= 0.0f) return;

    // Взвешенный случайный выбор.
    float r = (float)GetRandomValue(0, 10000) / 10000.0f * total;
    for (int i = 0; i < 5; i++)
    {
        if (!Tuning::IsUnlockedAt(ids[i], elapsed)) continue;
        r -= Tuning::GetRule(ids[i]).weight;
        if (r <= 0.0f)
        {
            e->mobility = kinds[i];
            float cd = Tuning::GetRule(ids[i]).minInterval;
            if (cd < 1.0f) cd = 1.0f;   // флангу кулдаун не нужен, но держим значение валидным
            e->mobilityCdMax = cd;
            // Десинхронизируем первый запуск, чтобы враги не применяли приём одновременно.
            e->mobilityCd = (float)GetRandomValue(0, (int)(cd * 1000.0f)) / 1000.0f;
            break;
        }
    }
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
        MaybeAssignMobility(e);   // Фаза 4: возможный приём перемещения
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

// Отладка (F6, Шаг 22): спавним по одному врагу с каждым приёмом мобильности
// вокруг точки center, игнорируя время разблокировки — для быстрой проверки.
void Spawner::SpawnMobilityTest(Vector2 center, const TileMap& map)
{
    const int kinds[5] = { MOB_JUMP_SLAM, MOB_TELEPORT, MOB_BLINK, MOB_DASH, MOB_FLANK };
    for (int i = 0; i < 5; i++)
    {
        Enemy* e = GetInactive();
        if (!e) break;
        float ang = (float)i / 5.0f * 2.0f * PI;
        Vector2 pos = { center.x + cosf(ang) * 340.0f, center.y + sinf(ang) * 340.0f };
        pos = map.FindFreeSpot(pos, 16.0f);
        e->Spawn(pos, ENEMY_GRUNT);
        AssignArt(e, ENEMY_GRUNT);
        e->mobility = kinds[i];
        e->mobilityCdMax = 3.0f;
        e->mobilityCd = 0.5f;
    }
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

    bool bossActive = false;   // есть ли сейчас живой босс
    Vector2 bossPos = { 0.0f, 0.0f };
    Color bossColor = Color{ 230, 60, 60, 255 };
    for (auto& e : enemies)
    {
        // Фаза 4: враги получают доступ к телеграфам и эффектам для приёмов мобильности.
        e.Update(deltaTime, player.position, map, telegraphs, effects);
        if (e.active && !e.dying && e.type == ENEMY_BOSS)
        {
            bossActive = true;
            bossPos = e.position;
            bossColor = e.color;
        }

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
                    MaybeAssignMobility(m);   // призванные миньоны тоже могут быть мобильными
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

    // ---- Дальние атаки босса (Фаза 3, Шаг 15): чередуем лазер и залп ----
    // Лазер идёт через систему телеграфов (слежение + фиксация), залп — через снаряды.
    if (bossActive && ranged && telegraphs)
    {
        bossRangedTimer += deltaTime;
        if (bossRangedTimer >= Tuning::kBossRangedInterval)
        {
            bossRangedTimer = 0.0f;
            if (bossRangedPattern == 0)
            {
                // Паттерн 0: следящий лазер из босса.
                telegraphs->SpawnLaser(bossPos, player.position,
                                       Tuning::kLaserLength, Tuning::kLaserWidth,
                                       Tuning::kLaserDamage, Tuning::kLaserFillTime,
                                       Tuning::kLaserTrackTime, bossColor);
            }
            else
            {
                // Паттерн 1: веер/залп снарядов в сторону игрока.
                ranged->FireVolley(bossPos, player.position,
                                   Tuning::kVolleyCount, Tuning::kVolleySpread,
                                   Tuning::kVolleyDamage, bossColor);
            }
            bossRangedPattern ^= 1;   // чередуем паттерны
        }
    }
    else
    {
        bossRangedTimer = 0.0f;
    }

    // ---- Враг-стрелок (Фаза 3, Шаг 14): обычный враг издали бьёт с упреждением ----
    // Разблокируется по времени (Tuning::IsUnlockedAt), интервал — из правила способности.
    if (ranged && Tuning::IsUnlockedAt(ABIL_RANGED_SHOOTER, elapsed))
    {
        shooterTimer += deltaTime;
        float interval = Tuning::GetRule(ABIL_RANGED_SHOOTER).minInterval;
        if (interval < 0.05f) interval = 0.05f;
        if (shooterTimer >= interval)
        {
            shooterTimer = 0.0f;
            // Ищем подходящего стрелка: активный не-босс, не умирающий, дальше kShooterMinRange.
            Enemy* shooter = nullptr;
            int candidates = 0;
            for (auto& e : enemies)
            {
                if (!e.active || e.dying || e.type == ENEMY_BOSS) continue;
                float dx = player.position.x - e.position.x;
                float dy = player.position.y - e.position.y;
                if (dx * dx + dy * dy < Tuning::kShooterMinRange * Tuning::kShooterMinRange) continue;
                // Резервуарный выбор (reservoir sampling) — равномерно случайный из подходящих.
                candidates++;
                if (GetRandomValue(1, candidates) == 1) shooter = &e;
            }
            if (shooter)
                ranged->FireAimed(shooter->position, player.position, Color{ 255, 170, 60, 255 });
        }
    }
    else
    {
        shooterTimer = 0.0f;
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
