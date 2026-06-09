#include "effects.h"
#include "textures.h"
#include <cmath>
#include <cstdio>

Effects::Effects()
    : shakeTime(0), shakeDur(0), shakeStrength(0), shakeOffset{ 0,0 },
      flashColor{ 255,255,255,255 }, flashAlpha(0.0f),
      fade(0.0f), fadeTarget(0.0f), fadeSpeed(0.0f)
{
    // Пулы заранее выделенного размера (не аллоцируем во время игры).
    particles.resize(600);
    numbers.resize(120);
    vfx.resize(64);
}

void Effects::LoadArt(TextureManager& tex)
{
    // Спрайт-листы VFX (если файлов нет — будет процедурный fallback).
    if (tex.IsReal("assets/vfx/explosion.png"))
        artExplosion = Animation(tex.Get("assets/vfx/explosion.png"), 6, 18.0f, false);
    else
        artExplosion = Animation();

    if (tex.IsReal("assets/vfx/magic_circle.png"))
        artMagic = Animation(tex.Get("assets/vfx/magic_circle.png"), 6, 14.0f, false);
    else
        artMagic = Animation();
}

Particle* Effects::GetInactiveParticle()
{
    for (Particle& p : particles) if (!p.active) return &p;
    return nullptr;
}
DamageNumber* Effects::GetInactiveNumber()
{
    for (DamageNumber& n : numbers) if (!n.active) return &n;
    return nullptr;
}
VfxInstance* Effects::GetInactiveVfx()
{
    for (VfxInstance& v : vfx) if (!v.active) return &v;
    return nullptr;
}

// Случайное float в диапазоне [a, b].
static float frand(float a, float b)
{
    return a + (b - a) * (GetRandomValue(0, 10000) / 10000.0f);
}

void Effects::SpawnSparks(Vector2 pos, Color color, int count)
{
    for (int i = 0; i < count; i++)
    {
        Particle* p = GetInactiveParticle();
        if (!p) return;
        float ang = frand(0.0f, 2.0f * PI);
        float spd = frand(60.0f, 220.0f);
        p->pos = pos;
        p->vel = { cosf(ang) * spd, sinf(ang) * spd };
        p->maxLife = p->life = frand(0.2f, 0.5f);
        p->radius = frand(1.5f, 3.5f);
        p->color = color;
        p->gravity = 0.0f;
        p->active = true;
    }
}

void Effects::SpawnBlood(Vector2 pos, int count)
{
    for (int i = 0; i < count; i++)
    {
        Particle* p = GetInactiveParticle();
        if (!p) return;
        float ang = frand(0.0f, 2.0f * PI);
        float spd = frand(40.0f, 180.0f);
        p->pos = pos;
        p->vel = { cosf(ang) * spd, sinf(ang) * spd - 40.0f };
        p->maxLife = p->life = frand(0.3f, 0.7f);
        p->radius = frand(2.0f, 4.0f);
        p->color = Color{ (unsigned char)GetRandomValue(150, 210), 20, 20, 255 };
        p->gravity = 320.0f;
        p->active = true;
    }
}

void Effects::SpawnDust(Vector2 pos, int count)
{
    for (int i = 0; i < count; i++)
    {
        Particle* p = GetInactiveParticle();
        if (!p) return;
        float ang = frand(0.0f, 2.0f * PI);
        float spd = frand(20.0f, 90.0f);
        p->pos = pos;
        p->vel = { cosf(ang) * spd, sinf(ang) * spd };
        p->maxLife = p->life = frand(0.3f, 0.6f);
        p->radius = frand(2.0f, 5.0f);
        unsigned char g = (unsigned char)GetRandomValue(120, 170);
        p->color = Color{ g, g, (unsigned char)(g - 20), 255 };
        p->gravity = 0.0f;
        p->active = true;
    }
}

void Effects::SpawnDamageNumber(Vector2 pos, int value, bool big)
{
    DamageNumber* n = GetInactiveNumber();
    if (!n) return;
    n->pos = { pos.x + frand(-6.0f, 6.0f), pos.y - 10.0f };
    n->maxLife = n->life = 0.8f;
    n->value = value;
    n->scale = big ? 1.7f : 1.0f;
    n->color = big ? Color{ 255, 210, 40, 255 } : Color{ 255, 255, 255, 255 };
    n->active = true;
}

void Effects::SpawnExplosion(Vector2 pos, float scale)
{
    VfxInstance* v = GetInactiveVfx();
    if (v)
    {
        v->pos = pos;
        v->scale = scale;
        v->kind = 0;
        v->anim = artExplosion;   // копия прототипа
        v->anim.Reset();
        v->maxLife = v->life = 0.4f;
        v->active = true;
    }
    // Juice даже без спрайта: искры + пыль.
    SpawnSparks(pos, Color{ 255, 170, 60, 255 }, (int)(8 * scale));
    SpawnDust(pos, (int)(6 * scale));
}

void Effects::SpawnMagicCircle(Vector2 pos, float scale)
{
    VfxInstance* v = GetInactiveVfx();
    if (!v) return;
    v->pos = pos;
    v->scale = scale;
    v->kind = 1;
    v->anim = artMagic;
    v->anim.Reset();
    v->maxLife = v->life = 0.6f;
    v->active = true;
}

