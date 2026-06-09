#pragma once
#include "raylib.h"
#include <vector>
#include <string>

// Предварительное объявление менеджера текстур.
class TextureManager;

// Тайловая карта: 'W' - стена, '.' - пол.
// Размер любой - карта генерируется процедурно (комнаты + коридоры).
class TileMap
{
public:
    // Размер чанка в тайлах — задел на бесконечный мир (Шаг 14).
    static constexpr int CHUNK_SIZE = 16;

    int tileSize;
    unsigned int worldSeed;            // seed текущего мира (для детерминированных тайлов)
    std::vector<std::string> grid;     // 'W' стена, '.' пол
    std::vector<std::string> decor;    // ' ' нет, 'b' кости, 'p' лужа, 'g' трава
    std::vector<Vector2> rooms;        // центры комнат (в тайлах)
    int spawnCol;
    int spawnRow;

    TileMap();

    void LoadArt(TextureManager& tex);            // загрузка текстур тайлсета (Шаг 10)
    void Generate(int width, int height, unsigned int seed);
    void Draw(const Camera2D& camera) const;      // отрисовка только видимых тайлов (Шаг 11)
    bool IsWall(int col, int row) const;
    bool CheckCollision(Rectangle rect) const;
    bool IsFree(Rectangle rect) const;
    Vector2 FindFreeSpot(Vector2 desired, float halfSize) const;
    Vector2 GetSpawnPoint() const;

    int Cols() const { return grid.empty() ? 0 : (int)grid[0].size(); }
    int Rows() const { return (int)grid.size(); }

private:
    bool hasArt;
    Texture2D texFloor, texWall, texBones, texPuddle, texGrass;

    // Детерминированный хеш координаты тайла — фундамент чанковой генерации (Шаг 14):
    // внешний вид тайла зависит только от seed и координат, поэтому одинаков
unsigned int TileHash(int col, int row) const;
};
