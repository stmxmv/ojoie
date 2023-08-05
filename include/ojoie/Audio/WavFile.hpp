//
// Created by Aleudillonam on 7/30/2022.
//

#ifndef OJOIE_WAVFILE_HPP
#define OJOIE_WAVFILE_HPP

#include <ojoie/Utility/Log.h>
#include "ojoie/Configuration/typedef.h"
#include <fstream>
#include <ojoie/Audio/Sound.hpp>


namespace AN {

struct Riff {
    int chuckID;
    int chuckSize;
    int format;
};

class AN_API WavFile : private NonCopyable {

    struct wav_format_chuck {
        short formatTag;
        short channel_number;
        int   sample_rate;
        int   byte_rate;
        short block_align;
        short bits_per_sample;
    };

    std::ifstream file;
    Riff riff;
    wav_format_chuck format_chuck;

    unsigned int dataOffset;
    unsigned int dataSize;
    unsigned int currentPos{};
public:

    WavFile() = default;

    bool init(const char *filePath);

    bool read(unsigned char *out);

    void rewind();

    unsigned int readChuck(unsigned char *out, unsigned int chuck_size);

    void setCurrentPosition(unsigned int position) { currentPos = position; }

    unsigned int getCurrentPosition() const { return currentPos; }

    unsigned int getDataSize() const { return dataSize; }

    short getFormatTag() const { return format_chuck.formatTag; }

    short getChannelNumber() const { return format_chuck.channel_number; }

    int getSampleRate() const { return format_chuck.sample_rate; }

    int getByteRate() const { return format_chuck.byte_rate; }

    short getBlockAlign() const { return format_chuck.block_align; }

    short getBitsPerSample() const { return format_chuck.bits_per_sample; }

};


class AN_API WavFileBufferProvider : private NonCopyable {
    WavFile wavFile;
    SoundStream *_soundStream{};

    AudioFormat format;
public:

    bool init(const char *filePath) {
        if (!wavFile.init(filePath)) {
            return false;
        }

        format.format_tag = wavFile.getFormatTag();
        format.channel_number = wavFile.getChannelNumber();
        format.sample_rate = wavFile.getSampleRate();
        format.block_align = wavFile.getBlockAlign();
        format.bits_per_sample = wavFile.getBitsPerSample();
        format.byte_rate = wavFile.getByteRate();

        return true;
    }

    ~WavFileBufferProvider() {
        if (_soundStream) {
            unBindSoundStream();
        }
    }

    void bindSoundStream(SoundStream *soundStream) {
        _soundStream = soundStream;
        soundStream->soundStreamNextBuffer.bind(this, &WavFileBufferProvider::soundStreamNextBuffer);
        soundStream->soundStreamRewind.bind(this, &WavFileBufferProvider::soundStreamRewind);
        soundStream->soundStreamGetTotalSize.bind(this, &WavFileBufferProvider::soundStreamGetTotalSize);
        soundStream->soundStreamGetCurrentPosition.bind(this, &WavFileBufferProvider::soundStreamGetCurrentPosition);
        soundStream->soundStreamSetCurrentPosition.bind(this, &WavFileBufferProvider::soundStreamSetCurrentPosition);
        soundStream->soundStreamGetFormat.bind(this, &WavFileBufferProvider::soundStreamGetFormat);
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

    void soundStreamNextBuffer(unsigned char *outBuffer, int maxByteRead, int *bytesRead) {
        if (wavFile.getCurrentPosition() == wavFile.getDataSize()) {
            *bytesRead = -1; // inform caller that the buffer reach the end
            return;
        }

        if ((*bytesRead = (int)wavFile.readChuck(outBuffer, maxByteRead)) == 0) {
            ANLog("SoundStream read file fail");
        }
    }

    void soundStreamRewind() {
            wavFile.rewind();
    }

    uint64_t soundStreamGetTotalSize() const { return wavFile.getDataSize(); }

    uint64_t soundStreamGetCurrentPosition() const { return wavFile.getCurrentPosition(); }

    void soundStreamSetCurrentPosition(uint64_t position) {
        /// align the position, or will kill the sound
        wavFile.setCurrentPosition(position);
    }

    AudioFormat soundStreamGetFormat() const {
        return format;
    }
};



}

#endif//OJOIE_WAVFILE_HPP
