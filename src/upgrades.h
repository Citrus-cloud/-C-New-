#pragma once
#include "player.h"
#include "combat.h"

// Применяет выбранный апгрейд (1, 2 или 3)
void ApplyUpgrade(int choice, Player& player, Weapon& weapon);

// Текст для экрана выбора
const char* GetUpgradeText(int choice);
