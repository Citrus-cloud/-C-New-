#pragma once
#include "raylib.h"
#include <vector>
#include <string>

// Тайловая карта: 'W' - стена, '.' - пол.
// Размер любой - карта генерируется процедурно (комнаты + коридоры).
class TileMap
{
public:
    int tileSize;
    std::vector<std::string> grid;
    std::vector<Vector2> rooms;  // центры комнат (в тайлах)
    int spawnCol;
    int spawnRow;

    TileMap();

    void Generate(int width, int height, unsigned int seed);
    void Draw() const;
    bool IsWall(int col, int row) const;
    bool CheckCollision(Rectangle rect) const;
    bool IsFree(Rectangle rect) const;
    Vector2 FindFreeSpot(Vector2 desired, float halfSize) const;
    Vector2 GetSpawnPoint() const;

    int Cols() const { return grid.empty() ? 0 : (int)grid[0].size(); }
    int Rows() const { return (int)grid.size(); }
};
