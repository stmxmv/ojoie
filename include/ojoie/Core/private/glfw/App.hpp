//
// Created by Aleudillonam on 7/26/2022.
//

#ifndef OJOIE_APP_HPP
#define OJOIE_APP_HPP


#include "ojoie/Configuration/typedef.h"
#include <ojoie/Template/CompletionHandler.hpp>
#include <ojoie/Template/delegate.hpp>

#include <vector>
#include <string>
#include <map>

namespace AN {


class Application : private NonCopyable {
    struct Impl;
    Impl *impl;

protected:

    Application();
    ~Application();

    friend class Window;
public:

    enum class ModalResponse {
        Cancel,
        Ok,
    };

    static Application &GetSharedApplication();

    Window *getFrontWindow();

    virtual void run();

    void pollEvent();

    /// \AnyActor
    virtual void terminate();

    Delegate<void(Application *application)> didFinishLaunching;

    Delegate<bool(Application *application)> shouldTerminateAfterLastWindowClosed;

    Delegate<void(Application *application)> willTerminate;

};

extern AN_EXPORT Application *App;

int GetDefaultScreenRefreshRate();

class Window;


class OpenPanel : private NonCopyable {
    struct Impl;
    Impl *impl;
    std::shared_ptr<CompletionInterface<Application::ModalResponse, const char *>> _completionHandler;

    std::map<std::string, std::vector<std::string>> allowContentExtension;

    void _beginSheetModal(Window *window, const std::shared_ptr<CompletionInterface<Application::ModalResponse, const char *>> &completionHandler);
public:

    OpenPanel();

    ~OpenPanel();

    virtual bool init();

    bool allowOtherTypes{};

    void addAllowContentExtension(const char *description, const char *extension) {
        allowContentExtension[description].emplace_back(extension);
    }

    template<typename Func>
    void beginSheetModal(Window *window, Func &&func) {
        _beginSheetModal(window, AllocateCompletionHandler(std::forward<Func>(func)));
    }

    /// \brief note that some platform have no effects
    void setTitle(const char *title);
};



}

#endif //OJOIE_APP_HPP
