#pragma once
#include "raylib.h"

class Player;
class Spawner;
class Weapon;

// Интерфейс: полоски, таймер, меню, пауза, экран смерти.
// Использует кириллический шрифт из assets/fonts/game.ttf (если есть).
class HUD
{
public:
    Font font;
    bool customFont;
    int screenWidth;
    int screenHeight;

    HUD();
    void Init(int sw, int sh);
    void Unload();

    void Text(const char* t, float x, float y, float size, Color c) const;
    float TextWidth(const char* t, float size) const;

    void DrawGame(const Player& player, const Spawner& spawner, const Weapon& weapon, float survivalTime) const;
    void DrawSubtitle(const char* speaker, const char* line, float alpha) const;
    void DrawMenu(int bestTime, int bestLevel, int coins) const;
    void DrawPause() const;
    void DrawGameOver(float survivalTime, int level, int bestTime, int bestLevel, bool newRecord) const;
    void DrawLevelUp(const char* o1, const char* o2, const char* o3) const;
};
