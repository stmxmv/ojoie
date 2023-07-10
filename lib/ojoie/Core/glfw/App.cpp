//
// Created by Aleudillonam on 7/26/2022.
//
#include "Core/App.hpp"
#include "Threads/Dispatch.hpp"
#include "Core/Exception.hpp"
#include "Core/private/App.hpp"

#include "Core/Game.hpp"

#include <format>


#include <GLFW/glfw3.h>


#ifdef _WIN32
#include <Windows.h>
#include <ojoie/Core/private/win32/App.hpp>
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

namespace AN {

AN_EXPORT Application *App = nullptr;

Application &Application::GetSharedApplication() {
#ifdef _WIN32
    static WIN::Application app;
#else
#error "not implement"
#endif
    return app;
}

Application::Application() : impl(new Impl()) {
    if (!App) {
        App = this;
        Dispatch::SetThreadID(Dispatch::Main, GetCurrentThreadID());
        Dispatch::GetDelegate()[Dispatch::Main] = [] (const TaskInterface &task) {
            App->impl->dispatchTasks.enqueue(task);
            glfwPostEmptyEvent();
        };
    } else {
        throw Exception("Executable can only has one AN::Application instance!");
    }
}

Application::~Application() {
    delete impl;
}

void Application::pollEvent() {
    TaskInterface task;
    while (impl->dispatchTasks.try_dequeue(task)) {
        task.run();
    }
    glfwPollEvents();
}

void Application::run() {
    glfwSetErrorCallback(
            [](int error, const char * description) {
                if (error == GLFW_INVALID_VALUE) {
                    /// some keyboard may send invalid key
                    ANLog("%s", std::format("Glfw Error {:x}: {}", error, description).c_str());
                    return;
                }

                auto task = [error, desc = std::string(description)] {
                    throw Exception(std::format("Glfw Error {:x}: {}", error, desc).c_str());
                };

                if (Dispatch::GetThreadID(Dispatch::Main) == GetCurrentThreadID()) {
                    task();
                } else {
                    Dispatch::async(Dispatch::Main, task);
                }

            });
    glfwInit();

#ifdef OJOIE_USE_OPENGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


#ifdef AN_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

#elif defined(OJOIE_USE_VULKAN)

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

#endif

    /// make newly created windows hidden by default
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    Game &game = GetGame();

    game.init();

    if (didFinishLaunching) {
        didFinishLaunching(this);
    }

    game.start();

    moodycamel::ConsumerToken token(impl->dispatchTasks);

    while (!impl->isTerminated) {
        TaskInterface task;
        while (impl->dispatchTasks.try_dequeue(token, task)) {
            task.run();
        }
        glfwWaitEvents();
        if (impl->numOfVisibleWindows == 0) {
            if (shouldTerminateAfterLastWindowClosed && shouldTerminateAfterLastWindowClosed(this)) {
                impl->isTerminated = true;
            }
        }
    }

    game.deinit();

    if (willTerminate) {
        willTerminate(this);
    }



    glfwTerminate();
}

void Application::terminate() {
    impl->isTerminated = true;
    glfwPostEmptyEvent();
}

Window *Application::getFrontWindow() {
    return impl->frontWindow;
}


int GetDefaultScreenRefreshRate() {
    const GLFWvidmode *vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    return vidMode->refreshRate;
}

#ifdef _WIN32

struct OpenPanel::Impl {
    OPENFILENAMEW ofn;
    std::wstring wFilter;
    std::wstring wFileName;
    std::wstring wTitle;
};

OpenPanel::OpenPanel() : impl(new Impl{}) {}

OpenPanel::~OpenPanel() {
    delete impl;
}

bool OpenPanel::init() {

    impl->ofn.lStructSize = sizeof(OPENFILENAME);

    impl->ofn.hInstance = GetModuleHandle(nullptr);
    impl->ofn.lpstrCustomFilter = nullptr;
    impl->ofn.nMaxCustFilter = 0;

    impl->ofn.nMaxFile = MAX_PATH;
    impl->ofn.lpstrFileTitle = nullptr;
    impl->ofn.nMaxFileTitle = 0;
//    impl->ofn.lpstrInitialDir = ;
    impl->ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    impl->ofn.lpstrDefExt = nullptr;
    impl->ofn.lCustData = 0;
    impl->ofn.lpfnHook = nullptr;
    impl->ofn.lpTemplateName = nullptr;

    return true;
}

void OpenPanel::_beginSheetModal(Window *window,
                                 const std::shared_ptr<CompletionInterface<Application::ModalResponse,
                                                                           const char *>> &completionHandler) {

    HWND hWnd = glfwGetWin32Window((GLFWwindow *)window->getUnderlyingWindow());
    impl->ofn.hwndOwner = hWnd;

    std::string filter;


    if (allowOtherTypes) {
        filter.append("All Files");
        filter.push_back('\0');
        filter.append("*.*");
        filter.push_back('\0');

        if (allowContentExtension.empty()) {
            impl->ofn.nFilterIndex = 1;
        } else {
            impl->ofn.nFilterIndex = 2;
        }
    }

    for (auto &&[desc, extensions] : allowContentExtension) {
        filter.append(desc);
        filter.push_back('\0');
        for (int i = 0; i < (int)extensions.size(); ++i) {
            filter.append("*.");
            filter.append(extensions[i]);
            if (i != (int)extensions.size() - 1) {
                filter.push_back(';');
            }
        }
        filter.push_back('\0');
    }
    filter.append("\0");

    impl->wFilter.resize(MultiByteToWideChar(CP_UTF8, 0, filter.c_str(), (int)filter.size(), nullptr, 0));

    MultiByteToWideChar(CP_UTF8, 0, filter.c_str(), (int)filter.size(), impl->wFilter.data(), (int)impl->wFilter.size());

    impl->ofn.lpstrFilter = impl->wFilter.c_str();

    impl->wFileName.resize(256 * MAX_PATH);

    impl->ofn.lpstrFile = impl->wFileName.data();

    if (GetOpenFileNameW(&impl->ofn)) {
        std::string fileName;
        fileName.resize(WideCharToMultiByte(CP_UTF8, 0, impl->wFileName.c_str(), -1, nullptr, 0, nullptr, nullptr));
        WideCharToMultiByte(CP_UTF8, 0, impl->wFileName.c_str(), -1, fileName.data(), (int)fileName.size(), nullptr, nullptr);
        completionHandler->run(Application::ModalResponse::Ok, fileName.c_str());
    } else {
        completionHandler->run(Application::ModalResponse::Cancel, nullptr);
    }

}

void OpenPanel::setTitle(const char *title) {
    impl->wTitle.resize(MultiByteToWideChar(CP_UTF8, 0, title, (int)strlen(title) + 1, nullptr, 0));
    MultiByteToWideChar(CP_UTF8, 0, title, (int)strlen(title) + 1, impl->wTitle.data(), impl->wTitle.size());
    impl->ofn.lpstrTitle = impl->wTitle.c_str();
}


#endif

}