void Effects::Shake(float strength, float duration)
{
    // Берём более сильную тряску, чтобы новая не «гасила» текущую.
    if (strength >= shakeStrength || shakeTime <= 0.0f)
    {
        shakeStrength = strength;
        shakeDur = duration;
        shakeTime = duration;
    }
}

void Effects::Flash(Color color, float strength)
{
    flashColor = color;
    if (strength > flashAlpha) flashAlpha = strength;
}

void Effects::SetFade(float target, float speed)
{
    fadeTarget = target;
    fadeSpeed = speed;
}

void Effects::Update(float dt)
{
    // Частицы
    for (Particle& p : particles)
    {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0.0f) { p.active = false; continue; }
        p.vel.y += p.gravity * dt;
        p.vel.x *= (1.0f - 2.0f * dt);   // лёгкое трение
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
    }
    // Числа урона — всплывают вверх
    for (DamageNumber& n : numbers)
    {
        if (!n.active) continue;
        n.life -= dt;
        if (n.life <= 0.0f) { n.active = false; continue; }
        n.pos.y -= 34.0f * dt;
    }
    // VFX
    for (VfxInstance& v : vfx)
    {
        if (!v.active) continue;
        v.life -= dt;
        if (v.anim.Valid())
        {
            v.anim.Update(dt);
            if (v.anim.Finished()) v.active = false;
        }
        if (v.life <= 0.0f) v.active = false;
    }
    // Тряска
    if (shakeTime > 0.0f)
    {
        shakeTime -= dt;
        float k = (shakeDur > 0.0f) ? (shakeTime / shakeDur) : 0.0f;
        if (k < 0.0f) k = 0.0f;
        float amp = shakeStrength * k;
        shakeOffset = { frand(-amp, amp), frand(-amp, amp) };
        if (shakeTime <= 0.0f) { shakeOffset = { 0,0 }; shakeStrength = 0.0f; }
    }
    else
    {
        shakeOffset = { 0,0 };
    }
    // Вспышка затухает
    if (flashAlpha > 0.0f)
    {
        flashAlpha -= dt * 2.5f;
        if (flashAlpha < 0.0f) flashAlpha = 0.0f;
    }
    // Затемнение плавно идёт к цели
    if (fade < fadeTarget) { fade += fadeSpeed * dt; if (fade > fadeTarget) fade = fadeTarget; }
    else if (fade > fadeTarget) { fade -= fadeSpeed * dt; if (fade < fadeTarget) fade = fadeTarget; }
}

void Effects::DrawWorld() const
{
    // Частицы
    for (const Particle& p : particles)
    {
        if (!p.active) continue;
        float a = p.life / p.maxLife;
        if (a > 1.0f) a = 1.0f;
        if (a < 0.0f) a = 0.0f;
        Color c = p.color;
        c.a = (unsigned char)(255 * a);
        DrawCircleV(p.pos, p.radius, c);
    }
    // VFX
    for (const VfxInstance& v : vfx)
    {
        if (!v.active) continue;
        if (v.anim.Valid())
        {
            float scale = 1.0f;
            int fh = v.anim.FrameHeight();
            if (fh > 0) scale = (96.0f * v.scale) / fh;   // нормируем размер
            v.anim.Draw(v.pos, scale, false, WHITE);
        }
        else
        {
            // Процедурный fallback: расширяющееся кольцо.
            float t = 1.0f - (v.life / v.maxLife);   // 0..1
            float r = (v.kind == 0 ? 40.0f : 50.0f) * v.scale * (0.2f + t);
            unsigned char a = (unsigned char)(220 * (1.0f - t));
            Color c = (v.kind == 0) ? Color{ 255, 160, 40, a } : Color{ 120, 160, 255, a };
            DrawCircleLines((int)v.pos.x, (int)v.pos.y, r, c);
            DrawCircleLines((int)v.pos.x, (int)v.pos.y, r * 0.7f, c);
        }
    }
    // Числа урона (цифры — обычный шрифт, кириллица не нужна)
    for (const DamageNumber& n : numbers)
    {
        if (!n.active) continue;
        float a = n.life / n.maxLife;
        if (a > 1.0f) a = 1.0f;
        if (a < 0.0f) a = 0.0f;
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", n.value);
        int fontSize = (int)(20 * n.scale);
        int w = MeasureText(buf, fontSize);
        Color c = n.color;
        c.a = (unsigned char)(255 * a);
        DrawText(buf, (int)n.pos.x - w / 2 + 1, (int)n.pos.y + 1, fontSize, Color{ 0, 0, 0, (unsigned char)(180 * a) });
        DrawText(buf, (int)n.pos.x - w / 2, (int)n.pos.y, fontSize, c);
    }
}

void Effects::DrawScreen(int screenWidth, int screenHeight) const
{
    if (flashAlpha > 0.0f)
    {
        Color c = flashColor;
        c.a = (unsigned char)(180 * flashAlpha);
        DrawRectangle(0, 0, screenWidth, screenHeight, c);
    }
    if (fade > 0.0f)
    {
        DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, (unsigned char)(255 * fade) });
    }
}

void Effects::Clear()
{
    for (Particle& p : particles) p.active = false;
    for (DamageNumber& n : numbers) n.active = false;
    for (VfxInstance& v : vfx) v.active = false;
    shakeTime = 0.0f; shakeStrength = 0.0f; shakeOffset = { 0,0 };
    flashAlpha = 0.0f;
    fade = 0.0f; fadeTarget = 0.0f;
}
