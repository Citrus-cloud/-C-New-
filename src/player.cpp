#include "player.h"
#include <cmath>

Player::Player(Vector2 startPos)
    : position(startPos), speed(250.0f), health(100), maxHealth(100),
      hitCooldown(0.0f), gotHit(false), xp(0), level(1), xpToNext(5)
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
        xpToNext = (int)(xpToNext * 1.5f);
        return true;
    }
    return false;
}

void Player::TakeDamage(int dmg)
{
    if (hitCooldown > 0.0f) return;
    health -= dmg;
    if (health < 0) health = 0;
    hitCooldown = 0.5f;
    gotHit = true;
}

void Player::Heal(int amount)
{
    health += amount;
    if (health > maxHealth) health = maxHealth;
}

void Player::ResolveStuck(const TileMap& map)
{
    if (!map.CheckCollision(GetRect())) return;
    position = map.FindFreeSpot(position, 20.0f);
}

void Player::Update(float deltaTime, const TileMap& map)
{
    gotHit = false;
    if (hitCooldown > 0.0f) hitCooldown -= deltaTime;
    ResolveStuck(map);

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
        float dx = dir.x * speed * deltaTime;
        float dy = dir.y * speed * deltaTime;

        position.x += dx;
        if (map.CheckCollision(GetRect())) position.x -= dx;
        position.y += dy;
        if (map.CheckCollision(GetRect())) position.y -= dy;
    }
}

void Player::Draw() const
{
    Color c = (hitCooldown > 0.0f) ? Color{ 255, 150, 150, 255 } : RED;
    DrawRectangle((int)(position.x - 20), (int)(position.y - 20), 40, 40, c);
}
