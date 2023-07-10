//
// Created by Aleudillonam on 7/29/2022.
//

#include "Audio/win32/xaudio_sound.hpp"
#include "Audio/WavFile.hpp"
#include "Threads/Dispatch.hpp"
#include "Utility/Log.h"
#include "Utility/Assert.h"

#include <processthreadsapi.h>

#include <queue>


#ifdef AN_DEBUG
#define CHECK_ON_GAME_THREAD()                                                               \
    do {                                                                                     \
        if (GetCurrentThreadID() != Dispatch::GetThreadID(Dispatch::Game)) { \
            throw Exception("Contexts must execute on game thread!");                        \
        }                                                                                    \
    } while (0)
#else
#define CHECK_ON_GAME_THREAD() (void) 0
#endif

namespace AN {


static std::string GetLastErrorAsString() {
    //Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0) {
        return std::string(); //No error message has been recorded
    }

    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    //Copy the error message into a std::string.
    std::string message(messageBuffer, size);

    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);

    return message;
}

struct xaudio_channel::VoiceCallback : IXAudio2VoiceCallback {
    STDMETHOD_(void, OnStreamEnd)() override {}
    STDMETHOD_(void, OnVoiceProcessingPassEnd)() override {}
    STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32 SamplesRequired) override {}
    STDMETHOD_(void, OnLoopEnd)(void *pBufferContext) override {}
    STDMETHOD_(void, OnVoiceError)(void *pBufferContext, HRESULT Error) override {}

    STDMETHOD_(void, OnBufferStart)(void *pBufferContext) override {
        Dispatch::async(Dispatch::Game, [=] {
            xaudio_channel *chan = (xaudio_channel *) pBufferContext;
            if (chan->bIsStreaming) {
                chan->pStream->onStreamBufferStart();
            }
        });
    }

    STDMETHOD_(void, OnBufferEnd)(void *pBufferContext) override {
        Dispatch::async(Dispatch::Game, [=] {
            xaudio_channel *chan = (xaudio_channel *) pBufferContext;
            if (chan->bIsStreaming) {
                chan->onStreamBufferEnd();
                return;
            }
            chan->onBufferEnd();
        });
    }
};

xaudio_channel::xaudio_channel(xaudio_engine *sys, const AudioFormat &audioFormat) : isActive(), bIsStreaming() {
    static VoiceCallback vcb;
    ZeroMemory(&xaBuffer, sizeof(xaBuffer));
    xaBuffer.pContext = this;
    WAVEFORMATEX waveFormat{};

    waveFormat.wFormatTag      = audioFormat.format_tag;
    waveFormat.nChannels       = audioFormat.channel_number;
    waveFormat.nSamplesPerSec  = audioFormat.sample_rate;
    waveFormat.nAvgBytesPerSec = audioFormat.byte_rate;
    waveFormat.nBlockAlign     = audioFormat.block_align;
    waveFormat.wBitsPerSample  = audioFormat.bits_per_sample;

    HRESULT hr;
    if (FAILED(hr = sys->pEngine->CreateSourceVoice(&pSource, &waveFormat, 0u, MaxFrequencyRatio, &vcb))) {
        ANLog("fail to create source voice");
    }

    format = audioFormat;
}

xaudio_engine &xaudio_engine::GetSharedAudioEngine() {
    static xaudio_engine xaudioEngine;
    return xaudioEngine;
}

bool xaudio_engine::init() {

    XAudio2Create(&pEngine);
    pEngine->CreateMasteringVoice(&pMaster);

    streamingThread = std::thread([this] {
        SetCurrentThreadName("com.an.xaudio_engine.streaming");

        while (!stream_quit) {
            cpu_relax();
            cpu_relax();
            TaskInterface task;
            {
                std::unique_lock lock(streaming_mutex);
                if (streamingTasks.empty()) {
                    streaming_cv.wait(lock, [this] { return !streamingTasks.empty() || stream_quit; });
                }
                if (stream_quit) {
                    break;
                }
                task = std::move(streamingTasks.front());
                streamingTasks.pop();
            }
            if (task) {
                task.run();
            }
        }
    });

    return true;
}

