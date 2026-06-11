#include "combat.h"
#include "effects.h"
#include "tuning.h"   // сила отбрасывания и параметры статусов (Фаза 6, Шаг 30-31)
#include <cmath>

// Общая функция урона по врагу: эффекты + награда при смерти.
// Используется и оружием, и способностями игрока (Фаза 5), и тиками статусов (Шаг 31).
void DamageEnemy(Enemy& e, int dmg, ExpOrbs& orbs, LootDrops& loot, Effects& effects)
{
    if (!e.active || e.dying) return;

    // Щит (Шаг 23): пока активен — урон не проходит, только синие искры отрикошета.
    if (e.shielded)
    {
        effects.SpawnSparks(e.position, Color{ 120, 200, 255, 255 }, 4);
        return;
    }

    e.health -= dmg;

    // Эффекты попадания (Шаг 15, 18).
    effects.SpawnSparks(e.position, Color{ 255, 200, 80, 255 }, 5);
    effects.SpawnDamageNumber(e.position, dmg, e.type == ENEMY_BOSS);

    if (e.health <= 0)
    {
        e.Kill();

        // Разделение при смерти (Шаг 24): помечаем врага — осколки породит спавнер.
        if (e.splitsOnDeath) e.wantSplit = true;

        // Эффекты смерти (Шаг 16, 17).
        bool boss = (e.type == ENEMY_BOSS);
        effects.SpawnBlood(e.position, boss ? 40 : 14);
        effects.SpawnExplosion(e.position, boss ? 2.2f : 1.0f);
        if (boss) effects.Shake(11.0f, 0.45f);

        for (int k = 0; k < e.xpValue; k++)
            orbs.Spawn(e.position);

        int dropRoll = GetRandomValue(0, 99);
        if (e.type == ENEMY_BOSS)
        {
            loot.Spawn(e.position, LOOT_HEALTH);
            loot.Spawn({ e.position.x + 30.0f, e.position.y }, LOOT_POWER);
        }
        else if (e.type == ENEMY_TANK && dropRoll < 35)
        {
            loot.Spawn(e.position, (dropRoll < 17) ? LOOT_HEALTH : LOOT_POWER);
        }
        else if (dropRoll < 4)
        {
            loot.Spawn(e.position, LOOT_HEALTH);
        }
    }
}

Weapon::Weapon(int maxProjectiles)
    : fireTimer(0.0f), fireInterval(0.5f), projectileSpeed(500.0f), damage(15),
      level(1), projectileCount(1), pierce(0), evolved(false), firedThisFrame(false)
{
    pool.resize(maxProjectiles);
}

Projectile* Weapon::GetInactive()
{
    for (Projectile& p : pool)
        if (!p.active) return &p;
    return nullptr;
}

Enemy* Weapon::FindNearestEnemy(Vector2 from, Spawner& spawner)
{
    Enemy* nearest = nullptr;
    float bestDist = 1e9f;
    for (Enemy& e : spawner.enemies)
    {
        if (!e.active || e.dying) continue;
        float dx = e.position.x - from.x;
        float dy = e.position.y - from.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < bestDist) { bestDist = dist; nearest = &e; }
    }
    return nearest;
}

void Weapon::Evolve()
{
    if (evolved) return;
    evolved = true;
    projectileCount = 3;
    pierce = 2;
    damage += 10;
}

void Weapon::Update(float dt, Vector2 playerPos, Spawner& spawner, ExpOrbs& orbs, LootDrops& loot, Effects& effects)
{
    // Тики статусов горения и яда (Шаг 31). Урон идёт через DamageEnemy, чтобы
    // корректно срабатывали награда (XP/лут) и эффекты при смерти от статуса.
    for (Enemy& e : spawner.enemies)
    {
        if (!e.active || e.dying) continue;
        if (e.burnTimer > 0.0f)
        {
            e.burnTimer -= dt;
            e.burnTick -= dt;
            if (e.burnTick <= 0.0f)
            {
                e.burnTick = Tuning::kBurnTick;
                effects.SpawnSparks(e.position, Color{ 255, 130, 40, 255 }, 3);
                DamageEnemy(e, Tuning::kBurnDamage, orbs, loot, effects);
            }
        }
        if (e.active && !e.dying && e.poisonTimer > 0.0f)
        {
            e.poisonTimer -= dt;
            e.poisonTick -= dt;
            if (e.poisonTick <= 0.0f)
            {
                e.poisonTick = Tuning::kPoisonStatusTick;
                effects.SpawnSparks(e.position, Color{ 120, 230, 90, 255 }, 3);
                DamageEnemy(e, Tuning::kPoisonStatusDamage * e.poisonStacks, orbs, loot, effects);
            }
            if (e.poisonTimer <= 0.0f) e.poisonStacks = 0;
        }
    }

    firedThisFrame = false;
    fireTimer += dt;
    if (fireTimer >= fireInterval)
    {
        Enemy* target = FindNearestEnemy(playerPos, spawner);
        if (target != nullptr)
        {
            fireTimer = 0.0f;
            Vector2 dir = { target->position.x - playerPos.x, target->position.y - playerPos.y };
            float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.01f) { dir.x /= len; dir.y /= len; }
            float baseAngle = atan2f(dir.y, dir.x);
            float spread = 0.25f;

            for (int i = 0; i < projectileCount; i++)
            {
                Projectile* p = GetInactive();
                if (!p) break;
                float offset = (i - (projectileCount - 1) / 2.0f) * spread;
                float a = baseAngle + offset;
                Vector2 vel = { cosf(a) * projectileSpeed, sinf(a) * projectileSpeed };
                p->Fire(playerPos, vel, pierce);
                p->damage = damage;
            }
            firedThisFrame = true;
        }
    }

    for (Projectile& p : pool)
        p.Update(dt);

    for (Projectile& p : pool)
    {
        if (!p.active) continue;
        for (Enemy& e : spawner.enemies)
        {
            if (!e.active || e.dying) continue;
            if (CheckCollisionRecs(p.GetRect(), e.GetRect()))
            {
                DamageEnemy(e, p.damage, orbs, loot, effects);
                // Отбрасывание от попадания (Шаг 30): толкаем врага прочь от игрока.
                e.ApplyKnockback({ e.position.x - playerPos.x, e.position.y - playerPos.y }, Tuning::kKnockbackForce);
                // Шанс наложить статус-эффект при попадании (Шаг 31): не более одного из трёх.
                int sroll = GetRandomValue(0, 99);
                if (sroll < Tuning::kBurnChance)
                    e.ApplyStatus(STATUS_BURN);
                else if (sroll < Tuning::kBurnChance + Tuning::kFreezeChance)
                    e.ApplyStatus(STATUS_FREEZE);
                else if (sroll < Tuning::kBurnChance + Tuning::kFreezeChance + Tuning::kPoisonStatusChance)
                    e.ApplyStatus(STATUS_POISON);
                if (p.pierce > 0) p.pierce--;
                else p.active = false;
                if (!p.active) break;
            }
        }
    }
}

void Weapon::Draw() const
{
    for (const Projectile& p : pool)
        p.Draw();
}
