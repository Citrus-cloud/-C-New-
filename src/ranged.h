#pragma once
#include "raylib.h"
#include <vector>
// ============================================================================
//  ranged.h — СИСТЕМА ДАЛЬНИХ АТАК / СНАРЯДОВ ВРАГОВ (Фаза 3)
// ----------------------------------------------------------------------------
//  Отдельный пул снарядов, которые летят в игрока (не путать с projectile.* —
//  те принадлежат игроку). Используется врагами-стрелками и боссами.
//  Два режима стрельбы:
//    * FireAimed  — одиночный выстрел с упреждением (лидом по скорости игрока)
//    * FireVolley — веер/залп из count снарядов в конусе spread (босс-паттерн)
//  Все числа — из tuning.h (скорость, радиус, урон, время жизни, упреждение).
// ============================================================================

class Player;
class Effects;

// Один снаряд врага.
struct EnemyProjectile {
    bool active = false;
    Vector2 pos = { 0.0f, 0.0f };
    Vector2 vel = { 0.0f, 0.0f };
    float radius = 6.0f;
    int   damage = 0;
    float life = 0.0f;      // остаток времени жизни (сек); <=0 — убрать
    Color color = RED;
};

class RangedSystem {
public:
    std::vector<EnemyProjectile> pool;

    // Для упреждения: оцениваем скорость игрока по разнице позиций между кадрами.
    Vector2 lastPlayerPos = { 0.0f, 0.0f };
    Vector2 playerVel = { 0.0f, 0.0f };
    bool havePlayerPos = false;

    RangedSystem(int poolSize = 256);
    EnemyProjectile* GetInactive();

    // Одиночный выстрел с упреждением: целимся не в текущую точку, а туда,
    // куда игрок придёт (target + playerVel * kShooterLead).
    void FireAimed(Vector2 origin, Vector2 target, Color c);

    // Залп/веер: count снарядов, равномерно разложенных в конусе spread (рад).
    void FireVolley(Vector2 origin, Vector2 target, int count, float spread, int damage, Color c);

    // Движение снарядов + столкновение с игроком + списание по времени жизни.
    void Update(float dt, Player& player, Effects& effects);
    void Clear();
    void Draw() const;
    void DrawDebug() const;
    int  ActiveCount() const;
};
