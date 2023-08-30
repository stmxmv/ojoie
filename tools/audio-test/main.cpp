//
// Created by Aleudillonam on 7/26/2022.
//
#include <ojoie/IMGUI/IMGUI.hpp>
#include <ojoie/Audio/FlacFile.hpp>
#include <ojoie/Audio/Mp3File.hpp>
#include <ojoie/Audio/Sound.hpp>
#include <ojoie/Audio/WavFile.hpp>
#include <ojoie/Threads/Dispatch.hpp>
#include <ojoie/Core/Game.hpp>
#include <ojoie/Utility//Log.h>
#include <ojoie/Threads/Task.hpp>
#include <ojoie/Core/Window.hpp>
#include <ojoie/Core/Configuration.hpp>
#include <ojoie/Core/Screen.hpp>
#include <ojoie/Core/Behavior.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <filesystem>
#include <ranges>
#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#endif


bool insensitiveEqual(const std::string_view& lhs, const std::string_view& rhs) {
    auto to_lower{ std::ranges::views::transform(tolower) };
    return std::ranges::equal(lhs | to_lower, rhs | to_lower);
}

bool floatEqual(float a, float b) {
    constexpr static float epsilon = 0.001f;
    return std::abs(a - b) < epsilon;
}

static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

using std::cin, std::cout, std::endl;

using namespace AN;

class ImguiNode : public IMGUI {
    typedef ImguiNode Self;
    std::unique_ptr<AN::Sound> sound;

    std::unique_ptr<AN::WavFileBufferProvider> wavFileBufferProvider;
    std::unique_ptr<AN::Mp3FileBufferProvider> mp3FileBufferProvider;
    std::unique_ptr<AN::FlacFileBufferProvider> flacFileBufferProvider;
    std::unique_ptr<AN::SoundStream> soundStream;

    /// render's data
    int selected_fps{};
    bool show_demo_window = true;
    bool show_main_window = true;
    float fre = 1.f, vol = 1.f;
    int loop = 0;
    float percentage;
    float userChoosePercentage;
    bool userDidChoosePercentage{};
    bool stream_loop{};

    AN::SoundStream::Duration currentTime;

    DECLARE_DERIVED_AN_CLASS(ImguiNode, IMGUI)

public:


    explicit ImguiNode(ObjectCreationMode mode) : Super(mode) {}

    virtual bool init() override {
        Super::init();
        sound = std::make_unique<AN::Sound>();

        if (!sound->init("./Resources/Audios/mixkit-light-saber-sword-1708.wav")) {
            return false;
        }

        soundStream = std::make_unique<AN::SoundStream>();
        if (!soundStream->init()) {
            return false;
        }

        wavFileBufferProvider = std::make_unique<AN::WavFileBufferProvider>();

        if (!wavFileBufferProvider->init("./Resources/Audios/02-The First Layer.wav")) {
            return false;
        }

        wavFileBufferProvider->bindSoundStream(soundStream.get());

        soundStream->prepare();

        return true;
    }

