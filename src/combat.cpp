#include "combat.h"
#include <cmath>

Weapon::Weapon(int maxProjectiles)
    : fireTimer(0.0f), fireInterval(0.5f), projectileSpeed(500.0f)
{
    pool.resize(maxProjectiles);
}

Projectile* Weapon::GetInactive()
{
    for (Projectile& p : pool)
        if (!p.active) return &p;
    return nullptr;
}

// Ищем ближайшего активного врага
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
    // 1) Автоатака по таймеру в ближайшего врага
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
            }
        }
    }

    // 2) Двигаем снаряды
    for (Projectile& p : pool)
        p.Update(dt);

    // 3) Коллизии "снаряд <-> враг"
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
                    orbs.Spawn(e.position);  // выпал опыт
                }
                break;  // снаряд исчез, дальше не проверяем
            }
        }
    }
}

void Weapon::Draw() const
{
    for (const Projectile& p : pool)
        p.Draw();
}
