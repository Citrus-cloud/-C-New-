#pragma once
#include "raylib.h"

// Снаряд. Тоже живёт в пуле (флаг active).
class Projectile
{
public:
    Vector2 position;
    Vector2 velocity;  // скорость по x и y (пиксели/сек)
    bool active;
    int damage;
    float lifetime;    // сколько секунд живёт до исчезновения

    Projectile();

    void Fire(Vector2 pos, Vector2 vel);
    void Update(float dt);
    void Draw() const;
    Rectangle GetRect() const;
};
