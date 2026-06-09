#pragma once
#include "raylib.h"
#include "enemy.h"
#include "tilemap.h"
#include "player.h"
#include "animation.h"
#include <vector>

// Предварительное объявление менеджера текстур.
class TextureManager;

class Spawner
{
public:
    std::vector<Enemy> enemies;
    float spawnTimer;
    float spawnInterval;
    float bossTimer;
    float bossInterval;
    float elapsed;
    int bossEventLine;  // -1 = ничего; иначе номер реплики появившегося босса

    Spawner(int poolSize);
    void LoadArt(TextureManager& textures);  // загрузить спрайты врагов и боссов
    Enemy* GetInactive();
    void SpawnWave(Vector2 center, const TileMap& map);
    void SpawnBoss(Vector2 center, const TileMap& map);
    void Update(float deltaTime, Player& player, const TileMap& map);
    void Draw() const;
    int ActiveCount() const;

private:
    // Прототипы анимаций — копируются в каждого врага при спавне.
    Animation artGruntWalk, artFastWalk, artTankWalk, artBossWalk;
    Animation artEnemyDeath, artBossDeath;
    // Назначает врагу нужные анимации по его типу.
    void AssignArt(Enemy* e, EnemyType t);
};
