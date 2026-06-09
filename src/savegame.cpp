#include "savegame.h"
#include "raylib.h"
#include <cstdio>

static const char* SAVE_FILE = "savegame.txt";

GameSave LoadGameSave()
{
    // Значения по умолчанию.
    GameSave s;
    s.bestTime = 0;
    s.bestLevel = 0;
    s.volume = 100;
    s.musicVolume = 80;
    s.sfxVolume = 100;
    s.fullscreen = 0;
    s.coins = 0;
    s.upgHealth = 0;
    s.upgDamage = 0;
    s.upgSpeed = 0;

    if (FileExists(SAVE_FILE))
    {
        char* text = LoadFileText(SAVE_FILE);
        if (text != nullptr)
        {
            // sscanf заполняет только разобранные поля; остальные остаются
            // со значениями по умолчанию (совместимость со старыми сейвами).
            sscanf(text, "%d %d %d %d %d %d %d %d %d %d",
                   &s.bestTime, &s.bestLevel, &s.volume,
                   &s.musicVolume, &s.sfxVolume, &s.fullscreen,
                   &s.coins, &s.upgHealth, &s.upgDamage, &s.upgSpeed);
            UnloadFileText(text);
        }
    }

    if (s.musicVolume < 0) s.musicVolume = 0;
    if (s.musicVolume > 100) s.musicVolume = 100;
    if (s.sfxVolume < 0) s.sfxVolume = 0;
    if (s.sfxVolume > 100) s.sfxVolume = 100;
    if (s.coins < 0) s.coins = 0;
    return s;
}

void SaveGameSave(const GameSave& save)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%d %d %d %d %d %d %d %d %d %d\n",
             save.bestTime, save.bestLevel, save.volume,
             save.musicVolume, save.sfxVolume, save.fullscreen,
             save.coins, save.upgHealth, save.upgDamage, save.upgSpeed);
    SaveFileText(SAVE_FILE, buffer);
}
