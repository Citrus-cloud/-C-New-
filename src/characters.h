#pragma once
#include "raylib.h"

// =====================================================================
// Классы персонажей (Шаг 24) — задел на будущее.
// Данные и применение модификаторов уже готовы; полный экран выбора
// в меню — это Шаг 25 (Фаза 6). Добавить класс = дописать строку.
// =====================================================================

enum CharacterId
{
    CHAR_WARRIOR = 0,   // больше здоровья и урона
    CHAR_RANGER  = 1,   // быстрее стреляет и двигается
    CHAR_MAGE    = 2,   // начинает с орбитальными клинками
    CHAR_COUNT
};

struct CharacterClass
{
    const char* name;
    const char* description;
    int bonusHealth;       // прибавка к максимальному здоровью
    float moveSpeedMul;    // множитель скорости движения
    float fireRateMul;     // множитель интервала стрельбы (<1 = быстрее)
    int bonusDamage;       // прибавка к урону оружия
    bool startWithOrbit;   // начинать с орбитальными клинками
};

inline const CharacterClass kCharacters[CHAR_COUNT] = {
    { "Воин", "больше здоровья и урона", 50, 1.0f, 1.0f, 5, false },
    { "Следопыт", "быстрая стрельба и движение", 0, 1.15f, 0.85f, 0, false },
    { "Маг", "старт с орбитальными клинками", 0, 1.0f, 1.0f, 0, true },
};

inline const CharacterClass& GetCharacter(int id)
{
    if (id < 0 || id >= CHAR_COUNT) id = 0;
    return kCharacters[id];
}
