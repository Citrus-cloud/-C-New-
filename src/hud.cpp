#include "hud.h"
#include "player.h"
#include "spawner.h"
#include "combat.h"
#include <vector>

HUD::HUD() : customFont(false), screenWidth(1280), screenHeight(720)
{
    font = GetFontDefault();
}

void HUD::Init(int sw, int sh)
{
    screenWidth = sw;
    screenHeight = sh;

    if (FileExists("assets/fonts/game.ttf"))
    {
        std::vector<int> cps;
        for (int c = 0x20; c <= 0x7E; c++) cps.push_back(c);       // латиница + цифры
        for (int c = 0x0400; c <= 0x045F; c++) cps.push_back(c);   // кириллица
        font = LoadFontEx("assets/fonts/game.ttf", 32, cps.data(), (int)cps.size());
        customFont = (font.texture.id != 0);
        if (!customFont) font = GetFontDefault();
    }
    else
    {
        font = GetFontDefault();
        customFont = false;
    }
}

void HUD::Unload()
{
    if (customFont) UnloadFont(font);
}

void HUD::Text(const char* t, float x, float y, float size, Color c) const
{
    DrawTextEx(font, t, { x, y }, size, 2.0f, c);
}

float HUD::TextWidth(const char* t, float size) const
{
    return MeasureTextEx(font, t, size, 2.0f).x;
}

void HUD::DrawGame(const Player& player, const Spawner& spawner, const Weapon& weapon, float survivalTime) const
{
    // Полоса здоровья
    float hpFrac = (player.maxHealth > 0) ? (float)player.health / player.maxHealth : 0.0f;
    if (hpFrac < 0.0f) hpFrac = 0.0f;
    DrawRectangle(20, 20, 220, 22, Color{ 40, 40, 40, 200 });
    DrawRectangle(20, 20, (int)(220 * hpFrac), 22, RED);
    DrawRectangleLines(20, 20, 220, 22, RAYWHITE);
    Text(TextFormat("HP %d/%d", player.health, player.maxHealth), 26, 22, 18, RAYWHITE);

    // Полоса опыта
    float xpFrac = (player.xpToNext > 0) ? (float)player.xp / player.xpToNext : 0.0f;
    if (xpFrac > 1.0f) xpFrac = 1.0f;
    DrawRectangle(20, 50, 220, 14, Color{ 40, 40, 40, 200 });
    DrawRectangle(20, 50, (int)(220 * xpFrac), 14, GREEN);
    DrawRectangleLines(20, 50, 220, 14, RAYWHITE);
    Text(TextFormat("Уровень %d", player.level), 26, 66, 18, RAYWHITE);

    // Таймер выживания по центру сверху
    int total = (int)survivalTime;
    const char* tm = TextFormat("%02d:%02d", total / 60, total % 60);
    Text(tm, screenWidth / 2.0f - TextWidth(tm, 32) / 2.0f, 18, 32, RAYWHITE);

    // Справа
    Text(TextFormat("Врагов: %d", spawner.ActiveCount()), screenWidth - 200.0f, 20, 20, RAYWHITE);
    const char* w = TextFormat("Оружие ур. %d%s", weapon.level, weapon.evolved ? " (эволюция)" : "");
    Text(w, screenWidth - 260.0f, 46, 20, weapon.evolved ? GOLD : RAYWHITE);
}

void HUD::DrawSubtitle(const char* speaker, const char* line, float alpha) const
{
    if (alpha <= 0.0f) return;
    unsigned char a = (unsigned char)(alpha * 255);
    float size = 30.0f;
    float y = screenHeight - 120.0f;
    DrawRectangle(0, (int)y - 30, screenWidth, 92, Color{ 0, 0, 0, (unsigned char)(alpha * 160) });
    Text(speaker, screenWidth / 2.0f - TextWidth(speaker, 20) / 2.0f, y - 28, 20, Color{ 255, 80, 80, a });
    Text(line, screenWidth / 2.0f - TextWidth(line, size) / 2.0f, y, size, Color{ 255, 255, 255, a });
}

