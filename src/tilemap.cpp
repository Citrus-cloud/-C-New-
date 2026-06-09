#include "tilemap.h"
#include "textures.h"
#include <cmath>
#include <random>
#include <algorithm>

TileMap::TileMap()
    : tileSize(64), worldSeed(20240609u), spawnCol(2), spawnRow(2), hasArt(false)
{
    // Обнуляем хендлы текстур (загружаются позже через LoadArt).
    texFloor = texWall = texBones = texPuddle = texGrass = Texture2D{ 0 };
    // Большая карта (Шаг 13). Фиксированный seed для первого экрана.
    Generate(96, 72, 20240609u);
}

// Загружает текстуры тайлсета (Шаг 10). Если файла нет — хендл остаётся нулевым
// и тайл рисуется цветным прямоугольником (поведение v0.1).
void TileMap::LoadArt(TextureManager& tex)
{
    texFloor  = tex.IsReal("assets/tiles/floor.png")        ? tex.Get("assets/tiles/floor.png")        : Texture2D{ 0 };
    texWall   = tex.IsReal("assets/tiles/wall.png")         ? tex.Get("assets/tiles/wall.png")         : Texture2D{ 0 };
    texBones  = tex.IsReal("assets/tiles/decor_bones.png")  ? tex.Get("assets/tiles/decor_bones.png")  : Texture2D{ 0 };
    texPuddle = tex.IsReal("assets/tiles/decor_puddle.png") ? tex.Get("assets/tiles/decor_puddle.png") : Texture2D{ 0 };
    texGrass  = tex.IsReal("assets/tiles/decor_grass.png")  ? tex.Get("assets/tiles/decor_grass.png")  : Texture2D{ 0 };
    if (texFloor.id != 0) SetTextureFilter(texFloor, TEXTURE_FILTER_POINT);
    if (texWall.id != 0)  SetTextureFilter(texWall,  TEXTURE_FILTER_POINT);
    hasArt = (texFloor.id != 0 || texWall.id != 0);
}

