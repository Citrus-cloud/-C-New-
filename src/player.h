#pragma once
#include "raylib.h"
#include "tilemap.h"

// Класс игрока (без рывка — простое движение)
class Player
{
public:
    Vector2 position;
    float speed;
    int health;
    int maxHealth;
    float hitCooldown;   // i-frames: неуязвимость после удара
    int xp;
    int level;
    int xpToNext;

    Player(Vector2 startPos);

    void Update(float deltaTime, const TileMap& map);
    void Draw() const;
    Rectangle GetRect() const;
    bool TryLevelUp();
    void ResolveStuck(const TileMap& map);
    void TakeDamage(int dmg);
    void Heal(int amount);
};
