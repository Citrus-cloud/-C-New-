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

// Загружает текстуры тайлсета. Если файла нет — хендл остаётся нулевым
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

// Процедурная генерация ОТКРЫТОГО поля (v0.4, Фаза 2, Шаг 7-9).
// В отличие от прежних «комнат с коридорами», поле стартует полностью
// проходимым ('.'), а стены добавляются РЕДКИМИ небольшими скоплениями.
// Так получается простор как в Vampire Survivors: минимум стен, широкие
// проходы, без узких коридоров и тупиков. Вся «открытость» крутится
// параметрами из tuning.h (Шаг 6): kWallDensity, kMinCorridorWidth,
// kBorderWallThickness, kObstacleClusterMin/Max, kOpenZoneRadius.
// Шаг 9 гарантирует проходимость (flood-fill от старта), а поверх пола
// (Шаг 8) раскидываются разрозненные декоративные пропы (камни/колонны/
// обломки/кусты) с частотой kDecorDensity — чистый визуал без коллизий.
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

    // 2.5) ГАРАНТИЯ ПРОХОДИМОСТИ (Шаг 9): всё свободное поле должно быть одним
    //      связным регионом, достижимым из точки старта. Запускаем волновой обход
    //      (BFS/flood-fill) по полу от спавна; если редкие скопления случайно
    //      отрезали клочок пола, заполняем такой недостижимый «карман» стеной —
    //      так на карте не остаётся зон-ловушек, куда нельзя дойти. После этого
    //      навигация врагов (FindPath/HasLineOfSight в pathfinding.cpp) гарантированно
    //      ведёт от кольца спавна к игроку по любому свободному тайлу.
    {
        std::vector<std::vector<unsigned char>> seen(height, std::vector<unsigned char>(width, 0));
        std::vector<int> stack;
        stack.reserve((size_t)width * (size_t)height);
        auto pushCell = [&](int cx, int cy) {
            seen[cy][cx] = 1;
            stack.push_back(cy * width + cx);
        };
        if (grid[spawnRow][spawnCol] == '.') pushCell(spawnCol, spawnRow);
        const int ndc[4] = { 1, -1, 0, 0 };
        const int ndr[4] = { 0, 0, 1, -1 };
        while (!stack.empty())
        {
            int cur = stack.back();
            stack.pop_back();
            int cx = cur % width, cy = cur / width;
            for (int d = 0; d < 4; d++)
            {
                int nx = cx + ndc[d], ny = cy + ndr[d];
                if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                if (seen[ny][nx] || grid[ny][nx] != '.') continue;
                pushCell(nx, ny);
            }
        }
        // Недостижимые клочки пола делаем стеной. На открытом поле их почти нет,
        // но так мы строго исключаем зоны, куда нельзя дойти.
        int sealed = 0;
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                if (grid[y][x] == '.' && !seen[y][x]) { grid[y][x] = 'W'; sealed++; }
        (void)sealed;
    }

    // 3) Точки интереса (ориентиры для спавна/лута; наполняет Фаза 3, Шаг 12).
    //    Раскиданы по открытому полю, подальше от рамки. Частота берётся из
    //    tuning.h: базовое число плюс по одному на kPoiAreaPerTiles тайлов площади.
    int poiCount = Tuning::kPoiBaseCount
                 + (Tuning::kPoiAreaPerTiles > 0 ? (width * height) / Tuning::kPoiAreaPerTiles : 0);
    if (bt + 2 <= width - bt - 3 && bt + 2 <= height - bt - 3)
    {
        std::uniform_int_distribution<int> pxd(bt + 2, width - bt - 3);
        std::uniform_int_distribution<int> pyd(bt + 2, height - bt - 3);
        for (int i = 0; i < poiCount; i++)
            rooms.push_back({ (float)pxd(rng), (float)pyd(rng) });
    }
    // Центр (точка старта) тоже считаем ориентиром.
    rooms.push_back({ (float)spawnCol, (float)spawnRow });

    // 4) Декоративные пропы-препятствия (Шаг 8): камни, колонны, обломки, кусты.
    //    Раскиданы РЕДКО по открытому полю с частотой kDecorDensity. Это чистый
    //    визуал — коллизий у них НЕТ (CheckCollision реагирует только на 'W'),
    //    поэтому они оживляют простор, но нигде не зажимают движение.
    int decorPct = (int)(Tuning::kDecorDensity * 100.0f);
    if (decorPct < 0) decorPct = 0;
    std::uniform_int_distribution<int> decorRoll(0, 99);
    std::uniform_int_distribution<int> decorType(0, 3);
    for (int y = 1; y < height - 1; y++)
        for (int x = 1; x < width - 1; x++)
        {
            if (grid[y][x] != '.') continue;
            // Саму точку старта оставляем чистой.
            if (x == spawnCol && y == spawnRow) continue;
            if (decorRoll(rng) < decorPct)
            {
                int t = decorType(rng);
                decor[y][x] = (t == 0) ? 'r' : (t == 1) ? 'c' : (t == 2) ? 'd' : 'g';
            }
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

// Рисует тайл цветным прямоугольником (fallback без текстур).
// v0.4 Шаг 11: пол рисуется как бесшовный «луг» — мягкая земляная база с
// лёгкой вариацией оттенка (kGroundTintVariation) и почти невидимой сеткой,
// чтобы большое поле смотрелось сплошным простором, а не «шахматкой». Поверх
// детерминированно разбрасываются мелкие травинки (kGroundDetailChance).
static void DrawFallbackTile(int x, int y, int tileSize, bool wall, unsigned int h)
{
    if (wall)
    {
        unsigned char d = (unsigned char)(h % 16);
        DrawRectangle(x, y, tileSize, tileSize,
                      Color{ (unsigned char)(96 + d), (unsigned char)(66 + d / 2), (unsigned char)(40 + d / 3), 255 });
        DrawRectangleLines(x, y, tileSize, tileSize, Color{ 0, 0, 0, 40 });
        return;
    }

    // Пол: землисто-зелёная база с мягкой вариацией оттенка между тайлами.
    int var = Tuning::kGroundTintVariation;
    int d  = (var > 0) ? (int)(h % (unsigned int)(var + 1)) : 0;
    int d2 = (var > 0) ? (int)((h >> 5) % (unsigned int)(var + 1)) : 0;
    DrawRectangle(x, y, tileSize, tileSize,
                  Color{ (unsigned char)(36 + d), (unsigned char)(52 + d2), (unsigned char)(40 + d / 2), 255 });
    // Почти невидимая сетка — только чтобы поле не выглядело идеально плоским.
    DrawRectangleLines(x, y, tileSize, tileSize, Color{ 0, 0, 0, 16 });

    // Мелкие травинки (детерминированно по хешу тайла) — оживляют простор.
    if ((int)(h % 100) < Tuning::kGroundDetailChance)
    {
        float ts = (float)tileSize;
        float gx = x + ts * (0.2f + 0.6f * (float)((h >> 7) % 100) / 100.0f);
        float gy = y + ts * (0.3f + 0.5f * (float)((h >> 11) % 100) / 100.0f);
        float bh = ts * 0.16f;
        int   bw = (int)(ts * 0.03f); if (bw < 1) bw = 1;
        Color blade{ 70, 132, 66, 90 };
        DrawRectangle((int)gx, (int)(gy - bh), bw, (int)bh, blade);
        DrawRectangle((int)gx + bw * 2, (int)(gy - bh * 0.7f), bw, (int)(bh * 0.7f), blade);
    }
}

// Рисует декоративный проп фолбэк-фигурой (Шаг 8): виден ДАЖЕ без текстур тайлсета.
// Пропы НЕ блокируют движение — это чисто визуальные ориентиры на открытом поле.
//   'r' — камень (валун), 'c' — колонна, 'd' — обломки, 'g' — куст травы.
static void DrawProp(int x, int y, int tileSize, char prop, unsigned int h)
{
    if (prop != 'r' && prop != 'c' && prop != 'd' && prop != 'g') return;
    float ts = (float)tileSize;
    float cx = x + ts * 0.5f;
    float cy = y + ts * 0.5f;
    // Небольшое детерминированное смещение, чтобы пропы не стояли по идеальной сетке.
    cx += ((int)(h % 7) - 3) * (ts * 0.04f);
    cy += ((int)((h >> 3) % 7) - 3) * (ts * 0.04f);

    switch (prop)
    {
        case 'r': // валун — серый камень с тенью
        {
            float r = ts * 0.26f;
            DrawCircle((int)cx, (int)(cy + r * 0.35f), r * 1.05f, Color{ 0, 0, 0, 45 });
            DrawCircle((int)cx, (int)cy, r, Color{ 120, 120, 130, 255 });
            DrawCircle((int)(cx - r * 0.3f), (int)(cy - r * 0.3f), r * 0.4f, Color{ 160, 160, 170, 255 });
            break;
        }
        case 'c': // колонна — светлый столб с основанием и капителью
        {
            float w = ts * 0.26f;
            float hgt = ts * 0.62f;
            float px = cx - w * 0.5f;
            float py = cy - hgt * 0.5f;
            DrawEllipse((int)cx, (int)(py + hgt), w * 0.7f, w * 0.28f, Color{ 0, 0, 0, 45 });
            DrawRectangle((int)px, (int)py, (int)w, (int)hgt, Color{ 200, 196, 178, 255 });
            DrawRectangle((int)(px - w * 0.18f), (int)py, (int)(w * 1.36f), (int)(ts * 0.10f), Color{ 214, 210, 192, 255 });
            DrawRectangle((int)(px - w * 0.18f), (int)(py + hgt - ts * 0.10f), (int)(w * 1.36f), (int)(ts * 0.10f), Color{ 186, 182, 165, 255 });
            break;
        }
        case 'd': // обломки — пара мелких камней
        {
            float s = ts * 0.16f;
            DrawRectangle((int)(cx - s), (int)cy, (int)s, (int)s, Color{ 110, 104, 96, 255 });
            DrawRectangle((int)(cx + s * 0.3f), (int)(cy - s * 0.6f), (int)(s * 1.1f), (int)(s * 1.1f), Color{ 134, 126, 116, 255 });
            DrawRectangle((int)(cx - s * 0.2f), (int)(cy + s * 0.7f), (int)(s * 0.8f), (int)(s * 0.8f), Color{ 96, 90, 84, 255 });
            break;
        }
        case 'g': // куст травы — несколько зелёных штрихов
        {
            float bw = ts * 0.05f;
            float bh = ts * 0.30f;
            DrawRectangle((int)(cx - bw * 2.5f), (int)(cy - bh * 0.2f), (int)bw, (int)bh, Color{ 74, 138, 70, 255 });
            DrawRectangle((int)(cx - bw * 0.5f), (int)(cy - bh * 0.5f), (int)bw, (int)(bh * 1.2f), Color{ 90, 158, 84, 255 });
            DrawRectangle((int)(cx + bw * 1.8f), (int)(cy - bh * 0.2f), (int)bw, (int)bh, Color{ 74, 138, 70, 255 });
            break;
        }
    }
}

// Рисует ОРИЕНТИР (зону интереса) — визуальный маркер на открытом поле (v0.4 Шаг 12).
// Тип ориентира выбирается детерминированно по хешу тайла: 0 — поляна, 1 — руины,
// 2 — алтарь. Это чистый визуал (коллизий нет), помогает не теряться на большом поле
// и служит «магнитом» для дальнего спавна (Шаг 13) и наград (Шаг 14).
static void DrawPoi(int cx, int cy, int tileSize, unsigned int h)
{
    float r = Tuning::kPoiMarkerRadius * (float)tileSize;
    int kind = (int)(h % 3);
    switch (kind)
    {
        case 0: // поляна — светло-зелёный круг с травинками по кругу
        {
            DrawCircle(cx, cy, r, Color{ 64, 116, 60, 70 });
            DrawCircleLines(cx, cy, r, Color{ 92, 156, 86, 120 });
            int bw = (int)(tileSize * 0.04f); if (bw < 1) bw = 1;
            for (int i = 0; i < 6; i++)
            {
                float a = (float)i / 6.0f * 6.2832f;
                float bx = cx + cosf(a) * r * 0.55f;
                float by = cy + sinf(a) * r * 0.55f;
                DrawRectangle((int)bx, (int)(by - tileSize * 0.18f), bw, (int)(tileSize * 0.18f), Color{ 90, 158, 84, 200 });
            }
            break;
        }
        case 1: // руины — каменное кольцо из блоков
        {
            DrawCircle(cx, cy, r, Color{ 60, 58, 64, 50 });
            int blocks = 8;
            for (int i = 0; i < blocks; i++)
            {
                float a = (float)i / (float)blocks * 6.2832f;
                float bx = cx + cosf(a) * r * 0.8f;
                float by = cy + sinf(a) * r * 0.8f;
                float s = tileSize * 0.34f;
                DrawRectangle((int)(bx - s * 0.5f), (int)(by - s * 0.5f), (int)s, (int)s, Color{ 120, 116, 110, 230 });
                DrawRectangleLines((int)(bx - s * 0.5f), (int)(by - s * 0.5f), (int)s, (int)s, Color{ 80, 76, 72, 255 });
            }
            break;
        }
        default: // алтарь — постамент с кристаллом
        {
            DrawCircle(cx, cy, r, Color{ 70, 60, 90, 50 });
            float pw = tileSize * 0.9f;
            float ph = tileSize * 0.5f;
            DrawRectangle((int)(cx - pw * 0.5f), (int)(cy - ph * 0.3f), (int)pw, (int)ph, Color{ 110, 104, 120, 235 });
            DrawRectangleLines((int)(cx - pw * 0.5f), (int)(cy - ph * 0.3f), (int)pw, (int)ph, Color{ 70, 66, 80, 255 });
            float cs = tileSize * 0.34f;
            DrawCircle(cx, (int)(cy - ph * 0.3f), cs * 0.6f, Color{ 120, 200, 230, 180 });
            DrawCircle(cx, (int)(cy - ph * 0.3f), cs * 0.3f, Color{ 200, 240, 255, 230 });
            break;
        }
    }
}

void TileMap::Draw(const Camera2D& camera) const
{
    // CULLING: вычисляем видимую область в мировых координатах
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
                // Вариативность: отражаем тайл по флагам хеша,
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

            // Декоративные пропы (Шаг 8): рисуем поверх пола, видны и без текстур.
            // Не блокируют движение — это визуальные ориентиры на открытом поле.
            if (!wall)
            {
                DrawProp(x, y, tileSize, decor[row][col], h);
            }
        }
    }

    // Ориентиры/зоны интереса (v0.4 Шаг 12): рисуем поверх поля, только в пределах
    // видимой области (тот же кулинг, что и у тайлов). Точку старта (центр) пропускаем —
    // там игрок. Тип маркера детерминирован хешом тайла, поэтому при перерисовке
    // ориентир всегда выглядит одинаково.
    for (size_t i = 0; i < rooms.size(); i++)
    {
        int rc = (int)rooms[i].x;
        int rr = (int)rooms[i].y;
        if (rc == spawnCol && rr == spawnRow) continue;
        if (rc < colStart || rc > colEnd || rr < rowStart || rr > rowEnd) continue;
        int px = rc * tileSize + tileSize / 2;
        int py = rr * tileSize + tileSize / 2;
        DrawPoi(px, py, tileSize, TileHash(rc, rr));
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
