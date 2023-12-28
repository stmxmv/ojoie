//
// Created by aojoie on 8/31/2023.
//

#include <ojoie/Audio/AudioSource.hpp>
#include <ojoie/Audio/fmod/AudioManager.hpp>
#include <ojoie/Audio/AudioClip.hpp>
#include <fmod.hpp>


namespace AN {


IMPLEMENT_AN_CLASS(AudioSource)
LOAD_AN_CLASS(AudioSource)

struct AudioSource::Impl {
    FMOD::Sound        *sound;
    FMOD::Channel      *channel;
    FMOD::ChannelGroup *effectGroup;
};

AudioSource::AudioSource(AN::ObjectCreationMode mode)
    : Super(mode), impl(new Impl{}), m_Pitch(1.f), m_Volume(1.f), m_Loop(), m_Clip() {}

AudioSource::~AudioSource() {
    delete impl;
}

bool AudioSource::init() {
    if (Super::init()) {
        FMod::AudioManager &audioManager =  (FMod::AudioManager &)GetAudioManager();
        FMOD_RESULT result;
        result = audioManager.getFMODSystem()->createChannelGroup("Effect Group", &impl->effectGroup);
        return result == FMOD_OK;
    }
    return false;
}

void AudioSource::dealloc() {
    if (impl->effectGroup) {
        impl->effectGroup->release();
        impl->effectGroup = nullptr;
    }
    stop();
    Super::dealloc();
}

void AudioSource::setClip(AudioClip *clip) {
    m_Clip = clip;
    if (m_Clip) {
        impl->sound = (FMOD::Sound *) m_Clip->getSound();
    }
}

void AudioSource::play() {
    if (impl->channel) {
        impl->channel->setPaused(false);
    } else {
        FMod::AudioManager &audioManager =  (FMod::AudioManager &)GetAudioManager();
        impl->channel = audioManager.getFreeChannel(impl->sound);
        impl->channel->setPitch(m_Pitch);
        impl->channel->setVolume(m_Volume);
        impl->channel->setMode(m_Loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
        impl->channel->setChannelGroup(impl->effectGroup);
        impl->channel->setPaused(false);
    }
}

void AudioSource::stop() {
    if (impl->channel) {
        impl->channel->stop();
        impl->channel = nullptr;
    }
}

void AudioSource::pause() {
    if (impl->channel) {
        impl->channel->setPaused(true);
    }
}

bool AudioSource::isPlaying() {
    if (impl->channel) {
        bool playing = false;
        impl->channel->isPlaying(&playing);
        return playing;
    }
    return false;
}

bool AudioSource::isLooping() {
    return m_Loop;
}

void AudioSource::setLooping(bool loop) {
    if (m_Loop == loop) return;
    m_Loop = loop;
    if (impl->channel) {
        impl->channel->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
    }
}

float AudioSource::getPitch() {
    return m_Pitch;
}

float AudioSource::getVolume() {
    return m_Volume;
}

void AudioSource::setPitch(float pitch) {
    if (m_Pitch == pitch) return;
    m_Pitch = pitch;
    if (impl->channel) {
        impl->channel->setPitch(pitch);
    }
}

void AudioSource::setVolume(float volume) {
    if (m_Volume == volume) return;
    m_Volume = volume;
    if (impl->channel) {
        impl->channel->setVolume(volume);
    }
}

float AudioSource::getTime() {
    if (impl->channel) {
        unsigned int length;
        impl->channel->getPosition(&length, FMOD_TIMEUNIT_MS);
        return (float) length / 1000.f;
    }
    return 0.f;
}

void AudioSource::setTime(float time) {
    if (impl->channel) {
        unsigned int pos = time * 1000.f;
        impl->channel->setPosition(pos, FMOD_TIMEUNIT_MS);
    }
}

UInt32 AudioSource::getTimeSamples() {
    if (impl->channel) {
        unsigned int samples;
        impl->channel->getPosition(&samples, FMOD_TIMEUNIT_PCM);
        return samples;
    }
    return 0;
}

}