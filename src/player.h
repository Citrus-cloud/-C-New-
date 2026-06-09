#pragma once
#include "raylib.h"
#include "tilemap.h"
#include "animation.h"

// Предварительное объявление, чтобы не тянуть весь заголовок текстур сюда.
class TextureManager;

// Какое действие сейчас проигрывается у игрока — определяет анимацию.
enum PlayerAnimState { PLAYER_IDLE, PLAYER_WALK, PLAYER_HURT };

// Класс игрока (без рывка — простое движение)
class Player
{
public:
    Vector2 position;
    float speed;
    int health;
    int maxHealth;
    float hitCooldown;   // i-frames: неуязвимость после удара
    bool gotHit;         // флаг: в этом кадре получили урон (для звука)
    int xp;
    int level;
    int xpToNext;

    bool facingLeft;            // куда смотрит спрайт (для отражения), Шаг 5
    PlayerAnimState animState;  // текущее состояние анимации, Шаг 4

    Player(Vector2 startPos);

    // Загружает спрайты игрока через менеджер текстур. Вызывать после создания игрока.
    void LoadSprites(TextureManager& textures);

    void Update(float deltaTime, const TileMap& map);
    void Draw() const;
    Rectangle GetRect() const;
    bool TryLevelUp();
    void ResolveStuck(const TileMap& map);
    void TakeDamage(int dmg);
    void Heal(int amount);

private:
    Animation animIdle;   // стойка
    Animation animWalk;   // ходьба
    Animation animHurt;   // получение урона
    bool spritesLoaded;
};