void HUD::DrawMenu(int bestTime, int bestLevel, int volume) const
{
    ClearBackground(Color{ 15, 12, 20, 255 });
    const char* title = "DUNGEON SURVIVORS: D20";
    Text(title, screenWidth / 2.0f - TextWidth(title, 46) / 2.0f, 150, 46, GOLD);
    const char* sub = "Лагерь";
    Text(sub, screenWidth / 2.0f - TextWidth(sub, 24) / 2.0f, 218, 24, LIGHTGRAY);

    const char* play = "Нажми ENTER, чтобы начать вылазку";
    Text(play, screenWidth / 2.0f - TextWidth(play, 28) / 2.0f, 330, 28, RAYWHITE);

    const char* rec = TextFormat("Рекорд: %02d:%02d,  уровень %d", bestTime / 60, bestTime % 60, bestLevel);
    Text(rec, screenWidth / 2.0f - TextWidth(rec, 22) / 2.0f, 400, 22, GOLD);

    const char* vol = TextFormat("Громкость: %d%%   (< / >)", volume);
    Text(vol, screenWidth / 2.0f - TextWidth(vol, 22) / 2.0f, 440, 22, LIGHTGRAY);

    const char* hint = "WASD — движение,  ESC — пауза";
    Text(hint, screenWidth / 2.0f - TextWidth(hint, 20) / 2.0f, 490, 20, GRAY);
}

void HUD::DrawPause() const
{
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 150 });
    const char* t = "ПАУЗА";
    Text(t, screenWidth / 2.0f - TextWidth(t, 52) / 2.0f, 280, 52, RAYWHITE);
    const char* r = "ESC — продолжить";
    Text(r, screenWidth / 2.0f - TextWidth(r, 24) / 2.0f, 360, 24, LIGHTGRAY);
}

void HUD::DrawGameOver(float survivalTime, int level, int bestTime, int bestLevel, bool newRecord) const
{
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 200 });
    const char* t = "ВЫ ПОГИБЛИ";
    Text(t, screenWidth / 2.0f - TextWidth(t, 56) / 2.0f, 180, 56, RED);

    int total = (int)survivalTime;
    const char* st = TextFormat("Продержался %02d:%02d, уровень %d", total / 60, total % 60, level);
    Text(st, screenWidth / 2.0f - TextWidth(st, 26) / 2.0f, 280, 26, RAYWHITE);

    if (newRecord)
    {
        const char* nr = "НОВЫЙ РЕКОРД!";
        Text(nr, screenWidth / 2.0f - TextWidth(nr, 30) / 2.0f, 320, 30, GOLD);
    }
    else
    {
        const char* br = TextFormat("Рекорд: %02d:%02d, уровень %d", bestTime / 60, bestTime % 60, bestLevel);
        Text(br, screenWidth / 2.0f - TextWidth(br, 22) / 2.0f, 322, 22, LIGHTGRAY);
    }

    const char* r = "Нажми ENTER, чтобы вернуться в лагерь";
    Text(r, screenWidth / 2.0f - TextWidth(r, 24) / 2.0f, 380, 24, RAYWHITE);
}

void HUD::DrawLevelUp(const char* o1, const char* o2, const char* o3) const
{
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 170 });
    const char* t = "ПОВЫШЕНИЕ УРОВНЯ!";
    Text(t, screenWidth / 2.0f - TextWidth(t, 46) / 2.0f, 200, 46, YELLOW);
    const char* s = "Выбери улучшение (1 / 2 / 3):";
    Text(s, screenWidth / 2.0f - TextWidth(s, 26) / 2.0f, 290, 26, RAYWHITE);
    Text(o1, screenWidth / 2.0f - 200.0f, 350, 26, RAYWHITE);
    Text(o2, screenWidth / 2.0f - 200.0f, 395, 26, RAYWHITE);
    Text(o3, screenWidth / 2.0f - 200.0f, 440, 26, RAYWHITE);
}
