#include "audio.h"

static Sound LoadSoundAny(const char* base, bool& ok)
{
    const char* exts[3] = { ".wav", ".ogg", ".mp3" };
    for (int i = 0; i < 3; i++)
    {
        const char* path = TextFormat("%s%s", base, exts[i]);
        if (FileExists(path)) { ok = true; return LoadSound(path); }
    }
    ok = false;
    Sound s = { 0 };
    return s;
}

static Music LoadMusicAny(const char* base, bool& ok)
{
    const char* exts[3] = { ".ogg", ".mp3", ".wav" };
    for (int i = 0; i < 3; i++)
    {
        const char* path = TextFormat("%s%s", base, exts[i]);
        if (FileExists(path)) { ok = true; return LoadMusicStream(path); }
    }
    ok = false;
    Music m = { 0 };
    return m;
}

AudioManager::AudioManager()
    : ready(false), musicVol(0.8f), sfxVol(1.0f),
      menuMusicOk(false), gameMusicOk(false), currentMusic(0),
      shootOk(false), hitOk(false), pickupOk(false), levelUpOk(false), bossOk(false),
      voiceSpiderOk(false), voiceKnightOk(false)
{
}

void AudioManager::Init()
{
    InitAudioDevice();
    ready = IsAudioDeviceReady();
    if (!ready) return;

    menuMusic = LoadMusicAny("assets/audio/music_menu", menuMusicOk);
    gameMusic = LoadMusicAny("assets/audio/music_game", gameMusicOk);
    sfxShoot   = LoadSoundAny("assets/audio/shoot", shootOk);
    sfxHit     = LoadSoundAny("assets/audio/hit", hitOk);
    sfxPickup  = LoadSoundAny("assets/audio/pickup", pickupOk);
    sfxLevelUp = LoadSoundAny("assets/audio/levelup", levelUpOk);
    sfxBoss    = LoadSoundAny("assets/audio/boss", bossOk);
    voiceSpider = LoadSoundAny("assets/audio/voice_spider", voiceSpiderOk);
    voiceKnight = LoadSoundAny("assets/audio/voice_knight", voiceKnightOk);

    // Применяем стартовые уровни громкости.
    SetMusicLevel((int)(musicVol * 100.0f));
    SetSfxLevel((int)(sfxVol * 100.0f));
}

void AudioManager::Unload()
{
    if (!ready) return;
    if (menuMusicOk) UnloadMusicStream(menuMusic);
    if (gameMusicOk) UnloadMusicStream(gameMusic);
    if (shootOk) UnloadSound(sfxShoot);
    if (hitOk) UnloadSound(sfxHit);
    if (pickupOk) UnloadSound(sfxPickup);
    if (levelUpOk) UnloadSound(sfxLevelUp);
    if (bossOk) UnloadSound(sfxBoss);
    if (voiceSpiderOk) UnloadSound(voiceSpider);
    if (voiceKnightOk) UnloadSound(voiceKnight);
    CloseAudioDevice();
}

void AudioManager::Update()
{
    if (!ready) return;
    if (currentMusic == 1 && menuMusicOk) UpdateMusicStream(menuMusic);
    else if (currentMusic == 2 && gameMusicOk) UpdateMusicStream(gameMusic);
}

// Громкость музыки 0..100. :: вызывает глобальную функцию raylib,
// чтобы не путать с методом класса.
void AudioManager::SetMusicLevel(int v)
{
    if (v < 0) v = 0; if (v > 100) v = 100;
    musicVol = v / 100.0f;
    if (!ready) return;
    if (menuMusicOk) ::SetMusicVolume(menuMusic, musicVol);
    if (gameMusicOk) ::SetMusicVolume(gameMusic, musicVol);
}

// Громкость звуковых эффектов 0..100.
void AudioManager::SetSfxLevel(int v)
{
    if (v < 0) v = 0; if (v > 100) v = 100;
    sfxVol = v / 100.0f;
    if (!ready) return;
    if (shootOk) ::SetSoundVolume(sfxShoot, sfxVol);
    if (hitOk) ::SetSoundVolume(sfxHit, sfxVol);
    if (pickupOk) ::SetSoundVolume(sfxPickup, sfxVol);
    if (levelUpOk) ::SetSoundVolume(sfxLevelUp, sfxVol);
    if (bossOk) ::SetSoundVolume(sfxBoss, sfxVol);
    if (voiceSpiderOk) ::SetSoundVolume(voiceSpider, sfxVol);
    if (voiceKnightOk) ::SetSoundVolume(voiceKnight, sfxVol);
}

void AudioManager::StopMusic()
{
    if (!ready) return;
    if (menuMusicOk) StopMusicStream(menuMusic);
    if (gameMusicOk) StopMusicStream(gameMusic);
    currentMusic = 0;
}

void AudioManager::PlayMenuMusic()
{
    if (!ready) return;
    if (currentMusic == 1) return;
    StopMusic();
    if (menuMusicOk) { PlayMusicStream(menuMusic); currentMusic = 1; }
}

void AudioManager::PlayGameMusic()
{
    if (!ready) return;
    if (currentMusic == 2) return;
    StopMusic();
    if (gameMusicOk) { PlayMusicStream(gameMusic); currentMusic = 2; }
}

void AudioManager::Shoot()   { if (ready && shootOk) PlaySound(sfxShoot); }
void AudioManager::Hit()     { if (ready && hitOk) PlaySound(sfxHit); }
void AudioManager::Pickup()  { if (ready && pickupOk) PlaySound(sfxPickup); }
void AudioManager::LevelUp() { if (ready && levelUpOk) PlaySound(sfxLevelUp); }

void AudioManager::PlayBossVoice(int line)
{
    if (!ready) return;
    if (bossOk) PlaySound(sfxBoss);
    if (line == 0 && voiceSpiderOk) PlaySound(voiceSpider);
    else if (line == 1 && voiceKnightOk) PlaySound(voiceKnight);
}
