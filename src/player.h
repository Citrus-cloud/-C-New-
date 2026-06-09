#pragma once
#include "raylib.h"
#include "tilemap.h"

// Класс игрока: хранит состояние и умеет обновляться и рисоваться
class Player
{
public:
    Vector2 position;  // позиция на карте (x, y) - центр игрока
    float speed;       // скорость в пикселях в секунду
    int health;        // здоровье
    int xp;            // накопленный опыт

    Player(Vector2 startPos);

    void Update(float deltaTime, const TileMap& map);  // ввод, движение и коллизии
    void Draw() const;
    Rectangle GetRect() const;                         // прямоугольник для коллизий
};
