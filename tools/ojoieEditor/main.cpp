//
// Created by Aleudillonam on 7/26/2022.
//
#include <imgui/imgui.h>
#include <ojoie/Audio/FlacFile.hpp>
#include <ojoie/Audio/Mp3File.hpp>
#include <ojoie/Audio/Sound.hpp>
#include <ojoie/Audio/WavFile.hpp>
#include <ojoie/Core/Dispatch.hpp>
#include <ojoie/Core/Game.hpp>
#include <ojoie/Core/Log.h>
#include <ojoie/Core/Node.hpp>
#include <ojoie/Core/Task.hpp>
#include <ojoie/Core/Window.hpp>
#include <ojoie/Core/Configuration.hpp>
#include <ojoie/UI/ImguiNode.hpp>

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

class ImguiNode : public AN::ImguiNode {
    typedef ImguiNode Self;
    typedef AN::ImguiNode Super;

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

public:
    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    ImguiNode() {
        tick = true;
    }

    virtual bool init() override {
        Super::init();

        return true;
    }

    virtual void update(float deltaTime) override {
        Super::update(deltaTime);

    }

    virtual void postRender(const AN::RenderContext &context) override {
        Super::postRender(context);
        newFrame(context);
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_window_flags = 0;
        host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
        host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        host_window_flags |= ImGuiWindowFlags_MenuBar;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("MainDockSpaceViewport", NULL, host_window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                if (ImGui::MenuItem("Open", NULL)) {
                    AN::Dispatch::async(AN::Dispatch::Main, [this, window = context.window]{
                        AN::OpenPanel openPanel;
                        openPanel.init();
                        openPanel.allowOtherTypes = true;
                        static const char *audioDescription = "Supported Audio File";
                        openPanel.addAllowContentExtension(audioDescription, "wav");
                        openPanel.addAllowContentExtension(audioDescription, "mp3");
                        openPanel.addAllowContentExtension(audioDescription, "flac");
                        openPanel.setTitle("Choose Wav File");
                        openPanel.beginSheetModal(window, [this](AN::Application::ModalResponse response, const char *filePath) {
                            if (response == AN::Application::ModalResponse::Ok && filePath) {


                            }
                        });
                    });
                }
                ImGui::Separator();

                ImGui::EndMenu();
            }
            HelpMarker(
                    "When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n"
                    "- Drag from window title bar or their tab to dock/undock." "\n"
                    "- Drag from window menu button (upper-left button) to undock an entire node (all windows)." "\n"
                    "- Hold SHIFT to disable docking (if io.ConfigDockingWithShift == false, default)" "\n"
                    "- Hold SHIFT to enable docking (if io.ConfigDockingWithShift == true)" "\n"
                    "This demo app has nothing to do with enabling docking!" "\n\n"
                    "This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window." "\n\n"
                    "Read comments in ShowExampleAppDockSpace() for more details.");

            ImGui::EndMenuBar();
        }
        ImGui::End();

        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        if (show_main_window) {
            ImGui::Begin("Main Window", &show_main_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f * context.deltaTime, 1.f / context.deltaTime);

            static const char* canSelectFPS[] = { "60", "120", "140", "200", "300", "400", "INF"};
            if (ImGui::Combo("Set FPS", &selected_fps, canSelectFPS, std::size(canSelectFPS))) {
                AN::Dispatch::async(AN::Dispatch::Game, [=, index = selected_fps]{
                    int fps;
                    switch (index) {
                        case 0:
                            fps = 60;
                            break;
                        case 1:
                            fps = 120;
                            break;
                        case 2:
                            fps = 140;
                            break;
                        case 3:
                            fps = 200;
                            break;
                        case 4:
                            fps = 300;
                            break;
                        case 5:
                            fps = 400;
                            break;
                        default:
                            fps = INT_MAX;
                    }
                    AN::GetGame().setMaxFrameRate(fps);
                });
            }

            ImGui::Separator();

            ImGui::Text(std::format("{:%T}", currentTime).c_str());

            ImGui::End();
        }

        endFrame(context);
    }
};
class MainNode : public AN::Node {
    typedef MainNode Self;

public:
    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }



    virtual bool init() override {
        Node::init();
        auto node = ImguiNode::Alloc();
        node->init();
        addChild(node);
        return true;
    }

};


struct AppDelegate {

    std::unique_ptr<AN::Window> mainWindow;

    void bind(AN::Application *application) {
        application->didFinishLaunching.bind(this, &AppDelegate::applicationDidFinishLaunching);
        application->shouldTerminateAfterLastWindowClosed = AppDelegate::applicationShouldTerminateAfterLastWindowClosed;
        application->willTerminate.bind(this, &AppDelegate::applicationWillTerminate);
    }


    static bool applicationShouldTerminateAfterLastWindowClosed(AN::Application *application) {
        return true;
    }

    void applicationDidFinishLaunching(AN::Application *application) {
        mainWindow = std::make_unique<AN::Window>();
        auto screenSize = AN::GetDefaultScreenSize();
        mainWindow->init({0, 0, screenSize.width * 0.8f, screenSize.height * 0.8f });
        mainWindow->setTitle("audio-test");
        mainWindow->center();
        mainWindow->makeCurrentContext();
        mainWindow->makeKeyAndOrderFront();

        auto node = MainNode::Alloc();
        AN::GetGame().entryNode = node;
        AN::GetGame().setMaxFrameRate(144);
        AN::GetConfiguration().setObject("forward-shading", true);
        AN::GetConfiguration().setObject("anti-aliasing", "NONE");
    }

    void applicationWillTerminate(AN::Application *application) {
        mainWindow = nullptr;
    }


};

int main(int argc, const char * argv[]) {

    AN::Application &app = AN::Application::GetSharedApplication();
    AppDelegate appDelegate;

    appDelegate.bind(&app);

    try {
        app.run();

    } catch (const std::exception &exception) {

        printf("Exception: %s\n", exception.what());

#ifdef _WIN32
        MessageBoxA(nullptr, exception.what(), "Exception", MB_OK|MB_ICONERROR);
#endif

    }


    return 0;
}