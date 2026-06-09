#include "tilemap.h"
#include <cmath>

TileMap::TileMap() : tileSize(64)
{
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

bool TileMap::IsFree(Rectangle rect) const
{
    return !CheckCollision(rect);
}

// Ищем ближайшую свободную точку по расширяющимся кольцам.
// Гарантирует, что объект никогда не останется внутри стены.
Vector2 TileMap::FindFreeSpot(Vector2 desired, float halfSize) const
{
    Rectangle r0 = { desired.x - halfSize, desired.y - halfSize, halfSize * 2.0f, halfSize * 2.0f };
    if (IsFree(r0)) return desired;

    int step = (int)halfSize;
    if (step < 1) step = 1;
    for (int radius = step; radius <= 4000; radius += step)
    {
        for (int angle = 0; angle < 360; angle += 15)
        {
            float rad = angle * DEG2RAD;
            Vector2 test = {
                desired.x + cosf(rad) * radius,
                desired.y + sinf(rad) * radius
            };
            Rectangle tr = { test.x - halfSize, test.y - halfSize, halfSize * 2.0f, halfSize * 2.0f };
            if (IsFree(tr)) return test;
        }
    }
    return desired;  // на крайний случай
}
