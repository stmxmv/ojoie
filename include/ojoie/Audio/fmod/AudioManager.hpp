//
// Created by aojoie on 8/31/2023.
//

#pragma once

#include <ojoie/Audio/AudioManager.hpp>
#include <fmod.hpp>

#include <string_view>
#include <unordered_map>

namespace AN::FMod {

typedef std::unordered_map<size_t, FMOD::Sound *> SoundMap;

class AudioManager : public AN::AudioManager {

    FMOD::System* system;

    static constexpr int MaxChannels = 100;

public:

    virtual bool init() override;

    virtual void deinit() override;

    virtual void update(float deltaTime) override;

    FMOD::Channel *getFreeChannel(FMOD::Sound *sound);

    FMOD::System* getFMODSystem() { return system; }
};


}
