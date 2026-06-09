#pragma once

// Сохранение: рекорд, настройки и мета-прокачка. Пишется в savegame.txt.
// Формат расширяемый: старые файлы читаются, недостающие поля = по умолчанию.
struct GameSave
{
    int bestTime;     // лучшее время выживания, секунды
    int bestLevel;    // лучший достигнутый уровень
    int volume;       // общая громкость (legacy, для совместимости)
    int musicVolume;  // громкость музыки 0..100 (Шаг 27)
    int sfxVolume;    // громкость звуков 0..100 (Шаг 27)
    int fullscreen;   // полный экран 0/1 (Шаг 27)
    int coins;        // валюта мета-прокачки (Шаг 26)
    int upgHealth;    // уровень постоянного бонуса к HP
    int upgDamage;    // уровень постоянного бонуса к урону
    int upgSpeed;     // уровень постоянного бонуса к скорости
};

GameSave LoadGameSave();
void SaveGameSave(const GameSave& save);
