#pragma once
#include "raylib.h"
#include <vector>
#include <string>

// Тайловая карта: 'W' - стена, '.' - пол.
// Размер карты любой - в будущем расширим до большого открытого мира.
class TileMap
{
public:
    int tileSize;
    std::vector<std::string> grid;

    TileMap();

    void Draw() const;
    bool IsWall(int col, int row) const;
    bool CheckCollision(Rectangle rect) const;
    bool IsFree(Rectangle rect) const;                          // свободно ли место
    Vector2 FindFreeSpot(Vector2 desired, float halfSize) const; // ближайшее свободное место
};
