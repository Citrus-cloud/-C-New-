#pragma once
#include "raylib.h"
#include <vector>
#include <string>

// Простая тайловая карта: сетка из символов, где 'W' - стена, '.' - пол
class TileMap
{
public:
    int tileSize;                   // размер одного тайла в пикселях
    std::vector<std::string> grid;  // карта по строкам

    TileMap();

    void Draw() const;
    bool IsWall(int col, int row) const;          // стена ли тайл (col, row)
    bool CheckCollision(Rectangle rect) const;    // пересекает ли прямоугольник стену
};
