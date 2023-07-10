//
// Created by Aleudillonam on 8/1/2022.
//

#include <ojoie/Utility/Log.h>
#include "Audio/Mp3File.hpp"

#define MINIMP3_IMPLEMENTATION
#include "minimp3/minimp3_ex.h"

namespace AN {

struct Mp3FileBufferProvider::Impl {
    mp3dec_ex_t dec;
};


Mp3FileBufferProvider::Mp3FileBufferProvider() : impl(new Impl{}) {
}

Mp3FileBufferProvider::~Mp3FileBufferProvider() {
    if (_soundStream) {
        unBindSoundStream();
    }
    mp3dec_ex_close(&impl->dec);
    delete impl;
}

bool Mp3FileBufferProvider::init(const char *filePath) {
#ifdef _WIN32
    std::wstring wFilePath;
    int len = (int)strlen(filePath) + 1;
    wFilePath.resize(MultiByteToWideChar(CP_UTF8, 0, filePath, len, nullptr, 0));
    MultiByteToWideChar(CP_UTF8, 0, filePath, len, wFilePath.data(), (int)wFilePath.size());
    if (mp3dec_ex_open_w(&impl->dec, wFilePath.c_str(), MP3D_SEEK_TO_SAMPLE) != 0) {
        ANLog("minimp3 open with error code %d", impl->dec.last_error);
        return false;
    }
#else
    if (mp3dec_ex_open(&impl->dec, filePath, MP3D_SEEK_TO_BYTE) != 0) {
        ANLog("minimp3 open with error code %d", impl->dec.last_error);
        return false;
    }
#endif


    /* dec.samples, dec.info.hz, dec.info.layer, dec.info.channels should be filled */
    if (mp3dec_ex_seek(&impl->dec, 0) != 0) {
        ANLog("minimp3 seek with error code %d", impl->dec.last_error);
        return false;
    }

    format.format_tag = 1;
    format.sample_rate = impl->dec.info.hz;
    format.bits_per_sample = 8 * sizeof(mp3d_sample_t);
    format.channel_number = impl->dec.info.channels;

    format.block_align = format.bits_per_sample / 8 * format.channel_number;
    format.byte_rate = format.bits_per_sample / 8 * format.channel_number * format.sample_rate;


    return true;
}




void Mp3FileBufferProvider::soundStreamNextBuffer(unsigned char *outBuffer, int maxByteRead, int *bytesRead) {
    uint64_t totalSize = soundStreamGetTotalSize();
    if (position >= totalSize) {
        *bytesRead = -1;
        return;
    }

    /// align
    maxByteRead = maxByteRead - (maxByteRead % format.block_align);

    if (position + maxByteRead > totalSize) {
        maxByteRead = (int)(totalSize - position);
    }

    if (mp3dec_ex_seek(&impl->dec, position / sizeof(mp3d_sample_t)) != 0) {
        ANLog("minimp3 seek with error code %d", impl->dec.last_error);
        *bytesRead = 0;
        return;
    }

    *bytesRead = (int)(mp3dec_ex_read(&impl->dec, (mp3d_sample_t *)outBuffer, maxByteRead / sizeof(mp3d_sample_t)) * sizeof(mp3d_sample_t));

    position += *bytesRead;
}


void Mp3FileBufferProvider::soundStreamRewind() {
    position = 0;
}


uint64_t Mp3FileBufferProvider::soundStreamGetTotalSize() {
    return impl->dec.samples * sizeof(mp3d_sample_t);
}


uint64_t Mp3FileBufferProvider::soundStreamGetCurrentPosition() const {
    return position;
}


void Mp3FileBufferProvider::soundStreamSetCurrentPosition(uint64_t pos) {
    position = pos;
}


}