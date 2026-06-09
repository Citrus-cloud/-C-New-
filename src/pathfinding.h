#pragma once
#include "raylib.h"
#include "tilemap.h"
#include <vector>

// Есть ли прямая видимость между двумя точками (без стен на пути).
bool HasLineOfSight(const TileMap& map, Vector2 a, Vector2 b);

// A*: путь от start до goal как список точек (центров тайлов).
// Пустой вектор = путь не найден.
std::vector<Vector2> FindPath(const TileMap& map, Vector2 start, Vector2 goal);
