//
// Created by Aleudillonam on 7/29/2022.
//

#ifndef OJOIE_SOUND_HPP
#define OJOIE_SOUND_HPP

#ifdef _WIN32
#include <ojoie/Audio/win32/xaudio_sound.hpp>
#endif




namespace AN {

#ifdef _WIN32
typedef xaudio_engine AudioEngine;
#endif


inline AudioEngine &GetAudioEngine() {
    return AudioEngine::GetSharedAudioEngine();
}


}




#endif//OJOIE_SOUND_HPP
