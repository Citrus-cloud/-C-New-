#include "abilities.h"
#include "combat.h"     // DamageEnemy, Spawner, ExpOrbs, LootDrops
#include "effects.h"
#include <cmath>

Abilities::Abilities() { Reset(); }

void Abilities::Reset()
{
    orbitUnlocked = false;
    orbitCount = 2;
    orbitRadius = 75.0f;
    orbitAngle = 0.0f;
    orbitDamage = 10;
    orbitTickTimer = 0.0f;

    novaUnlocked = false;
    novaTimer = 6.0f;
    novaInterval = 6.0f;
    novaRadius = 0.0f;
    novaMaxRadius = 340.0f;
    novaDamage = 25;
    novaCenter = { 0.0f, 0.0f };
}

void Abilities::UnlockOrbit()
{
    orbitUnlocked = true;
    if (orbitCount < 2) orbitCount = 2;
}

void Abilities::AddOrbit()
{
    if (!orbitUnlocked) { UnlockOrbit(); return; }
    orbitCount++;
    orbitDamage += 5;
}

void Abilities::UnlockNova()
{
    novaUnlocked = true;
    novaTimer = 2.0f;
}

void Abilities::OrbitPositions(Vector2 playerPos, Vector2* out, int maxOut) const
{
    int n = orbitCount;
    if (n > maxOut) n = maxOut;
    if (n < 1) n = 1;
    for (int i = 0; i < n; i++)
    {
        float a = orbitAngle + (float)i * (2.0f * PI / n);
        out[i] = { playerPos.x + cosf(a) * orbitRadius, playerPos.y + sinf(a) * orbitRadius };
    }
}

void Abilities::Update(float dt, Vector2 playerPos, Spawner& spawner, ExpOrbs& orbs, LootDrops& loot, Effects& effects)
{
    // --- Орбитальные клинки ---
    if (orbitUnlocked)
    {
        orbitAngle += 3.0f * dt;   // скорость вращения, рад/с
        if (orbitTickTimer > 0.0f) orbitTickTimer -= dt;

        bool canHit = (orbitTickTimer <= 0.0f);
        if (canHit) orbitTickTimer = 0.2f;

        Vector2 blades[8];
        int n = orbitCount; if (n > 8) n = 8;
        OrbitPositions(playerPos, blades, 8);

        if (canHit)
        {
            for (int i = 0; i < n; i++)
            {
                for (Enemy& e : spawner.enemies)
                {
                    if (!e.active || e.dying) continue;
                    float dx = e.position.x - blades[i].x;
                    float dy = e.position.y - blades[i].y;
                    float rr = e.size + 14.0f;
                    if (dx * dx + dy * dy <= rr * rr)
                        DamageEnemy(e, orbitDamage, orbs, loot, effects);
                }
            }
        }
    }

    // --- Нова (ударная волна) ---
    if (novaUnlocked)
    {
        if (novaRadius > 0.0f)
        {
            float prev = novaRadius;
            novaRadius += 700.0f * dt;
            for (Enemy& e : spawner.enemies)
            {
                if (!e.active || e.dying) continue;
                float dx = e.position.x - novaCenter.x;
                float dy = e.position.y - novaCenter.y;
                float d = sqrtf(dx * dx + dy * dy);
                if (d >= prev && d < novaRadius)
                    DamageEnemy(e, novaDamage, orbs, loot, effects);
            }
            if (novaRadius >= novaMaxRadius) novaRadius = 0.0f;
        }
        else
        {
            novaTimer -= dt;
            if (novaTimer <= 0.0f)
            {
                novaTimer = novaInterval;
                novaCenter = playerPos;
                novaRadius = 1.0f;
                effects.SpawnMagicCircle(playerPos, 2.0f);
            }
        }
    }
}

void Abilities::Draw(Vector2 playerPos) const
{
    if (orbitUnlocked)
    {
        Vector2 blades[8];
        int n = orbitCount; if (n > 8) n = 8;
        OrbitPositions(playerPos, blades, 8);
        for (int i = 0; i < n; i++)
        {
            DrawCircleV(blades[i], 10.0f, SKYBLUE);
            DrawCircleLines((int)blades[i].x, (int)blades[i].y, 10.0f, WHITE);
        }
    }
    if (novaUnlocked && novaRadius > 0.0f)
    {
        DrawCircleLines((int)novaCenter.x, (int)novaCenter.y, novaRadius, Color{ 120, 200, 255, 220 });
        DrawCircleLines((int)novaCenter.x, (int)novaCenter.y, novaRadius - 4.0f, Color{ 120, 200, 255, 120 });
    }
}
