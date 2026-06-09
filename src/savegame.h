#pragma once

// Простое сохранение: рекорд + настройки. Пишется в savegame.txt.
struct GameSave
{
    int bestTime;   // лучшее время выживания, секунды
    int bestLevel;  // лучший достигнутый уровень
    int volume;     // громкость 0..100
};

GameSave LoadGameSave();
void SaveGameSave(const GameSave& save);
