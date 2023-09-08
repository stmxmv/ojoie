//
// Created by Aleudillonam on 7/29/2022.
//

#include <fmod.hpp>

#include <map>
#include <string>
#include <thread>
#include <iostream>
#include <filesystem>
#include <unordered_map>

typedef std::unordered_map<size_t, FMOD::Sound *> SoundMap;

namespace AN {

class AudioManager {

public:

    virtual ~AudioManager() = default;

    virtual bool init();

    virtual void deinit();

    virtual void update(float deltaTime);

};

namespace FMod {

class AudioSource {

    FMOD::Sound *m_Sound;

public:

    AudioSource() : m_Sound() {}



};

class AudioManager : public AN::AudioManager {

    FMOD::System* system;
    SoundMap sounds;

    static constexpr int MaxChannels = 100;

    FMOD::Sound* loadOrStream(const char *path, bool stream) {
        size_t hash = std::hash<std::string_view>()(path);
        // Ignore call if sound is already loaded
        if (auto it = sounds.find(hash); it != sounds.end()) {
            return it->second;
        }

        // Load (or stream) file into a sound object
        FMOD::Sound* sound = nullptr;
        FMOD_RESULT result;
        if (stream)
            result = system->createStream(path, FMOD_DEFAULT, nullptr, &sound);
        else
            result = system->createSound(path, FMOD_DEFAULT, nullptr, &sound);

        if (result == FMOD_OK && sound) {
            // Store the sound object in the map using the path as key
            sounds.insert({ hash, sound });
        }

        return sound;
    }

public:

    virtual bool init() override {
        FMOD::System_Create(&system);
        system->init(MaxChannels, FMOD_INIT_NORMAL, 0);
    }

    virtual void deinit() override {
        // Release every sound object and clear the map
        SoundMap::iterator iter;
        for (iter = sounds.begin(); iter != sounds.end(); ++iter)
            iter->second->release();
        sounds.clear();
        // Release the system object
        system->release();
        system = nullptr;
    }

    virtual void update(float deltaTime) override {
        system->update();
    }

    FMOD::Sound* load(const char *path) {
        return loadOrStream(path, false);
    }

    FMOD::Sound* stream(const char *path) {
        return loadOrStream(path, true);
    }

    FMOD::Channel *play(FMOD::Sound *sound) {
        if (sound == nullptr) return nullptr;

        FMOD::Channel *channel;
        system->playSound(sound, nullptr, false, &channel);
        return channel;
    }

    FMOD::System* getFMODSystem() { return system; }
};

}


}


int main(int argc, const char * argv[]) {

//    using namespace AN;
    using namespace AN::FMod;

    // Place your initialization logic here
    AudioManager audio;
    audio.init();

    FMOD::Sound *sound =  audio.stream("C:\\Users\\aojoie\\CLionProjects\\ojoie\\cmake-build-ninja\\bin\\Debug\\Resources\\Audios\\02-The First Layer.flac");

    FMOD::Channel *channel = audio.play(sound);

    channel->setMode(FMOD_LOOP_NORMAL);

    // Set 3D attributes for the sound
//    FMOD_VECTOR soundPosition = { 100.0f, 0.0f, 0.0f };  // Position of the sound in 3D space
//    FMOD_VECTOR soundVelocity = { 0.0f, 0.0f, 0.0f };  // Velocity of the sound (optional)
//    channel->set3DAttributes(&soundPosition, &soundVelocity);

    FMOD_VECTOR position = { -3.0f, -4.0f, -2.0f };
    FMOD_VECTOR velocity = { 0.0f, 0.0f, 0.0f };
    channel->set3DAttributes(&position, &velocity);

    FMOD_VECTOR direction = { 1.0f, 2.0f, 3.0f };
    channel->set3DConeOrientation(&direction);
    channel->set3DConeSettings(30.0f, 60.0f, 0.5f);

    // Set listener attributes (position and orientation)
    FMOD_VECTOR listenerPosition = { 0.0f, 0.0f, 0.0f };  // Position of the listener in 3D space
    FMOD_VECTOR listenerVelocity = { 0.0f, 0.0f, 0.0f };  // Velocity of the listener (optional)
    FMOD_VECTOR listenerForward = { 0.0f, 0.0f, -1.0f };  // Forward vector of the listener (direction the listener is facing)
    FMOD_VECTOR listenerUp = { 0.0f, 1.0f, 0.0f };       // Up vector of the listener
    audio.getFMODSystem()->set3DListenerAttributes(0, &listenerPosition, &listenerVelocity, &listenerForward, &listenerUp);

    std::string line;
    for (;;) {
        audio.update(0);
    }

    audio.deinit();

    return 0;
}