#include "director.h"

WaveDirector::WaveDirector() { Reset(); }

void WaveDirector::Reset()
{
    elapsed = 0.0f;
    // -1e9 = «давно не использовали», чтобы приём был готов сразу после разблокировки.
    for (int i = 0; i < Tuning::ABILITY_COUNT; i++) lastUsed[i] = -1.0e9f;
}

void WaveDirector::Update(float dt)
{
    elapsed += dt;
}

float WaveDirector::SpawnInterval() const
{
    return Tuning::CurrentSpawnInterval(elapsed);
}

int WaveDirector::WaveCount() const
{
    return Tuning::CurrentWaveCount(elapsed);
}

bool WaveDirector::IsUnlocked(Tuning::AbilityId id) const
{
    return Tuning::IsUnlockedAt(id, elapsed);
}

bool WaveDirector::CanUse(Tuning::AbilityId id) const
{
    if (!IsUnlocked(id)) return false;
    const Tuning::AbilityRule& r = Tuning::GetRule(id);
    return (elapsed - lastUsed[id]) >= r.minInterval;
}

void WaveDirector::NotifyUsed(Tuning::AbilityId id)
{
    lastUsed[id] = elapsed;
}

float WaveDirector::CooldownLeft(Tuning::AbilityId id) const
{
    const Tuning::AbilityRule& r = Tuning::GetRule(id);
    float left = r.minInterval - (elapsed - lastUsed[id]);
    return (left < 0.0f) ? 0.0f : left;
}
