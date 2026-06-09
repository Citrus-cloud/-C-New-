#include "combat.h"
#include <cmath>

Weapon::Weapon(int maxProjectiles)
    : fireTimer(0.0f), fireInterval(0.5f), projectileSpeed(500.0f), damage(15)
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
    for (Enemy& e : spawner.pool)
    {
        if (!e.active) continue;
        float dx = e.position.x - from.x;
        float dy = e.position.y - from.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < bestDist) { bestDist = dist; nearest = &e; }
    }
    return nearest;
}

void Weapon::Update(float dt, Vector2 playerPos, Spawner& spawner, ExpOrbs& orbs)
{
    fireTimer += dt;
    if (fireTimer >= fireInterval)
    {
        Enemy* target = FindNearestEnemy(playerPos, spawner);
        if (target != nullptr)
        {
            fireTimer = 0.0f;
            Projectile* p = GetInactive();
            if (p != nullptr)
            {
                Vector2 dir = { target->position.x - playerPos.x, target->position.y - playerPos.y };
                float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
                if (len > 0.01f) { dir.x /= len; dir.y /= len; }
                Vector2 vel = { dir.x * projectileSpeed, dir.y * projectileSpeed };
                p->Fire(playerPos, vel);
                p->damage = damage;  // урон берём из оружия
            }
        }
    }

    for (Projectile& p : pool)
        p.Update(dt);

    for (Projectile& p : pool)
    {
        if (!p.active) continue;
        for (Enemy& e : spawner.pool)
        {
            if (!e.active) continue;
            if (CheckCollisionRecs(p.GetRect(), e.GetRect()))
            {
                e.health -= p.damage;
                p.active = false;
                if (e.health <= 0)
                {
                    e.active = false;
                    orbs.Spawn(e.position);
                }
                break;
            }
        }
    }
}

void Weapon::Draw() const
{
    for (const Projectile& p : pool)
        p.Draw();
}
