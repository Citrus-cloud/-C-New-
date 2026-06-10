#pragma once
#include "raylib.h"
#include "tilemap.h"
#include "animation.h"
#include "bosses.h"
#include <vector>

// Типы врагов
enum EnemyType { ENEMY_GRUNT, ENEMY_FAST, ENEMY_TANK, ENEMY_BOSS };

// Приёмы мобильности врага (Фаза 4, Шаг 17-21). Назначаются при спавне среди
// разблокированных по времени забега приёмов (см. Spawner::MaybeAssignMobility).
// Вся геометрия и тайминги каждого приёма берутся из tuning.h.
enum MobilityKind {
    MOB_NONE = 0,    // обычное преследование игрока
    MOB_JUMP_SLAM,   // прыжок-наскок с ударом по площади (Шаг 17)
    MOB_TELEPORT,    // телепорт к игроку (Шаг 18)
    MOB_BLINK,       // серия коротких блинков (Шаг 19)
    MOB_DASH,        // разгон-рывок с телеграфом пути (Шаг 20)
    MOB_FLANK        // держит дистанцию и заходит сбоку (Шаг 21)
};

class TelegraphSystem;   // форвард: враг «заказывает» зоны (slam, путь рывка)
class Effects;           // форвард: визуальные эффекты телепорта/приземления

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

    // Рывок (Шаг 20): подготовка (телеграф-линия) -> активный рывок.
    bool dashing;
    float dashTimer;
    Vector2 dashDir;
    bool dashTelegraphing;     // фаза предупреждения (стоим, показываем линию пути)
    float dashTelegraphTimer;  // остаток времени предупреждения, сек

    // Идентичность босса (Фаза 5). Для обычных врагов bossId = -1.
    int bossId;
    bool canDash;       // включён ли рывок (боссы используют тот же приём, что MOB_DASH)
    bool canSummon;     // включён ли призыв миньонов
    float summonTimer;  // отсчёт до следующего призыва

    // --- Мобильность (Фаза 4) ---
    int mobility;          // MobilityKind: назначенный приём перемещения
    float mobilityCd;      // кулдаун до следующего применения приёма, сек
    float mobilityCdMax;   // полный кулдаун приёма (из tuning.h), сек

    // Прыжок-наскок (Шаг 17)
    bool jumping;
    float jumpTimer;
    float jumpDuration;
    Vector2 jumpStart;
    Vector2 jumpEnd;

    // Блинк-серия (Шаг 19)
    int blinkBurstLeft;    // сколько блинков осталось в текущей серии
    float blinkStepTimer;  // пауза до следующего блинка

    // Фланг (Шаг 21)
    int flankSide;          // сторона захода: -1 или +1
    float flankSwitchTimer; // время до смены стороны

    float drawYOffset;      // визуальное смещение по Y (дуга прыжка)

    Enemy();
    void Spawn(Vector2 pos, EnemyType t);
    void ApplyBoss(const BossDef& def);   // применить статы и механики босса (Шаг 20-22)
    void Update(float deltaTime, Vector2 playerPos, const TileMap& map,
                TelegraphSystem* telegraphs = nullptr, Effects* effects = nullptr);
    void Draw() const;
    void Kill();
    Rectangle GetRect() const;

private:
    void DrawHealthBar() const;
    // Обрабатывает приёмы мобильности (Фаза 4). Возвращает true, если приём
    // занял этот кадр и обычное движение выполнять не нужно.
    bool UpdateMobility(float dt, Vector2 playerPos, const TileMap& map,
                        TelegraphSystem* telegraphs, Effects* effects);
};
