#include "pathfinding.h"
#include <queue>
#include <unordered_map>
#include <functional>
#include <cmath>
#include <algorithm>

bool HasLineOfSight(const TileMap& map, Vector2 a, Vector2 b)
{
    float dx = b.x - a.x, dy = b.y - a.y;
    float dist = sqrtf(dx * dx + dy * dy);
    int steps = (int)(dist / (map.tileSize * 0.5f)) + 1;
    for (int i = 0; i <= steps; i++)
    {
        float t = (float)i / steps;
        float x = a.x + dx * t;
        float y = a.y + dy * t;
        int col = (int)(x / map.tileSize);
        int row = (int)(y / map.tileSize);
        if (map.IsWall(col, row)) return false;
    }
    return true;
}

std::vector<Vector2> FindPath(const TileMap& map, Vector2 start, Vector2 goal)
{
    int cols = map.Cols();
    int rows = map.Rows();
    if (cols == 0 || rows == 0) return {};

    int sc = (int)(start.x / map.tileSize), sr = (int)(start.y / map.tileSize);
    int gc = (int)(goal.x / map.tileSize),  gr = (int)(goal.y / map.tileSize);

    auto inBounds = [&](int c, int r) { return c >= 0 && r >= 0 && c < cols && r < rows; };
    if (!inBounds(sc, sr) || !inBounds(gc, gr)) return {};
    if (map.IsWall(gc, gr)) return {};

    auto key  = [&](int c, int r) { return r * cols + c; };
    auto heur = [&](int c, int r) { return fabsf((float)(c - gc)) + fabsf((float)(r - gr)); };

    std::priority_queue<std::pair<float, int>,
                        std::vector<std::pair<float, int>>,
                        std::greater<std::pair<float, int>>> open;
    std::unordered_map<int, float> g;
    std::unordered_map<int, int> came;

    int startK = key(sc, sr);
    int goalK  = key(gc, gr);
    g[startK] = 0.0f;
    open.push({ heur(sc, sr), startK });

    const int dc[4] = { 1, -1, 0, 0 };
    const int dr[4] = { 0, 0, 1, -1 };
    bool found = false;
    int iterations = 0;

    while (!open.empty())
    {
        if (++iterations > 6000) break;  // защита от зависания
        int cur = open.top().second;
        open.pop();
        if (cur == goalK) { found = true; break; }

        int cc = cur % cols, cr = cur / cols;
        for (int d = 0; d < 4; d++)
        {
            int nc = cc + dc[d], nr = cr + dr[d];
            if (!inBounds(nc, nr) || map.IsWall(nc, nr)) continue;
            int nk = key(nc, nr);
            float ng = g[cur] + 1.0f;
            auto it = g.find(nk);
            if (it == g.end() || ng < it->second)
            {
                g[nk] = ng;
                came[nk] = cur;
                open.push({ ng + heur(nc, nr), nk });
            }
        }
    }

    if (!found) return {};

    std::vector<Vector2> path;
    int cur = goalK;
    while (cur != startK)
    {
        int c = cur % cols, r = cur / cols;
        path.push_back({
            c * (float)map.tileSize + map.tileSize * 0.5f,
            r * (float)map.tileSize + map.tileSize * 0.5f
        });
        auto it = came.find(cur);
        if (it == came.end()) break;
        cur = it->second;
    }
    std::reverse(path.begin(), path.end());
    return path;
}
