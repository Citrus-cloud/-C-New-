#include "enemy.h"
#include "pathfinding.h"
#include "telegraph.h"   // враг «заказывает» зоны: slam при приземлении, линия пути рывка
#include "effects.h"     // визуальные эффекты телепорта/блинка/приземления
#include "tuning.h"      // вся геометрия и тайминги приёмов мобильности
#include <cmath>

Enemy::Enemy()
    : position({ 0.0f, 0.0f }), speed(120.0f), health(30), maxHealth(30), active(false),
      type(ENEMY_GRUNT), size(16.0f), color(PURPLE), damage(5), xpValue(1),
      facingLeft(false), dying(false), deathTimer(0.0f),
      pathIndex(0), repathTimer(0.0f),
      dashing(false), dashTimer(0.0f), dashDir({ 0.0f, 0.0f }),
      dashTelegraphing(false), dashTelegraphTimer(0.0f),
      bossId(-1), canDash(false), canSummon(false), summonTimer(0.0f),
      mobility(MOB_NONE), mobilityCd(0.0f), mobilityCdMax(5.0f),
      jumping(false), jumpTimer(0.0f), jumpDuration(Tuning::kJumpDuration),
      jumpStart({ 0.0f, 0.0f }), jumpEnd({ 0.0f, 0.0f }),
      blinkBurstLeft(0), blinkStepTimer(0.0f),
      flankSide(1), flankSwitchTimer(0.0f), drawYOffset(0.0f),
      special(SPEC_NONE), shielded(false), shieldTimer(0.0f), shieldCd(0.0f),
      splitsOnDeath(false), wantSplit(false), poisonDropTimer(0.0f), healTimer(0.0f),
      knockbackVel({ 0.0f, 0.0f }),
      burnTimer(0.0f), burnTick(0.0f), poisonTimer(0.0f), poisonTick(0.0f),
      poisonStacks(0), freezeTimer(0.0f),
      elite(ELITE_NONE), wantExplode(false),
      projectileDodger(false), dodgeReactCd(0.0f)
{
}

void Enemy::Spawn(Vector2 pos, EnemyType t)
{
    position = pos;
    active = true;
    dying = false;
    deathTimer = 0.0f;
    facingLeft = false;
    type = t;
    path.clear();
    pathIndex = 0;
    repathTimer = 0.0f;

    dashing = false;
    dashTimer = 0.0f;
    dashDir = { 0.0f, 0.0f };
    dashTelegraphing = false;
    dashTelegraphTimer = 0.0f;

    // По умолчанию обычный враг (не босс).
    bossId = -1;
    canDash = false;
    canSummon = false;
    summonTimer = 0.0f;

    // Сброс мобильности (Фаза 4) — конкретный приём назначает спавнер отдельно.
    mobility = MOB_NONE;
    mobilityCd = 0.0f;
    mobilityCdMax = 5.0f;
    jumping = false;
    jumpTimer = 0.0f;
    jumpDuration = Tuning::kJumpDuration;
    jumpStart = pos;
    jumpEnd = pos;
    blinkBurstLeft = 0;
    blinkStepTimer = 0.0f;
    flankSide = (GetRandomValue(0, 1) == 0) ? -1 : 1;
    flankSwitchTimer = Tuning::kFlankSwitch;
    drawYOffset = 0.0f;

    // Сброс особых способностей (Фаза 5) — конкретную назначает спавнер.
    special = SPEC_NONE;
    shielded = false;
    shieldTimer = 0.0f;
    shieldCd = 0.0f;
    splitsOnDeath = false;
    wantSplit = false;
    poisonDropTimer = 0.0f;
    healTimer = 0.0f;

    // Сброс отбрасывания (Фаза 6, Шаг 30).
    knockbackVel = { 0.0f, 0.0f };

    // Сброс статусов-эффектов (Фаза 6, Шаг 31).
    burnTimer = 0.0f;
    burnTick = 0.0f;
    poisonTimer = 0.0f;
    poisonTick = 0.0f;
    poisonStacks = 0;
    freezeTimer = 0.0f;

    // Сброс элитного модификатора (Фаза 6, Шаг 32) — конкретный назначает спавнер.
    elite = ELITE_NONE;
    wantExplode = false;

    // Сброс улучшенного ИИ (Фаза 6, Шаг 33) — флаг уклонения назначает спавнер.
    projectileDodger = false;
    dodgeReactCd = 0.0f;

    switch (t)
    {
        case ENEMY_FAST:
            health = 15; speed = 220.0f; size = 12.0f; color = ORANGE; damage = 4; xpValue = 1;
            break;
        case ENEMY_TANK:
            health = 90; speed = 70.0f; size = 24.0f; color = DARKGRAY; damage = 10; xpValue = 3;
            break;
        case ENEMY_BOSS:
            health = 700; speed = 95.0f; size = 38.0f; color = MAROON; damage = 20; xpValue = 25;
            break;
        case ENEMY_GRUNT:
        default:
            health = 30; speed = 120.0f; size = 16.0f; color = PURPLE; damage = 5; xpValue = 1;
            break;
    }

    maxHealth = health;
}

