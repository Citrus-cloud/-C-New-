#pragma once
#include "raylib.h"
#include "tilemap.h"
#include <vector>

class Enemy
{
public:
    Vector2 position;
    float speed;
    int health;
    bool active;

    // Для поиска пути A*
    std::vector<Vector2> path;
    int pathIndex;
    float repathTimer;

    Enemy();
    void Spawn(Vector2 pos);
    void Update(float deltaTime, Vector2 playerPos, const TileMap& map);
    void Draw() const;
    Rectangle GetRect() const;
};
