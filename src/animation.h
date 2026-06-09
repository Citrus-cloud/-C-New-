#pragma once
#include "raylib.h"

// =====================================================================
// Animation (Шаг 3)
// Универсальная анимация по спрайт-листу (sprite sheet).
// Кадры лежат в один горизонтальный ряд; мы двигаем "окошко" (исходный
// прямоугольник) по таймеру и рисуем нужный кадр.
// Класс общий: используется для игрока, а позже для врагов, боссов и
// эффектов (задел на масштабирование).
// =====================================================================
class Animation
{
public:
    Animation() = default;
    // texture   — спрайт-лист; frameCount — число кадров в ряду;
    // frameSpeed — кадров в секунду; loop — зацикливать ли анимацию.
    Animation(Texture2D texture, int frameCount, float frameSpeed, bool loop = true);

    // Продвигает анимацию по времени dt.
    void Update(float dt);

    // Сброс на первый кадр (например при смене анимации).
    void Reset();

    // Закончилась ли незацикленная анимация.
    bool Finished() const { return finished; }

    // Нарисовать текущий кадр по центру позиции pos.
    // scale — масштаб, flipX — отражение по горизонтали (направление),
    // tint — цветовая подсветка (WHITE = без изменений).
    void Draw(Vector2 pos, float scale, bool flipX, Color tint) const;

    // Готова ли анимация к отрисовке (есть текстура и кадры).
    bool Valid() const { return texture.id != 0 && frameCount > 0; }

private:
    Texture2D texture{};
    int frameCount = 0;
    int frameWidth = 0;
    int frameHeight = 0;
    float frameSpeed = 0.0f;
    float timer = 0.0f;
    int current = 0;
    bool loop = true;
    bool finished = false;
};