// Применяет характеристики и механики конкретного босса из таблицы (Шаг 20-22).
void Enemy::ApplyBoss(const BossDef& def)
{
    health = def.health;
    maxHealth = def.health;
    speed = def.speed;
    size = def.size;
    damage = def.damage;
    xpValue = def.xpValue;
    color = def.color;
    canDash = def.canDash;
    canSummon = def.canSummon;
    summonTimer = def.summonInterval;
    // Рывок босса использует тот же механизм, что и MOB_DASH (Фаза 4), но со своим кулдауном.
    if (canDash)
    {
        mobilityCdMax = Tuning::kBossDashCooldown;
        mobilityCd = mobilityCdMax * 0.5f;
    }
}

// Назначает особую способность врага (Шаг 23-28).
// Таймеры десинхронизируются, чтобы враги не действовали «в унисон».
void Enemy::ApplySpecial(int kind)
{
    special = kind;
    splitsOnDeath = (kind == SPEC_SPLIT);
    shielded = false;
    shieldTimer = 0.0f;
    shieldCd = (float)GetRandomValue(0, 100) / 100.0f *
               Tuning::GetRule(Tuning::ABIL_SHIELD).minInterval;
    poisonDropTimer = (float)GetRandomValue(0, 100) / 100.0f * Tuning::kPoisonDropInterval;
    healTimer = (float)GetRandomValue(0, 100) / 100.0f * Tuning::kHealInterval;
}

// Придаёт врагу импульс отбрасывания от попадания (Шаг 30).
// Боссы не сдвигаются, танки сдвигаются хуже (kKnockbackTankResist).
void Enemy::ApplyKnockback(Vector2 dir, float force)
{
    if (type == ENEMY_BOSS) return;
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.0001f) return;
    float resist = (type == ENEMY_TANK) ? Tuning::kKnockbackTankResist : 1.0f;
    knockbackVel.x += dir.x / len * force * resist;
    knockbackVel.y += dir.y / len * force * resist;
}

// Накладывает статус-эффект на врага (Шаг 31). Длительности/сила — из tuning.h.
// Горение/яд тикают в Weapon::Update (там есть доступ к награде), заморозка — в Update.
void Enemy::ApplyStatus(int kind)
{
    switch (kind)
    {
        case STATUS_BURN:
            if (burnTimer <= 0.0f) burnTick = Tuning::kBurnTick;
            burnTimer = Tuning::kBurnDuration;
            break;
        case STATUS_FREEZE:
            freezeTimer = Tuning::kFreezeDuration;
            break;
        case STATUS_POISON:
            if (poisonTimer <= 0.0f) poisonTick = Tuning::kPoisonStatusTick;
            poisonTimer = Tuning::kPoisonStatusDuration;
            if (poisonStacks < Tuning::kPoisonMaxStacks) poisonStacks++;
            break;
        default:
            break;
    }
}

// Назначает элитный модификатор и применяет усиление статов (Шаг 32).
// Вызывается спавнером ПОСЛЕ Spawn(), поэтому базовые статы уже выставлены —
// здесь мы их домножаем. Элиты дают больше опыта (kEliteXpMul).
void Enemy::ApplyElite(int kind)
{
    elite = kind;
    switch (kind)
    {
        case ELITE_SWIFT:
            speed *= Tuning::kEliteSwiftSpeedMul;
            break;
        case ELITE_ARMORED:
            maxHealth = (int)(maxHealth * Tuning::kEliteArmoredHealthMul);
            health = maxHealth;
            speed *= Tuning::kEliteArmoredSpeedMul;
            break;
        case ELITE_EXPLOSIVE:
            maxHealth = (int)(maxHealth * Tuning::kEliteExplosiveHealthMul);
            health = maxHealth;
            break;
        case ELITE_GIANT:
            size *= Tuning::kEliteGiantSizeMul;
            maxHealth = (int)(maxHealth * Tuning::kEliteGiantHealthMul);
            health = maxHealth;
            damage = (int)(damage * Tuning::kEliteGiantDamageMul);
            speed *= Tuning::kEliteGiantSpeedMul;
            break;
        default:
            elite = ELITE_NONE;
            return;
    }
    xpValue = (int)(xpValue * Tuning::kEliteXpMul);
    if (xpValue < 1) xpValue = 1;
}

