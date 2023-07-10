//
// Created by Aleudillonam on 8/1/2022.
//

#ifndef OJOIE_FLACFILE_HPP
#define OJOIE_FLACFILE_HPP

#include "ojoie/Configuration/typedef.h"
#include <ojoie/Audio/Sound.hpp>

namespace AN {

class AN_API FlacFileBufferProvider : private NonCopyable {
    struct Impl;
    Impl *impl;
    SoundStream *_soundStream{};
    uint64_t frameBufferPosition{};
    uint64_t position{};

    AudioFormat format;
public:
    FlacFileBufferProvider();
    
    bool init(const char *filePath);

    ~FlacFileBufferProvider();

    void bindSoundStream(SoundStream *soundStream) {
        _soundStream = soundStream;
        soundStream->soundStreamNextBuffer.bind(this, &FlacFileBufferProvider::soundStreamNextBuffer);
        soundStream->soundStreamRewind.bind(this, &FlacFileBufferProvider::soundStreamRewind);
        soundStream->soundStreamGetTotalSize.bind(this, &FlacFileBufferProvider::soundStreamGetTotalSize);
        soundStream->soundStreamGetCurrentPosition.bind(this, &FlacFileBufferProvider::soundStreamGetCurrentPosition);
        soundStream->soundStreamSetCurrentPosition.bind(this, &FlacFileBufferProvider::soundStreamSetCurrentPosition);
        soundStream->soundStreamGetFormat.bind(this, &FlacFileBufferProvider::soundStreamGetFormat);
        soundStream->didSetDelegate();
    }

    void unBindSoundStream() {
        _soundStream->soundStreamNextBuffer.unBind();
        _soundStream->soundStreamRewind.unBind();
        _soundStream->soundStreamGetTotalSize.unBind();
        _soundStream->soundStreamGetCurrentPosition.unBind();
        _soundStream->soundStreamSetCurrentPosition.unBind();
        _soundStream = nullptr;
    }


    /// soundStream delegate methods

    void soundStreamNextBuffer(unsigned char *outBuffer, int maxByteRead, int *bytesRead);

    void soundStreamRewind();

    uint64_t soundStreamGetTotalSize();

    uint64_t soundStreamGetCurrentPosition() const;

    void soundStreamSetCurrentPosition(uint64_t position);

    AudioFormat soundStreamGetFormat() const {
        return format;
    }
};



}

#endif//OJOIE_FLACFILE_HPP
