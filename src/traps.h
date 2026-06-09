#pragma once
#include "raylib.h"
#include "tilemap.h"
#include "player.h"
#include <vector>

// Ловушка-шипы: циклически выдвигаются и наносят урон.
struct Trap
{
    Vector2 position;
    float timer;
    bool active;
    bool damagedThisCycle;
};

class Traps
{
public:
    std::vector<Trap> traps;
    float cycleTime;   // полный цикл
    float activeTime;  // сколько шипы опасны
    float warnTime;    // предупреждение перед активацией
    int damage;

    Traps();
    void Generate(const TileMap& map, int count, unsigned int seed);
    void Update(float deltaTime, Player& player);
    void Draw() const;
};
