#include "pickups.h"
#include <cmath>

ExpOrb::ExpOrb() : position({ 0.0f, 0.0f }), active(false) {}

ExpOrbs::ExpOrbs(int maxOrbs) : collectedThisFrame(0)
{
    pool.resize(maxOrbs);
}

ExpOrb* ExpOrbs::GetInactive()
{
    for (ExpOrb& o : pool)
        if (!o.active) return &o;
    return nullptr;
}

void ExpOrbs::Spawn(Vector2 pos)
{
    ExpOrb* o = GetInactive();
    if (o == nullptr) return;
    o->position = pos;
    o->active = true;
}

void ExpOrbs::Update(float dt, Player& player)
{
    collectedThisFrame = 0;
    const float magnetRadius = 120.0f;
    const float pickupRadius = 24.0f;
    const float magnetSpeed = 320.0f;

    for (ExpOrb& o : pool)
    {
        if (!o.active) continue;
        float dx = player.position.x - o.position.x;
        float dy = player.position.y - o.position.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < pickupRadius)
        {
            o.active = false;
            player.xp += 1;
            collectedThisFrame++;
        }
        else if (dist < magnetRadius && dist > 0.01f)
        {
            o.position.x += (dx / dist) * magnetSpeed * dt;
            o.position.y += (dy / dist) * magnetSpeed * dt;
        }
    }
}

void ExpOrbs::Draw() const
{
    for (const ExpOrb& o : pool)
        if (o.active) DrawCircle((int)o.position.x, (int)o.position.y, 5.0f, GREEN);
}
