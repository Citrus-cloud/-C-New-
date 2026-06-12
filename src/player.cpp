#include "player.h"
#include "textures.h"
#include "tuning.h"
#include <cmath>

Player::Player(Vector2 startPos)
    : position(startPos), speed(250.0f), health(100), maxHealth(100),
      hitCooldown(0.0f), gotHit(false), xp(0), level(1), xpToNext(5),
      facingLeft(false), animState(PLAYER_IDLE),
      slowFactor(1.0f), slowTimer(0.0f),
      dodgeTimer(0.0f), dodgeIFrameTimer(0.0f), dodgeCdTimer(0.0f), dodgeDir({ 0.0f, 0.0f }),
      spritesLoaded(false)
{
    // Стартовый порог опыта масштабируем множителем профиля сложности
    // (Фаза 4, Шаг 17). На профиле «Тест» требуемый опыт ниже, поэтому
    // вся кривая уровней (порог × рост 1.5) едет вниз пропорционально,
    // а полоса опыта в HUD (xp / xpToNext) остаётся корректной.
    xpToNext = (int)(xpToNext * Tuning::Diff().xpRequiredMul);
    if (xpToNext < 1) xpToNext = 1;   // не допускаем нулевой/отрицательный порог
}

// Загружает спрайты игрока через менеджер текстур (Шаг 2 и 4).
// Анимация создаётся только если реальный файл найден; иначе остаётся
// пустой, и Draw() откатывается на прямоугольник (как в v0.1).
void Player::LoadSprites(TextureManager& textures)
{
    const char* idlePath = "assets/sprites/player_idle.png";
    const char* walkPath = "assets/sprites/player_walk.png";
    const char* hurtPath = "assets/sprites/player_hurt.png";

    if (textures.IsReal(idlePath))
        animIdle = Animation(textures.Get(idlePath), 4, 6.0f, true);
    if (textures.IsReal(walkPath))
        animWalk = Animation(textures.Get(walkPath), 6, 12.0f, true);
    if (textures.IsReal(hurtPath))
        animHurt = Animation(textures.Get(hurtPath), 2, 10.0f, true);

    spritesLoaded = true;
}

Rectangle Player::GetRect() const
{
    return { position.x - 20.0f, position.y - 20.0f, 40.0f, 40.0f };
}

bool Player::TryLevelUp()
{
    if (xp >= xpToNext)
    {
        xp -= xpToNext;
        level += 1;
        xpToNext = (int)(xpToNext * 1.5f);
        return true;
    }
    return false;
}

void Player::TakeDamage(int dmg)
{
    // Чит: неуязвимость (v0.4, Шаг 29) — пока чит включён, урон не проходит вовсе.
    if (Tuning::IsInvulnerable()) return;
    // Урон не проходит во время i-frames от удара или от рывка-уворота (Шаг 29).
    if (hitCooldown > 0.0f || dodgeIFrameTimer > 0.0f) return;
    health -= dmg;
    if (health < 0) health = 0;
    hitCooldown = 0.5f;
    gotHit = true;
}

void Player::Heal(int amount)
{
    health += amount;
    if (health > maxHealth) health = maxHealth;
}

// Замедляет игрока ауройврага (Шаг 26). Вызывается каждый кадр, пока игрок
// в ауре: таймер обновляется, и замедление снимается лишь после выхода.
void Player::ApplySlow(float factor)
{
    slowFactor = factor;
    slowTimer = 0.2f;   // переживёт несколько кадров после выхода из ауры
}

void Player::ResolveStuck(const TileMap& map)
{
    if (!map.CheckCollision(GetRect())) return;
    position = map.FindFreeSpot(position, 20.0f);
}