    virtual void onGUI() override {

        if (soundStream) {
            /// note that long on windows is 32 bits!!
            if (soundStream->getTotalSize() != 0) {
                currentTime = soundStream->getCurrentTime();
                float percent = 100.f * soundStream->getCurrentTime() / soundStream->getTotalDuration();
                if (!userDidChoosePercentage) {
                    percentage = percent;
                }
            }
        }

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_window_flags = 0;
        host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
        host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("MainDockSpaceViewport", NULL, host_window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
        ImGui::End();

        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        if (show_main_window) {
            ImGui::Begin("Main Window", &show_main_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f * GetGame().deltaTime, 1.f / GetGame().deltaTime);

            static const char* canSelectFPS[] = { "10", "60", "120", "140", "200", "300", "400", "INF"};
            if (ImGui::Combo("Set FPS", &selected_fps, canSelectFPS, std::size(canSelectFPS))) {
                AN::Dispatch::async(AN::Dispatch::Game, [=, index = selected_fps]{
                    int fps;
                    switch (index) {
                        case 0:
                            fps = 10;
                            break;
                        case 1:
                            fps = 60;
                            break;
                        case 2:
                            fps = 120;
                            break;
                        case 3:
                            fps = 140;
                            break;
                        case 4:
                            fps = 200;
                            break;
                        case 5:
                            fps = 300;
                            break;
                        case 6:
                            fps = 400;
                            break;
                        default:
                            fps = INT_MAX;
                    }
                    AN::GetGame().setMaxFrameRate(fps);
                });
            }

            if (ImGui::SliderFloat("FreMod", &fre, 0.1f, AN::SoundStream::MaxFrequencyRatio())) {
                AN::Dispatch::async(AN::Dispatch::Game, [=, this]{
                    soundStream->setFreMod(fre);
                });
            }

            if (ImGui::SliderFloat("Volume", &vol, 0.f, 5.f)) {
                AN::Dispatch::async(AN::Dispatch::Game, [=, this]{
                    soundStream->setVolume(vol);
                });
            }

            ImGui::SliderInt("Loop times", &loop, 0, 100);

            if (ImGui::Button("Play Sound")) {
                AN::Dispatch::async(AN::Dispatch::Game, [=, this]{
                    this->sound->play(vol, fre);
                });
            }
            ImGui::SameLine();
            if (ImGui::Button("Play Loop")) {
                AN::Dispatch::async(AN::Dispatch::Game, [=, this]{
                    sound->playLoop(loop, vol, fre);
                });
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop Loop")) {
                AN::Dispatch::async(AN::Dispatch::Game, [=, this]{
                    sound->stopLoop();
                });
            }

            ImGui::Separator();

            if (ImGui::Button("Open File")) {
                AN::OpenPanel *openPanel = OpenPanel::Alloc();
                openPanel->init();
                openPanel->setAllowOtherTypes(true);
                static const char *audioDescription = "Supported Audio File";
                openPanel->addAllowContentExtension(audioDescription, "wav");
                openPanel->addAllowContentExtension(audioDescription, "mp3");
                openPanel->addAllowContentExtension(audioDescription, "flac");
                openPanel->setTitle("Choose Wav File");
                openPanel->beginSheetModal(App->getMainWindow(), [this](ModalResponse response, const char *filePath) {
                    if (response == kModalResponseOk && filePath) {
                        soundStream->stop();

                        std::string extension = std::filesystem::path((char8_t *)filePath).extension().string();

                        if (insensitiveEqual(extension, ".wav")) {

                            wavFileBufferProvider = nullptr;
                            wavFileBufferProvider = std::make_unique<AN::WavFileBufferProvider>();

                            if (!wavFileBufferProvider->init(filePath)) {
                                ANLog("wavFileBufferProvider init fail");
                                wavFileBufferProvider = nullptr;
                                return;
                            }
                            wavFileBufferProvider->bindSoundStream(soundStream.get());

                        } else if (insensitiveEqual(extension, ".mp3")) {

                            mp3FileBufferProvider = nullptr;
                            mp3FileBufferProvider = std::make_unique<AN::Mp3FileBufferProvider>();

                            if (!mp3FileBufferProvider->init(filePath)) {
                                ANLog("mp3FileBufferProvider init fail");
                                mp3FileBufferProvider = nullptr;
                                return;
                            }

                            mp3FileBufferProvider->bindSoundStream(soundStream.get());

                        } else if (insensitiveEqual(extension, ".flac")) {

                            flacFileBufferProvider = nullptr;
                            flacFileBufferProvider = std::make_unique<AN::FlacFileBufferProvider>();

                            if (!flacFileBufferProvider->init(filePath)) {
                                ANLog("flacFileBufferProvider init fail");
                                flacFileBufferProvider = nullptr;
                                return;
                            }
                            flacFileBufferProvider->bindSoundStream(soundStream.get());
                        }

                        soundStream->prepare();
                    }
                });

                openPanel->release();
            }

            ImGui::Separator();

            ImGui::Text(std::format("{:%T}", currentTime).c_str());

            if (ImGui::SliderFloat("Percentage", &percentage, 0.f, 100.f,
                                   (floatEqual(percentage, 0.f) || floatEqual(percentage, 100.f)) ? "%.0f" : "%.2f")) {
                userChoosePercentage = percentage;
                userDidChoosePercentage = true;
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                AN::Dispatch::async(AN::Dispatch::Game, [per = userChoosePercentage, this]{
                    auto time = soundStream->getTotalDuration() / 100.f * per;
//                    uint64_t expectedPosition = (uint64_t)(soundStream->getTotalDuration().count() / 100.f * per);
//                    if (expectedPosition > soundStream->getTotalSize()) {
//                        expectedPosition = soundStream->getTotalSize();
//                    }
//                    soundStream->setCurrentPosition(expectedPosition);
                    soundStream->setCurrentTime(time);
                    userDidChoosePercentage = false;
                });
            }

            if (ImGui::Button("Play Stream")) {
                AN::Dispatch::async(AN::Dispatch::Game, [=, this]{
                    soundStream->setFreMod(fre);
                    soundStream->setVolume(vol);
                    soundStream->play();
                });
            }
            ImGui::SameLine();
            if (ImGui::Button("Pause Stream")) {
                AN::Dispatch::async(AN::Dispatch::Game, [=, this]{
                    soundStream->pause();
                });
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop Stream")) {
                AN::Dispatch::async(AN::Dispatch::Game, [=, this]{
                    soundStream->stop();
                });
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Stream Loop", &stream_loop)) {
                AN::Dispatch::async(AN::Dispatch::Game, [=, this]{
                    soundStream->isLooping = stream_loop;
                });
            }

            ImGui::End();
        }
    }
};

ImguiNode::~ImguiNode() {}

IMPLEMENT_AN_CLASS(ImguiNode)
LOAD_AN_CLASS(ImguiNode)


struct AppDelegate : public ApplicationDelegate {

