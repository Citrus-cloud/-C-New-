#pragma once
#include "raylib.h"
#include "projectile.h"
#include "spawner.h"
#include "pickups.h"
#include "loot.h"
#include <vector>

// Оружие: автоатака по ближайшему врагу + пул снарядов
class Weapon
{
public:
    std::vector<Projectile> pool;
    float fireTimer;
    float fireInterval;
    float projectileSpeed;
    int damage;
    int level;            // уровень оружия (растёт от апгрейдов)
    int projectileCount;  // снарядов за выстрел
    int pierce;           // сколько врагов пробивает снаряд
    bool evolved;         // эволюционировало ли оружие

    Weapon(int maxProjectiles);

    Projectile* GetInactive();
    Enemy* FindNearestEnemy(Vector2 from, Spawner& spawner);
    void Evolve();
    void Update(float dt, Vector2 playerPos, Spawner& spawner, ExpOrbs& orbs, LootDrops& loot);
    void Draw() const;
};
