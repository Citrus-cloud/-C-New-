#pragma once
#include "raylib.h"
#include "tilemap.h"
#include "animation.h"
#include <vector>

// Типы врагов
enum EnemyType { ENEMY_GRUNT, ENEMY_FAST, ENEMY_TANK, ENEMY_BOSS };

class Enemy
{
public:
    Vector2 position;
    float speed;
    int health;
    int maxHealth;   // полное здоровье — для полоски (Шаг 9)
    bool active;
    EnemyType type;
    float size;      // полу-размер прямоугольника
    Color color;
    int damage;      // урон игроку при касании
    int xpValue;     // сколько опыта выпадает

    bool facingLeft; // куда смотрит спрайт (Шаг 6)

    // Смерть (Шаг 7): короткая анимация перед исчезновением
    bool dying;
    float deathTimer;

    // Спрайты (назначаются спавнером при создании)
    Animation walkAnim;
    Animation deathAnim;

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
    void Kill();   // начать смерть (или сразу убрать, если нет спрайта смерти)
    Rectangle GetRect() const;

private:
    // Рисует полоску здоровья над врагом (Шаг 9).
    void DrawHealthBar() const;
};
