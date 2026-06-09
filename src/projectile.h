#pragma once
#include "raylib.h"

// Снаряд. Живёт в пуле (флаг active).
class Projectile
{
public:
    Vector2 position;
    Vector2 velocity;
    bool active;
    int damage;
    float lifetime;
    int pierce;        // сколько ещё врагов может пробить

    Projectile();

    void Fire(Vector2 pos, Vector2 vel, int pierceCount);
    void Update(float dt);
    void Draw() const;
    Rectangle GetRect() const;
};
