//
// Created by Aleudillonam on 7/29/2022.
//

#include <ojoie/Audio/WavFile.hpp>

#include <Windows.h>
#include <fmod.hpp>
#include <map>
#include <string>
#include <thread>
#include <iostream>
#include <filesystem>

#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <wrl\client.h>

using Microsoft::WRL::ComPtr;

using std::cin, std::cout, std::endl;


typedef std::map<std::string, FMOD::Sound*> SoundMap;
class SimpleAudioManager {
public:
    SimpleAudioManager(){
        FMOD::System_Create(&system);
        system->init(100, FMOD_INIT_NORMAL, 0);
    }
    ~SimpleAudioManager(){
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
    void Load(const std::string& path){
        LoadOrStream(path, false);
    }
    void Stream(const std::string& path){
        LoadOrStream(path, true);
    }

    void Play(const std::string& path){
        // Search for a matching sound in the map
        SoundMap::iterator sound = sounds.find(path);
        // Ignore call if no sound was found
        if (sound == sounds.end()) return;
        // Otherwise play the sound
        system->playSound(sound->second, nullptr, false, nullptr);
    }
private:
    void LoadOrStream(const std::string& path, bool stream){
        // Ignore call if sound is already loaded
        if (sounds.find(path) != sounds.end()) return;
        // Load (or stream) file into a sound object
        FMOD::Sound* sound;
        if (stream)
            system->createStream(path.c_str(), FMOD_DEFAULT, 0, &sound);
        else
            system->createSound(path.c_str(), FMOD_DEFAULT, 0, &sound);
        // Store the sound object in the map using the path as key
        sounds.insert(std::make_pair(path, sound));
    }
    FMOD::System* system;
    SoundMap sounds;
};

int main(int argc, const char * argv[]) {


    // Place your initialization logic here
//    SimpleAudioManager audio;
//    audio.Load("C:\\Users\\Aleudillonam\\Desktop\\02-The First Layer.flac");
//
//    audio.Play("C:\\Users\\Aleudillonam\\Desktop\\02-The First Layer.flac");
//
//    audio.Update();
//
//    std::this_thread::sleep_for(std::chrono::seconds(120));

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);


    ComPtr<IMMDeviceEnumerator> pEnumerator;

    HRESULT hr;

    if (FAILED(hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator))) {
        cout << "Create MMDeviceEnumerator fail" << endl;
    }


    ComPtr<IMMDeviceCollection> pDevices;
    pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATEMASK_ALL, &pDevices);

    UINT cDevices;
    pDevices->GetCount(&cDevices);

    for (UINT i = 0; i < cDevices; ++i) {
        ComPtr<IMMDevice> device;
        pDevices->Item(i, &device);

        /// get device id here
//        LPWSTR deviceID;
//        device->GetId(&deviceID);

        ComPtr<IPropertyStore> pProperties;
        device->OpenPropertyStore(STGM_READ, &pProperties);
        PROPVARIANT propVariant{};
        propVariant.vt = VT_LPWSTR;
        pProperties->GetValue(PKEY_Device_FriendlyName, &propVariant);

        std::string u8String;
        u8String.resize(WideCharToMultiByte(CP_UTF8, 0, propVariant.pwszVal, -1, nullptr, 0, nullptr, 0));
        WideCharToMultiByte(CP_UTF8, 0, propVariant.pwszVal, -1, u8String.data(), (int)u8String.size(), nullptr, 0);
        std::cout << u8String << endl;

    }


    CoUninitialize();

    return 0;
}