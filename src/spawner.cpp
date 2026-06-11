#include "spawner.h"
#include "textures.h"
#include "bosses.h"
#include "tuning.h"      // все числовые параметры спавна берём отсюда (Фаза 1)
#include "telegraph.h"  // система телеграфов (Фаза 2)
#include "ranged.h"     // система снарядов (Фаза 3)
#include "effects.h"    // визуальные эффекты (Фаза 4-5: магический круг призыва/лечения)
#include "hazards.h"    // система опасных зон (Фаза 5, ядовитый след)
#include "combat.h"     // полное определение Weapon/Projectile для уклонения (Фаза 6, Шаг 33)
#include <cmath>

Spawner::Spawner(int poolSize)
    : spawnTimer(0.0f), spawnInterval(Tuning::kSpawnBaseInterval),
      bossTimer(0.0f), bossInterval(Tuning::kBossInterval), elapsed(0.0f),
      bossEventLine(-1), bossSpawnCount(0), telegraphTimer(0.0f),
      shooterTimer(0.0f), bossRangedTimer(0.0f), bossRangedPattern(0),
      telegraphs(nullptr), ranged(nullptr), effects(nullptr), hazards(nullptr),
      weapon(nullptr)
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

void Spawner::SetHazards(HazardSystem* h)
{
    hazards = h;
}

