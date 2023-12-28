//
// Created by aojoie on 8/31/2023.
//

#include <ojoie/Audio/AudioClip.hpp>
#include <ojoie/Audio/fmod/AudioManager.hpp>
#include <fmod.hpp>

namespace AN {

IMPLEMENT_AN_CLASS(AudioClip)
LOAD_AN_CLASS(AudioClip)

struct AudioClip::Impl {
    FMOD::Sound *sound;

};

AudioClip::AudioClip(ObjectCreationMode mode) : Super(mode), impl(new Impl{}) {}

AudioClip::~AudioClip() {
    delete impl;
}

bool AudioClip::init(const char *name, bool stream) {
    if (Super::init()) {
        FMod::AudioManager &audioManager =  (FMod::AudioManager &)GetAudioManager();
        if (stream) {
            audioManager.getFMODSystem()->createStream(name, FMOD_DEFAULT, nullptr, &impl->sound);
        } else {
            audioManager.getFMODSystem()->createSound(name, FMOD_DEFAULT, nullptr, &impl->sound);
        }

        return impl->sound != nullptr;
    }
    return false;
}


void AudioClip::dealloc() {
    if (impl->sound) {
        impl->sound->release();
    }
    Super::dealloc();
}

float AudioClip::getLength() {
    unsigned int length;
    impl->sound->getLength(&length, FMOD_TIMEUNIT_MS);
    return (float) length / 1000.f;
}

UInt32 AudioClip::getSamples() {
    unsigned int samples;
    impl->sound->getLength(&samples, FMOD_TIMEUNIT_PCM);
    return samples;
}

void *AudioClip::getSound() {
    return impl->sound;
}

}