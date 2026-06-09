#include "combat.h"
#include "effects.h"
#include <cmath>

// Общая функция урона по врагу: эффекты + награда при смерти.
// Используется и оружием, и способностями игрока (Фаза 5).
void DamageEnemy(Enemy& e, int dmg, ExpOrbs& orbs, LootDrops& loot, Effects& effects)
{
    if (!e.active || e.dying) return;

    e.health -= dmg;

    // Эффекты попадания (Шаг 15, 18).
    effects.SpawnSparks(e.position, Color{ 255, 200, 80, 255 }, 5);
    effects.SpawnDamageNumber(e.position, dmg, e.type == ENEMY_BOSS);

    if (e.health <= 0)
    {
        e.Kill();

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
