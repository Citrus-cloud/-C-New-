#include "loot.h"
#include "player.h"
#include "combat.h"
#include "tuning.h"   // параметры сундука (kChestHealAmount / kChestPowerBonus), v0.4 Шаг 14

LootDrops::LootDrops(int maxDrops)
{
    drops.resize(maxDrops);
}

Loot* LootDrops::GetInactive()
{
    for (auto& d : drops)
        if (!d.active) return &d;
    return nullptr;
}

void LootDrops::Spawn(Vector2 pos, LootType type)
{
    Loot* d = GetInactive();
    if (!d) return;
    d->position = pos;
    d->type = type;
    d->active = true;
}

void LootDrops::Update(float dt, Player& player, Weapon& weapon)
{
    (void)dt;
    const float pickup = 30.0f;
    for (auto& d : drops)
    {
        if (!d.active) continue;
        float dx = player.position.x - d.position.x;
        float dy = player.position.y - d.position.y;
        if (dx * dx + dy * dy < pickup * pickup)
        {
            d.active = false;
            if (d.type == LOOT_HEALTH) player.Heal(25);
            else if (d.type == LOOT_POWER) weapon.damage += 5;
            else if (d.type == LOOT_CHEST)
            {
                // Сундук (Шаг 14): сразу и лечит, и усиливает оружие — награда за путь по полю.
                player.Heal(Tuning::kChestHealAmount);
                weapon.damage += Tuning::kChestPowerBonus;
            }
        }
    }
}

void LootDrops::Draw() const
{
    for (const auto& d : drops)
    {
        if (!d.active) continue;
        if (d.type == LOOT_HEALTH)
        {
            DrawRectangle((int)(d.position.x - 8), (int)(d.position.y - 8), 16, 16, GREEN);
            DrawText("+", (int)(d.position.x - 4), (int)(d.position.y - 8), 18, DARKGREEN);
        }
        else if (d.type == LOOT_POWER)
        {
            DrawRectangle((int)(d.position.x - 8), (int)(d.position.y - 8), 16, 16, SKYBLUE);
            DrawText("P", (int)(d.position.x - 4), (int)(d.position.y - 8), 16, DARKBLUE);
        }
        else // LOOT_CHEST — коричневый сундук с золотой полосой и замком
        {
            DrawRectangle((int)(d.position.x - 11), (int)(d.position.y - 9), 22, 18, Color{ 120, 72, 32, 255 });
            DrawRectangleLines((int)(d.position.x - 11), (int)(d.position.y - 9), 22, 18, Color{ 70, 42, 18, 255 });
            DrawRectangle((int)(d.position.x - 11), (int)(d.position.y - 3), 22, 5, Color{ 200, 160, 60, 255 });
            DrawRectangle((int)(d.position.x - 2), (int)(d.position.y - 5), 4, 8, Color{ 240, 200, 80, 255 });
        }
    }
}
