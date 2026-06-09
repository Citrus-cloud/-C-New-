#include "savegame.h"
#include "raylib.h"
#include <cstdio>

static const char* SAVE_FILE = "savegame.txt";

GameSave LoadGameSave()
{
    GameSave s;
    s.bestTime = 0;
    s.bestLevel = 0;
    s.volume = 100;

    if (FileExists(SAVE_FILE))
    {
        char* text = LoadFileText(SAVE_FILE);
        if (text != nullptr)
        {
            int bt = 0, bl = 0, vol = 100;
            if (sscanf(text, "%d %d %d", &bt, &bl, &vol) == 3)
            {
                s.bestTime = bt;
                s.bestLevel = bl;
                s.volume = vol;
            }
            UnloadFileText(text);
        }
    }

    if (s.volume < 0) s.volume = 0;
    if (s.volume > 100) s.volume = 100;
    return s;
}

void SaveGameSave(const GameSave& save)
{
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%d %d %d\n", save.bestTime, save.bestLevel, save.volume);
    SaveFileText(SAVE_FILE, buffer);
}
