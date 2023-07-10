//
// Created by Aleudillonam on 7/29/2022.
//

#ifndef OJOIE_XAUDIO_SOUND_HPP
#define OJOIE_XAUDIO_SOUND_HPP

#include "ojoie/Configuration/typedef.h"
#include <ojoie/Core/Exception.hpp>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Template/delegate.hpp>
#include <ojoie/Threads/Task.hpp>

#include <Windows.h>
#include <wrl\client.h>
#include <xaudio2.h>

#include <cassert>
#include <format>
#include <mutex>
#include <string>
#include <vector>
#include <queue>
#include <unordered_set>

#include <fstream>

#define AN_STREAMING_BUFFER_SIZE         65536
#define AN_STREAMING_MAX_BUFFER_COUNT    4
#define AN_STREAMING_REMAKE_BUFFER_COUNT 1

namespace AN {

class xaudio_engine;

struct AudioFormat {
    short format_tag;
    short channel_number;
    int sample_rate;
    int byte_rate;
    short block_align;/// bytes per sample * channel number
    short bits_per_sample;

    bool operator==(const AudioFormat &) const = default;

    constexpr static AudioFormat Default() {
        return {
                .format_tag      = 1,
                .channel_number  = 2,
                .sample_rate     = 44100,
                .byte_rate       = 176400,
                .block_align     = 4,
                .bits_per_sample = 16};
    }
};
}

template<>
struct std::hash<AN::AudioFormat> {
    size_t operator()(AN::AudioFormat &format) const {
        size_t ret = 0;
        AN::Math::hash_combine(ret, format.byte_rate);
        AN::Math::hash_combine(ret, format.bits_per_sample);
        AN::Math::hash_combine(ret, format.block_align);
        AN::Math::hash_combine(ret, format.channel_number);
        AN::Math::hash_combine(ret, format.format_tag);
        AN::Math::hash_combine(ret, format.sample_rate);
        return ret;
    }
};

namespace AN {

class AN_API xaudio_channel : private NonCopyable {

    struct VoiceCallback;
    bool isActive;
    bool bIsStreaming;
    XAUDIO2_BUFFER xaBuffer;
    IXAudio2SourceVoice *pSource{};
    class Sound *pSound{};
    class SoundStream *pStream{};
    std::shared_ptr<unsigned char[]> data;

    AudioFormat format;

public:

    inline constexpr static float MaxFrequencyRatio = 2.f;

    explicit xaudio_channel(xaudio_engine *sys, const AudioFormat &format);

    ~xaudio_channel();

    bool isStreaming() const { return bIsStreaming; };

    void playSoundBuffer(class Sound *s, float freqMod, float vol);

    void playSoundBufferLoop(class Sound *s, float freqMod, float vol, int times);

    void attachSoundStream(class SoundStream *stream);

    /// \GameActor
    void stop();


    void onBufferEnd();

    void onStreamBufferEnd();

    /// \brief called on the streaming thread
    /// \result false if need to process again to fill the next buffer index
    void processStream();

    /// \brief stream pause
    void pause();

    void resume();

    void setVolume(float vol);

    void setFreMod(float freMod);

    void flushBuffer();

    int getQueueBufferNum() const {
        XAUDIO2_VOICE_STATE state;
        pSource->GetState(&state);
        return (int)state.BuffersQueued;
    }

    const AudioFormat &getFormat() const {
        return format;
    }
};

class AN_API xaudio_engine : private NonCopyable {

    Microsoft::WRL::ComPtr<IXAudio2> pEngine;
    IXAudio2MasteringVoice *pMaster = nullptr;

    std::unordered_set<xaudio_channel *> idleChannelPtrs;
    std::unordered_set<xaudio_channel *> activeChannelPtrs;

    volatile bool stream_quit{};
    std::mutex streaming_mutex;
    std::condition_variable streaming_cv;
    std::queue<TaskInterface> streamingTasks;
    std::thread streamingThread;

    xaudio_engine() = default;

    void deactivateChannel(xaudio_channel *channel);

    xaudio_channel *new_channel(const AudioFormat &format);

    friend class xaudio_channel;

public:

    class FileError : public Exception {
    public:
#ifdef __cpp_lib_source_location
        FileError(const char *msg, const std::source_location location = std::source_location::current()) noexcept
            : Exception(std::format("xaudio_engine::FileError: {}", msg).c_str(), location) {}
#else
        explicit FileError(const char *msg) noexcept
            : Exception(std::format("xaudio_engine::FileError: {}", msg).c_str()) {}
#endif
    };

    static xaudio_engine &GetSharedAudioEngine();

    bool init();

    void deinit();

    template<typename Func>
    void addStreamingTask(Func &&func) {
        {
            std::lock_guard lock(streaming_mutex);
            streamingTasks.push(TaskItem(std::forward<Func>(func)));
        }
        streaming_cv.notify_one();
    }

    void playSoundBuffer(class Sound *s, float freqMod, float vol);

    void playSoundBufferLoop(class Sound *s, float freqMod, float vol, int times);

    void attachSoundStream(class SoundStream *stream);
};

class AN_API Sound : private NonCopyable {
    UINT32 nBytes = 0;
    xaudio_channel *loop_channel{};
    std::shared_ptr<unsigned char[]> pData;
    std::vector<xaudio_channel *> activeChannelPtrs;

