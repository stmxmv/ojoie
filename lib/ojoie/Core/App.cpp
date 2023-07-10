//
// Created by Aleudillonam on 7/26/2022.
//
#include "Core/App.hpp"
#include "Threads/Dispatch.hpp"
#include "Core/Exception.hpp"
#include "Core/Game.hpp"

#include <format>
#include <argparse/argparse.hpp>

#ifdef _WIN32
#include <ojoie/Core/private/win32/App.hpp>
#endif


namespace AN {

AN_API Application *App = nullptr;

Application &Application::GetSharedApplication() {
#ifdef _WIN32
    static WIN::Application app;
#else
#error "not implement"
#endif
    return app;
}

Application::Application() : appName("ojoie"), appVersion("0.0.0") {}


static std::unique_ptr<argparse::ArgumentParser> gProgram;

void Application::run(int argc, const char **argv) {

    gProgram = std::make_unique<argparse::ArgumentParser>(getName(), getVersion());

#ifdef OJOIE_WITH_EDITOR
    gProgram->add_argument("--enable-render-doc")
            .help("load RenderDoc for frame capture")
            .default_value(false)
            .implicit_value(true);

    gProgram->add_argument("--project-root")
            .help("ojoie Project Root")
            .required();

#endif//OJOIE_WITH_EDITOR

    try {
        gProgram->parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::string usage;
        messageBox("ojoie", std::format("Command Line Argument Error:\n{}\n\n{}",
                                        err.what(),
                                        gProgram->help().str()).c_str(),
                   kMessageBoxStyleError);
        exit(-1);
    }
}

std::any Application::getCommandLineArg(const char *name) {
    return (*gProgram)[name].getValue();
}

void Application::setName(const char *aName) {
    appName = aName;
}

void Application::setVersion(const char *aVersion) {
    appVersion = aVersion;
}

const char *Application::getName() {
    return appName.c_str();
}

const char *Application::getVersion() {
    return appVersion.c_str();
}


OpenPanel *OpenPanel::Alloc() {
#ifdef _WIN32
    return new WIN::OpenPanel();
#else
#error "not implement"
#endif
}

SavePanel *SavePanel::Alloc() {
#ifdef _WIN32
    return new WIN::SavePanel();
#else
#error "not implement"
#endif
}

#ifdef _WIN32

//struct OpenPanel::Impl {
//    OPENFILENAMEW ofn;
//    std::wstring wFilter;
//    std::wstring wFileName;
//    std::wstring wTitle;
//};
//
//OpenPanel::OpenPanel() : impl(new Impl{}) {}
//
//OpenPanel::~OpenPanel() {
//    delete impl;
//}
//
//bool OpenPanel::init() {
//
//    impl->ofn.lStructSize = sizeof(OPENFILENAME);
//
//    impl->ofn.hInstance = GetModuleHandle(nullptr);
//    impl->ofn.lpstrCustomFilter = nullptr;
//    impl->ofn.nMaxCustFilter = 0;
//
//    impl->ofn.nMaxFile = MAX_PATH;
//    impl->ofn.lpstrFileTitle = nullptr;
//    impl->ofn.nMaxFileTitle = 0;
////    impl->ofn.lpstrInitialDir = ;
//    impl->ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
//    impl->ofn.lpstrDefExt = nullptr;
//    impl->ofn.lCustData = 0;
//    impl->ofn.lpfnHook = nullptr;
//    impl->ofn.lpTemplateName = nullptr;
//
//    return true;
//}
//
//void OpenPanel::_beginSheetModal(Window *window,
//                                 const std::shared_ptr<CompletionInterface<Application::ModalResponse,
//                                                                           const char *>> &completionHandler) {
//
//    HWND hWnd = glfwGetWin32Window((GLFWwindow *)window->getUnderlyingWindow());
//    impl->ofn.hwndOwner = hWnd;
//
//    std::string filter;
//
//
//    if (allowOtherTypes) {
//        filter.append("All Files");
//        filter.push_back('\0');
//        filter.append("*.*");
//        filter.push_back('\0');
//
//        if (allowContentExtension.empty()) {
//            impl->ofn.nFilterIndex = 1;
//        } else {
//            impl->ofn.nFilterIndex = 2;
//        }
//    }
//
//    for (auto &&[desc, extensions] : allowContentExtension) {
//        filter.append(desc);
//        filter.push_back('\0');
//        for (int i = 0; i < (int)extensions.size(); ++i) {
//            filter.append("*.");
//            filter.append(extensions[i]);
//            if (i != (int)extensions.size() - 1) {
//                filter.push_back(';');
//            }
//        }
//        filter.push_back('\0');
//    }
//    filter.append("\0");
//
//    impl->wFilter.resize(MultiByteToWideChar(CP_UTF8, 0, filter.c_str(), (int)filter.size(), nullptr, 0));
//
//    MultiByteToWideChar(CP_UTF8, 0, filter.c_str(), (int)filter.size(), impl->wFilter.data(), (int)impl->wFilter.size());
//
//    impl->ofn.lpstrFilter = impl->wFilter.c_str();
//
//    impl->wFileName.resize(256 * MAX_PATH);
//
//    impl->ofn.lpstrFile = impl->wFileName.data();
//
//    if (GetOpenFileNameW(&impl->ofn)) {
//        std::string fileName;
//        fileName.resize(WideCharToMultiByte(CP_UTF8, 0, impl->wFileName.c_str(), -1, nullptr, 0, nullptr, nullptr));
//        WideCharToMultiByte(CP_UTF8, 0, impl->wFileName.c_str(), -1, fileName.data(), (int)fileName.size(), nullptr, nullptr);
//        completionHandler->run(Application::ModalResponse::Ok, fileName.c_str());
//    } else {
//        completionHandler->run(Application::ModalResponse::Cancel, nullptr);
//    }
//
//}
//
//void OpenPanel::setTitle(const char *title) {
//    impl->wTitle.resize(MultiByteToWideChar(CP_UTF8, 0, title, (int)strlen(title) + 1, nullptr, 0));
//    MultiByteToWideChar(CP_UTF8, 0, title, (int)strlen(title) + 1, impl->wTitle.data(), impl->wTitle.size());
//    impl->ofn.lpstrTitle = impl->wTitle.c_str();
//}


#endif

}