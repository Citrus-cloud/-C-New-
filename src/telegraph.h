#pragma once
#include "raylib.h"
#include <vector>
// ============================================================================
//  telegraph.h — СИСТЕМА ТЕЛЕГРАФОВ (Фаза 2, Шаг 6-9)
// ----------------------------------------------------------------------------
//  Телеграф — это «предупреждение» об атаке: зона показывается заранее,
//  заполняется за fillTime секунд, перед ударом мигает, затем бьёт один раз.
//  Это общий механизм для лазеров, прыжков-ударов и любых зон урона (Фазы 3-5).
//  Все тайминги берутся из tuning.h.
// ============================================================================

class Player;   // форвард-объявления, чтобы не тянуть тяжёлые заголовки в хеде
class Effects;

// Формы зоны телеграфа.
enum TelegraphShape {
    TELEGRAPH_CIRCLE = 0, // круг радиуса radius вокруг origin
    TELEGRAPH_LINE,       // луч из origin по направлению angle: длина length, ширина width
    TELEGRAPH_CONE        // конус из origin: направление angle, дальность length, полу-угол halfAngle
};

// Одно предупреждение об атаке.
struct Telegraph {
    bool  active = false;
    TelegraphShape shape = TELEGRAPH_CIRCLE;
    Vector2 origin = { 0.0f, 0.0f };
    float angle = 0.0f;       // направление (рад) для линии/конуса
    float radius = 0.0f;      // круг
    float length = 0.0f;      // линия/конус: длина
    float width = 0.0f;       // линия: ширина
    float halfAngle = 0.0f;   // конус: половина угла (рад)
    float fillTime = 1.0f;    // за сколько заполняется до удара, сек
    float timer = 0.0f;       // прошедшее время
    float linger = 0.0f;      // остаток времени горения после удара
    int   damage = 0;
    bool  triggered = false;  // удар уже нанесён (однократно)
    Color color = RED;

    float Progress() const;            // 0..1
    bool  HitsPoint(Vector2 p) const;  // попадает ли точка в зону
    void  Draw() const;                // контур + заливка + мигание
    void  DrawOutline(Color c) const;  // только контур (для отладки)
};

// Пул телеграфов. Враги/спавнер «заказывают» зоны через Spawn*-методы.
class TelegraphSystem {
public:
    std::vector<Telegraph> pool;

    TelegraphSystem(int poolSize = 64);
    Telegraph* GetInactive();

    // Удобные конструкторы зон (fillTime обычно из Tuning::kTelegraphDefaultFill).
    Telegraph* SpawnCircle(Vector2 origin, float radius, int damage, float fillTime, Color c);
    Telegraph* SpawnLine(Vector2 origin, float angle, float length, float width, int damage, float fillTime, Color c);
    Telegraph* SpawnCone(Vector2 origin, float angle, float length, float halfAngle, int damage, float fillTime, Color c);

    // Обновление: продвигает таймеры; на срабатывании бьёт игрока, если он в зоне.
    void Update(float dt, Player& player, Effects& effects);
    void Clear();
    void Draw() const;        // обычная отрисовка зон (по земле)
    void DrawDebug() const;   // отладка: яркие контуры всех активных зон
    int  ActiveCount() const;
};