// Процедурная генерация: случайные комнаты, соединённые коридорами.
void TileMap::Generate(int width, int height, unsigned int seed)
{
    worldSeed = seed;  // запоминаем seed для детерминированных вариаций тайлов
    std::mt19937 rng(seed);
    grid.assign(height, std::string(width, 'W'));
    decor.assign(height, std::string(width, ' '));
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

    // Больше комнат для большей карты (Шаг 13).
    std::uniform_int_distribution<int> roomCount(22, 32);
    int n = roomCount(rng);
    int prevcx = 0, prevcy = 0;
    bool first = true;

    for (int i = 0; i < n; i++)
    {
        std::uniform_int_distribution<int> wd(4, 10), hd(4, 9);
        int rw = wd(rng), rh = hd(rng);
        if (width - rw - 2 < 1 || height - rh - 2 < 1) continue;
        std::uniform_int_distribution<int> xd(1, width - rw - 2), yd(1, height - rh - 2);
        int rx = xd(rng), ry = yd(rng);
        carveRoom(rx, ry, rw, rh);

        int cx = rx + rw / 2, cy = ry + rh / 2;
        if (!first)
        {
            carveH(prevcx, cx, prevcy);  // горизонтальный коридор
            carveV(prevcy, cy, cx);      // и вертикальный - получается L
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

    // Декорации на полу (Шаг 12): кости / лужи / трава.
    std::uniform_int_distribution<int> decorRoll(0, 99);
    for (int y = 1; y < height - 1; y++)
        for (int x = 1; x < width - 1; x++)
            if (grid[y][x] == '.')
            {
                int r = decorRoll(rng);
                if (r < 4)       decor[y][x] = 'b';
                else if (r < 8)  decor[y][x] = 'p';
                else if (r < 14) decor[y][x] = 'g';
            }
}

bool TileMap::IsWall(int col, int row) const
{
    if (row < 0 || row >= (int)grid.size()) return true;
    if (col < 0 || col >= (int)grid[row].size()) return true;
    return grid[row][col] == 'W';
}

// Детерминированный хеш координаты тайла.
// Зависит только от seed и (col,row) — это позволяет в будущем генерировать
// мир чанками без хранения всей карты в памяти.
unsigned int TileMap::TileHash(int col, int row) const
{
    unsigned int h = worldSeed;
    h ^= (unsigned int)(col * 374761393);
    h ^= (unsigned int)(row * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Рисует тайл цветным прямоугольником (fallback без текстур), с лёгкой вариацией оттенка.
static void DrawFallbackTile(int x, int y, int tileSize, bool wall, unsigned int h)
{
    if (wall)
    {
        unsigned char d = (unsigned char)(h % 16);
        DrawRectangle(x, y, tileSize, tileSize,
                      Color{ (unsigned char)(96 + d), (unsigned char)(66 + d / 2), (unsigned char)(40 + d / 3), 255 });
    }
    else
    {
        unsigned char d = (unsigned char)(h % 12);
        DrawRectangle(x, y, tileSize, tileSize,
                      Color{ (unsigned char)(28 + d), (unsigned char)(28 + d), (unsigned char)(38 + d), 255 });
    }
    DrawRectangleLines(x, y, tileSize, tileSize, Color{ 0, 0, 0, 40 });
}

void TileMap::Draw(const Camera2D& camera) const
{
    // CULLING (Шаг 11): вычисляем видимую область в мировых координатах
    // и рисуем только тайлы, попавшие в кадр — это даёт большую карту без просадки FPS.
    float halfW = (GetScreenWidth()  * 0.5f) / camera.zoom;
    float halfH = (GetScreenHeight() * 0.5f) / camera.zoom;
    float minX = camera.target.x - halfW;
    float maxX = camera.target.x + halfW;
    float minY = camera.target.y - halfH;
    float maxY = camera.target.y + halfH;

    int colStart = (int)(minX / tileSize) - 1;
    int colEnd   = (int)(maxX / tileSize) + 1;
    int rowStart = (int)(minY / tileSize) - 1;
    int rowEnd   = (int)(maxY / tileSize) + 1;
    if (colStart < 0) colStart = 0;
    if (rowStart < 0) rowStart = 0;
    if (colEnd > Cols() - 1) colEnd = Cols() - 1;
    if (rowEnd > Rows() - 1) rowEnd = Rows() - 1;

    for (int row = rowStart; row <= rowEnd; row++)
    {
        for (int col = colStart; col <= colEnd; col++)
        {
            int x = col * tileSize;
            int y = row * tileSize;
            bool wall = (grid[row][col] == 'W');
            unsigned int h = TileHash(col, row);

            Texture2D tex = wall ? texWall : texFloor;
            if (hasArt && tex.id != 0)
            {
                // Вариативность (Шаг 12): отражаем тайл по флагам хеша,
                // чтобы одна текстура не выглядела однообразно.
                float fw = (float)tex.width;
                float fh = (float)tex.height;
                Rectangle src = { 0.0f, 0.0f, (h & 1u) ? -fw : fw, (h & 2u) ? -fh : fh };
                Rectangle dst = { (float)x, (float)y, (float)tileSize, (float)tileSize };
                DrawTexturePro(tex, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
            }
            else
            {
                DrawFallbackTile(x, y, tileSize, wall, h);
            }

            // Декорации рисуем поверх пола (только если есть текстуры).
            if (!wall && hasArt)
            {
                char dch = decor[row][col];
                Texture2D dt = Texture2D{ 0 };
                if (dch == 'b')      dt = texBones;
                else if (dch == 'p') dt = texPuddle;
                else if (dch == 'g') dt = texGrass;
                if (dt.id != 0)
                {
                    Rectangle src = { 0.0f, 0.0f, (float)dt.width, (float)dt.height };
                    Rectangle dst = { (float)x, (float)y, (float)tileSize, (float)tileSize };
                    DrawTexturePro(dt, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
                }
            }
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
