//
// Created by aojoie on 4/22/2023.
//

#ifndef OJOIE_WIN32_APP_H
#define OJOIE_WIN32_APP_H

#include <ojoie/Core/App.hpp>
#include "concurrentqueue/concurrentqueue.hpp"
#include <ojoie/Threads/Task.hpp>
#include <unordered_set>

#include <Windows.h>
#include <Shobjidl.h>
#include <wrl/client.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.ViewManagement.h>

namespace AN::WIN {

using Microsoft::WRL::ComPtr;

class Window;

class Application : public AN::Application {

    bool bActive;
    std::atomic_bool _running;
    DarkMode _darkMode;
    winrt::Windows::UI::ViewManagement::UISettings uiSettings;

    bool isDarkMode();

public:

    Application();

    virtual bool pollEvent() override;

    virtual void run() override;

    virtual void run(int argc, const char *argv[]) override;

    virtual void terminate() override;

    virtual AN::Window *getMainWindow() override;

    virtual void showAboutWindow() override;

    virtual void setDarkMode(DarkMode mode) override;

    virtual DarkMode getDarkMode() const override { return _darkMode; }

    virtual void messageBox(const char *title, const char *message, MessageBoxStyleFlag flags, AN::Window *window = nullptr) override;

    void setActiveInternal(bool active) { bActive = active; }
};



class OpenPanel : public AN::SavePanel {

protected:

    ComPtr<IFileDialog> pFileDialog;

    bool allowOtherTypes{};

    std::map<std::string, std::vector<std::string>> allowContentExtension;

    std::vector<std::string> extensionList;

    std::string fileName;
    std::string defaultExtension;
    std::vector<COMDLG_FILTERSPEC> fileTypes;

    void processFilter();

public:

    virtual bool init() override;

    virtual void setAllowOtherTypes(bool val) override { allowOtherTypes = val; }

    virtual void addAllowContentExtension(const char *description, const char *extension) override {
        allowContentExtension[description].emplace_back(extension);
    }

    virtual void beginSheetModal(AN::Window *window, OpenPanelCallback completionHandler, void *userdata) override;

    /// \brief note that some platform have no effects
    virtual void setTitle(const char *title) override;

    virtual void        setFileName(const char *name) override;
    virtual const char *getFileName() override;

    virtual void setDefaultExtension(const char *extension) override { defaultExtension = extension; }

    virtual const char *getExtension() override;
};

class SavePanel : public OpenPanel {

public:

    virtual bool init() override;

};


std::wstring TranslateErrorCodeW(HRESULT hr) noexcept;
std::string TranslateErrorCode(HRESULT hr) noexcept;

std::string GetLastErrorString() noexcept;
std::wstring GetLastErrorStringW() noexcept;

}

#endif//OJOIE_WIN32_APP_H
