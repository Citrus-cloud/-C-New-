#pragma once
#include "raylib.h"

// Звук и музыка. Файлы берутся из assets/audio/.
// Если файла нет — звук просто не играет (ничего не ломается).
class AudioManager
{
public:
    bool ready;

    float musicVol;   // 0..1, текущий множитель музыки (Шаг 27)
    float sfxVol;     // 0..1, текущий множитель звуков (Шаг 27)

    Music menuMusic; bool menuMusicOk;
    Music gameMusic; bool gameMusicOk;
    int currentMusic;  // 0 нет, 1 меню, 2 игра

    Sound sfxShoot;   bool shootOk;
    Sound sfxHit;     bool hitOk;
    Sound sfxPickup;  bool pickupOk;
    Sound sfxLevelUp; bool levelUpOk;
    Sound sfxBoss;    bool bossOk;
    Sound voiceSpider; bool voiceSpiderOk;
    Sound voiceKnight; bool voiceKnightOk;

    AudioManager();
    void Init();
    void Unload();
    void Update();
    void SetMusicLevel(int v);   // громкость музыки 0..100 (Шаг 27)
    void SetSfxLevel(int v);     // громкость звуков 0..100 (Шаг 27)
    void PlayMenuMusic();
    void PlayGameMusic();
    void StopMusic();
    void Shoot();
    void Hit();
    void Pickup();
    void LevelUp();
    void PlayBossVoice(int line);  // 0 — королева пауков, 1 — чёрный рыцарь
};
