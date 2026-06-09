#include "upgrades.h"
#include "abilities.h"

// Прогресс по способностям (Шаг 23): клинки -> нова -> +клинок -> +скорость -> ...
static int abilityStage = 0;

void ResetUpgrades()
{
    abilityStage = 0;
}

void ApplyUpgrade(int choice, Player& player, Weapon& weapon, Abilities& abilities)
{
    switch (choice)
    {
        case 1: weapon.damage += 5; weapon.level++;           break;
        case 2: weapon.fireInterval *= 0.85f; weapon.level++; break;
        case 3:
            switch (abilityStage % 4)
            {
                case 0: abilities.UnlockOrbit(); break;
                case 1: abilities.UnlockNova();  break;
                case 2: abilities.AddOrbit();    break;
                case 3: player.speed += 30.0f;   break;
            }
            abilityStage++;
            break;
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
        case 3: return "3 — способность / усиление";
        default: return "";
    }
}
