#include "player.h"
#include <cmath>

Player::Player(Vector2 startPos)
    : position(startPos), speed(250.0f), health(100),
      xp(0), level(1), xpToNext(5),
      lastDir({ 1.0f, 0.0f }), dashTimer(0.0f), dashCooldown(0.0f)
{
}

Rectangle Player::GetRect() const
{
    return { position.x - 20.0f, position.y - 20.0f, 40.0f, 40.0f };
}

bool Player::TryLevelUp()
{
    if (xp >= xpToNext)
    {
        xp -= xpToNext;
        level += 1;
        xpToNext = (int)(xpToNext * 1.5f);  // каждый уровень дороже
        return true;
    }
    return false;
}

void Player::Update(float deltaTime, const TileMap& map)
{
    // Таймеры рывка
    if (dashCooldown > 0.0f) dashCooldown -= deltaTime;
    if (dashTimer > 0.0f)    dashTimer -= deltaTime;

    Vector2 dir = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dir.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dir.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dir.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1.0f;

    if (IsGamepadAvailable(0))
    {
        float gx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        float gy = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        if (fabsf(gx) > 0.2f) dir.x += gx;
        if (fabsf(gy) > 0.2f) dir.y += gy;
    }

    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.0f)
    {
        dir.x /= len;
        dir.y /= len;
        lastDir = dir;  // запоминаем направление движения
    }

    // Старт рывка: Space / Shift / кнопка геймпада, если перезарядка готова
    bool dashPressed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT_SHIFT) ||
        (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
    if (dashPressed && dashCooldown <= 0.0f)
    {
        dashTimer = 0.15f;
        dashCooldown = 1.5f;
    }

    // Выбор скорости: во время рывка летим быстро по lastDir
    Vector2 moveDir = { 0.0f, 0.0f };
    float moveSpeed = speed;
    if (dashTimer > 0.0f)
    {
        moveDir = lastDir;
        moveSpeed = 900.0f;
    }
    else if (len > 0.0f)
    {
        moveDir = dir;
    }

    float dx = moveDir.x * moveSpeed * deltaTime;
    float dy = moveDir.y * moveSpeed * deltaTime;

    position.x += dx;
    if (map.CheckCollision(GetRect())) position.x -= dx;

    position.y += dy;
    if (map.CheckCollision(GetRect())) position.y -= dy;
}

void Player::Draw() const
{
    // Во время рывка подсвечиваем игрока
    Color c = (dashTimer > 0.0f) ? SKYBLUE : RED;
    DrawRectangle((int)(position.x - 20), (int)(position.y - 20), 40, 40, c);
}
