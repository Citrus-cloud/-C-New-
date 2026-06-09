#include "upgrades.h"

void ApplyUpgrade(int choice, Player& player, Weapon& weapon)
{
    switch (choice)
    {
        case 1: weapon.damage += 5; weapon.level++;            break;  // +урон
        case 2: weapon.fireInterval *= 0.85f; weapon.level++;  break;  // +скорострельность
        case 3: player.speed += 30.0f;                         break;  // +скорость
        default: break;
    }

    // Эволюция оружия при достижении 5 уровня
    if (weapon.level >= 5 && !weapon.evolved)
        weapon.Evolve();
}

const char* GetUpgradeText(int choice)
{
    switch (choice)
    {
        case 1: return "1 - +5 uron (damage)";
        case 2: return "2 - +skorostrelnost (fire rate)";
        case 3: return "3 - +skorost (move speed)";
        default: return "";
    }
}
