#include "player.h"
#include <cmath>

// Список инициализации: задаём начальные значения полей
Player::Player(Vector2 startPos)
    : position(startPos), speed(250.0f), health(100)
{
}

void Player::Update(float deltaTime)
{
    Vector2 dir = { 0.0f, 0.0f };  // направление движения

    // Клавиатура: WASD или стрелки
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dir.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dir.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dir.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1.0f;

    // Геймпад (левый стик), если подключён
    if (IsGamepadAvailable(0))
    {
        float gx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        float gy = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        if (fabsf(gx) > 0.2f) dir.x += gx;  // 0.2 - мёртвая зона стика
        if (fabsf(gy) > 0.2f) dir.y += gy;
    }

    // Нормализация: чтобы по диагонали не двигаться быстрее
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.0f)
    {
        dir.x /= len;
        dir.y /= len;
        // Движение с учётом deltaTime - одинаково на любом железе
        position.x += dir.x * speed * deltaTime;
        position.y += dir.y * speed * deltaTime;
    }
}

void Player::Draw() const
{
    // Пока игрок - просто красный квадрат 40x40 (потом будет спрайт)
    DrawRectangle((int)(position.x - 20), (int)(position.y - 20), 40, 40, RED);
}
