#pragma once
#include "raylib.h"
#include "tilemap.h"
#include <vector>

// Типы врагов
enum EnemyType { ENEMY_GRUNT, ENEMY_FAST, ENEMY_TANK, ENEMY_BOSS };

class Enemy
{
public:
    Vector2 position;
    float speed;
    int health;
    bool active;
    EnemyType type;
    float size;     // полу-размер прямоугольника
    Color color;
    int damage;     // урон игроку при касании
    int xpValue;    // сколько опыта выпадает

    // поиск пути A*
    std::vector<Vector2> path;
    int pathIndex;
    float repathTimer;

    // рывок (только боссы)
    bool dashing;
    float dashTimer;
    float dashCooldown;
    Vector2 dashDir;

    Enemy();
    void Spawn(Vector2 pos, EnemyType t);
    void Update(float deltaTime, Vector2 playerPos, const TileMap& map);
    void Draw() const;
    Rectangle GetRect() const;
};