    AN::Window *mainWindow;

    Actor *actor;

    virtual bool applicationShouldTerminateAfterLastWindowClosed(AN::Application *application) override {
        return true;
    }

    virtual bool gameShouldPauseWhenNotActive(Game &game) override {
        return false;
    }

    virtual void applicationDidFinishLaunching(AN::Application *application) override {
        mainWindow = Window::Alloc();
        auto screenSize = GetScreen().getSize();;
        mainWindow->init({0, 0, screenSize.width / 2, screenSize.height / 2 });
        mainWindow->setTitle("audio-test");
        mainWindow->center();
        mainWindow->makeKeyAndOrderFront();

        AN::GetGame().setMaxFrameRate(60);
    }

    virtual void gameStart(Game &game) override {
        actor = NewObject<Actor>();
        actor->init("MainActor");
        actor->addComponent<ImguiNode>();
    }

    virtual void gameStop(Game &game) override {
        DestroyActor(actor);
    }

    virtual void applicationWillTerminate(AN::Application *application) override {
        mainWindow->release();
        mainWindow = nullptr;
    }
};

int main(int argc, const char * argv[]) {

    AN::Application &app = AN::Application::GetSharedApplication();
    AppDelegate *appDelegate = new AppDelegate();
    app.setDelegate(appDelegate);
    appDelegate->release();

    try {
        app.run(argc, argv);

    } catch (const std::exception &exception) {

        printf("Exception: %s\n", exception.what());

#ifdef _WIN32
        MessageBoxA(nullptr, exception.what(), "Exception", MB_OK|MB_ICONERROR);
#endif

    }


    return 0;
}