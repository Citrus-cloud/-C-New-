#include "projectile.h"

Projectile::Projectile()
    : position({ 0.0f, 0.0f }), velocity({ 0.0f, 0.0f }),
      active(false), damage(15), lifetime(0.0f)
{
}

void Projectile::Fire(Vector2 pos, Vector2 vel)
{
    position = pos;
    velocity = vel;
    active = true;
    lifetime = 2.0f;
}

void Projectile::Update(float dt)
{
    if (!active) return;
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
    lifetime -= dt;
    if (lifetime <= 0.0f) active = false;  // снаряд истёк - выключаем
}

void Projectile::Draw() const
{
    if (!active) return;
    DrawCircle((int)position.x, (int)position.y, 6.0f, YELLOW);
}

Rectangle Projectile::GetRect() const
{
    return { position.x - 6.0f, position.y - 6.0f, 12.0f, 12.0f };
}
