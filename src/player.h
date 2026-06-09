#pragma once
#include "raylib.h"
#include "tilemap.h"

// Класс игрока
class Player
{
public:
    Vector2 position;
    float speed;
    int health;
    int xp;          // текущий опыт
    int level;       // уровень
    int xpToNext;    // сколько опыта до следующего уровня

    Vector2 lastDir;     // последнее направление (для рывка)
    float dashTimer;     // сколько ещё длится рывок
    float dashCooldown;  // перезарядка рывка

    Player(Vector2 startPos);

    void Update(float deltaTime, const TileMap& map);
    void Draw() const;
    Rectangle GetRect() const;
    bool TryLevelUp();   // если набрали опыт - повышаем уровень
};
