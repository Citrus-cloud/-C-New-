#include "ranged.h"
#include "player.h"
#include "effects.h"
#include "tuning.h"
#include <cmath>

RangedSystem::RangedSystem(int poolSize)
{
    pool.resize(poolSize);
}

EnemyProjectile* RangedSystem::GetInactive()
{
    for (auto& p : pool)
        if (!p.active) return &p;
    return nullptr;
}

// Одиночный выстрел с упреждением. target — текущая позиция игрока;
// прибавляем playerVel * kShooterLead, чтобы стрелять туда, куда он бежит.
void RangedSystem::FireAimed(Vector2 origin, Vector2 target, Color c)
{
    EnemyProjectile* p = GetInactive();
    if (!p) return;

    // Точка упреждения.
    Vector2 aim = {
        target.x + playerVel.x * Tuning::kShooterLead,
        target.y + playerVel.y * Tuning::kShooterLead
    };
    float dx = aim.x - origin.x;
    float dy = aim.y - origin.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) { dx = 1.0f; dy = 0.0f; len = 1.0f; }

    p->active = true;
    p->pos = origin;
    p->vel = { dx / len * Tuning::kEnemyProjSpeed, dy / len * Tuning::kEnemyProjSpeed };
    p->radius = Tuning::kEnemyProjRadius;
    p->damage = Tuning::kEnemyProjDamage;
    p->life = Tuning::kEnemyProjLifetime;
    p->color = c;
}

// Веер/залп: count снарядов, равномерно разложенных вокруг направления на игрока.
void RangedSystem::FireVolley(Vector2 origin, Vector2 target, int count, float spread, int damage, Color c)
{
    if (count < 1) count = 1;
    float base = atan2f(target.y - origin.y, target.x - origin.x);
    // Равномерно от -spread/2 до +spread/2.
    float step = (count > 1) ? (spread / (float)(count - 1)) : 0.0f;
    float start = base - spread * 0.5f;
    for (int i = 0; i < count; i++)
    {
        EnemyProjectile* p = GetInactive();
        if (!p) return;
        float a = (count > 1) ? (start + step * i) : base;
        p->active = true;
        p->pos = origin;
        p->vel = { cosf(a) * Tuning::kEnemyProjSpeed, sinf(a) * Tuning::kEnemyProjSpeed };
        p->radius = Tuning::kEnemyProjRadius;
        p->damage = damage;
        p->life = Tuning::kEnemyProjLifetime;
        p->color = c;
    }
}

void RangedSystem::Update(float dt, Player& player, Effects& effects)
{
    // Оценка скорости игрока (для упреждения в FireAimed).
    if (havePlayerPos && dt > 0.0001f)
    {
        playerVel.x = (player.position.x - lastPlayerPos.x) / dt;
        playerVel.y = (player.position.y - lastPlayerPos.y) / dt;
    }
    lastPlayerPos = player.position;
    havePlayerPos = true;

    for (auto& p : pool)
    {
        if (!p.active) continue;

        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.life -= dt;
        if (p.life <= 0.0f) { p.active = false; continue; }

        // Столкновение с игроком (круг против круга).
        float dx = player.position.x - p.pos.x;
        float dy = player.position.y - p.pos.y;
        float rr = p.radius + Tuning::kPlayerHitRadius;
        if (dx * dx + dy * dy <= rr * rr)
        {
            player.TakeDamage(p.damage);
            effects.SpawnBlood(player.position, 5);
            effects.Flash(Fade(WHITE, 0.25f), 0.08f);
            p.active = false;
        }
    }
}

void RangedSystem::Clear()
{
    for (auto& p : pool) p.active = false;
    havePlayerPos = false;
    playerVel = { 0.0f, 0.0f };
}

void RangedSystem::Draw() const
{
    for (auto& p : pool)
    {
        if (!p.active) continue;
        // Ядро + лёгкий ореол, чтобы снаряд был заметен на фоне.
        DrawCircleV(p.pos, p.radius + 2.0f, Fade(p.color, 0.35f));
        DrawCircleV(p.pos, p.radius, p.color);
        DrawCircleV(p.pos, p.radius * 0.45f, Fade(WHITE, 0.8f));
    }
}

void RangedSystem::DrawDebug() const
{
    for (auto& p : pool)
    {
        if (!p.active) continue;
        DrawCircleLines((int)p.pos.x, (int)p.pos.y, p.radius + Tuning::kPlayerHitRadius, YELLOW);
    }
}

int RangedSystem::ActiveCount() const
{
    int n = 0;
    for (auto& p : pool) if (p.active) n++;
    return n;
}
