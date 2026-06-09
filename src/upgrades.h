#pragma once
#include "player.h"
#include "combat.h"

class Abilities;

// Применяет выбранный апгрейд (1, 2 или 3).
void ApplyUpgrade(int choice, Player& player, Weapon& weapon, Abilities& abilities);

// Сброс прогрессии способностей (вызывать в начале нового забега).
void ResetUpgrades();

// Текст для экрана выбора.
const char* GetUpgradeText(int choice);
