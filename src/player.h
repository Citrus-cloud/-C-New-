#pragma once
#include "raylib.h"

// Класс игрока: хранит состояние и умеет обновляться и рисоваться
class Player
{
public:
    Vector2 position;  // позиция на карте (x, y)
    float speed;       // скорость в пикселях в секунду
    int health;        // здоровье

    // Конструктор: вызывается при создании игрока
    Player(Vector2 startPos);

    void Update(float deltaTime);  // обработка ввода и движение
    void Draw() const;             // отрисовка
};