void xaudio_engine::deinit() {
    CHECK_ON_GAME_THREAD();
    stream_quit = true;
    streaming_cv.notify_one();
    streamingThread.join();

    for (xaudio_channel *chan : idleChannelPtrs) {
        delete chan;
    }
    for (xaudio_channel *chan : activeChannelPtrs) {
        delete chan;
    }


    pEngine.Reset();
}

xaudio_channel *xaudio_engine::new_channel(const AudioFormat &format) {
    if (!idleChannelPtrs.empty()) {
        for (auto iter = idleChannelPtrs.begin(); iter != idleChannelPtrs.end(); ++iter) {
            xaudio_channel *free_channel = *iter;
            if (free_channel->getFormat() == format) {
                activeChannelPtrs.insert(free_channel);
                idleChannelPtrs.erase(iter);
                return free_channel;
            }
        }
    }
    xaudio_channel *new_channel = new xaudio_channel(this, format);
    activeChannelPtrs.insert(new_channel);
    return new_channel;
}

void xaudio_engine::playSoundBuffer(struct Sound *s, float freqMod, float vol) {
    new_channel(s->getFormat())->playSoundBuffer(s, freqMod, vol);
}

void xaudio_engine::playSoundBufferLoop(struct Sound *s, float freqMod, float vol, int times) {
    new_channel(s->getFormat())->playSoundBufferLoop(s, freqMod, vol, times);
}

void xaudio_engine::attachSoundStream(struct SoundStream *stream) {
    new_channel(stream->getFormat())->attachSoundStream(stream);
}

void xaudio_engine::deactivateChannel(xaudio_channel *channel) {
    auto iter = activeChannelPtrs.find(channel);
    if (iter != activeChannelPtrs.end()) {
        idleChannelPtrs.insert(*iter);
        activeChannelPtrs.erase(iter);
    }
}


void xaudio_channel::playSoundBuffer(struct Sound *s, float freqMod, float vol) {
    assert(pSource && !pSound);
    if (!isActive) {
        isActive = true;
        data     = s->pData;// retain the data
        s->addChannel(this);
        pSound              = s;
        xaBuffer.Flags      = 0;
        xaBuffer.pAudioData = s->pData.get();
        xaBuffer.AudioBytes = s->nBytes;
        xaBuffer.LoopBegin  = 0;
        xaBuffer.LoopLength = 0;
        xaBuffer.LoopCount  = 0;
        pSource->SubmitSourceBuffer(&xaBuffer, nullptr);
        pSource->SetFrequencyRatio(freqMod);
        pSource->SetVolume(vol);
        pSource->Start();
    }
}

void xaudio_channel::playSoundBufferLoop(Sound *s, float freqMod, float vol, int times) {
    assert(pSource && !pSound);
    if (!isActive) {
        isActive            = true;
        data                = s->pData;// retain the data
        s->loop_channel     = this;
        pSound              = s;
        xaBuffer.Flags      = 0;
        xaBuffer.pAudioData = s->pData.get();
        xaBuffer.AudioBytes = s->nBytes;
        xaBuffer.LoopBegin  = 0;
        xaBuffer.LoopLength = 0;
        if (times == 0) {
            xaBuffer.LoopCount = XAUDIO2_LOOP_INFINITE;
        } else {
            xaBuffer.LoopCount = times - 1;
        }

        pSource->SubmitSourceBuffer(&xaBuffer, nullptr);
        pSource->SetFrequencyRatio(freqMod);
        pSource->SetVolume(vol);
        pSource->Start();
    }
}

void xaudio_channel::attachSoundStream(struct SoundStream *stream) {
    if (!isActive) {
        isActive         = true;
        bIsStreaming     = true;
        pStream          = stream;
        pStream->channel = this;

        xaBuffer.LoopBegin  = 0;
        xaBuffer.LoopLength = 0;
        xaBuffer.LoopCount  = 0;
    }
}

