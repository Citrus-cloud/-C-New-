#include "animation.h"

// Конструктор: вычисляет размер одного кадра из ширины листа.
Animation::Animation(Texture2D tex, int frames, float speed, bool loopAnim)
    : texture(tex), frameCount(frames), frameSpeed(speed), loop(loopAnim)
{
    if (frameCount > 0)
    {
        frameWidth = texture.width / frameCount; // ширина кадра = ширина листа / число кадров
        frameHeight = texture.height;
    }
}

// Продвигает анимацию: по накоплению времени переключает кадр.
void Animation::Update(float dt)
{
    if (frameCount <= 1 || frameSpeed <= 0.0f) return;
    timer += dt;
    float frameTime = 1.0f / frameSpeed;
    while (timer >= frameTime)
    {
        timer -= frameTime;
        current++;
        if (current >= frameCount)
        {
            if (loop) current = 0;
            else { current = frameCount - 1; finished = true; }
        }
    }
}

// Сбрасывает анимацию в начало.
void Animation::Reset()
{
    current = 0;
    timer = 0.0f;
    finished = false;
}

// Рисует текущий кадр по центру позиции pos.
void Animation::Draw(Vector2 pos, float scale, bool flipX, Color tint) const
{
    if (!Valid()) return;
    // Исходный прямоугольник = текущий кадр в листе.
    // Отрицательная ширина в raylib = отражение спрайта по горизонтали.
    Rectangle src = {
        (float)(current * frameWidth), 0.0f,
        flipX ? -(float)frameWidth : (float)frameWidth,
        (float)frameHeight
    };
    float w = frameWidth * scale;
    float h = frameHeight * scale;
    Rectangle dst = { pos.x, pos.y, w, h };
    Vector2 origin = { w / 2.0f, h / 2.0f }; // центрируем спрайт по позиции
    DrawTexturePro(texture, src, dst, origin, 0.0f, tint);
}
