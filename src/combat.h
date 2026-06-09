#pragma once
#include "raylib.h"
#include "projectile.h"
#include "spawner.h"
#include "pickups.h"
#include <vector>

// Оружие: автоатака по ближайшему врагу + пул снарядов
class Weapon
{
public:
    std::vector<Projectile> pool;
    float fireTimer;
    float fireInterval;     // раз в сколько секунд выстрел
    float projectileSpeed;

    Weapon(int maxProjectiles);

    Projectile* GetInactive();
    Enemy* FindNearestEnemy(Vector2 from, Spawner& spawner);
    void Update(float dt, Vector2 playerPos, Spawner& spawner, ExpOrbs& orbs);
    void Draw() const;
};