    AudioFormat format;

    friend xaudio_channel;

    void removeChannel(xaudio_channel *channel) {
        if (channel == loop_channel) {
            loop_channel = nullptr;
        }
        auto iter = std::find(activeChannelPtrs.begin(), activeChannelPtrs.end(), channel);
        if (iter != activeChannelPtrs.end()) {
            activeChannelPtrs.erase(iter);
        }
    }

    void addChannel(xaudio_channel *channel) {
        activeChannelPtrs.push_back(channel);
    }

public:
    /// \brief construct a null sound object
    Sound() = default;

    Sound(Sound &&donor) noexcept : nBytes(donor.nBytes),
                                    pData(std::move(donor.pData)),
                                    activeChannelPtrs(std::move(donor.activeChannelPtrs)) {}

    ~Sound();

    bool init(const char *wavFile);

    void play(float vol = 1.f, float freqMod = 1.f) {
        xaudio_engine::GetSharedAudioEngine().playSoundBuffer(this, freqMod, vol);
    }

    void playLoop(int times = 0, float vol = 1.f, float freqMod = 1.f) {
        if (!loop_channel) {
            xaudio_engine::GetSharedAudioEngine().playSoundBufferLoop(this, freqMod, vol, times);
        }
    }

    void stopLoop() {
        if (loop_channel) {
            loop_channel->stop();
        }
    }

    const AudioFormat &getFormat() const {
        return format;
    }

};


class AN_API SoundStream : private NonCopyable {
    bool isEnd{};
    std::atomic_bool isPaused{ true };
    std::atomic_bool shouldRewind;
    std::atomic_bool isProcessing; /// true means the next_buffer and delegate will soon call in the streaming thread

    float _freMod{ 1.f }, _volume{ 1.f };

    int diskReadBufferIndex{}, currentStreamingBufferIndex{};

    std::atomic_uint64_t currentPosition, totalSize;

    xaudio_channel *channel{};

    int bufferSize[AN_STREAMING_MAX_BUFFER_COUNT];
    unsigned char buffer[AN_STREAMING_MAX_BUFFER_COUNT][AN_STREAMING_BUFFER_SIZE];

    AudioFormat currentFormat;

    friend xaudio_channel;


    void onStreamBufferStart();

    void onStreamBufferEnd();

    /// \brief block until next buffer come, called by the channel
    /// \result index of the buffer, -1 if meets end of stream
    int next_buffer(int *bytesRead);

public:

    typedef std::chrono::duration<float> Duration;

    SoundStream() = default;

    bool init();

    ~SoundStream();

    /// \attention setting and unsetting delegate must in a paused state, must call didSetDelegate after setting all delegates.
    /// \brief called when the soundStream want to fill the next buffer, callback should fill the outBuffer
    /// \param byteRead assign it to -1 to indicate that reach the whole buffer end
    Delegate<void(unsigned char *outBuffer, int maxByteRead, int *bytesRead)> soundStreamNextBuffer;

    /// \optional
    Delegate<void()> soundStreamRewind;

    Delegate<uint64_t()> soundStreamGetTotalSize;

    Delegate<uint64_t()> soundStreamGetCurrentPosition;

    Delegate<void(uint64_t position)> soundStreamSetCurrentPosition;

    Delegate<AudioFormat()> soundStreamGetFormat;

    void didSetDelegate();


    bool isLooping{};

    void prepare() {
        if (channel == nullptr) {
            xaudio_engine::GetSharedAudioEngine().attachSoundStream(this);
        }
    }

    void play() {
        if (!isPaused || isEnd) { return; }
        prepare();
        isPaused = false;
        isEnd = false;
        channel->setVolume(_volume);
        channel->setFreMod(_freMod);
        channel->resume();
    }


    /// \brief pause may block, only after calling this method, caller can reset delegates,
    ///        but prefer stop(), whiches also flush the underlying buffer
    void pause() {
        if (!isPaused) {
            isPaused = true;

            isProcessing.wait(true);

            channel->pause();
        }
    }

    void stop();

    void setVolume(float vol) {
        _volume = vol;
        channel->setVolume(vol);
    }

    void setFreMod(float freMod) {
        _freMod = freMod;
        channel->setFreMod(freMod);
    }

    uint64_t getTotalSize() { return totalSize; }
    uint64_t getCurrentPosition() { return currentPosition; }

    Duration getCurrentTime() {
        return Duration{ (float)currentPosition / (float)currentFormat.byte_rate };
    }

    Duration getTotalDuration() {
        return Duration{ (float)totalSize / (float)currentFormat.byte_rate };
    }

    template<typename Rep, typename Period>
    void setCurrentTime(std::chrono::duration<Rep, Period> duration) {
        Duration castedDuration{ duration };
        uint64_t position = (uint64_t)(castedDuration.count()
                                        * (float)currentFormat.byte_rate);
        setCurrentPosition(position);
    }

    void setCurrentPosition(uint64_t position);

    constexpr static float MaxFrequencyRatio() {
        return xaudio_channel::MaxFrequencyRatio;
    }


    AudioFormat getFormat() const {
        return currentFormat;
    }

};

}// namespace AN

#endif//OJOIE_XAUDIO_SOUND_HPP