void Spawner::SetWeapon(Weapon* w)
{
    weapon = w;
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

// Назначает обычному врагу ОСОБУЮ способность (Фаза 5, Шаг 23-28). С вероятностью
// kSpecialChance выбирает одну из РАЗБЛОКИРОВАННЫХ по весам из kAbilityRules. Не зависит
// от приёма мобильности: враг может иметь и перемещение, и особую способность.
void Spawner::MaybeAssignSpecial(Enemy* e)
{
    if (!e || e->type == ENEMY_BOSS) return;
    if (GetRandomValue(0, 99) >= Tuning::kSpecialChance) return;

    const Tuning::AbilityId ids[5]  = { Tuning::ABIL_SHIELD, Tuning::ABIL_SPLIT,
                                        Tuning::ABIL_SLOW_AURA, Tuning::ABIL_POISON_TRAIL,
                                        Tuning::ABIL_HEALER };
    const int               kinds[5] = { SPEC_SHIELD, SPEC_SPLIT, SPEC_SLOW_AURA,
                                         SPEC_POISON_TRAIL, SPEC_HEALER };

    // Суммарный вес разблокированных способностей.
    float total = 0.0f;
    for (int i = 0; i < 5; i++)
        if (Tuning::IsUnlockedAt(ids[i], elapsed)) total += Tuning::GetRule(ids[i]).weight;
    if (total <= 0.0f) return;

    float r = (float)GetRandomValue(0, 10000) / 10000.0f * total;
    for (int i = 0; i < 5; i++)
    {
        if (!Tuning::IsUnlockedAt(ids[i], elapsed)) continue;
        r -= Tuning::GetRule(ids[i]).weight;
        if (r <= 0.0f) { e->ApplySpecial(kinds[i]); break; }
    }
}

// Назначает обычному врагу ЭЛИТНЫЙ модификатор (Фаза 6, Шаг 32). С вероятностью
// kEliteChance после kEliteUnlockTime выбирает один из модификаторов равновероятно.
// Не зависит от мобильности/особой способности: элита может иметь и их.
void Spawner::MaybeAssignElite(Enemy* e)
{
    if (!e || e->type == ENEMY_BOSS) return;
    if (elapsed < Tuning::kEliteUnlockTime) return;
    if (GetRandomValue(0, 99) >= Tuning::kEliteChance) return;

    const int kinds[4] = { ELITE_SWIFT, ELITE_ARMORED, ELITE_EXPLOSIVE, ELITE_GIANT };
    e->ApplyElite(kinds[GetRandomValue(0, 3)]);
}

// Назначает обычному врагу улучшенный ИИ (Фаза 6, Шаг 33): с вероятностью
// kProjDodgeChance после kProjDodgeUnlockTime враг начинает уклоняться от снарядов игрока.
// Не зависит от других приёмов — это пассивное улучшение поведения.
void Spawner::MaybeAssignAI(Enemy* e)
{
    if (!e || e->type == ENEMY_BOSS) return;
    e->projectileDodger = false;
    if (elapsed < Tuning::kProjDodgeUnlockTime) return;
    if (GetRandomValue(0, 99) < Tuning::kProjDodgeChance) e->projectileDodger = true;
}

// Групповое расталкивание (Шаг 33): враги, оказавшиеся ближе kSeparationRadius,
// мягко толкают друг друга, чтобы не слипаться в одну точку. Сдвиг — с проверкой стен.
// O(n^2) по пулу, но число соседей ограничено kSeparationMaxNeighbors.
void Spawner::ApplySeparation(const TileMap& map, float dt)
{
    const float r = Tuning::kSeparationRadius;
    for (auto& e : enemies)
    {
        if (!e.active || e.dying) continue;
        if (e.type == ENEMY_BOSS) continue;          // боссов не расталкиваем
        if (e.jumping || e.dashing) continue;         // в полёте/рывке не мешаем приёму

        float pushX = 0.0f, pushY = 0.0f;
        int neighbors = 0;
        for (auto& o : enemies)
        {
            if (&o == &e) continue;
            if (!o.active || o.dying) continue;
            float dx = e.position.x - o.position.x;
            float dy = e.position.y - o.position.y;
            float d2 = dx * dx + dy * dy;
            if (d2 >= r * r || d2 <= 0.0001f) continue;
            float d = sqrtf(d2);
            float w = (r - d) / r;                    // ближе = сильнее (0..1)
            pushX += dx / d * w;
            pushY += dy / d * w;
            if (++neighbors >= Tuning::kSeparationMaxNeighbors) break;
        }
        if (neighbors == 0) continue;

        float step = Tuning::kSeparationForce * dt;
        float mx = pushX * step;
        float my = pushY * step;
        e.position.x += mx;
        if (map.CheckCollision(e.GetRect())) e.position.x -= mx;
        e.position.y += my;
        if (map.CheckCollision(e.GetRect())) e.position.y -= my;
    }
}

// Уклонение от снарядов (Шаг 33): враг-«уклонист» ищет ближайший снаряд игрока,
// летящий примерно в него, и делает короткий шаг перпендикулярно линии полёта.
// Снаряды живут в weapon->pool; спавнер идёт ДО оружия, поэтому видим снаряды прошлого кадра.
void Spawner::ApplyProjectileDodge(const TileMap& map, float dt)
{
    if (!weapon) return;
    const float sense = Tuning::kProjDodgeSenseRadius;
    for (auto& e : enemies)
    {
        if (!e.active || e.dying) continue;
        if (!e.projectileDodger) continue;
        if (e.type == ENEMY_BOSS) continue;
        if (e.jumping || e.dashing || e.dashTelegraphing) continue;   // занят приёмом
        if (e.dodgeReactCd > 0.0f) { e.dodgeReactCd -= dt; continue; }

        // Ищем ближайший угрожающий снаряд, нацеленный на этого врага.
        float bestD2 = sense * sense;
        const Projectile* threat = nullptr;
        for (auto& p : weapon->pool)
        {
            if (!p.active) continue;
            float dx = e.position.x - p.position.x;
            float dy = e.position.y - p.position.y;
            float d2 = dx * dx + dy * dy;
            if (d2 >= bestD2) continue;
            float vlen = sqrtf(p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y);
            if (vlen < 0.0001f) continue;
            float d = sqrtf(d2);
            if (d < 0.0001f) continue;
            // Насколько снаряд нацелен на врага (dot единичных векторов).
            float dot = (p.velocity.x / vlen) * (dx / d) + (p.velocity.y / vlen) * (dy / d);
            if (dot < Tuning::kProjDodgeThreatDot) continue;   // летит мимо
            bestD2 = d2;
            threat = &p;
        }
        if (!threat) continue;

        // Шаг в сторону от линии полёта снаряда (перпендикуляр к скорости).
        float vlen = sqrtf(threat->velocity.x * threat->velocity.x +
                           threat->velocity.y * threat->velocity.y);
        float vx = threat->velocity.x / vlen;
        float vy = threat->velocity.y / vlen;
        float tx = e.position.x - threat->position.x;
        float ty = e.position.y - threat->position.y;
        float cross = vx * ty - vy * tx;          // с какой стороны линии находится враг
        float side = (cross >= 0.0f) ? 1.0f : -1.0f;
        float px = -vy * side;                    // перпендикуляр к направлению снаряда
        float py =  vx * side;

        float step = Tuning::kProjDodgeSpeed * dt;
        float mx = px * step;
        float my = py * step;
        e.position.x += mx;
        if (map.CheckCollision(e.GetRect())) e.position.x -= mx;
        e.position.y += my;
        if (map.CheckCollision(e.GetRect())) e.position.y -= my;

        e.dodgeReactCd = Tuning::kProjDodgeCooldown;
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
        float r = Tuning::kWaveSpawnRadius;   // радиус кольца появления — из конфига (за краем обзора, Шаг 4)
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
        MaybeAssignSpecial(e);    // Фаза 5: возможная особая способность
        MaybeAssignElite(e);      // Фаза 6 (Шаг 32): возможный элитный модификатор
        MaybeAssignAI(e);         // Фаза 6 (Шаг 33): возможное уклонение от снарядов
    }
}

void Spawner::SpawnBoss(Vector2 center, const TileMap& map)
{
    Enemy* e = GetInactive();
    if (!e) return;
    Vector2 pos = { center.x + Tuning::kBossSpawnDistance, center.y };   // дистанция спавна босса — из конфига (Шаг 4)
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

// Отладка (F7, Шаг 28): спавним по одному врагу с каждой особой способностью
// вокруг точки center, игнорируя время разблокировки — для быстрой проверки.
void Spawner::SpawnSpecialTest(Vector2 center, const TileMap& map)
{
    const int kinds[5] = { SPEC_SHIELD, SPEC_SPLIT, SPEC_SLOW_AURA, SPEC_POISON_TRAIL, SPEC_HEALER };
    for (int i = 0; i < 5; i++)
    {
        Enemy* e = GetInactive();
        if (!e) break;
        float ang = (float)i / 5.0f * 2.0f * PI;
        Vector2 pos = { center.x + cosf(ang) * 260.0f, center.y + sinf(ang) * 260.0f };
        pos = map.FindFreeSpot(pos, 16.0f);
        // Танк лучше виден для теста щита/деления.
        e->Spawn(pos, ENEMY_TANK);
        AssignArt(e, ENEMY_TANK);
        e->ApplySpecial(kinds[i]);
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

        // --- Разделение при смерти (Шаг 24): флаг выставлен в DamageEnemy ---
        if (e.wantSplit)
        {
            e.wantSplit = false;
            for (int s = 0; s < Tuning::kSplitCount; s++)
            {
                Enemy* shard = GetInactive();
                if (!shard) break;
                float ang = (float)s / Tuning::kSplitCount * 2.0f * PI;
                Vector2 sp = { e.position.x + cosf(ang) * Tuning::kSplitSpread,
                               e.position.y + sinf(ang) * Tuning::kSplitSpread };
                sp = map.FindFreeSpot(sp, 12.0f);
                shard->Spawn(sp, ENEMY_FAST);
                AssignArt(shard, ENEMY_FAST);
                // Осколки слабее и мельче родителя; повторно НЕ делятся (splitsOnDeath=false из Spawn).
                int hp = (int)(e.maxHealth * Tuning::kSplitHealthFrac);
                if (hp < 1) hp = 1;
                shard->maxHealth = hp;
                shard->health = hp;
                shard->size = e.size * Tuning::kSplitSizeFrac;
            }
        }

        // --- Взрывной элит (Шаг 32): при гибели взрывается по площади ---
        // Урон наносится через телеграф-зону (короткое предупреждение), а не мгновенно.
        if (e.wantExplode)
        {
            e.wantExplode = false;
            if (telegraphs)
                telegraphs->SpawnCircle(e.position, Tuning::kEliteExplosiveRadius,
                                        Tuning::kEliteExplosiveDamage, Tuning::kEliteExplosiveFill,
                                        Color{ 255, 120, 30, 255 });
            if (effects)
            {
                effects->SpawnExplosion(e.position, 1.6f);
                effects->SpawnSparks(e.position, Color{ 255, 140, 40, 255 }, 20);
                effects->Shake(6.0f, 0.2f);
            }
        }

        // --- Аура замедления (Шаг 26): если игрок в радиусе — замедляем ---
        if (e.active && !e.dying && e.special == SPEC_SLOW_AURA)
        {
            float dx = player.position.x - e.position.x;
            float dy = player.position.y - e.position.y;
            if (dx * dx + dy * dy <= Tuning::kSlowAuraRadius * Tuning::kSlowAuraRadius)
                player.ApplySlow(Tuning::kSlowAuraFactor);
        }

        // --- Ядовитый след (Шаг 27): периодически роняем лужу в систему хазардов ---
        if (e.active && !e.dying && e.special == SPEC_POISON_TRAIL && hazards)
        {
            e.poisonDropTimer -= deltaTime;
            if (e.poisonDropTimer <= 0.0f)
            {
                e.poisonDropTimer = Tuning::kPoisonDropInterval;
                hazards->Spawn(e.position, Tuning::kPoisonRadius, Tuning::kPoisonDamage,
                               Tuning::kPoisonTick, Tuning::kPoisonLifetime,
                               Color{ 80, 220, 60, 255 });
            }
        }

        // --- Лекарь (Шаг 28): периодически лечим союзников в радиусе ---
        if (e.active && !e.dying && e.special == SPEC_HEALER)
        {
            e.healTimer -= deltaTime;
            if (e.healTimer <= 0.0f)
            {
                e.healTimer = Tuning::kHealInterval;
                for (auto& ally : enemies)
                {
                    if (&ally == &e) continue;            // себя не лечим
                    if (!ally.active || ally.dying) continue;
                    float dx = ally.position.x - e.position.x;
                    float dy = ally.position.y - e.position.y;
                    if (dx * dx + dy * dy <= Tuning::kHealerRadius * Tuning::kHealerRadius)
                    {
                        ally.health += Tuning::kHealAmount;
                        if (ally.health > ally.maxHealth) ally.health = ally.maxHealth;
                    }
                }
                if (effects) effects->SpawnMagicCircle(e.position, 2.0f);   // зелёный импульс лечения
            }
        }

        // Призыв миньонов (Шаг 25): типы миньонов + лимит активных. Интервал — из BossDef.
        if (e.active && !e.dying && e.canSummon)
        {
            e.summonTimer -= deltaTime;
            if (e.summonTimer <= 0.0f)
            {
                e.summonTimer = GetBossDef(e.bossId).summonInterval;
                // Не заспавним карту: призыв работает только пока активных меньше лимита.
                if (ActiveCount() < Tuning::kSummonMaxActive)
                {
                    for (int s = 0; s < Tuning::kSummonMinionCount; s++)
                    {
                        Enemy* m = GetInactive();
                        if (!m) break;
                        float ang = (float)s / Tuning::kSummonMinionCount * 2.0f * PI;
                        Vector2 mp = { e.position.x + cosf(ang) * 60.0f, e.position.y + sinf(ang) * 60.0f };
                        mp = map.FindFreeSpot(mp, 12.0f);
                        // Тип миньона: танк (после kSummonTankTime), иначе быстрый, иначе обычный.
                        EnemyType mt = ENEMY_GRUNT;
                        int roll = GetRandomValue(0, 99);
                        if (elapsed > Tuning::kSummonTankTime && roll < Tuning::kSummonTankChance) mt = ENEMY_TANK;
                        else if (roll < Tuning::kSummonFastChance) mt = ENEMY_FAST;
                        m->Spawn(mp, mt);
                        AssignArt(m, mt);
                        MaybeAssignMobility(m);   // призванные миньоны тоже могут быть мобильными
                    }
                    if (effects) effects->SpawnMagicCircle(e.position, 1.4f);   // fx призыва
                }
            }
        }

        if (e.active && !e.dying && CheckCollisionRecs(e.GetRect(), player.GetRect()))
            player.TakeDamage(e.damage);
    }

    // Улучшенный ИИ (Шаг 33): после движения врагов — групповое расталкивание
    // и уклонение «уклонистов» от летящих снарядов игрока.
    ApplySeparation(map, deltaTime);
    ApplyProjectileDodge(map, deltaTime);

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
    if (ranged && Tuning::IsUnlockedAt(Tuning::ABIL_RANGED_SHOOTER, elapsed))
    {
        shooterTimer += deltaTime;
        float interval = Tuning::GetRule(Tuning::ABIL_RANGED_SHOOTER).minInterval;
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
