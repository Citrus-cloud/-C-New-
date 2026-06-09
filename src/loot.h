#pragma once
#include "raylib.h"
#include <vector>

// Предварительные объявления (избегаем циклических include)
class Player;
class Weapon;

enum LootType { LOOT_HEALTH, LOOT_POWER };

struct Loot
{
    Vector2 position;
    LootType type;
    bool active;
};

// Выпадающий лут (хилки и усиления оружия)
class LootDrops
{
public:
    std::vector<Loot> drops;

    LootDrops(int maxDrops);
    Loot* GetInactive();
    void Spawn(Vector2 pos, LootType type);
    void Update(float dt, Player& player, Weapon& weapon);
    void Draw() const;
};