Rectangle Enemy::GetRect() const
{
    return { position.x - size, position.y - size, size * 2.0f, size * 2.0f };
}

void Enemy::Kill()
{
    health = 0;
    if (dying) return;
    if (deathAnim.Valid())
    {
        dying = true;
        deathTimer = 0.6f;
        deathAnim.Reset();
    }
    else
    {
        active = false;
    }
}

static void MoveToward(Vector2& pos, Vector2 target, float dist, float halfSize, const TileMap& map)
{
    float dx = target.x - pos.x, dy = target.y - pos.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;
    dx /= len; dy /= len;
    float mx = dx * dist, my = dy * dist;

    pos.x += mx;
    if (map.CheckCollision({ pos.x - halfSize, pos.y - halfSize, halfSize * 2.0f, halfSize * 2.0f })) pos.x -= mx;
    pos.y += my;
    if (map.CheckCollision({ pos.x - halfSize, pos.y - halfSize, halfSize * 2.0f, halfSize * 2.0f })) pos.y -= my;
}

// === Фаза 4: приёмы мобильности врага ======================================
// Возвращает true, если приём занял этот кадр (обычное движение пропускаем).
bool Enemy::UpdateMobility(float dt, Vector2 playerPos, const TileMap& map,
                           TelegraphSystem* telegraphs, Effects* effects)
{
    // 1) Прыжок в полёте (Шаг 17): летим по дуге, на приземлении создаём slam-зону.
    if (jumping)
    {
        jumpTimer += dt;
        float t = (jumpDuration > 0.0f) ? jumpTimer / jumpDuration : 1.0f;
        if (t > 1.0f) t = 1.0f;
        position.x = jumpStart.x + (jumpEnd.x - jumpStart.x) * t;
        position.y = jumpStart.y + (jumpEnd.y - jumpStart.y) * t;
        drawYOffset = -sinf(t * PI) * Tuning::kJumpArcHeight;   // визуальная дуга прыжка
        if (t >= 1.0f)
        {
            jumping = false;
            drawYOffset = 0.0f;
            position = jumpEnd;
            // Приземление = удар по площади через систему телеграфов (она и наносит урон).
            if (telegraphs)
                telegraphs->SpawnCircle(position, Tuning::kSlamRadius, Tuning::kSlamDamage,
                                        Tuning::kSlamFill, Color{ 255, 140, 40, 255 });
            if (effects) { effects->SpawnDust(position, 14); effects->Shake(4.0f, 0.15f); }
        }
        return true;
    }

    // 2) Подготовка рывка (Шаг 20): стоим, показывая линию-предупреждение пути.
    if (dashTelegraphing)
    {
        dashTelegraphTimer -= dt;
        if (dashTelegraphTimer <= 0.0f)
        {
            dashTelegraphing = false;
            dashing = true;
            dashTimer = Tuning::kDashDuration;
        }
        return true;
    }

    // 3) Активный рывок (Шаг 20).
    if (dashing)
    {
        float ds = Tuning::kDashSpeed * dt;
        float mx = dashDir.x * ds, my = dashDir.y * ds;
        position.x += mx;
        if (map.CheckCollision(GetRect())) position.x -= mx;
        position.y += my;
        if (map.CheckCollision(GetRect())) position.y -= my;
        dashTimer -= dt;
        if (dashTimer <= 0.0f) dashing = false;
        return true;
    }

    // 4) Серия блинков (Шаг 19): короткие телепорты к игроку с паузой между ними.
    if (blinkBurstLeft > 0)
    {
        blinkStepTimer -= dt;
        if (blinkStepTimer <= 0.0f)
        {
            float dx = playerPos.x - position.x, dy = playerPos.y - position.y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0.01f) { dx /= len; dy /= len; }
            Vector2 np = { position.x + dx * Tuning::kBlinkDistance, position.y + dy * Tuning::kBlinkDistance };
            if (effects) effects->SpawnSparks(position, Color{ 120, 180, 255, 255 }, 8);
            // Не блинкуем в стену.
            Rectangle r = { np.x - size, np.y - size, size * 2.0f, size * 2.0f };
            if (!map.CheckCollision(r)) position = np;
            if (effects) effects->SpawnSparks(position, Color{ 120, 180, 255, 255 }, 8);
            blinkBurstLeft--;
            blinkStepTimer = Tuning::kBlinkStep;
        }
        return true;
    }

    // --- Кулдаун и запуск нового приёма ---
    if (mobilityCd > 0.0f) mobilityCd -= dt;

    int kind = mobility;
    if (kind == MOB_NONE && canDash) kind = MOB_DASH;   // боссы с рывком используют тот же приём

    // Фланг — пассивное движение (обрабатывается в обычном перемещении), кадр не «занимает».
    if (kind == MOB_NONE || kind == MOB_FLANK) return false;
    if (mobilityCd > 0.0f) return false;

    float dx = playerPos.x - position.x, dy = playerPos.y - position.y;
    float dist = sqrtf(dx * dx + dy * dy);
    float nx = (dist > 0.01f) ? dx / dist : 1.0f;
    float ny = (dist > 0.01f) ? dy / dist : 0.0f;

    switch (kind)
    {
        case MOB_JUMP_SLAM:
            if (dist <= Tuning::kJumpTriggerRange && dist >= Tuning::kJumpMinRange)
            {
                jumping = true;
                jumpTimer = 0.0f;
                jumpDuration = Tuning::kJumpDuration;
                jumpStart = position;
                float jd = (dist < Tuning::kJumpMaxDist) ? dist : Tuning::kJumpMaxDist;
                jumpEnd = { position.x + nx * jd, position.y + ny * jd };
                mobilityCd = mobilityCdMax;
                return true;
            }
            break;

        case MOB_TELEPORT:
            if (dist >= Tuning::kTeleportTriggerRange)
            {
                if (effects) effects->SpawnSparks(position, Color{ 200, 120, 255, 255 }, 14);
                float a = (float)GetRandomValue(0, 359) * DEG2RAD;
                Vector2 np = { playerPos.x + cosf(a) * Tuning::kTeleportRange,
                               playerPos.y + sinf(a) * Tuning::kTeleportRange };
                np = map.FindFreeSpot(np, size);
                position = np;
                if (effects) effects->SpawnSparks(position, Color{ 200, 120, 255, 255 }, 14);
                mobilityCd = mobilityCdMax;
                return true;
            }
            break;

        case MOB_BLINK:
            if (dist >= Tuning::kBlinkMinRange)
            {
                blinkBurstLeft = Tuning::kBlinkBurst;
                blinkStepTimer = 0.0f;
                mobilityCd = mobilityCdMax;
                return true;
            }
            break;

        case MOB_DASH:
            // Боссы рвутся с любой дистанции; обычные враги — только вблизи (kDashTriggerRange).
            if (canDash || dist <= Tuning::kDashTriggerRange)
            {
                dashDir = { nx, ny };
                dashTelegraphing = true;
                dashTelegraphTimer = Tuning::kDashTelegraphTime;
                if (telegraphs)
                {
                    float ang = atan2f(ny, nx);
                    telegraphs->SpawnLine(position, ang, Tuning::kDashLength, Tuning::kDashWidth,
                                          Tuning::kDashTelegraphDamage, Tuning::kDashTelegraphTime,
                                          Color{ 255, 90, 90, 255 });
                }
                mobilityCd = mobilityCdMax;
                return true;
            }
            break;

        default:
            break;
    }

    return false;
}

