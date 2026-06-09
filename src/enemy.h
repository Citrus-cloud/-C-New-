#pragma once
#include "raylib.h"

// Враг. Поле active нужно для пула объектов:
// вместо создания/удаления мы просто включаем/выключаем врагов.
class Enemy
{
public:
    Vector2 position;
    float speed;
    int health;
    bool active;   // используется ли этот враг сейчас

    Enemy();

    void Spawn(Vector2 pos);                        // включить врага в точке
    void Update(float deltaTime, Vector2 playerPos); // ИИ: идём к игроку
    void Draw() const;
    Rectangle GetRect() const;
};