void xaudio_channel::processStream() {
    if (!bIsStreaming) {
        return;
    }

    assert(isActive && pStream);

    XAUDIO2_VOICE_STATE state;
    pSource->GetState(&state);

    if (state.BuffersQueued >= AN_STREAMING_MAX_BUFFER_COUNT - 1 || pStream->isPaused) {
        pStream->isProcessing = false;
        pStream->isProcessing.notify_one();/// notify the game thread
        return;
    }

    int bytesRead;
    int buffer_index = pStream->next_buffer(&bytesRead);

    if (buffer_index >= 0) {
        if (bytesRead <= 0) {
            xaudio_engine::GetSharedAudioEngine().addStreamingTask([this] {
                processStream();
            });
            return;
        }
        xaBuffer.Flags      = 0;
        xaBuffer.pAudioData = pStream->buffer[buffer_index];
        xaBuffer.AudioBytes = bytesRead;
    } else {
        /// reach the whole buffer end
        pStream->isProcessing = false;
        //        pStream->pause();
        pStream->isProcessing.notify_one();/// notify the game thread
        return;
    }
    HRESULT hr;
    if (FAILED(hr = pSource->SubmitSourceBuffer(&xaBuffer, nullptr))) {
        ANLog("Fail to submit to source voice code %lx", hr);
    }

    xaudio_engine::GetSharedAudioEngine().addStreamingTask([this] {
        processStream();
    });
}

void xaudio_channel::onBufferEnd() {
    data = nullptr;// release the data
    stop();
}

void xaudio_channel::onStreamBufferEnd() {
    assert(bIsStreaming);
    pStream->onStreamBufferEnd();
}

xaudio_channel::~xaudio_channel() {
    assert(!pSound);
    if (pSource) {
        pSource->DestroyVoice();
        pSource = nullptr;
    }
}

void xaudio_channel::stop() {
    if (isActive) {
        assert(pSource);
        isActive     = false;
        bIsStreaming = false;
        pSource->Stop();
        if (xaBuffer.LoopCount) {
            pSource->ExitLoop();
        }
        pSource->FlushSourceBuffers();
        if (pSound) {
            pSound->removeChannel(this);
            pSound = nullptr;
        }
        if (pStream) {
            pStream->channel = nullptr;
            pStream = nullptr;
        }
        xaudio_engine::GetSharedAudioEngine().deactivateChannel(this);
    }
}

void xaudio_channel::pause() {
    pSource->Stop();
}

void xaudio_channel::setVolume(float vol) {
    pSource->SetVolume(vol);
}

void xaudio_channel::setFreMod(float freMod) {
    pSource->SetFrequencyRatio(freMod);
}

void xaudio_channel::flushBuffer() {
    pSource->FlushSourceBuffers();
}

void xaudio_channel::resume() {
    assert(bIsStreaming);
    if (!pStream->isProcessing) {
        pStream->isProcessing = true;
        xaudio_engine::GetSharedAudioEngine().addStreamingTask([this] {
            processStream();
        });
    }
    pSource->Start();
}

bool Sound::init(const char *filePath) {
    WavFile wavFile;
    if (wavFile.init(filePath)) {
        format.format_tag = wavFile.getFormatTag();
        format.channel_number = wavFile.getChannelNumber();
        format.sample_rate = wavFile.getSampleRate();
        format.block_align = wavFile.getBlockAlign();
        format.bits_per_sample = wavFile.getBitsPerSample();
        format.byte_rate = wavFile.getByteRate();


        nBytes = wavFile.getDataSize();
        pData  = std::make_shared<unsigned char[]>(nBytes);

        if (!wavFile.read(pData.get())) {
            ANLog("AN::SOUND Error: read wav file %s fail", filePath);
            return false;
        }
        return true;
    }

    return false;
}


Sound::~Sound() {
    if (loop_channel) {
        loop_channel->stop();
    }
    for (auto pChannel : activeChannelPtrs) {
        pChannel->stop();
    }
}


bool SoundStream::init() {
    return true;
}

void SoundStream::onStreamBufferStart() {
    currentPosition += bufferSize[currentStreamingBufferIndex];
    ++currentStreamingBufferIndex;
    currentStreamingBufferIndex %= AN_STREAMING_MAX_BUFFER_COUNT;
}

