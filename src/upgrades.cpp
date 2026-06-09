#include "upgrades.h"

void ApplyUpgrade(int choice, Player& player, Weapon& weapon)
{
    switch (choice)
    {
        case 1: weapon.damage += 5; weapon.level++;            break;
        case 2: weapon.fireInterval *= 0.85f; weapon.level++;  break;
        case 3: player.speed += 30.0f;                         break;
        default: break;
    }

    if (weapon.level >= 5 && !weapon.evolved)
        weapon.Evolve();
}

const char* GetUpgradeText(int choice)
{
    switch (choice)
    {
        case 1: return "1 — +5 урона";
        case 2: return "2 — +скорострельность";
        case 3: return "3 — +скорость движения";
        default: return "";
    }
}
