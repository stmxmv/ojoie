//
// Created by aojoie on 8/31/2023.
//

#pragma once

#include <ojoie/Core/Component.hpp>

namespace AN {

class AudioClip;

class AN_API AudioSource : public Component {

    struct Impl;
    Impl *impl;

    float      m_Pitch;
    float      m_Volume;
    bool       m_Loop;
    AudioClip *m_Clip;


    AN_CLASS(AudioSource, Component)

public:

    explicit AudioSource(ObjectCreationMode mode);

    virtual bool init() override;
    virtual void dealloc() override;

    void setClip(AudioClip *clip);

    void play();

    void stop();

    void pause();

    bool isPlaying();

    bool isLooping();
    void setLooping(bool loop);

    float getPitch();
    float getVolume();

    void setPitch(float pitch);
    void setVolume(float volume);

    float getTime();
    UInt32 getTimeSamples();

    void setTime(float time);
};


}
