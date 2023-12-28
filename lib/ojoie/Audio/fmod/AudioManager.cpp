//
// Created by aojoie on 8/31/2023.
//

#include <ojoie/Allocator/MemoryDefines.h>
#include <ojoie/Audio/fmod/AudioManager.hpp>

namespace AN::FMod {

static void* F_CALLBACK FMODMemoryAlloc(unsigned int size, FMOD_MEMORY_TYPE type, const char *sourcestr) {
    return AN_MALLOC_ALIGNED(size, 16);
}

static void F_CALLBACK FMODMemoryFree(void *ptr, FMOD_MEMORY_TYPE type, const char *sourcestr) {
    AN_FREE(ptr);
}

static void* F_CALLBACK FMODMemoryRealloc (void *ptr, unsigned int size, FMOD_MEMORY_TYPE type, const char *sourcestr) {
    return AN_REALLOC_ALIGNED(ptr, size, 16);
}

bool AudioManager::init() {
    FMOD::Memory_Initialize(nullptr, 0, FMODMemoryAlloc, FMODMemoryRealloc, FMODMemoryFree);
    if (FMOD_OK != FMOD::System_Create(&system)) {
        return false;
    }
    return FMOD_OK == system->init(MaxChannels, FMOD_INIT_NORMAL, 0);
}

void AudioManager::deinit() {
    // Release the system object
    system->release();
    system = nullptr;
}

void AudioManager::update(float deltaTime) {
    system->update();
}


FMOD::Channel *AudioManager::getFreeChannel(FMOD::Sound *sound) {
    if (sound == nullptr) return nullptr;

    FMOD::Channel *channel;
    system->playSound(sound, nullptr, true, &channel);
    return channel;
}

}