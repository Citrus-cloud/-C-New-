#pragma once
#include "raylib.h"
#include "projectile.h"
#include "spawner.h"
#include "pickups.h"
#include "loot.h"
#include <vector>

class Effects;   // вперёд-объявление системы эффектов (Фаза 4)

class Weapon
{
public:
    std::vector<Projectile> pool;
    float fireTimer;
    float fireInterval;
    float projectileSpeed;
    int damage;
    int level;
    int projectileCount;
    int pierce;
    bool evolved;
    bool firedThisFrame;  // выстрелили ли в этом кадре (для звука)

    Weapon(int maxProjectiles);

    Projectile* GetInactive();
    Enemy* FindNearestEnemy(Vector2 from, Spawner& spawner);
    void Evolve();
    // effects — искры/числа урона/взрывы при попаданиях и смерти (Фаза 4).
    void Update(float dt, Vector2 playerPos, Spawner& spawner, ExpOrbs& orbs, LootDrops& loot, Effects& effects);
    void Draw() const;
};