void Enemy::Update(float deltaTime, Vector2 playerPos, const TileMap& map,
                   TelegraphSystem* telegraphs, Effects* effects)
{
    if (!active) return;

    if (dying)
    {
        deathAnim.Update(deltaTime);
        deathTimer -= deltaTime;
        if (deathTimer <= 0.0f) active = false;
        return;
    }

    facingLeft = (playerPos.x < position.x);

    walkAnim.Update(deltaTime);

    // Щит (Шаг 23): периодическая неуязвимость. Сам урон блокируется в DamageEnemy.
    if (special == SPEC_SHIELD)
    {
        if (shielded)
        {
            shieldTimer -= deltaTime;
            if (shieldTimer <= 0.0f)
            {
                shielded = false;
                shieldCd = Tuning::GetRule(Tuning::ABIL_SHIELD).minInterval;
            }
        }
        else
        {
            shieldCd -= deltaTime;
            if (shieldCd <= 0.0f)
            {
                shielded = true;
                shieldTimer = Tuning::kShieldDuration;
            }
        }
    }

    // Отбрасывание (Шаг 30): смещаем по импульсу и плавно гасим его.
    if (knockbackVel.x != 0.0f || knockbackVel.y != 0.0f)
    {
        float kx = knockbackVel.x * deltaTime;
        float ky = knockbackVel.y * deltaTime;
        position.x += kx;
        if (map.CheckCollision(GetRect())) position.x -= kx;
        position.y += ky;
        if (map.CheckCollision(GetRect())) position.y -= ky;
        float decay = Tuning::kKnockbackDecay * deltaTime;
        if (decay > 1.0f) decay = 1.0f;
        knockbackVel.x -= knockbackVel.x * decay;
        knockbackVel.y -= knockbackVel.y * decay;
        if (fabsf(knockbackVel.x) < 4.0f) knockbackVel.x = 0.0f;
        if (fabsf(knockbackVel.y) < 4.0f) knockbackVel.y = 0.0f;
    }

    // Заморозка (Шаг 31): отсчитываем время статуса; замедление применяется к шагу ниже.
    float freezeFactor = 1.0f;
    if (freezeTimer > 0.0f)
    {
        freezeTimer -= deltaTime;
        freezeFactor = Tuning::kFreezeFactor;
    }

    // Фаза 4: приёмы мобильности. Если приём занял кадр — обычное движение пропускаем.
    if (UpdateMobility(deltaTime, playerPos, map, telegraphs, effects)) return;

    float step = speed * deltaTime * freezeFactor;

    // Фланг (Шаг 21): держим дистанцию и заходим сбоку, а не в лоб.
    if (mobility == MOB_FLANK)
    {
        flankSwitchTimer -= deltaTime;
        if (flankSwitchTimer <= 0.0f) { flankSide = -flankSide; flankSwitchTimer = Tuning::kFlankSwitch; }
        float ang = atan2f(position.y - playerPos.y, position.x - playerPos.x);
        ang += (float)flankSide * Tuning::kFlankAngle;
        Vector2 target = { playerPos.x + cosf(ang) * Tuning::kFlankRadius,
                           playerPos.y + sinf(ang) * Tuning::kFlankRadius };
        MoveToward(position, target, step, size, map);
        return;
    }

    // Обычное преследование (прямая видимость + A*).
    if (HasLineOfSight(map, position, playerPos))
    {
        path.clear();
        MoveToward(position, playerPos, step, size, map);
        return;
    }

    repathTimer -= deltaTime;
    if (repathTimer <= 0.0f || path.empty() || pathIndex >= (int)path.size())
    {
        path = FindPath(map, position, playerPos);
        pathIndex = 0;
        repathTimer = 0.5f;
    }

    if (!path.empty() && pathIndex < (int)path.size())
    {
        Vector2 target = path[pathIndex];
        MoveToward(position, target, step, size, map);
        float dx = target.x - position.x, dy = target.y - position.y;
        if (dx * dx + dy * dy < 100.0f) pathIndex++;
    }
}

