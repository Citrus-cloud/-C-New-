#pragma once
#include "raylib.h"
#include "player.h"
#include <vector>

// Сфера опыта, выпадающая из врагов
class ExpOrb
{
public:
    Vector2 position;
    bool active;
    ExpOrb();
};

// Пул сфер опыта
class ExpOrbs
{
public:
    std::vector<ExpOrb> pool;

    ExpOrbs(int maxOrbs);

    void Spawn(Vector2 pos);
    void Update(float dt, Player& player);  // притяжение и сбор опыта
    void Draw() const;
    ExpOrb* GetInactive();
};
