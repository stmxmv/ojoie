//
// Created by Aleudillonam on 7/29/2022.
//

#include <fmod.hpp>

#include <map>
#include <string>
#include <thread>
#include <iostream>
#include <filesystem>

typedef std::map<std::string, FMOD::Sound*> SoundMap;
class SimpleAudioManager {
public:
    SimpleAudioManager() {
        FMOD::System_Create(&system);
        system->init(100, FMOD_INIT_NORMAL, 0);
    }
    ~SimpleAudioManager() {
        // Release every sound object and clear the map
        SoundMap::iterator iter;
        for (iter = sounds.begin(); iter != sounds.end(); ++iter)
            iter->second->release();
        sounds.clear();
        // Release the system object
        system->release();
        system = 0;
    }
    void Update() {
        system->update();
    }
    void Load(const std::string& path) {
        LoadOrStream(path, false);
    }
    void Stream(const std::string& path) {
        LoadOrStream(path, true);
    }
    FMOD::Channel * Play(const std::string& path) {
        // Search for a matching sound in the map
        SoundMap::iterator sound = sounds.find(path);
        // Ignore call if no sound was found
        if (sound == sounds.end()) return nullptr;
        // Otherwise play the sound
        FMOD::Channel *channel;
        system->playSound(sound->second, nullptr, false, &channel);
        return channel;
    }

    FMOD::System* getFMODSystem() { return system; }

private:
    void LoadOrStream(const std::string& path, bool stream) {
        // Ignore call if sound is already loaded
        if (sounds.find(path) != sounds.end()) return;
        // Load (or stream) file into a sound object
        FMOD::Sound* sound;
        if (stream)
            system->createStream(path.c_str(), FMOD_DEFAULT, nullptr, &sound);
        else
            system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &sound);
        // Store the sound object in the map using the path as key
        sounds.insert(std::make_pair(path, sound));
    }
    FMOD::System* system;
    SoundMap sounds;
};

int main(int argc, const char * argv[]) {

    // Place your initialization logic here
    SimpleAudioManager audio;
    audio.Stream("C:\\Users\\aojoie\\CLionProjects\\ojoie\\cmake-build-ninja\\bin\\Debug\\Resources\\Audios\\02-The First Layer.flac");

    FMOD::Channel *channel = audio.Play("C:\\Users\\aojoie\\CLionProjects\\ojoie\\cmake-build-ninja\\bin\\Debug\\Resources\\Audios\\02-The First Layer.flac");

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
        audio.Update();
    }

    return 0;
}