#include "hazards.h"
#include "player.h"
#include <cmath>

HazardSystem::HazardSystem(int poolSize)
{
    pool.resize(poolSize);
}

Hazard* HazardSystem::GetInactive()
{
    for (Hazard& h : pool)
        if (!h.active) return &h;
    return nullptr;
}

Hazard* HazardSystem::Spawn(Vector2 pos, float radius, int damage, float tick, float life, Color c)
{
    Hazard* h = GetInactive();
    if (!h) return nullptr;
    h->active = true;
    h->pos = pos;
    h->radius = radius;
    h->damage = damage;
    h->tick = (tick > 0.01f) ? tick : 0.5f;
    h->tickTimer = 0.0f;   // первый тик — сразу, как только игрок войдёт
    h->life = life;
    h->maxLife = (life > 0.01f) ? life : 1.0f;
    h->color = c;
    return h;
}

void HazardSystem::Update(float dt, Player& player)
{
    for (Hazard& h : pool)
    {
        if (!h.active) continue;
        h.life -= dt;
        if (h.life <= 0.0f) { h.active = false; continue; }
        if (h.tickTimer > 0.0f) h.tickTimer -= dt;

        // Урон по игроку, если он внутри зоны и тик «созрел».
        // Player::TakeDamage сам учитывает i-frames, так что урон не «застрочит».
        float dx = player.position.x - h.pos.x;
        float dy = player.position.y - h.pos.y;
        if (dx * dx + dy * dy <= h.radius * h.radius && h.tickTimer <= 0.0f)
        {
            player.TakeDamage(h.damage);
            h.tickTimer = h.tick;
        }
    }
}

void HazardSystem::Clear()
{
    for (Hazard& h : pool) h.active = false;
}

void HazardSystem::Draw() const
{
    for (const Hazard& h : pool)
    {
        if (!h.active) continue;
        float k = h.life / h.maxLife;   // 1 в начале, 0 к концу жизни
        if (k < 0.0f) k = 0.0f;
        if (k > 1.0f) k = 1.0f;
        unsigned char fillA = (unsigned char)(70.0f * k + 25.0f);
        unsigned char edgeA = (unsigned char)(160.0f * k);
        DrawCircleV(h.pos, h.radius, Color{ h.color.r, h.color.g, h.color.b, fillA });
        DrawCircleLines((int)h.pos.x, (int)h.pos.y, h.radius,
                        Color{ h.color.r, h.color.g, h.color.b, edgeA });
    }
}

void HazardSystem::DrawDebug() const
{
    for (const Hazard& h : pool)
    {
        if (!h.active) continue;
        DrawCircleLines((int)h.pos.x, (int)h.pos.y, h.radius, YELLOW);
    }
}

int HazardSystem::ActiveCount() const
{
    int c = 0;
    for (const Hazard& h : pool)
        if (h.active) c++;
    return c;
}
