//
// Created by aojoie on 8/31/2023.
//

#include "Audio/AudioManager.hpp"

#include "./fmod/AudioManager.cpp"

namespace AN {

AudioManager &GetAudioManager() {
    static FMod::AudioManager audioManager;
    return audioManager;
}


}