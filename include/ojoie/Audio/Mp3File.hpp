//
// Created by Aleudillonam on 8/1/2022.
//

#ifndef OJOIE_MP3FILE_HPP
#define OJOIE_MP3FILE_HPP

#include "ojoie/Configuration/typedef.h"
#include <ojoie/Audio/Sound.hpp>

namespace AN {


class AN_API Mp3FileBufferProvider : private NonCopyable {
    struct Impl;
    Impl *impl;
    SoundStream *_soundStream{};
    uint64_t position{};

    AudioFormat format;
public:
    Mp3FileBufferProvider();

    bool init(const char *filePath);

    ~Mp3FileBufferProvider();

    void bindSoundStream(SoundStream *soundStream) {
        _soundStream = soundStream;
        soundStream->soundStreamNextBuffer.bind(this, &Mp3FileBufferProvider::soundStreamNextBuffer);
        soundStream->soundStreamRewind.bind(this, &Mp3FileBufferProvider::soundStreamRewind);
        soundStream->soundStreamGetTotalSize.bind(this, &Mp3FileBufferProvider::soundStreamGetTotalSize);
        soundStream->soundStreamGetCurrentPosition.bind(this, &Mp3FileBufferProvider::soundStreamGetCurrentPosition);
        soundStream->soundStreamSetCurrentPosition.bind(this, &Mp3FileBufferProvider::soundStreamSetCurrentPosition);
        soundStream->soundStreamGetFormat.bind(this, &Mp3FileBufferProvider::soundStreamGetFormat);
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

#endif//OJOIE_MP3FILE_HPP