void Enemy::DrawHealthBar() const
{
    if (maxHealth <= 0) return;
    if (type != ENEMY_BOSS && health >= maxHealth) return;

    float frac = (float)health / (float)maxHealth;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    bool boss = (type == ENEMY_BOSS);
    float w = boss ? size * 2.4f : size * 2.0f;
    float barH = boss ? 8.0f : 4.0f;
    float x = position.x - w / 2.0f;
    float y = position.y - size - (boss ? 16.0f : 8.0f) + drawYOffset;

    DrawRectangle((int)(x - 1), (int)(y - 1), (int)(w + 2), (int)(barH + 2), Fade(BLACK, 0.6f));
    DrawRectangle((int)x, (int)y, (int)w, (int)barH, Color{ 60, 60, 60, 255 });
    DrawRectangle((int)x, (int)y, (int)(w * frac), (int)barH, boss ? RED : GREEN);
}

void Enemy::Draw() const
{
    if (!active) return;

    Vector2 drawPos = { position.x, position.y + drawYOffset };

    // Особые способности (Фаза 5): «наземные» кольца рисуем ПОД сущностью.
    if (!dying)
    {
        if (special == SPEC_SLOW_AURA)   // Шаг 26: полупрозрачная аура замедления
        {
            DrawCircleV(position, Tuning::kSlowAuraRadius, Color{ 80, 120, 255, 28 });
            DrawCircleLines((int)position.x, (int)position.y, Tuning::kSlowAuraRadius,
                            Color{ 120, 160, 255, 90 });
        }
        if (special == SPEC_HEALER)      // Шаг 28: зелёное кольцо радиуса лечения
            DrawCircleLines((int)position.x, (int)position.y, Tuning::kHealerRadius,
                            Color{ 80, 255, 140, 60 });
    }

    if (dying)
    {
        if (deathAnim.Valid())
        {
            float target = size * 2.6f;
            float h = (float)deathAnim.FrameHeight();
            float sc = (h > 0.0f) ? target / h : 1.0f;
            deathAnim.Draw(drawPos, sc, facingLeft, WHITE);
        }
        return;
    }

    bool charging = dashing || dashTelegraphing;   // рывок и его подготовка — оранжевая подсветка

    // Цветовая подсветка статусов (Шаг 31): заморозка > горение > яд.
    if (walkAnim.Valid())
    {
        float target = size * 2.6f;
        float h = (float)walkAnim.FrameHeight();
        float sc = (h > 0.0f) ? target / h : 1.0f;
        Color tint = WHITE;
        if (charging)                tint = Color{ 255, 180, 120, 255 };
        else if (freezeTimer > 0.0f) tint = Color{ 150, 210, 255, 255 };
        else if (burnTimer > 0.0f)   tint = Color{ 255, 130, 70, 255 };
        else if (poisonTimer > 0.0f) tint = Color{ 150, 255, 120, 255 };
        walkAnim.Draw(drawPos, sc, facingLeft, tint);
    }
    else
    {
        Color c = color;
        if (charging)                c = ORANGE;
        else if (freezeTimer > 0.0f) c = Color{ 120, 190, 255, 255 };
        else if (burnTimer > 0.0f)   c = Color{ 255, 110, 60, 255 };
        else if (poisonTimer > 0.0f) c = Color{ 120, 230, 90, 255 };
        DrawRectangle((int)(drawPos.x - size), (int)(drawPos.y - size),
                      (int)(size * 2.0f), (int)(size * 2.0f), c);
        if (type == ENEMY_BOSS)
            DrawRectangleLines((int)(drawPos.x - size), (int)(drawPos.y - size),
                               (int)(size * 2.0f), (int)(size * 2.0f), YELLOW);
    }

    // Щит (Шаг 23): синие кольца ПОВЕРХ врага, пока активна неуязвимость.
    if (shielded)
    {
        DrawCircleLines((int)drawPos.x, (int)drawPos.y, size + 6.0f, Color{ 90, 180, 255, 255 });
        DrawCircleLines((int)drawPos.x, (int)drawPos.y, size + 10.0f, Color{ 90, 180, 255, 160 });
    }

    // Элитная подсветка (Шаг 32): пульсирующее цветное кольцо вокруг элиты.
    if (elite != ELITE_NONE)
    {
        Color ec = WHITE;
        switch (elite)
        {
            case ELITE_SWIFT:     ec = Color{ 120, 230, 255, 255 }; break; // голубой — скорость
            case ELITE_ARMORED:   ec = Color{ 210, 210, 235, 255 }; break; // серебристый — броня
            case ELITE_EXPLOSIVE: ec = Color{ 255, 140, 40, 255 };  break; // оранжевый — взрыв
            case ELITE_GIANT:     ec = Color{ 220, 120, 255, 255 }; break; // фиолетовый — гигант
            default: break;
        }
        float pulse = 4.0f + 2.0f * sinf((float)GetTime() * 6.0f);
        DrawCircleLines((int)drawPos.x, (int)drawPos.y, size + pulse, ec);
        DrawCircleLines((int)drawPos.x, (int)drawPos.y, size + pulse + 3.0f, Fade(ec, 0.5f));
    }

    DrawHealthBar();
}
