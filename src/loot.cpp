#include "loot.h"
#include "player.h"
#include "combat.h"

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
        else
        {
            DrawRectangle((int)(d.position.x - 8), (int)(d.position.y - 8), 16, 16, SKYBLUE);
            DrawText("P", (int)(d.position.x - 4), (int)(d.position.y - 8), 16, DARKBLUE);
        }
    }
}
