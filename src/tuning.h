#pragma once
// ============================================================================
//  tuning.h — ЕДИНЫЙ ПУЛЬТ НАСТРОЙКИ БОЯ И ВОЛН (Фаза 1, Шаг 1-2)
// ----------------------------------------------------------------------------
//  Здесь собраны ВСЕ числовые параметры боевой системы: когда что
//  появляется, как часто, с какими паузами. Цель — менять баланс
//  в одном месте, не трогая логику. Любая новая способность врага
//  должна брать свои параметры отсюда, а не хранить «магические числа» у себя.
//
//  Договорённость: время измеряется в секундах от начала забега (elapsed).
// ============================================================================

namespace Tuning {

// ---------------------------------------------------------------------------
//  СПАВН ВОЛН
//  Интервал между волнами сокращается со временем (сложность растёт),
//  но не ниже минимума. Размер волны растёт со временем до потолка.
// ---------------------------------------------------------------------------
inline constexpr float kSpawnBaseInterval = 2.0f;   // стартовый интервал между волнами, сек
inline constexpr float kSpawnMinInterval  = 0.6f;   // минимальный интервал (потолок сложности), сек
inline constexpr float kSpawnRampPerSec   = 0.01f;  // на сколько сокращается интервал за секунду забега

inline constexpr int   kWaveBaseCount     = 6;      // базовый размер волны
inline constexpr float kWaveCountRampDiv  = 30.0f;  // +1 враг к волне каждые N секунд
inline constexpr int   kWaveMaxCount      = 12;     // максимальный размер волны

// Состав волны (шансы в процентах 0..99).
inline constexpr float kTankUnlockTime    = 60.0f;  // танки появляются после этого времени
inline constexpr int   kTankChance        = 20;     // шанс танка (когда разблокирован)
inline constexpr int   kFastChance        = 35;     // шанс быстрого врага

// ---------------------------------------------------------------------------
//  БОССЫ
// ---------------------------------------------------------------------------
inline constexpr bool  kBossEnabled       = true;
inline constexpr float kBossInterval      = 30.0f;  // период появления боссов, сек
inline constexpr int   kSummonMinionCount = 3;      // сколько миньонов за один призыв

// ---------------------------------------------------------------------------
//  СПОСОБНОСТИ И ПОВЕДЕНИЕ ВРАГОВ
//  Перечень всех приёмов, которые будут добавляться в следующих фазах.
//  Они объявлены заранее, чтобы менеджер волн (director) и враги
//  могли ссылаться на них уже сейчас. Реализация — по фазам плана v0.3.
// ---------------------------------------------------------------------------
enum AbilityId {
    ABIL_RANGED_SHOOTER = 0, // дальние снаряды с упреждением (Фаза 3)
    ABIL_LASER,              // лазер с задержкой и фиксацией (Фаза 3)
    ABIL_VOLLEY,             // веер/залп снарядов, босс-паттерн (Фаза 3)
    ABIL_JUMP_SLAM,          // прыжок-наскок с ударом по площади (Фаза 4)
    ABIL_TELEPORT,           // телепорт к игроку (Фаза 4)
    ABIL_BLINK,              // короткие блинк-телепорты (Фаза 4)
    ABIL_DASH,               // разгон-рывок с телеграфом пути (Фаза 4)
    ABIL_FLANK,              // окружение / заход сбоку (Фаза 4)
    ABIL_SHIELD,             // временный щит/неуязвимость (Фаза 5)
    ABIL_SPLIT,              // разделение при смерти (Фаза 5)
    ABIL_SUMMON,             // призыв миньонов (Фаза 5)
    ABIL_SLOW_AURA,          // аура замедления (Фаза 5)
    ABIL_POISON_TRAIL,       // ядовитый след/лужи (Фаза 5)
    ABIL_HEALER,             // лечение/усиление союзников (Фаза 5)
    ABILITY_COUNT
};

// Правило появления и использования одного приёма.
struct AbilityRule {
    const char* name;        // короткое имя для отладки (латиница, читаемо без шрифта)
    float       unlockTime;  // с какой секунды забега приём доступен
    float       minInterval; // мин. пауза между использованиями (кулдаун), сек; 0 = пассивный
    float       weight;      // относительный вес выбора приёма (для будущего director)
    bool        enabled;     // глобальный выключатель приёма
};

// Таблица правил. Индекс = AbilityId. Меняй значения здесь — и весь баланс
// поведения врагов поедет за тобой. Времена подобраны по нарастанию сложности.
inline const AbilityRule kAbilityRules[ABILITY_COUNT] = {
    /* ABIL_RANGED_SHOOTER */ { "Ranged",   20.0f,  4.0f, 1.5f, true },
    /* ABIL_LASER          */ { "Laser",    45.0f,  6.0f, 1.0f, true },
    /* ABIL_VOLLEY         */ { "Volley",   90.0f, 10.0f, 0.8f, true },
    /* ABIL_JUMP_SLAM      */ { "JumpSlam", 30.0f,  7.0f, 1.0f, true },
    /* ABIL_TELEPORT       */ { "Teleport", 60.0f,  8.0f, 0.8f, true },
    /* ABIL_BLINK          */ { "Blink",    75.0f,  5.0f, 0.7f, true },
    /* ABIL_DASH           */ { "Dash",     15.0f,  5.0f, 1.2f, true },
    /* ABIL_FLANK          */ { "Flank",    25.0f,  0.0f, 1.0f, true },
    /* ABIL_SHIELD         */ { "Shield",   70.0f, 12.0f, 0.6f, true },
    /* ABIL_SPLIT          */ { "Split",    40.0f,  0.0f, 1.0f, true },
    /* ABIL_SUMMON         */ { "Summon",   30.0f,  8.0f, 1.0f, true },
    /* ABIL_SLOW_AURA      */ { "SlowAura", 50.0f,  0.0f, 0.8f, true },
    /* ABIL_POISON_TRAIL   */ { "Poison",   55.0f,  0.0f, 0.8f, true },
    /* ABIL_HEALER         */ { "Healer",   65.0f,  0.0f, 0.7f, true },
};

// --- Хелперы доступности и темпа (Шаг 4) ---

// Правило по идентификатору.
inline const AbilityRule& GetRule(AbilityId id) { return kAbilityRules[id]; }

// Разблокирован ли приём к моменту времени elapsed.
inline bool IsUnlockedAt(AbilityId id, float elapsed) {
    const AbilityRule& r = kAbilityRules[id];
    return r.enabled && elapsed >= r.unlockTime;
}

// Текущий интервал между волнами (с учётом роста сложности).
inline float CurrentSpawnInterval(float elapsed) {
    float v = kSpawnBaseInterval - elapsed * kSpawnRampPerSec;
    return (v < kSpawnMinInterval) ? kSpawnMinInterval : v;
}

// Текущий размер волны (растёт со временем до потолка).
inline int CurrentWaveCount(float elapsed) {
    int c = kWaveBaseCount + (int)(elapsed / kWaveCountRampDiv);
    return (c > kWaveMaxCount) ? kWaveMaxCount : c;
}

} // namespace Tuning
