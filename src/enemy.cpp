#include "enemy.h"
#include "pathfinding.h"
#include <cmath>

Enemy::Enemy()
    : position({ 0.0f, 0.0f }), speed(120.0f), health(30), active(false),
      pathIndex(0), repathTimer(0.0f)
{
}

void Enemy::Spawn(Vector2 pos)
{
    position = pos;
    health = 30;
    active = true;
    path.clear();
    pathIndex = 0;
    repathTimer = 0.0f;
}

Rectangle Enemy::GetRect() const
{
    return { position.x - 16.0f, position.y - 16.0f, 32.0f, 32.0f };
}

// Движение к точке с учётом стен (раздельно по осям)
static void MoveToward(Vector2& pos, Vector2 target, float dist, const TileMap& map)
{
    float dx = target.x - pos.x, dy = target.y - pos.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;
    dx /= len; dy /= len;
    float mx = dx * dist, my = dy * dist;

    pos.x += mx;
    if (map.CheckCollision({ pos.x - 16.0f, pos.y - 16.0f, 32.0f, 32.0f })) pos.x -= mx;
    pos.y += my;
    if (map.CheckCollision({ pos.x - 16.0f, pos.y - 16.0f, 32.0f, 32.0f })) pos.y -= my;
}

void Enemy::Update(float deltaTime, Vector2 playerPos, const TileMap& map)
{
    if (!active) return;
    float step = speed * deltaTime;

    // Если игрок виден напрямую - бежим прямо (дешёво и быстро)
    if (HasLineOfSight(map, position, playerPos))
    {
        path.clear();
        MoveToward(position, playerPos, step, map);
        return;
    }

    // Иначе - ищем путь A* (редко, с кешем)
    repathTimer -= deltaTime;
    if (repathTimer <= 0.0f || path.empty() || pathIndex >= (int)path.size())
    {
        path = FindPath(map, position, playerPos);
        pathIndex = 0;
        repathTimer = 0.5f;
    }

    if (!path.empty() && pathIndex < (int)path.size())
    {
        Vector2 target = path[pathIndex];
        MoveToward(position, target, step, map);
        float dx = target.x - position.x, dy = target.y - position.y;
        if (dx * dx + dy * dy < 100.0f) pathIndex++;  // дошли до узла (~10px)
    }
}

void Enemy::Draw() const
{
    if (!active) return;
    DrawRectangle((int)(position.x - 16), (int)(position.y - 16), 32, 32, PURPLE);
}
