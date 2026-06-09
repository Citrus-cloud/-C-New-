#include "traps.h"
#include <random>

Traps::Traps() : cycleTime(3.0f), activeTime(1.0f), warnTime(0.6f), damage(10)
{
}

void Traps::Generate(const TileMap& map, int count, unsigned int seed)
{
    std::mt19937 rng(seed);
    traps.clear();
    int cols = map.Cols(), rows = map.Rows();
    if (cols < 3 || rows < 3) return;

    std::uniform_int_distribution<int> cd(1, cols - 2), rd(1, rows - 2);
    std::uniform_real_distribution<float> ph(0.0f, cycleTime);

    int attempts = 0;
    while ((int)traps.size() < count && attempts < count * 50)
    {
        attempts++;
        int c = cd(rng), r = rd(rng);
        if (map.IsWall(c, r)) continue;  // только на полу
        Vector2 pos = {
            c * (float)map.tileSize + map.tileSize * 0.5f,
            r * (float)map.tileSize + map.tileSize * 0.5f
        };
        // случайная фаза - ловушки срабатывают не одновременно
        traps.push_back({ pos, ph(rng), false, false });
    }
}

void Traps::Update(float deltaTime, Player& player)
{
    for (auto& t : traps)
    {
        t.timer += deltaTime;
        if (t.timer >= cycleTime) t.timer -= cycleTime;

        bool nowActive = (t.timer < activeTime);
        if (nowActive && !t.active) { t.active = true; t.damagedThisCycle = false; }
        if (!nowActive) t.active = false;

        if (t.active && !t.damagedThisCycle)
        {
            Rectangle trapRect = { t.position.x - 24.0f, t.position.y - 24.0f, 48.0f, 48.0f };
            if (CheckCollisionRecs(trapRect, player.GetRect()))
            {
                player.health -= damage;
                if (player.health < 0) player.health = 0;
                t.damagedThisCycle = true;
            }
        }
    }
}

void Traps::Draw() const
{
    for (auto& t : traps)
    {
        Rectangle base = { t.position.x - 24.0f, t.position.y - 24.0f, 48.0f, 48.0f };
        bool warning = (t.timer >= (cycleTime - warnTime));  // скоро активируются
        if (t.active)
        {
            DrawRectangleRec(base, RED);
            DrawText("^^^", (int)(t.position.x - 18), (int)(t.position.y - 8), 20, MAROON);
        }
        else if (warning)
        {
            DrawRectangleLinesEx(base, 3.0f, ORANGE);
        }
        else
        {
            DrawRectangleLinesEx(base, 2.0f, Color{ 120, 120, 120, 180 });
        }
    }
}
