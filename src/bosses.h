#pragma once
#include "raylib.h"

// =====================================================================
// Менеджер данных боссов (Шаг 20)
// Все характеристики боссов вынесены в таблицу. Чтобы добавить
// нового босса, достаточно дописать строку в массив kBossDefs ниже —
// спавн, механики и озвучка подхватят его автоматически.
// =====================================================================

enum BossId
{
    BOSS_SPIDER_QUEEN = 0,   // Королева пауков — призывает паучков
    BOSS_BLACK_KNIGHT = 1,   // Чёрный рыцарь — рывок
    BOSS_COUNT
};

// Данные одного босса: статы + какие механики включены.
struct BossDef
{
    const char* name;        // имя для субтитра
    const char* line;        // фраза при появлении
    int voiceLine;           // индекс голоса в AudioManager
    const char* sprite;      // путь к спрайту (если нет — общий boss.png)
    Color color;             // цвет для fallback-прямоугольника
    int health;
    float speed;
    float size;
    int damage;
    int xpValue;
    bool canDash;            // механика рывка (Чёрный рыцарь)
    bool canSummon;          // механика призыва (Королева пауков)
    float summonInterval;    // период призыва в секундах (если canSummon)
};

// Таблица боссов. inline-переменная (C++17) — определяем прямо в заголовке.
inline const BossDef kBossDefs[BOSS_COUNT] = {
    { "Королева пауков", "Ты будешь кормом моих детей.", 0, "assets/sprites/boss_spider.png", Color{ 120, 40, 160, 255 }, 650, 90.0f, 38.0f, 18, 25, false, true, 5.0f },
    { "Чёрный рыцарь", "Жалкая пародия.", 1, "assets/sprites/boss_knight.png", Color{ 70, 70, 95, 255 }, 800, 100.0f, 38.0f, 22, 25, true, false, 0.0f },
};

// Безопасный доступ по индексу.
inline const BossDef& GetBossDef(int id)
{
    if (id < 0 || id >= BOSS_COUNT) id = 0;
    return kBossDefs[id];
}
