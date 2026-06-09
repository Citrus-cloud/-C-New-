#pragma once
#include "raylib.h"
#include "animation.h"
#include <vector>

class TextureManager;

// =====================================================================
// Effects (Фаза 4) — «сочность» (juice) игры:
//  - система частиц (искры, кровь, пыль)         — Шаг 15
//  - тряска экрана и вспышки                      — Шаг 16
//  - VFX-спрайты (взрыв, магический круг)         — Шаг 17
//  - всплывающие числа урона                      — Шаг 18
//  - плавные затемнения/переходы                  — Шаг 19
// Всё на пулах объектов и переиспользуемо, с заделом на расширение.
// =====================================================================

// Одна частица (искра/кровь/пыль). Живёт в пуле (флаг active).
struct Particle
{
    Vector2 pos;
    Vector2 vel;
    float life;       // оставшееся время жизни
    float maxLife;    // полное время жизни (для прозрачности)
    float radius;
    Color color;
    float gravity;    // притяжение вниз (для крови)
    bool active;
    Particle() : pos{ 0,0 }, vel{ 0,0 }, life(0), maxLife(1), radius(2), color(WHITE), gravity(0), active(false) {}
};

// Всплывающее число урона (Шаг 18).
struct DamageNumber
{
    Vector2 pos;
    float life;
    float maxLife;
    int value;
    float scale;
    Color color;
    bool active;
    DamageNumber() : pos{ 0,0 }, life(0), maxLife(1), value(0), scale(1), color(WHITE), active(false) {}
};

// VFX-эффект на спрайт-анимации (Шаг 17): взрыв, магический круг.
struct VfxInstance
{
    Vector2 pos;
    float scale;
    Animation anim;   // если валидна — рисуем спрайт, иначе процедурный круг
    float life;
    float maxLife;
    int kind;         // 0 = взрыв, 1 = магический круг (для fallback)
    bool active;
    VfxInstance() : pos{ 0,0 }, scale(1), life(0), maxLife(0.4f), kind(0), active(false) {}
};

class Effects
{
public:
    Effects();

    void LoadArt(TextureManager& tex);   // загрузка VFX-спрайтов (Шаг 17)

    // --- Частицы (Шаг 15) ---
    void SpawnSparks(Vector2 pos, Color color, int count);
    void SpawnBlood(Vector2 pos, int count);
    void SpawnDust(Vector2 pos, int count);

    // --- Числа урона (Шаг 18) ---
    void SpawnDamageNumber(Vector2 pos, int value, bool big);

    // --- VFX-спрайты (Шаг 17) ---
    void SpawnExplosion(Vector2 pos, float scale);
    void SpawnMagicCircle(Vector2 pos, float scale);

    // --- Тряска и вспышки (Шаг 16) ---
    void Shake(float strength, float duration);
    void Flash(Color color, float strength);   // strength 0..1

    // --- Плавные переходы (Шаг 19) ---
    void SetFade(float target, float speed);    // target 0..1
    float Fade() const { return fade; }

    void Update(float dt);

    // Отрисовка в мировых координатах (внутри BeginMode2D).
    void DrawWorld() const;
    // Отрисовка поверх экрана: вспышка + затемнение (вне BeginMode2D).
    void DrawScreen(int screenWidth, int screenHeight) const;

    // Текущее смещение тряски — прибавляется к camera.offset.
    Vector2 ShakeOffset() const { return shakeOffset; }

    void Clear();   // сброс всех эффектов (новый забег)

private:
    std::vector<Particle> particles;
    std::vector<DamageNumber> numbers;
    std::vector<VfxInstance> vfx;

    // тряска
    float shakeTime, shakeDur, shakeStrength;
    Vector2 shakeOffset;

    // вспышка
    Color flashColor;
    float flashAlpha;     // текущее 0..1

    // затемнение / переход
    float fade, fadeTarget, fadeSpeed;

    // прототипы VFX-анимаций
    Animation artExplosion;
    Animation artMagic;

    Particle*     GetInactiveParticle();
    DamageNumber* GetInactiveNumber();
    VfxInstance*  GetInactiveVfx();
};
