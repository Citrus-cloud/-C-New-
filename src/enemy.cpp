#include "enemy.h"
#include "pathfinding.h"
#include <cmath>

Enemy::Enemy()
    : position({ 0.0f, 0.0f }), speed(120.0f), health(30), active(false),
      type(ENEMY_GRUNT), size(16.0f), color(PURPLE), damage(5), xpValue(1),
      pathIndex(0), repathTimer(0.0f),
      dashing(false), dashTimer(0.0f), dashCooldown(3.0f), dashDir({ 0.0f, 0.0f })
{
}

void Enemy::Spawn(Vector2 pos, EnemyType t)
{
    position = pos;
    active = true;
    type = t;
    path.clear();
    pathIndex = 0;
    repathTimer = 0.0f;
    dashing = false;
    dashTimer = 0.0f;
    dashDir = { 0.0f, 0.0f };

    switch (t)
    {
        case ENEMY_FAST:
            health = 15; speed = 220.0f; size = 12.0f; color = ORANGE; damage = 4; xpValue = 1;
            break;
        case ENEMY_TANK:
            health = 90; speed = 70.0f; size = 24.0f; color = DARKGRAY; damage = 10; xpValue = 3;
            break;
        case ENEMY_BOSS:
            health = 700; speed = 95.0f; size = 38.0f; color = MAROON; damage = 20; xpValue = 25;
            dashCooldown = 2.5f;
            break;
        case ENEMY_GRUNT:
        default:
            health = 30; speed = 120.0f; size = 16.0f; color = PURPLE; damage = 5; xpValue = 1;
            break;
    }
}

Rectangle Enemy::GetRect() const
{
    return { position.x - size, position.y - size, size * 2.0f, size * 2.0f };
}

// Движение к точке с учётом стен (раздельно по осям)
static void MoveToward(Vector2& pos, Vector2 target, float dist, float halfSize, const TileMap& map)
{
    float dx = target.x - pos.x, dy = target.y - pos.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;
    dx /= len; dy /= len;
    float mx = dx * dist, my = dy * dist;

    pos.x += mx;
    if (map.CheckCollision({ pos.x - halfSize, pos.y - halfSize, halfSize * 2.0f, halfSize * 2.0f })) pos.x -= mx;
    pos.y += my;
    if (map.CheckCollision({ pos.x - halfSize, pos.y - halfSize, halfSize * 2.0f, halfSize * 2.0f })) pos.y -= my;
}

void Enemy::Update(float deltaTime, Vector2 playerPos, const TileMap& map)
{
    if (!active) return;

    // РЫВОК БОССОВ: периодический рывок в сторону игрока
    if (type == ENEMY_BOSS)
    {
        if (dashing)
        {
            float ds = 750.0f * deltaTime;
            float mx = dashDir.x * ds, my = dashDir.y * ds;
            position.x += mx;
            if (map.CheckCollision(GetRect())) position.x -= mx;
            position.y += my;
            if (map.CheckCollision(GetRect())) position.y -= my;
            dashTimer -= deltaTime;
            if (dashTimer <= 0.0f) dashing = false;
            return;
        }
        else
        {
            dashCooldown -= deltaTime;
            if (dashCooldown <= 0.0f)
            {
                Vector2 d = { playerPos.x - position.x, playerPos.y - position.y };
                float len = sqrtf(d.x * d.x + d.y * d.y);
                if (len > 0.01f) { d.x /= len; d.y /= len; }
                dashDir = d;
                dashing = true;
                dashTimer = 0.35f;
                dashCooldown = 3.0f;
                return;
            }
        }
    }

    float step = speed * deltaTime;

    if (HasLineOfSight(map, position, playerPos))
    {
        path.clear();
        MoveToward(position, playerPos, step, size, map);
        return;
    }

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
        MoveToward(position, target, step, size, map);
        float dx = target.x - position.x, dy = target.y - position.y;
        if (dx * dx + dy * dy < 100.0f) pathIndex++;
    }
}

void Enemy::Draw() const
{
    if (!active) return;
    Color c = dashing ? ORANGE : color;  // во время рывка — телеграф
    DrawRectangle((int)(position.x - size), (int)(position.y - size),
                  (int)(size * 2.0f), (int)(size * 2.0f), c);
    if (type == ENEMY_BOSS)
        DrawRectangleLines((int)(position.x - size), (int)(position.y - size),
                           (int)(size * 2.0f), (int)(size * 2.0f), YELLOW);
}
