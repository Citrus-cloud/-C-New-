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
//  РАЗМЕР И МАСШТАБ КАРТЫ (v0.4, Фаза 1, Шаг 1)
//  В v0.4 карта заметно больше и «открытее» (минимум стен — см. Фазу 2).
//  Размеры поля задаются здесь и применяются при генерации tilemap (Шаг 2).
//  Базовый размер умножается на kMapScale — так масштаб всего поля меняется
//  одним числом, без правок логики генерации.
// ---------------------------------------------------------------------------
inline constexpr int   kMapTileSize        = 64;   // размер одного тайла, пикс (визуал и шаг сетки коллизий)
inline constexpr int   kMapBaseWidthTiles  = 96;   // базовая ширина поля в тайлах (при масштабе 1.0)
inline constexpr int   kMapBaseHeightTiles = 72;   // базовая высота поля в тайлах (при масштабе 1.0)
inline constexpr float kMapScale           = 2.0f; // множитель размера поля (2.0 = вчетверо большая площадь)
inline constexpr int   kMapMinTiles        = 16;   // нижняя граница размера по любой стороне (защита от слишком маленькой карты)

// Итоговые размеры карты в тайлах с учётом масштаба (используются при генерации, Шаг 2).
inline int MapWidthTiles()  { int w = (int)(kMapBaseWidthTiles  * kMapScale); return w < kMapMinTiles ? kMapMinTiles : w; }
inline int MapHeightTiles() { int h = (int)(kMapBaseHeightTiles * kMapScale); return h < kMapMinTiles ? kMapMinTiles : h; }

// ---------------------------------------------------------------------------
//  КАМЕРА / ОБЗОР (v0.4, Фаза 1, Шаг 3)
//  На большой открытой карте полезно видеть больше пространства вокруг игрока.
//  kCameraZoom < 1.0 отдаляет камеру (шире обзор), > 1.0 приближает.
//  Обзор настраивается одним числом — без правок логики камеры в main.cpp.
//  Куллинг отрисовки tilemap уже учитывает zoom, поэтому широкий обзор не бьёт по FPS.
// ---------------------------------------------------------------------------
inline constexpr float kCameraZoom = 0.8f; // базовый зум камеры (1.0 = прежний обзор; <1 = шире для большой карты)

// ---------------------------------------------------------------------------
//  СЛОЖНОСТЬ / ПРОФИЛИ БАЛАНСА (v0.4, Фаза 4, Шаг 16, 20)
//  Глобальные множители сложности, собранные в переключаемые ПРОФИЛИ. Идея:
//  быстрый тест («Тест», мягче ~50%) и честная игра («Норма») сосуществуют и
//  меняются профилем в рантайме (Шаг 20) — без правок логики. Множители
//  применяются в HP/уроне врагов, темпе спавна и требуемом опыте (Шаги 17-19).
//  По умолчанию для разработки включён «Тест».
//
//  Значение >1 усложняет, <1 смягчает. «Норма» = строго 1.0 (эталонный баланс).
// ---------------------------------------------------------------------------
enum DifficultyProfile {
    DIFF_TEST = 0,    // мягкий профиль для быстрого теста (~50% сложности)
    DIFF_NORMAL,      // честный/эталонный баланс (все множители 1.0)
    DIFFICULTY_COUNT
};

// Набор множителей одного профиля. Каждое поле — отдельная «ручка» баланса.
struct DifficultyMods {
    const char* name;        // имя профиля для UI (кириллица — рисуется игровым шрифтом)
    float enemyHealthMul;    // множитель HP врагов (меньше = дохнут быстрее)
    float enemyDamageMul;    // множитель урона врагов по игроку (меньше = бьют слабее)
    float spawnIntervalMul;  // множитель паузы между волнами (больше = реже волны = легче)
    float waveCountMul;      // множитель размера волны (меньше = меньше врагов за волну)
    float xpRequiredMul;     // множитель требуемого опыта на уровень (меньше = быстрее прокачка)
};

