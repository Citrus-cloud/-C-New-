#include "telegraph.h"
#include "player.h"
#include "effects.h"
#include "tuning.h"
#include <cmath>

// --------------------------- Telegraph (одна зона) ---------------------------

float Telegraph::Progress() const
{
    if (fillTime <= 0.0f) return 1.0f;
    float p = timer / fillTime;
    if (p < 0.0f) p = 0.0f;
    if (p > 1.0f) p = 1.0f;
    return p;
}

bool Telegraph::HitsPoint(Vector2 p) const
{
    float dx = p.x - origin.x;
    float dy = p.y - origin.y;
    switch (shape)
    {
        case TELEGRAPH_CIRCLE:
            return (dx * dx + dy * dy) <= (radius * radius);

        case TELEGRAPH_LINE:
        {
            // Проекция на направление луча + перпендикулярное расстояние.
            float cx = cosf(angle), sy = sinf(angle);
            float along = dx * cx + dy * sy;
            if (along < 0.0f || along > length) return false;
            float perp = fabsf(-dx * sy + dy * cx);
            return perp <= width * 0.5f;
        }

        case TELEGRAPH_CONE:
        {
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > length) return false;
            if (dist < 0.0001f) return true;   // в самом центре — считаем попаданием
            float a = atan2f(dy, dx);
            float diff = a - angle;
            while (diff > PI) diff -= 2.0f * PI;
            while (diff < -PI) diff += 2.0f * PI;
            return fabsf(diff) <= halfAngle;
        }
    }
    return false;
}

void Telegraph::Draw() const
{
    if (!active) return;

    float fillA, lineA;
    Color lineCol = color;

    if (triggered)
    {
        // Момент удара: яркая вспышка.
        fillA = 0.55f; lineA = 1.0f; lineCol = WHITE;
    }
    else
    {
        float prog = Progress();
        float timeLeft = fillTime - timer;
        fillA = 0.12f + 0.33f * prog;   // заливка постепенно ярче
        lineA = 0.50f + 0.50f * prog;
        // Мигание перед самым ударом (порог и частота — из конфига).
        if (timeLeft <= Tuning::kTelegraphBlinkStart)
        {
            float b = 0.5f + 0.5f * sinf(timer * Tuning::kTelegraphBlinkRate * 2.0f * PI);
            fillA *= 0.4f + 0.6f * b;
            lineA = b;
        }
    }

    Color fillCol = Fade(color, fillA);
    Color outCol  = Fade(lineCol, lineA);

    switch (shape)
    {
        case TELEGRAPH_CIRCLE:
            DrawCircleV(origin, radius, fillCol);
            DrawCircleLines((int)origin.x, (int)origin.y, radius, outCol);
            break;

        case TELEGRAPH_LINE:
        {
            // Заливка — повёрнутый прямоугольник (левый центр — в origin).
            Rectangle rec = { origin.x, origin.y, length, width };
            Vector2 piv = { 0.0f, width * 0.5f };
            DrawRectanglePro(rec, piv, angle * RAD2DEG, fillCol);
            // Контур — четыре угла повёрнутого прямоугольника.
            float cx = cosf(angle), sy = sinf(angle);
            Vector2 px = { -sy, cx };   // перпендикуляр
            Vector2 a0 = { origin.x + px.x * width * 0.5f, origin.y + px.y * width * 0.5f };
            Vector2 a1 = { origin.x - px.x * width * 0.5f, origin.y - px.y * width * 0.5f };
            Vector2 a2 = { a1.x + cx * length, a1.y + sy * length };
            Vector2 a3 = { a0.x + cx * length, a0.y + sy * length };
            DrawLineEx(a0, a3, 2.0f, outCol);
            DrawLineEx(a1, a2, 2.0f, outCol);
            DrawLineEx(a0, a1, 2.0f, outCol);
            DrawLineEx(a3, a2, 2.0f, outCol);
            break;
        }

        case TELEGRAPH_CONE:
        {
            float sa = angle * RAD2DEG - halfAngle * RAD2DEG;
            float ea = angle * RAD2DEG + halfAngle * RAD2DEG;
            DrawCircleSector(origin, length, sa, ea, 24, fillCol);
            DrawCircleSectorLines(origin, length, sa, ea, 24, outCol);
            break;
        }
    }
}

