#pragma once
#include "raylib.h"
#include "tilemap.h"

// Класс игрока (без рывка - простое движение)
class Player
{
public:
    Vector2 position;
    float speed;
    int health;
    int xp;          // текущий опыт
    int level;       // уровень
    int xpToNext;    // сколько опыта до следующего уровня

    Player(Vector2 startPos);

    void Update(float deltaTime, const TileMap& map);
    void Draw() const;
    Rectangle GetRect() const;
    bool TryLevelUp();
    void ResolveStuck(const TileMap& map);  // вытолкнуть из стены, если застряли
};
