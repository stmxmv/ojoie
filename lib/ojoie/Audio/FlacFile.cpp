//
// Created by Aleudillonam on 8/1/2022.
//
#include <ojoie/Utility/Log.h>
#include "Audio/FlacFile.hpp"
#include <FLAC++/decoder.h>

namespace AN {


struct FlacDecoder : FLAC::Decoder::File {
    uint32_t totalBytesRead{};
    uint64_t totalSamples{};
    uint64_t totalSize{};
    uint32_t channel_num{};
    uint32_t bytesPerSample{};
    uint32_t sample_rate{};

    uint64_t frameBufferSize{};
    std::unique_ptr<unsigned char []> frameBuffer;

#ifdef _WIN32
    FILE *flacFile{};
#endif

    virtual ~FlacDecoder() override {
#ifdef _WIN32
        if (flacFile) {
            fclose(flacFile);
        }
#endif
    }

    bool initWithUTF8(const char *filePath) {
#ifdef _WIN32
        /// because filePath contains utf-8 string, tricky way to use w suffix function.
        std::wstring wFilePath;
        wFilePath.resize(MultiByteToWideChar(CP_UTF8, 0, filePath, (int)strlen(filePath) + 1, nullptr, 0));
        MultiByteToWideChar(CP_UTF8, 0, filePath, (int)strlen(filePath) + 1, wFilePath.data(), (int)wFilePath.size());
        FILE* fp = _wfopen(wFilePath.c_str(), L"rb");
        if (!fp) {
            ANLog("_wfopen fail at file %s", filePath);
            return false;
        }

        flacFile = fp;
        if (FLAC::Decoder::File::init(flacFile) != 0) {
            ANLog("Flac decoder init fail with file path %s", filePath);
            return false;
        }
#else
        if (impl->flacDecoder.init(filePath) != 0) {
            ANLog("Flac decoder init fail with file path %s", filePath);
            return false;
        }
#endif

        if (!process_until_end_of_metadata()) {
            ANLog("Flac decoder process metadata fail");
            return false;
        }

        return true;
    }

    virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]) final override {
        uint32_t sample_num = frame->header.blocksize;
        uint32_t bytesRead = 0;
        uint64_t index = 0;

        /// resize frame buffer if needed
        uint64_t requireFrameBufferSize = (uint64_t)sample_num * bytesPerSample * channel_num;
        if (frameBufferSize < requireFrameBufferSize) {
            frameBuffer = std::make_unique<unsigned char[]>(requireFrameBufferSize);
        }

        frameBufferSize = requireFrameBufferSize;

        for (uint32_t i = 0; i < sample_num; ++i) {
            /// merge channels into pcm
            for (uint32_t j = 0; j < channel_num; ++j) {
                memcpy(frameBuffer.get() + bytesRead, &buffer[j][i], bytesPerSample);
                bytesRead += bytesPerSample;
                totalBytesRead += bytesPerSample;
                ++index;
            }
        }

        assert(bytesRead == requireFrameBufferSize);

        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata) final override {
        if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
            totalSamples += metadata->data.stream_info.total_samples;
            if (!channel_num) {
                channel_num = metadata->data.stream_info.channels;
            }
            if (!bytesPerSample) {
                bytesPerSample = metadata->data.stream_info.bits_per_sample / 8;
            }
            if (!sample_rate) {
                sample_rate = metadata->data.stream_info.sample_rate;
            }
            totalSize = totalSamples * bytesPerSample * channel_num;
        }
    }

    virtual void error_callback(::FLAC__StreamDecoderErrorStatus status) final override {
        ANLog("flac decoder error status %d", status);
    }
};

struct FlacFileBufferProvider::Impl {
    FlacDecoder flacDecoder;

};

FlacFileBufferProvider::FlacFileBufferProvider() : impl(new Impl{}) {
}

FlacFileBufferProvider::~FlacFileBufferProvider() {

    delete impl;
}

bool FlacFileBufferProvider::init(const char *filePath) {
    if (!impl->flacDecoder.initWithUTF8(filePath)) {
        return false;
    }


    format.format_tag = 1;
    format.sample_rate = impl->flacDecoder.sample_rate;
    format.bits_per_sample = impl->flacDecoder.bytesPerSample * 8;
    format.channel_number = impl->flacDecoder.channel_num;

    format.block_align = format.bits_per_sample / 8 * format.channel_number;
    format.byte_rate = format.bits_per_sample / 8 * format.channel_number * format.sample_rate;

    return true;
}



void FlacFileBufferProvider::soundStreamNextBuffer(unsigned char *outBuffer, int maxByteRead, int *bytesRead) {

    if (position >= soundStreamGetTotalSize()) {
        *bytesRead = -1;
        return;
    }
    /// align
    maxByteRead = maxByteRead - (maxByteRead % format.block_align);

    if (position + maxByteRead > soundStreamGetTotalSize()) {
        maxByteRead = (int)(soundStreamGetTotalSize() - position);
    }

    int read = 0;
    if (frameBufferPosition < impl->flacDecoder.frameBufferSize) {
        read += (int)std::min((uint64_t)maxByteRead, impl->flacDecoder.frameBufferSize - frameBufferPosition);
        memcpy(outBuffer, impl->flacDecoder.frameBuffer.get() + frameBufferPosition, read);
        frameBufferPosition += read;
        if (read == maxByteRead) {
            *bytesRead = read;
            position += read;
            return;
        }
    }

    while (read < maxByteRead) {
        frameBufferPosition = 0;
        if (!impl->flacDecoder.process_single()) {
            ANLog("flac decoder process_single() return false value");
        }
        int willRead = (int)std::min((uint64_t)maxByteRead - read, impl->flacDecoder.frameBufferSize);
        memcpy(outBuffer + read, impl->flacDecoder.frameBuffer.get(), willRead);
        read += willRead;
        frameBufferPosition += willRead;
    }

    *bytesRead = read;
    position += read;

}

void FlacFileBufferProvider::soundStreamRewind() {
    soundStreamSetCurrentPosition(0);
}

uint64_t FlacFileBufferProvider::soundStreamGetTotalSize() {
    return impl->flacDecoder.totalSize;
}

uint64_t FlacFileBufferProvider::soundStreamGetCurrentPosition() const {
    return position;
}

void FlacFileBufferProvider::soundStreamSetCurrentPosition(uint64_t pos) {
    if (pos < soundStreamGetTotalSize()) {
        position = pos;
        frameBufferPosition = 0;
        uint64_t sample_index = position / impl->flacDecoder.bytesPerSample / impl->flacDecoder.channel_num;
        if (!impl->flacDecoder.seek_absolute(sample_index)) {
            impl->flacDecoder.flush();
            impl->flacDecoder.reset();
            ANLog("flacDecoder seek fail with position %llu, sample index %llu, total size %llu",
                  position, sample_index, soundStreamGetTotalSize());
        }
    }
}

}