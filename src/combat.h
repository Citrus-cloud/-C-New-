#pragma once
#include "raylib.h"
#include "projectile.h"
#include "spawner.h"
#include "pickups.h"
#include "loot.h"
#include <vector>

class Effects;

// Наносит урон врагу с эффектами (искры, число урона) и, при смерти,
// наградой (опыт, лут) и взрывом. Общая логика для оружия и способностей —
// чтобы любая система урона вела себя одинаково (Фаза 5, задел на масштаб).
void DamageEnemy(Enemy& e, int dmg, ExpOrbs& orbs, LootDrops& loot, Effects& effects);

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
    bool firedThisFrame;

    Weapon(int maxProjectiles);

    Projectile* GetInactive();
    Enemy* FindNearestEnemy(Vector2 from, Spawner& spawner);
    void Evolve();
    void Update(float dt, Vector2 playerPos, Spawner& spawner, ExpOrbs& orbs, LootDrops& loot, Effects& effects);
    void Draw() const;
};
