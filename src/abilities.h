#pragma once
#include "raylib.h"

class Spawner;
class ExpOrbs;
class LootDrops;
class Effects;

// =====================================================================
// Abilities (Шаг 23) — пассивные авто-способности в духе bullet-heaven.
// Разблокируются прокачкой и бьют сами. Спроектировано на расширение:
// новые способности добавляются по тому же образцу.
// =====================================================================
class Abilities
{
public:
    // Орбитальные клинки: вращаются вокруг игрока и режут врагов.
    bool orbitUnlocked;
    int orbitCount;
    float orbitRadius;
    float orbitAngle;
    int orbitDamage;
    float orbitTickTimer;   // как часто клинки наносят урон

    // Нова: периодическая расширяющаяся ударная волна.
    bool novaUnlocked;
    float novaTimer;        // до следующей волны
    float novaInterval;
    float novaRadius;       // текущий радиус активной волны (0 = неактивна)
    float novaMaxRadius;
    int novaDamage;
    Vector2 novaCenter;     // центр текущей волны

    Abilities();
    void Reset();

    void UnlockOrbit();
    void AddOrbit();
    void UnlockNova();

    void Update(float dt, Vector2 playerPos, Spawner& spawner, ExpOrbs& orbs, LootDrops& loot, Effects& effects);
    void Draw(Vector2 playerPos) const;

private:
    void OrbitPositions(Vector2 playerPos, Vector2* out, int maxOut) const;
};
