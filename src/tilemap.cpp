#include "tilemap.h"

TileMap::TileMap() : tileSize(64)
{
    // 'W' - стена, '.' - пол. Каждая строка - ряд тайлов.
    grid = {
        "WWWWWWWWWWWWWWWWWWWW",
        "W..................W",
        "W..................W",
        "W....WW....WW......W",
        "W....WW....WW......W",
        "W..................W",
        "W........WW........W",
        "W........WW........W",
        "W..................W",
        "W....WW....WW......W",
        "W....WW....WW......W",
        "W..................W",
        "W..................W",
        "WWWWWWWWWWWWWWWWWWWW",
    };
}

bool TileMap::IsWall(int col, int row) const
{
    // Всё, что за пределами карты, считаем стеной
    if (row < 0 || row >= (int)grid.size()) return true;
    if (col < 0 || col >= (int)grid[row].size()) return true;
    return grid[row][col] == 'W';
}

void TileMap::Draw() const
{
    for (int row = 0; row < (int)grid.size(); row++)
    {
        for (int col = 0; col < (int)grid[row].size(); col++)
        {
            int x = col * tileSize;
            int y = row * tileSize;
            if (grid[row][col] == 'W')
                DrawRectangle(x, y, tileSize, tileSize, DARKBROWN);
            else
                DrawRectangle(x, y, tileSize, tileSize, Color{ 30, 30, 40, 255 });
            DrawRectangleLines(x, y, tileSize, tileSize, Color{ 0, 0, 0, 40 });
        }
    }
}

bool TileMap::CheckCollision(Rectangle rect) const
{
    // Смотрим только тайлы, которые может задевать прямоугольник
    int left   = (int)(rect.x / tileSize);
    int right  = (int)((rect.x + rect.width) / tileSize);
    int top    = (int)(rect.y / tileSize);
    int bottom = (int)((rect.y + rect.height) / tileSize);

    for (int row = top; row <= bottom; row++)
        for (int col = left; col <= right; col++)
            if (IsWall(col, row))
            {
                Rectangle tile = {
                    (float)(col * tileSize), (float)(row * tileSize),
                    (float)tileSize, (float)tileSize
                };
                if (CheckCollisionRecs(rect, tile)) return true;
            }
    return false;
}
