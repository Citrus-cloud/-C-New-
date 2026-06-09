#pragma once
#include "raylib.h"
#include "tilemap.h"
#include "animation.h"
#include "bosses.h"
#include <vector>

// Типы врагов
enum EnemyType { ENEMY_GRUNT, ENEMY_FAST, ENEMY_TANK, ENEMY_BOSS };

class Enemy
{
public:
    Vector2 position;
    float speed;
    int health;
    int maxHealth;
    bool active;
    EnemyType type;
    float size;
    Color color;
    int damage;
    int xpValue;

    bool facingLeft;

    bool dying;
    float deathTimer;

    Animation walkAnim;
    Animation deathAnim;

    std::vector<Vector2> path;
    int pathIndex;
    float repathTimer;

    bool dashing;
    float dashTimer;
    float dashCooldown;
    Vector2 dashDir;

    // Идентичность босса (Фаза 5). Для обычных врагов bossId = -1.
    int bossId;
    bool canDash;       // включён ли рывок
    bool canSummon;     // включён ли призыв миньонов
    float summonTimer;  // отсчёт до следующего призыва

    Enemy();
    void Spawn(Vector2 pos, EnemyType t);
    void ApplyBoss(const BossDef& def);   // применить статы и механики босса (Шаг 20-22)
    void Update(float deltaTime, Vector2 playerPos, const TileMap& map);
    void Draw() const;
    void Kill();
    Rectangle GetRect() const;

private:
    void DrawHealthBar() const;
};