void Telegraph::DrawOutline(Color c) const
{
    if (!active) return;
    switch (shape)
    {
        case TELEGRAPH_CIRCLE:
            DrawCircleLines((int)origin.x, (int)origin.y, radius, c);
            break;
        case TELEGRAPH_LINE:
        {
            float cx = cosf(angle), sy = sinf(angle);
            Vector2 px = { -sy, cx };
            Vector2 a0 = { origin.x + px.x * width * 0.5f, origin.y + px.y * width * 0.5f };
            Vector2 a1 = { origin.x - px.x * width * 0.5f, origin.y - px.y * width * 0.5f };
            Vector2 a2 = { a1.x + cx * length, a1.y + sy * length };
            Vector2 a3 = { a0.x + cx * length, a0.y + sy * length };
            DrawLineEx(a0, a3, 2.0f, c);
            DrawLineEx(a1, a2, 2.0f, c);
            DrawLineEx(a0, a1, 2.0f, c);
            DrawLineEx(a3, a2, 2.0f, c);
            break;
        }
        case TELEGRAPH_CONE:
        {
            float sa = angle * RAD2DEG - halfAngle * RAD2DEG;
            float ea = angle * RAD2DEG + halfAngle * RAD2DEG;
            DrawCircleSectorLines(origin, length, sa, ea, 24, c);
            break;
        }
    }
}

// --------------------------- TelegraphSystem (пул) ---------------------------

TelegraphSystem::TelegraphSystem(int poolSize)
{
    pool.resize(poolSize);
}

Telegraph* TelegraphSystem::GetInactive()
{
    for (auto& t : pool)
        if (!t.active) return &t;
    return nullptr;
}

Telegraph* TelegraphSystem::SpawnCircle(Vector2 origin, float radius, int damage, float fillTime, Color c)
{
    Telegraph* t = GetInactive();
    if (!t) return nullptr;
    *t = Telegraph();
    t->active = true;
    t->shape = TELEGRAPH_CIRCLE;
    t->origin = origin;
    t->radius = radius;
    t->damage = damage;
    t->fillTime = fillTime;
    t->color = c;
    return t;
}

Telegraph* TelegraphSystem::SpawnLine(Vector2 origin, float angle, float length, float width, int damage, float fillTime, Color c)
{
    Telegraph* t = GetInactive();
    if (!t) return nullptr;
    *t = Telegraph();
    t->active = true;
    t->shape = TELEGRAPH_LINE;
    t->origin = origin;
    t->angle = angle;
    t->length = length;
    t->width = width;
    t->damage = damage;
    t->fillTime = fillTime;
    t->color = c;
    return t;
}

Telegraph* TelegraphSystem::SpawnCone(Vector2 origin, float angle, float length, float halfAngle, int damage, float fillTime, Color c)
{
    Telegraph* t = GetInactive();
    if (!t) return nullptr;
    *t = Telegraph();
    t->active = true;
    t->shape = TELEGRAPH_CONE;
    t->origin = origin;
    t->angle = angle;
    t->length = length;
    t->halfAngle = halfAngle;
    t->damage = damage;
    t->fillTime = fillTime;
    t->color = c;
    return t;
}

// Лазер: линия, которая следит за target до timer == lockTime, затем фиксируется.
Telegraph* TelegraphSystem::SpawnLaser(Vector2 origin, Vector2 target, float length, float width, int damage, float fillTime, float lockTime, Color c)
{
    Telegraph* t = GetInactive();
    if (!t) return nullptr;
    *t = Telegraph();
    t->active = true;
    t->shape = TELEGRAPH_LINE;
    t->origin = origin;
    t->angle = atan2f(target.y - origin.y, target.x - origin.x);   // начальное направление на игрока
    t->length = length;
    t->width = width;
    t->damage = damage;
    t->fillTime = fillTime;
    t->color = c;
    t->track = true;          // следить за игроком...
    t->lockAt = lockTime;     // ...до этого момента, потом фиксация
    return t;
}

void TelegraphSystem::Update(float dt, Player& player, Effects& effects)
{
    for (auto& t : pool)
    {
        if (!t.active) continue;

        if (!t.triggered)
        {
            // Лазер: пока не наступила фиксация — перенацеливаемся на игрока (Шаг 11-12).
            if (t.track && t.timer < t.lockAt)
            {
                float dx = player.position.x - t.origin.x;
                float dy = player.position.y - t.origin.y;
                t.angle = atan2f(dy, dx);
            }

            t.timer += dt;
            if (t.timer >= t.fillTime)
            {
                // Срабатывание: урон один раз, если игрок в зоне.
                t.triggered = true;
                t.linger = Tuning::kTelegraphLinger;
                if (t.HitsPoint(player.position))
                {
                    player.TakeDamage(t.damage);
                    effects.SpawnBlood(player.position, 6);
                    effects.Shake(5.0f, 0.2f);
                }
            }
        }
        else
        {
            t.linger -= dt;
            if (t.linger <= 0.0f) t.active = false;
        }
    }
}

void TelegraphSystem::Clear()
{
    for (auto& t : pool) t.active = false;
}

void TelegraphSystem::Draw() const
{
    for (auto& t : pool) t.Draw();
}

void TelegraphSystem::DrawDebug() const
{
    for (auto& t : pool)
        if (t.active) t.DrawOutline(YELLOW);
}

int TelegraphSystem::ActiveCount() const
{
    int n = 0;
    for (auto& t : pool) if (t.active) n++;
    return n;
}
