#include "tilemap.h"
#include <cmath>
#include <random>
#include <algorithm>

TileMap::TileMap() : tileSize(64), spawnCol(2), spawnRow(2)
{
    // Генерируем карту 64x44 тайла (фиксированный seed для повторяемости)
    Generate(64, 44, 20240609u);
}

// Процедурная генерация: случайные комнаты, соединённые коридорами.
void TileMap::Generate(int width, int height, unsigned int seed)
{
    std::mt19937 rng(seed);
    grid.assign(height, std::string(width, 'W'));
    rooms.clear();

    auto carveRoom = [&](int rx, int ry, int rw, int rh) {
        for (int y = ry; y < ry + rh; y++)
            for (int x = rx; x < rx + rw; x++)
                if (x > 0 && y > 0 && x < width - 1 && y < height - 1)
                    grid[y][x] = '.';
    };
    auto carveH = [&](int x1, int x2, int y) {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++)
            if (x > 0 && y > 0 && x < width - 1 && y < height - 1)
                grid[y][x] = '.';
    };
    auto carveV = [&](int y1, int y2, int x) {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); y++)
            if (x > 0 && y > 0 && x < width - 1 && y < height - 1)
                grid[y][x] = '.';
    };

    std::uniform_int_distribution<int> roomCount(14, 20);
    int n = roomCount(rng);
    int prevcx = 0, prevcy = 0;
    bool first = true;

    for (int i = 0; i < n; i++)
    {
        std::uniform_int_distribution<int> wd(4, 9), hd(4, 8);
        int rw = wd(rng), rh = hd(rng);
        if (width - rw - 2 < 1 || height - rh - 2 < 1) continue;
        std::uniform_int_distribution<int> xd(1, width - rw - 2), yd(1, height - rh - 2);
        int rx = xd(rng), ry = yd(rng);
        carveRoom(rx, ry, rw, rh);

        int cx = rx + rw / 2, cy = ry + rh / 2;
        if (!first)
        {
            carveH(prevcx, cx, prevcy);  // горизонтальный коридор
            carveV(prevcy, cy, cx);      // и вертикальный - получается «L»
        }
        else
        {
            spawnCol = cx;
            spawnRow = cy;
            first = false;
        }
        rooms.push_back({ (float)cx, (float)cy });
        prevcx = cx;
        prevcy = cy;
    }
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

Vector2 TileMap::FindFreeSpot(Vector2 desired, float halfSize) const
{
    Rectangle r0 = { desired.x - halfSize, desired.y - halfSize, halfSize * 2.0f, halfSize * 2.0f };
    if (IsFree(r0)) return desired;

    int step = (int)halfSize;
    if (step < 1) step = 1;
    for (int radius = step; radius <= 8000; radius += step)
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
    return desired;
}

Vector2 TileMap::GetSpawnPoint() const
{
    return {
        spawnCol * (float)tileSize + tileSize * 0.5f,
        spawnRow * (float)tileSize + tileSize * 0.5f
    };
}
