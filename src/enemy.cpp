#include "enemy.h"
#include <cmath>

Enemy::Enemy()
    : position({ 0.0f, 0.0f }), speed(120.0f), health(30), active(false)
{
}

void Enemy::Spawn(Vector2 pos)
{
    position = pos;
    health = 30;
    active = true;
}

void Enemy::Update(float deltaTime, Vector2 playerPos)
{
    if (!active) return;  // неактивных пропускаем

    // Простой ИИ: вектор от врага к игроку, нормализуем и идём
    Vector2 dir = { playerPos.x - position.x, playerPos.y - position.y };
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.01f)
    {
        dir.x /= len;
        dir.y /= len;
        position.x += dir.x * speed * deltaTime;
        position.y += dir.y * speed * deltaTime;
    }
}

void Enemy::Draw() const
{
    if (!active) return;
    DrawRectangle((int)(position.x - 16), (int)(position.y - 16), 32, 32, PURPLE);
}

Rectangle Enemy::GetRect() const
{
    return { position.x - 16.0f, position.y - 16.0f, 32.0f, 32.0f };
}