void SoundStream::onStreamBufferEnd() {
    /// on game thread
    if (shouldRewind && currentStreamingBufferIndex == diskReadBufferIndex) {
        shouldRewind = false;
        if (soundStreamRewind) {
            currentPosition = 0;
            xaudio_engine::GetSharedAudioEngine().addStreamingTask([this] {
                soundStreamRewind();
            });
        }
    }
    assert(currentPosition <= totalSize);
    if (!isLooping && currentPosition == totalSize) {
        pause();
        isEnd = true;
        return;
    }
    if (!isPaused && !isProcessing && channel->getQueueBufferNum() <= 1) {
        isProcessing = true;
        xaudio_engine::GetSharedAudioEngine().addStreamingTask([this] {
            channel->processStream();
        });
    }
}


int SoundStream::next_buffer(int *bytesRead) {
    totalSize = soundStreamGetTotalSize();

    soundStreamNextBuffer(buffer[diskReadBufferIndex], AN_STREAMING_BUFFER_SIZE, bytesRead);

    if (*bytesRead < 0) {
        /// buffer read the end
        if (!isLooping) {
            return -1;
        }

        shouldRewind = true;

        //        soundStreamNextBuffer(buffer[diskReadBufferIndex], AN_STREAMING_BUFFER_SIZE, bytesRead);
        //
        //        if (*bytesRead < 0) {
        //            ANLog("SoundStream rewind fail");
        //            return -1;
        //        }

        return -1;
    }

    bufferSize[diskReadBufferIndex] = *bytesRead;

    if (*bytesRead == 0) {

        //        ANLog("SoundStream read buffer fail");
        return diskReadBufferIndex;
    }

    int ret = diskReadBufferIndex;
    ++diskReadBufferIndex;
    diskReadBufferIndex %= AN_STREAMING_MAX_BUFFER_COUNT;

    return ret;


    //    if (streamFile.getCurrentPosition() == streamFile.getDataSize()) {
    //        if (!isLooping) {
    //            return -1;
    //        }
    //        streamFile.rewind();
    //    }
    //
    //    if ((*bytesRead = streamFile.readChuck(buffer.get() + diskReadBuffer * STREAMING_BUFFER_SIZE, STREAMING_BUFFER_SIZE)) == 0) {
    //        ANLog("SoundStream read file fail");
    //        return diskReadBuffer;
    //    }
    //    int ret = diskReadBuffer;
    //    ++diskReadBuffer;
    //    diskReadBuffer %= STREAMING_MAX_BUFFER_COUNT;
    //
    //    return ret;
}

SoundStream::~SoundStream() {
    pause();
    channel->flushBuffer();
    channel->stop();
}

void SoundStream::stop() {
    if (!isPaused) {
        pause();
    }
    soundStreamRewind();

    channel->flushBuffer();
    currentStreamingBufferIndex = diskReadBufferIndex;
    currentPosition             = 0;
    isEnd                       = false;
}


void SoundStream::setCurrentPosition(uint64_t position) {
    if (position > totalSize) {
        ANLog("AN::SoundStream set current postion out of range");
        position = totalSize;
    }

    if (position % currentFormat.block_align) {
        position -= position % currentFormat.block_align;
    }

    if (position >= totalSize) {
        position = totalSize;
        isEnd    = true;
    } else {
        isEnd = false;
    }

    bool isPlaying = !isPaused;

    if (isPlaying) {
        pause();
    }

    currentPosition = position;

    soundStreamSetCurrentPosition(position);

    if (isEnd) {
        return;
    }

    currentStreamingBufferIndex = diskReadBufferIndex;

    channel->flushBuffer();

    if (isPlaying) {
        play();
    }
}

void SoundStream::didSetDelegate() {
    ANAssert(isPaused, "sound stream should set delegate in a paused state");
    currentPosition = soundStreamGetCurrentPosition();
    totalSize       = soundStreamGetTotalSize();
    shouldRewind    = false;

    if (!channel) {
        currentFormat = soundStreamGetFormat();
    } else {

        const AudioFormat &streamFormat = soundStreamGetFormat();

        if (currentFormat != streamFormat) {
            pause();
            channel->flushBuffer();
            channel->stop();

            currentFormat = streamFormat;
        }
    }
}

}// namespace AN