void Player::Update(float deltaTime, const TileMap& map)
{
    gotHit = false;
    if (hitCooldown > 0.0f) hitCooldown -= deltaTime;

    // Уворот (Шаг 29): гасим таймеры неуязвимости и кулдауна рывка.
    if (dodgeIFrameTimer > 0.0f) dodgeIFrameTimer -= deltaTime;
    if (dodgeCdTimer > 0.0f) dodgeCdTimer -= deltaTime;

    // Замедление от ауры (Шаг 26): по истечении таймера скорость восстанавливается.
    if (slowTimer > 0.0f)
    {
        slowTimer -= deltaTime;
        if (slowTimer <= 0.0f) slowFactor = 1.0f;
    }

    ResolveStuck(map);

    Vector2 dir = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dir.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dir.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dir.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1.0f;

    if (IsGamepadAvailable(0))
    {
        float gx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        float gy = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        if (fabsf(gx) > 0.2f) dir.x += gx;
        if (fabsf(gy) > 0.2f) dir.y += gy;
    }

    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);

    // Триггер рывка-уворота (Шаг 29): Пробел/Shift или нижняя кнопка геймпада.
    // Рывок идёт в сторону ввода, а если игрок стоит на месте — по направлению взгляда.
    bool dodgePressed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT_SHIFT) ||
                        (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
    if (dodgePressed && dodgeTimer <= 0.0f && dodgeCdTimer <= 0.0f)
    {
        if (len > 0.0f) dodgeDir = { dir.x / len, dir.y / len };
        else dodgeDir = { facingLeft ? -1.0f : 1.0f, 0.0f };
        dodgeTimer = Tuning::kDodgeDuration;
        dodgeIFrameTimer = Tuning::kDodgeIFrames;
        dodgeCdTimer = Tuning::kDodgeCooldown;
    }

    bool moving = false;
    if (dodgeTimer > 0.0f)
    {
        // Во время рывка движемся по dodgeDir на повышенной скорости (Шаг 29).
        dodgeTimer -= deltaTime;
        float dx = dodgeDir.x * Tuning::kDodgeSpeed * deltaTime;
        float dy = dodgeDir.y * Tuning::kDodgeSpeed * deltaTime;

        position.x += dx;
        if (map.CheckCollision(GetRect())) position.x -= dx;
        position.y += dy;
        if (map.CheckCollision(GetRect())) position.y -= dy;

        moving = true;
        if (dodgeDir.x < -0.01f) facingLeft = true;
        else if (dodgeDir.x > 0.01f) facingLeft = false;
    }
    else if (len > 0.0f)
    {
        dir.x /= len;
        dir.y /= len;
        // Скорость умножаем на slowFactor (Шаг 26: аура замедления).
        float curSpeed = speed * slowFactor;
        float dx = dir.x * curSpeed * deltaTime;
        float dy = dir.y * curSpeed * deltaTime;

        position.x += dx;
        if (map.CheckCollision(GetRect())) position.x -= dx;
        position.y += dy;
        if (map.CheckCollision(GetRect())) position.y -= dy;

        moving = true;
        // Запоминаем направление взгляда для отражения спрайта (Шаг 5).
        if (dir.x < -0.01f) facingLeft = true;
        else if (dir.x > 0.01f) facingLeft = false;
    }

    // Выбор анимации: урон важнее ходьбы, ходьба важнее покоя.
    if (hitCooldown > 0.25f) animState = PLAYER_HURT;
    else if (moving) animState = PLAYER_WALK;
    else animState = PLAYER_IDLE;

    // Продвигаем только активную анимацию.
    if (animState == PLAYER_WALK) animWalk.Update(deltaTime);
    else if (animState == PLAYER_HURT) animHurt.Update(deltaTime);
    else animIdle.Update(deltaTime);
}

void Player::Draw() const
{
    // Подсветка: голубая во время рывка (i-frames рывка), красная — при обычной неуязвимости.
    Color tint = WHITE;
    if (dodgeIFrameTimer > 0.0f) tint = Color{ 150, 220, 255, 255 };
    else if (hitCooldown > 0.0f) tint = Color{ 255, 150, 150, 255 };

    // Выбираем анимацию под состояние; если её спрайта нет — откат к idle.
    const Animation* anim = &animIdle;
    if (animState == PLAYER_WALK && animWalk.Valid()) anim = &animWalk;
    else if (animState == PLAYER_HURT && animHurt.Valid()) anim = &animHurt;

    if (anim->Valid())
    {
        // Спрайт рисуем по центру позиции игрока, с учётом направления.
        anim->Draw(position, 1.0f, facingLeft, tint);
    }
    else
    {
        // Fallback к v0.1: цветной прямоугольник, если спрайтов ещё нет.
        Color c = RED;
        if (dodgeIFrameTimer > 0.0f) c = Color{ 150, 220, 255, 255 };
        else if (hitCooldown > 0.0f) c = Color{ 255, 150, 150, 255 };
        DrawRectangle((int)(position.x - 20), (int)(position.y - 20), 40, 40, c);
    }
}
