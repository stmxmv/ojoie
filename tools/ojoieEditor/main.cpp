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
#include <ojoie/Node/StaticModelNode.hpp>
#include <ojoie/Node/CameraNode.hpp>
#include <ojoie/Input/InputManager.hpp>


#include "Panel/DemoPanel.hpp"
#include "Panel/Settings.hpp"
#include "Panel/DockSpace.hpp"
#include "Panel/ViewportPanel.hpp"
#include "Panel/SceneHierarchyPanel.hpp"

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

template<typename _ANObject, typename ..._Args>
inline static auto AllocInit(_Args &&...args) -> decltype(_ANObject::Alloc()) {
    auto object = _ANObject::Alloc();
    if (object->init(std::forward<_Args>(args)...)) {
        return object;
    }
    return nullptr;
}

using std::cin, std::cout, std::endl;


class ImguiNode : public AN::ImguiNode {
    typedef ImguiNode Self;
    typedef AN::ImguiNode Super;

    std::vector<std::shared_ptr<AN::Editor::Panel>> windows;

public:
    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    ImguiNode() {
        tick = true;
    }

    virtual bool init() override {
        Super::init();

        auto dockSpace = AllocInit<AN::Editor::DockSpace>();
        auto viewPortWindow = AllocInit<AN::Editor::ViewportPanel>();
        auto settingsWindow = AllocInit<AN::Editor::Settings>();
        auto demoWindow = AllocInit<AN::Editor::DemoPanel>();
        auto sceneHierachyPanel = AllocInit<AN::Editor::SceneHierarchyPanel>();
        if (dockSpace && viewPortWindow && demoWindow && settingsWindow) {
            windows.push_back(dockSpace);
//            windows.push_back(settingsWindow);
            windows.push_back(viewPortWindow);
            windows.push_back(demoWindow);
            windows.push_back(sceneHierachyPanel);


            sceneHierachyPanel->setRootNode(AN::GetGame().entryNode);
            return true;
        }
        return false;
    }

    virtual void update(float deltaTime) override {
        Super::update(deltaTime);
        for (auto &window : windows) {
            window->update(deltaTime);
        }
    }

    virtual void postRender(const AN::RenderContext &context) override {
        Super::postRender(context);
        newFrame(context);

        for (auto &window : windows) {
            window->draw(context);
        }

        endFrame(context);
    }
};
class MainNode : public AN::Node {
    typedef MainNode Self;
    typedef AN::Node Super;

    std::shared_ptr<AN::CameraNode> camera;

public:
    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual bool init() override {
        Super::init();

        camera = AN::CameraNode::Alloc();
        if (!camera->init()) {
            return false;
        }

        camera->setPosition({ 0.f, 10.f, 13.f });

        addChild(camera);

        auto model = AN::StaticModelNode::Alloc();

        /// ./Resources/Models/Sponza/sponza.obj
        if (!model->init("./Resources/Models/qiqi.fbx")) {
            return false;
        }
        model->setPosition({ 0.f, 0.f, 0.f });

        model->setScale({ 10.f, 10.f, 10.f });

        addChild(model);

        auto node = ImguiNode::Alloc();
        node->init();
        addChild(node);

        AN::InputManager &manager = AN::GetInputManager();
        manager.addAxisMapping("__ViewportCameraPitch", AN::InputKey::MouseY, 0.1f);
        manager.addAxisMapping("__ViewportCameraYaw", AN::InputKey::MouseX, 0.1f);

        manager.addAxisMapping("__ViewportMoveForward", AN::InputKey::W, 1.f);
        manager.addAxisMapping("__ViewportMoveForward", AN::InputKey::S, -1.f);

        manager.addAxisMapping("__ViewportMoveRight", AN::InputKey::A, -1.f);
        manager.addAxisMapping("__ViewportMoveRight", AN::InputKey::D, 1.f);

        manager.addActionMapping("__ViewportCharacterMoveEnable", AN::InputKey::MouseRightButton);
        manager.addActionMapping("__ViewportCharacterMoveEnable", AN::InputKey::MouseMiddleButton);

        manager.addActionMapping("__ViewportCenterRotate", AN::InputKey::LeftShift);
        manager.addActionMapping("__ViewportCenterRotate", AN::InputKey::RightShift);
        manager.addActionMapping("__ViewportCenterRotate", AN::InputKey::LeftShift, AN::InputModifierFlags::Shift);
        manager.addActionMapping("__ViewportCenterRotate", AN::InputKey::RightShift, AN::InputModifierFlags::Shift);

        manager.addActionMapping("__ViewportAction", AN::InputKey::MouseLeftButton, AN::InputModifierFlags::Shift);
        manager.addActionMapping("__ViewportAction", AN::InputKey::MouseLeftButton);

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
        mainWindow->setTitle("ojoieEditor");
        mainWindow->center();
        mainWindow->makeCurrentContext();
        mainWindow->makeKeyAndOrderFront();

        auto node = MainNode::Alloc();
        AN::GetGame().entryNode = node;
        AN::GetGame().setMaxFrameRate(144);
        AN::GetConfiguration().setObject("forward-shading", false);
        AN::GetConfiguration().setObject("anti-aliasing", "TAA");
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