#pragma once
#include "raylib.h"

// =====================================================================
// Таблица карт (Шаг 25). Разные размеры подземелий — выбираются
// в меню перед забегом. Добавить карту = дописать строку в kMaps.
// =====================================================================

enum MapId { MAP_CATACOMBS = 0, MAP_DUNGEON = 1, MAP_HALLS = 2, MAP_COUNT };

struct MapDef
{
    const char* name;
    const char* description;
    int width;
    int height;
};

inline const MapDef kMaps[MAP_COUNT] = {
    { "Тесные катакомбы", "маленькая карта, плотные схватки", 64, 48 },
    { "Большое подземелье", "стандартный размер", 96, 72 },
    { "Бескрайние залы", "огромная карта", 128, 96 },
};

inline const MapDef& GetMap(int id)
{
    if (id < 0 || id >= MAP_COUNT) id = 0;
    return kMaps[id];
}
