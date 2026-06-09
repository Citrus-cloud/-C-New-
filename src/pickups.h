#pragma once
#include "raylib.h"
#include "player.h"
#include <vector>

class ExpOrb
{
public:
    Vector2 position;
    bool active;
    ExpOrb();
};

class ExpOrbs
{
public:
    std::vector<ExpOrb> pool;
    int collectedThisFrame;  // сколько сфер собрано в этом кадре (для звука)

    ExpOrbs(int maxOrbs);

    void Spawn(Vector2 pos);
    void Update(float dt, Player& player);
    void Draw() const;
    ExpOrb* GetInactive();
};
