#pragma once
#include "raylib.h"
#include <vector>

// Предварительные объявления (избегаем циклических include)
class Player;
class Weapon;

// Типы лута. LOOT_CHEST (v0.4 Шаг 14) — сундук-награда на поле: даёт сразу
// и лечение (kChestHealAmount), и усиление урона (kChestPowerBonus) — стимул дойти.
enum LootType { LOOT_HEALTH, LOOT_POWER, LOOT_CHEST };

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
