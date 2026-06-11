#include "tilemap.h"
#include "textures.h"
#include "tuning.h"
#include <cmath>
#include <random>
#include <algorithm>

TileMap::TileMap()
    : tileSize(64), worldSeed(20240609u), spawnCol(2), spawnRow(2), hasArt(false)
{
    // Обнуляем хендлы текстур (загружаются позже через LoadArt).
    texFloor = texWall = texBones = texPuddle = texGrass = Texture2D{ 0 };
    // Размер карты берём из tuning.h (v0.4, Фаза 1, Шаг 2): масштабируемое открытое поле.
    // Размер тайла тоже из конфига, чтобы всё крутилось в одном месте.
    tileSize = Tuning::kMapTileSize;
    Generate(Tuning::MapWidthTiles(), Tuning::MapHeightTiles(), 20240609u);
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

// Процедурная генерация ОТКРЫТОГО поля (v0.4, Фаза 2, Шаг 7).
// В отличие от прежних «комнат с коридорами», поле стартует полностью
// проходимым ('.'), а стены добавляются РЕДКИМИ небольшими скоплениями.
// Так получается простор как в Vampire Survivors: минимум стен, широкие
// проходы, без узких коридоров и тупиков. Вся «открытость» крутится
// параметрами из tuning.h (Шаг 6): kWallDensity, kMinCorridorWidth,
// kBorderWallThickness, kObstacleClusterMin/Max, kOpenZoneRadius.
void TileMap::Generate(int width, int height, unsigned int seed)
{
    worldSeed = seed;  // запоминаем seed для детерминированных вариаций тайлов
    std::mt19937 rng(seed);
    grid.assign(height, std::string(width, '.'));
    decor.assign(height, std::string(width, ' '));
    rooms.clear();

    // Точка старта игрока — центр поля; вокруг неё держим гарантированно пустую зону.
    spawnCol = width / 2;
    spawnRow = height / 2;

    // 1) Внешняя стена-рамка по краю, чтобы игрок и враги не уходили за поле.
    int bt = Tuning::kBorderWallThickness;
    if (bt < 1) bt = 1;
    if (bt > width / 2)  bt = width / 2;
    if (bt > height / 2) bt = height / 2;
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            if (x < bt || y < bt || x >= width - bt || y >= height - bt)
                grid[y][x] = 'W';

    // Есть ли ВНУТРЕННЯЯ стена рядом с (px,py) в радиусе r тайлов.
    // Рамку при этом не учитываем — препятствия можно ставить близко к краю.
    auto interiorWallNear = [&](int px, int py, int r) -> bool {
        for (int y = py - r; y <= py + r; y++)
            for (int x = px - r; x <= px + r; x++)
            {
                if (x < bt || y < bt || x >= width - bt || y >= height - bt) continue;
                if (grid[y][x] == 'W') return true;
            }
        return false;
    };

    // 2) Редкие скопления препятствий. Доля стен стремится к kWallDensity, но
    //    кляксами (kObstacleClusterMin..Max), разнесёнными минимум на
    //    kMinCorridorWidth тайлов — отсюда широкие проходы без коридоров.
    double targetWalls = (double)(width * height) * (double)Tuning::kWallDensity;
    int wallBudget = (int)targetWalls;
    if (wallBudget < 0) wallBudget = 0;
    int placed = 0;
    long long openR2 = (long long)Tuning::kOpenZoneRadius * Tuning::kOpenZoneRadius;
    int gap = Tuning::kMinCorridorWidth; if (gap < 1) gap = 1;
    int cmin = Tuning::kObstacleClusterMin; if (cmin < 1) cmin = 1;
    int cmax = Tuning::kObstacleClusterMax; if (cmax < cmin) cmax = cmin;
    if (width - bt - 1 >= bt && height - bt - 1 >= bt)
    {
        std::uniform_int_distribution<int> xd(bt, width - bt - 1);
        std::uniform_int_distribution<int> yd(bt, height - bt - 1);
        std::uniform_int_distribution<int> clusterDist(cmin, cmax);
        int guard = 0, guardMax = wallBudget * 80 + 2000;
        while (placed < wallBudget && guard < guardMax)
        {
            guard++;
            int ox = xd(rng), oy = yd(rng);
            // Скопление не должно лезть в стартовую зону игрока.
            long long ddx = ox - spawnCol, ddy = oy - spawnRow;
            if (ddx * ddx + ddy * ddy <= openR2) continue;
            // Держим дистанцию до других препятствий — это и есть ширина проходов.
            if (interiorWallNear(ox, oy, gap)) continue;
            int cw = clusterDist(rng), ch = clusterDist(rng);
            for (int y = oy; y < oy + ch && y < height - bt; y++)
                for (int x = ox; x < ox + cw && x < width - bt; x++)
                {
                    long long dx = x - spawnCol, dy = y - spawnRow;
                    if (dx * dx + dy * dy <= openR2) continue;
                    if (grid[y][x] == '.') { grid[y][x] = 'W'; placed++; }
                }
        }
    }

    // 3) Точки интереса (ориентиры для спавна/лута; наполнит Фаза 3).
    //    Раскиданы по открытому полю, подальше от рамки.
    int poiCount = 6 + (width * height) / 5000;
    if (bt + 2 <= width - bt - 3 && bt + 2 <= height - bt - 3)
    {
        std::uniform_int_distribution<int> pxd(bt + 2, width - bt - 3);
        std::uniform_int_distribution<int> pyd(bt + 2, height - bt - 3);
        for (int i = 0; i < poiCount; i++)
            rooms.push_back({ (float)pxd(rng), (float)pyd(rng) });
    }
    // Центр (точка старта) тоже считаем ориентиром.
    rooms.push_back({ (float)spawnCol, (float)spawnRow });

    // Декор на полу: частота из конфига (kDecorDensity). Декор НЕ блокирует движение.
    int decorPct = (int)(Tuning::kDecorDensity * 100.0f);
    if (decorPct < 0) decorPct = 0;
    std::uniform_int_distribution<int> decorRoll(0, 99);
    std::uniform_int_distribution<int> decorType(0, 2);
    for (int y = 1; y < height - 1; y++)
        for (int x = 1; x < width - 1; x++)
            if (grid[y][x] == '.' && decorRoll(rng) < decorPct)
            {
                int t = decorType(rng);
                decor[y][x] = (t == 0) ? 'b' : (t == 1) ? 'p' : 'g';
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

    // Запас тайлов по краям видимой области берём из tuning.h (Шаг 5),
    // чтобы при движении камеры по границе кадра не мелькала пустота.
    const int margin = Tuning::kTileCullMargin;
    int colStart = (int)(minX / tileSize) - margin;
    int colEnd   = (int)(maxX / tileSize) + margin;
    int rowStart = (int)(minY / tileSize) - margin;
    int rowEnd   = (int)(maxY / tileSize) + margin;
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