// Таблица профилей. Индекс = DifficultyProfile. «Норма» — строго 1.0 везде;
// «Тест» смягчает примерно вдвое там, где это ускоряет проверку контента.
inline const DifficultyMods kDifficultyProfiles[DIFFICULTY_COUNT] = {
    /* DIFF_TEST   */ { "\u0422\u0435\u0441\u0442",  0.5f, 0.5f, 1.5f, 0.6f, 0.5f },
    /* DIFF_NORMAL */ { "\u041d\u043e\u0440\u043c\u0430", 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
};

// Активный профиль. РАНТАЙМ-переключаемый (Шаг 20): хранится в изменяемой inline-
// переменной (одна на все единицы трансляции, C++17). Стартовое значение —
// «Тест» для разработки; в бою меняется по F2 через CycleDifficulty().
inline DifficultyProfile gDifficultyProfile = DIFF_TEST;

// Доступ к множителям активного профиля сложности.
inline const DifficultyMods& Diff() { return kDifficultyProfiles[gDifficultyProfile]; }

// Имя активного профиля (для UI/индикатора на экране).
inline const char* DifficultyName() { return kDifficultyProfiles[gDifficultyProfile].name; }

// Переключить профиль сложности по кругу (Шаг 20): Тест -> Норма -> Тест ...
inline void CycleDifficulty() {
    gDifficultyProfile = (DifficultyProfile)((gDifficultyProfile + 1) % DIFFICULTY_COUNT);
}

// Явно установить профиль сложности (с защитой диапазона).
inline void SetDifficulty(DifficultyProfile p) {
    if (p >= 0 && p < DIFFICULTY_COUNT) gDifficultyProfile = p;
}

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

// -- Зоны появления (v0.4, Фаза 1, Шаг 4): ГДЕ относительно игрока рождаются враги --
// На большой карте с широким обзором (kCameraZoom) враги должны появляться ЗА краем
// экрана, иначе видно их «возникновение» прямо в кадре. Радиус кольца волны и дистанцию
// босса берём отсюда, с запасом относительно видимой области (видимая полуширина при
// зуме 0.8 и экране 1280 ≈ 800 пикс, поэтому кольцо ставим заметно дальше).
inline constexpr float kWaveSpawnRadius   = 980.0f;  // радиус кольца появления волны вокруг игрока, пикс (за краем обзора)
inline constexpr float kBossSpawnDistance = 1000.0f; // на каком расстоянии сбоку появляется босс, пикс

// ---------------------------------------------------------------------------
//  БОССЫ
// ---------------------------------------------------------------------------
inline constexpr bool  kBossEnabled       = true;
inline constexpr float kBossInterval      = 30.0f;  // период появления боссов, сек
inline constexpr int   kSummonMinionCount = 3;      // сколько миньонов за один призыв

// ---------------------------------------------------------------------------
//  ТЕЛЕГРАФЫ (Фаза 2) — общие параметры «предупреждающих зон».
//  Зона показывается, заполняется за fillTime, затем бьёт один раз.
// ---------------------------------------------------------------------------
inline constexpr float kTelegraphDefaultFill = 1.2f;  // время заполнения зоны по умолчанию, сек
inline constexpr float kTelegraphLinger      = 0.15f; // сколько зона «горит» после удара, сек
inline constexpr float kTelegraphBlinkStart  = 0.40f; // за сколько секунд до удара начинается мигание
inline constexpr float kTelegraphBlinkRate   = 14.0f; // частота мигания перед ударом, Гц

// Демо-атака босса через телеграф (Фаза 2 — тест конвейера; полноценные приёмы — дальше).
inline constexpr float kBossTelegraphInterval = 3.5f;   // как часто активный босс «заказывает» зону, сек
inline constexpr float kBossTelegraphRadius   = 140.0f; // радиус предупреждающего круга
inline constexpr int   kBossTelegraphDamage   = 18;     // урон при срабатывании

// ---------------------------------------------------------------------------
//  ДАЛЬНИЕ АТАКИ И ЛАЗЕРЫ (Фаза 3, Шаг 11-16)
//  Лазер строится на телеграфе-линии: прицел следит за игроком
//  kLaserTrackTime секунд, затем фиксируется (окно на уход) и бьёт.
//  Снаряды врагов летят по прямой, стрелок берёт упреждение.
// ---------------------------------------------------------------------------

// -- Снаряды врагов (общие) --
inline constexpr float kEnemyProjSpeed    = 260.0f; // скорость снаряда врага, пикс/сек
inline constexpr float kEnemyProjRadius   = 7.0f;   // радиус снаряда (визуал и попадание)
inline constexpr int   kEnemyProjDamage   = 8;      // урон обычного снаряда
inline constexpr float kEnemyProjLifetime = 4.0f;   // время жизни снаряда, сек
inline constexpr float kPlayerHitRadius   = 18.0f;  // радиус попадания по игроку для снарядов

// -- Враг-стрелок (Шаг 14) --
inline constexpr float kShooterMinRange   = 180.0f; // ближе這 этого стрелок не стреляет (даёт уйти в ближний бой)
inline constexpr float kShooterLead       = 0.35f;  // доля упреждения по скорости игрока (0 = без упреждения)

// -- Лазер босса (Шаг 11-13) --
inline constexpr float kLaserFillTime     = 1.6f;    // полное время «зарядки» лазера, сек
inline constexpr float kLaserTrackTime    = 1.0f;    // сколько прицел СЛЕДИТ за игроком до фиксации, сек
inline constexpr float kLaserLength       = 1400.0f; // длина луча (через весь экран), пикс
inline constexpr float kLaserWidth        = 46.0f;   // ширина луча, пикс
inline constexpr int   kLaserDamage       = 26;      // урон лазера при срабатывании

// -- Залп/веер босса (Шаг 15) --
inline constexpr int   kVolleyCount       = 9;       // число снарядов в веере
inline constexpr float kVolleySpread      = 1.4f;    // полный угол веера, рад (~80°)
inline constexpr int   kVolleyDamage      = 10;      // урон снаряда залпа

// -- Темп дальних атак босса --
inline constexpr float kBossRangedInterval = 4.5f;   // пауза между дальними приёмами босса, сек

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

// Текущий интервал между волнами (с учётом роста сложности и профиля, Шаг 19).
// Множитель профиля spawnIntervalMul применяется ПОСЛЕ потолка минимума:
// >1 = реже волны = легче, <1 = чаще = сложнее. Нижняя страховка — 0.05 сек.
inline float CurrentSpawnInterval(float elapsed) {
    float v = kSpawnBaseInterval - elapsed * kSpawnRampPerSec;
    if (v < kSpawnMinInterval) v = kSpawnMinInterval;   // потолок сложности по времени
    v *= Diff().spawnIntervalMul;                       // Шаг 19: масштаб паузы профилем сложности
    if (v < 0.05f) v = 0.05f;                           // страховка от слишком частого спавна
    return v;
}

// Текущий размер волны (растёт со временем до потолка, затем масштаб профилем, Шаг 19).
// Множитель профиля waveCountMul: <1 = меньше врагов за волну = легче. Минимум — 1 враг.
inline int CurrentWaveCount(float elapsed) {
    int c = kWaveBaseCount + (int)(elapsed / kWaveCountRampDiv);
    if (c > kWaveMaxCount) c = kWaveMaxCount;            // потолок размера волны по времени
    c = (int)(c * Diff().waveCountMul);                 // Шаг 19: масштаб размера волны профилем сложности
    if (c < 1) c = 1;                                   // минимум один враг в волне
    return c;
}

// ---------------------------------------------------------------------------
//  МОБИЛЬНОСТЬ ВРАГОВ (Фаза 4, Шаг 17-21)
//  Времена разблокировки и кулдауны приёмов уже заданы в kAbilityRules выше
//  (ABIL_JUMP_SLAM, ABIL_TELEPORT, ABIL_BLINK, ABIL_DASH, ABIL_FLANK).
//  Здесь — геометрия и тайминги самих перемещений. Каждый параметр — с комментарием.
// ---------------------------------------------------------------------------

// Доля обычных врагов (не боссов), получающих приём мобильности при спавне (0..99).
// Конкретный приём выбирается среди РАЗБЛОКИРОВАННЫХ по весам из kAbilityRules.
inline constexpr int kMobilityChance = 30;

// -- Прыжок-наскок (Шаг 17): враг прыгает к игроку, приземление = slam-зона --
inline constexpr float kJumpTriggerRange = 360.0f; // с какой дистанции враг решается прыгнуть
inline constexpr float kJumpMinRange     = 120.0f; // ближе этого прыгать незачем
inline constexpr float kJumpDuration     = 0.55f;  // длительность полёта, сек
inline constexpr float kJumpMaxDist      = 340.0f; // максимальная дальность прыжка, пикс
inline constexpr float kJumpArcHeight    = 90.0f;  // высота визуальной дуги прыжка, пикс
inline constexpr float kSlamRadius       = 95.0f;  // радиус зоны удара при приземлении
inline constexpr int   kSlamDamage       = 16;     // урон slam-зоны
inline constexpr float kSlamFill         = 0.45f;  // время заполнения slam-зоны (короткое), сек

// -- Телепорт (Шаг 18): враг исчезает и появляется рядом с игроком --
inline constexpr float kTeleportTriggerRange = 520.0f; // дальше этого враг телепортируется ближе
inline constexpr float kTeleportRange        = 150.0f; // на каком расстоянии от игрока появляется

// -- Блинк-преследователь (Шаг 19): короткие телепорты-рывки серией --
inline constexpr float kBlinkDistance = 110.0f; // длина одного блинка, пикс
inline constexpr int   kBlinkBurst    = 3;      // сколько блинков в серии
inline constexpr float kBlinkStep     = 0.18f;  // пауза между блинками внутри серии, сек
inline constexpr float kBlinkMinRange = 90.0f;  // ближе этого блинкать не нужно

// -- Улучшённый рывок (Шаг 20): сначала линия-предупреждение пути, потом разгон --
inline constexpr float kDashTriggerRange    = 420.0f; // с какой дистанции обычный враг готовит рывок
inline constexpr float kDashTelegraphTime   = 0.5f;   // сколько показывается линия-предупреждение, сек
inline constexpr float kDashSpeed           = 780.0f; // скорость во время рывка, пикс/сек
inline constexpr float kDashDuration        = 0.35f;  // длительность рывка, сек
inline constexpr float kDashLength          = 360.0f; // длина телеграф-линии пути, пикс
inline constexpr float kDashWidth           = 46.0f;  // ширина линии-предупреждения, пикс
inline constexpr int   kDashTelegraphDamage = 0;      // урон линии (0 = только предупреждение; контакт бьёт сам)
inline constexpr float kBossDashCooldown    = 3.0f;   // кулдаун рывка у боссов (Чёрный рыцарь), сек

// -- Окружение/фланг (Шаг 21): держать дистанцию и заходить сбоку --
inline constexpr float kFlankRadius = 160.0f; // дистанция, которую держит фланговый враг, пикс
inline constexpr float kFlankAngle  = 1.1f;   // угловое смещение точки захода, рад (~63°)
inline constexpr float kFlankSwitch = 2.5f;   // как часто фланговый меняет сторону захода, сек

// ---------------------------------------------------------------------------
//  ОСОБЫЕ СПОСОБНОСТИ ВРАГОВ (Фаза 5, Шаг 23-28)
//  Времена разблокировки и кулдауны уже заданы в kAbilityRules выше
//  (ABIL_SHIELD, ABIL_SPLIT, ABIL_SUMMON, ABIL_SLOW_AURA, ABIL_POISON_TRAIL, ABIL_HEALER).
//  Здесь — геометрия, длительности и сила каждого приёма. Каждый параметр — с комментарием.
// ---------------------------------------------------------------------------

// Доля обычных врагов (не боссов), получающих ОСОБУЮ способность при спавне (0..99).
// Способность выбирается среди РАЗБЛОКИРОВАННЫХ по весам из kAbilityRules.
// Не зависит от приёма мобильности — враг может иметь и приём перемещения, и особую способность.
inline constexpr int kSpecialChance = 22;

// -- Щит/неуязвимость (Шаг 23): враг периодически становится неуязвим, надо переждать --
inline constexpr float kShieldDuration = 3.0f;   // сколько держится щит (полная неуязвимость), сек
// Кулдаун между щитами берётся из kAbilityRules[ABIL_SHIELD].minInterval.

// -- Разделение при смерти (Шаг 24): крупный враг делится на мелких (slime split) --
inline constexpr int   kSplitCount      = 3;     // на сколько осколков делится при гибели
inline constexpr float kSplitSpread     = 46.0f; // на каком радиусе появляются осколки, пикс
inline constexpr float kSplitHealthFrac = 0.35f; // доля HP осколка от HP родителя (0..1)
inline constexpr float kSplitSizeFrac   = 0.6f;  // доля размера осколка от размера родителя (0..1)

// -- Улучшённый призыв (Шаг 25): типы миньонов и лимит активных --
inline constexpr int kSummonMaxActive  = 60;     // если активных врагов больше — призыв пропускается
inline constexpr int kSummonTankTime   = 80;     // с этой секунды среди призванных могут быть танки
inline constexpr int kSummonTankChance = 18;     // шанс танка среди призванных (когда доступен), %
inline constexpr int kSummonFastChance = 55;     // шанс быстрого миньона (иначе обычный grunt), %

// -- Аура замедления (Шаг 26): зона вокруг врага замедляет игрока --
inline constexpr float kSlowAuraRadius = 150.0f; // радиус ауры, пикс
inline constexpr float kSlowAuraFactor = 0.55f;  // множитель скорости игрока в ауре (0..1)

// -- Ядовитый след/лужи (Шаг 27): враг оставляет за собой зоны урона со временем --
inline constexpr float kPoisonDropInterval = 0.6f;  // как часто враг роняет лужу, сек
inline constexpr float kPoisonRadius       = 42.0f; // радиус лужи, пикс
inline constexpr int   kPoisonDamage       = 5;     // урон за тик нахождения в луже
inline constexpr float kPoisonTick         = 0.5f;  // период тиков урона лужи, сек
inline constexpr float kPoisonLifetime     = 4.0f;  // сколько живёт лужа, сек
inline constexpr int   kHazardPoolSize     = 96;    // размер пула опасных зон (луж)

// -- Лекарь/баффер (Шаг 28): враг лечит союзников в радиусе --
inline constexpr float kHealerRadius = 220.0f; // радиус действия лекаря, пикс
inline constexpr int   kHealAmount   = 12;     // сколько HP восстанавливает за импульс
inline constexpr float kHealInterval = 2.5f;   // период лечебных импульсов, сек

// ---------------------------------------------------------------------------
//  ГЛУБИНА БОЯ И РЕАКЦИИ ИГРОКА (Фаза 6, Шаг 29-34)
//  Реакции игрока на угрозы и обратная связь боя. Каждый параметр — с комментарием.
// ---------------------------------------------------------------------------

// -- Уворот/рывок игрока (Шаг 29): быстрый рывок с неуязвимостью (i-frames) --
// Пробел или Shift (или нижняя лицевая кнопка геймпада) бросают игрока в сторону
// движения; на время рывка он неуязвим, затем приём уходит в кулдаун.
inline constexpr float kDodgeSpeed    = 760.0f; // скорость во время рывка, пикс/сек
inline constexpr float kDodgeDuration = 0.18f;  // длительность самого рывка, сек
inline constexpr float kDodgeIFrames  = 0.28f;  // длительность неуязвимости от начала рывка, сек (чуть длиннее рывка)
inline constexpr float kDodgeCooldown = 0.9f;   // пауза между рывками, сек

// -- Отбрасывание врагов (Шаг 30): попадания оружия игрока толкают врага --
// Импульс направлен от игрока к врагу и плавно затухает. Танки тяжелее,
// боссы не сдвигаются вовсе (см. Enemy::ApplyKnockback).
inline constexpr float kKnockbackForce      = 280.0f; // импульс от обычного попадания, пикс/сек
inline constexpr float kKnockbackDecay      = 9.0f;   // скорость затухания импульса (больше = резче гаснет)
inline constexpr float kKnockbackTankResist = 0.35f;  // множитель импульса для танков (тяжелее сдвинуть, 0..1)

// -- Статусы-эффекты на врагах (Шаг 31): горение, заморозка, отравление --
// Оружие игрока при попадании с шансом накладывает ОДИН статус. Горение и яд
// наносят урон по тикам (через DamageEnemy, поэтому награда за смерть работает);
// заморозка временно замедляет врага. Все шансы — в процентах 0..99.
inline constexpr int   kBurnChance   = 18;    // шанс поджечь врага при попадании, %
inline constexpr float kBurnDuration = 3.0f;  // сколько времени враг горит, сек
inline constexpr float kBurnTick     = 0.5f;  // период тиков урона горения, сек
inline constexpr int   kBurnDamage   = 4;     // урон за один тик горения

inline constexpr int   kFreezeChance   = 12;    // шанс заморозить врага при попадании, %
inline constexpr float kFreezeDuration = 1.6f;  // длительность заморозки, сек
inline constexpr float kFreezeFactor   = 0.35f; // множитель скорости замороженного врага (0..1)

inline constexpr int   kPoisonStatusChance   = 18;   // шанс отравить врага при попадании, %
inline constexpr float kPoisonStatusDuration = 4.0f; // длительность яда, сек (обновляется новым стаком)
inline constexpr float kPoisonStatusTick     = 0.6f; // период тиков урона яда, сек
inline constexpr int   kPoisonStatusDamage   = 3;    // урон за тик за КАЖДЫЙ стак яда
inline constexpr int   kPoisonMaxStacks      = 5;    // максимум одновременных стаков яда

// -- Реакции статусов (Шаг 34): взаимодействия стихий добавляют глубину тактике --
// «Перелом»: прямое попадание оружия игрока по ЗАМОРОЖЕННОМУ врагу наносит
// бонусный урон и мгновенно снимает заморозку (лёд раскалывается). «Пар»: если
// оружие выпадает на заморозку ГОРЯЩЕГО врага, обе стихии гасятся, а враг
// получает мгновенный всплеск урона вместо самой заморозки. Реакции работают
// только от попаданий оружия игрока, не от тиков самих статусов (см. combat.cpp).
inline constexpr float kShatterDamageMul = 1.6f; // множитель урона удара по замороженному врагу (Перелом)
inline constexpr int   kSteamBurstDamage = 12;   // мгновенный урон реакции «Пар» (заморозка по горящему)

// -- Элитные враги (Шаг 32): случайные модификаторы усиливают обычных врагов --
// С шансом kEliteChance после kEliteUnlockTime обычный (не босс) враг при спавне
// получает ОДИН модификатор: быстрый, бронированный, взрывной или гигант. Элиты
// дают больше опыта (kEliteXpMul). Модификатор не зависит от приёма мобильности
// и особой способности — враг может совмещать их.
inline constexpr int   kEliteChance     = 14;    // шанс сделать врага элитой, %
inline constexpr float kEliteUnlockTime = 35.0f; // с какой секунды забега возможны элиты
inline constexpr float kEliteXpMul      = 2.5f;  // множитель опыта за убийство элиты

// Быстрый: заметно выше скорость, запас HP не трогаем.
inline constexpr float kEliteSwiftSpeedMul = 1.7f;

// Бронированный: много HP, чуть медленнее обычного.
inline constexpr float kEliteArmoredHealthMul = 3.0f;
inline constexpr float kEliteArmoredSpeedMul  = 0.85f;

// Взрывной: повышенный HP и взрыв по площади при гибели (через телеграф-зону).
inline constexpr float kEliteExplosiveHealthMul = 1.5f;
inline constexpr float kEliteExplosiveRadius    = 110.0f; // радиус взрыва, пикс
inline constexpr int   kEliteExplosiveDamage    = 22;     // урон взрыва
inline constexpr float kEliteExplosiveFill      = 0.35f;  // время заполнения зоны взрыва, сек

// Гигант: крупнее, много HP, больно бьёт, чуть медленнее.
inline constexpr float kEliteGiantSizeMul   = 1.6f;
inline constexpr float kEliteGiantHealthMul = 2.5f;
inline constexpr float kEliteGiantDamageMul = 1.8f;
inline constexpr float kEliteGiantSpeedMul  = 0.9f;

// -- Улучшенный ИИ врагов (Шаг 33): уклонение от снарядов и групповое поведение --
// Часть обычных врагов (после kProjDodgeUnlockTime) умеет «чувствовать» летящий
// в них снаряд игрока и делать короткий шаг в сторону от его линии полёта.
// Групповое расталкивание (separation) не даёт врагам слипаться в одну точку —
// они держат небольшую дистанцию друг от друга, окружая игрока естественнее.
// Обе механики работают как мягкие сдвиги позиции с проверкой стен (см. Spawner).
inline constexpr int   kProjDodgeChance      = 25;    // доля врагов, умеющих уклоняться, %
inline constexpr float kProjDodgeUnlockTime  = 45.0f; // с какой секунды забега появляются «уклонисты»
inline constexpr float kProjDodgeSenseRadius = 95.0f; // на каком расстоянии враг замечает снаряд, пикс
inline constexpr float kProjDodgeThreatDot   = 0.55f; // насколько «в лоб» летит снаряд, чтобы реагировать (косинус)
inline constexpr float kProjDodgeSpeed       = 240.0f;// скорость шага уклонения, пикс/сек
inline constexpr float kProjDodgeCooldown    = 0.7f;  // пауза между реакциями уклонения, сек

inline constexpr float kSeparationRadius       = 28.0f; // ближе этого враги расталкиваются, пикс
inline constexpr float kSeparationForce        = 90.0f; // сила расталкивания, пикс/сек
inline constexpr int   kSeparationMaxNeighbors = 6;     // сколько соседей максимум учитываем (для скорости)

// ---------------------------------------------------------------------------
//  ПРОИЗВОДИТЕЛЬНОСТЬ НА БОЛЬШОЙ КАРТЕ (v0.4, Фаза 1, Шаг 5)
//  Большая карта (kMapScale) держит в памяти больше тайлов, но на экран рисуются
//  только видимые (куллинг в TileMap::Draw с учётом kCameraZoom). Здесь — параметры
//  этого куллинга и целевой FPS, чтобы крутить производительность одним местом,
//  не трогая логику отрисовки.
//  kTileCullMargin — запас тайлов вокруг видимой области, чтобы при движении камеры
//  по краям кадра не мелькала пустота (раньше был зашит числом 1 в tilemap.cpp).
// ---------------------------------------------------------------------------
inline constexpr int kTargetFps      = 60; // целевой FPS забега (SetTargetFPS в main.cpp)
inline constexpr int kTileCullMargin = 2;  // запас тайлов по краям видимой области при отрисовке карты

// ---------------------------------------------------------------------------
//  ОТКРЫТОСТЬ КАРТЫ (v0.4, Фаза 2, Шаг 6)
//  Главная цель Фазы 2 — простор как в Vampire Survivors: почти нет стен,
//  движение нигде не зажато узкими коридорами. Эти параметры управляют тем,
//  насколько «пусто» генерируется поле (применяются в tilemap, Шаг 7-9).
//  Меняй здесь долю стен и ширину проходов — открытость крутится одним местом.
//  Значения по умолчанию намеренно низкие: поле должно быть в основном открытым.
// ---------------------------------------------------------------------------
inline constexpr float kWallDensity         = 0.05f; // целевая доля поля под стенами/блоками (0 = совсем пусто, 1 = всё стены)
inline constexpr int   kMinCorridorWidth    = 6;     // мин. ширина свободного прохода между препятствиями, тайлов (широкие проходы, без щелей)
inline constexpr int   kBorderWallThickness = 2;     // толщина внешней стены-рамки по краю карты, тайлов (держит игрока в пределах поля)
inline constexpr int   kObstacleClusterMax  = 3;     // макс. размер одного скопления стен, тайлов (чтобы блоки не срастались в сплошные стены)
inline constexpr int   kObstacleClusterMin  = 1;     // мин. размер скопления стен, тайлов
inline constexpr float kDecorDensity        = 0.04f; // частота разрозненного декора (камни/колонны/обломки), доля поля — НЕ блокирует движение массово
inline constexpr int   kOpenZoneRadius      = 10;    // радиус гарантированно пустой зоны вокруг точки старта игрока, тайлов

// ---------------------------------------------------------------------------
//  НАПОЛНЕНИЕ БОЛЬШОГО ПОЛЯ (v0.4, Фаза 3, Шаг 11-14)
//  Большое открытое поле (Фазы 1-2) не должно казаться пустым. Здесь — параметры
//  фона/травы (Шаг 11), редких ориентиров-зон интереса (Шаг 12), распределения
//  спавна врагов по полю (Шаг 13) и раскидывания опыта/лута/сундуков (Шаг 14).
//  Плотность наполнения крутится отсюда — одним местом, без правок логики.
// ---------------------------------------------------------------------------

// -- Шаг 11: фон и трава (бесшовный вид открытого поля) --
// Пол рисуется как сплошной «луг» с мягкой вариацией оттенка между тайлами и
// редкими статичными травинками. Это чистый визуал, без коллизий.
inline constexpr int kGroundTintVariation = 10; // сила вариации оттенка земли между тайлами (0 = ровный фон, больше = пестрее)
inline constexpr int kGroundDetailChance  = 22; // шанс мелкой травяной детали на тайле пола, % (оживляет простор)

// -- Шаг 12: редкие зоны интереса (ориентиры) --
// Поляны/руины/алтари как визуальные ориентиры на поле. Частота — базовое число
// плюс по одному ориентиру на каждые kPoiAreaPerTiles тайлов площади.
inline constexpr int   kPoiBaseCount    = 6;    // базовое число ориентиров на карте
inline constexpr int   kPoiAreaPerTiles = 5000; // +1 ориентир на столько тайлов площади (больше число = реже)
inline constexpr float kPoiMarkerRadius = 2.4f; // радиус площадки ориентира, тайлов (визуал)

// -- Шаг 13: распределение спавна врагов по полю --
// База — кольцо вокруг игрока (kWaveSpawnRadius, Шаг 4). Здесь добавляем разброс,
// чтобы враги не вставали идеальным кольцом, и шанс появления у дальнего ориентира.
inline constexpr float kWaveRingJitter   = 220.0f; // разброс радиуса кольца спавна, пикс (+/-)
inline constexpr float kWaveAngleJitter  = 0.35f;  // угловой разброс позиции в кольце, рад
inline constexpr int   kFarSpawnChance   = 25;     // доля врагов волны, спавнящихся у дальнего ориентира вместо кольца, %
inline constexpr float kFarSpawnMinDist  = 600.0f; // ближе этого к игроку ориентир для «дальнего спавна» не используем, пикс

// -- Шаг 14: опыт/лут/сундуки по полю --
// Раскидываются при старте забега на свободных тайлах, чтобы было выгодно двигаться.
inline constexpr int kFieldOrbCount   = 120; // сколько сфер опыта раскидать по полю при старте забега
inline constexpr int kFieldLootCount  = 14;  // сколько предметов лута (хилки/усиления) раскидать по полю
inline constexpr int kFieldChestCount = 8;   // сколько сундуков-наград разместить (приоритетно у ориентиров)
inline constexpr int kChestHealAmount = 40;  // сколько HP даёт сундук
inline constexpr int kChestPowerBonus = 8;   // прибавка к урону оружия от сундука

// ---------------------------------------------------------------------------
//  ЧИТ-ПАНЕЛЬ РАЗРАБОТЧИКА (v0.4, Фаза 5, Шаг 22-31)
//  Оверлей по F9 для отладки баланса: бессмертие, множители урона/опыта/скорости,
//  мгновенный левел-ап, управление спавном и т.д. Это инструмент РАЗРАБОТЧИКА —
//  в релизной сборке всю панель можно отключить одним флагом kDevPanelAvailable.
//  Шаг 22 — КАРКАС: мастер-флаг + рантайм-состояние видимости + геометрия/вид
//  оверлея. Сами читы и их переключатели добавляются в шагах 23-31, каждый со
//  своей «ручкой» здесь же.
// ---------------------------------------------------------------------------

// Мастер-выключатель чит-панели (для релиза). true = F9 открывает панель;
// false = панель и все читы полностью недоступны (как будто их нет).
inline constexpr bool kDevPanelAvailable = true;

// Видна ли панель сейчас — РАНТАЙМ-состояние, переключается по F9 (см. ToggleDevPanel).
// Изменяемая inline-переменная (одна на все единицы трансляции, C++17). Вручную не трогать.
inline bool gDevPanelOpen = false;

// -- Геометрия и вид оверлея панели (Шаг 22). Всё в пикселях экрана. --
inline constexpr int kDevPanelX         = 40;  // левый край панели, пикс
inline constexpr int kDevPanelY         = 90;  // верхний край панели, пикс
inline constexpr int kDevPanelWidth     = 380; // ширина панели, пикс
inline constexpr int kDevPanelPad       = 16;  // внутренний отступ панели, пикс
inline constexpr int kDevPanelRowHeight = 26;  // высота одной строки меню, пикс
inline constexpr int kDevPanelTitleSize = 24;  // кегль заголовка панели, пикс
inline constexpr int kDevPanelTextSize  = 18;  // кегль строк меню, пикс

// Переключить видимость чит-панели (Шаг 22): вызывается по F9 из main.cpp.
// При выключенном мастер-флаге не делает ничего.
inline void ToggleDevPanel() {
    if (kDevPanelAvailable) gDevPanelOpen = !gDevPanelOpen;
}

// Открыта ли панель сейчас (с учётом мастер-выключателя) — для отрисовки и ввода.
inline bool IsDevPanelOpen() { return kDevPanelAvailable && gDevPanelOpen; }

// -- Чит: бессмертие / god mode (Шаг 23). Пока чит включён, HP игрока не падает
//    (восстанавливается каждый кадр в main.cpp). Переключается цифрой 1 при
//    открытой панели. Рантайм-флаг, по умолчанию выключен. --
inline bool gCheatGodMode = false;

// Переключить бессмертие (Шаг 23): только при доступной чит-панели.
inline void ToggleGodMode() {
    if (kDevPanelAvailable) gCheatGodMode = !gCheatGodMode;
}

// Включено ли бессмертие сейчас (с учётом мастер-выключателя) — для боевой логики.
inline bool IsGodMode() { return kDevPanelAvailable && gCheatGodMode; }

} // namespace Tuning
