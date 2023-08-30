//
// Created by Aleudillonam on 7/26/2022.
//

#ifndef OJOIE_APP_HPP
#define OJOIE_APP_HPP


#include "ojoie/Configuration/typedef.h"
#include <ojoie/Template/CompletionHandler.hpp>
#include <ojoie/Template/RC.hpp>

#include <vector>
#include <string>
#include <map>
#include <any>

namespace AN {

class Application;

class ApplicationDelegate : public RefCounted<ApplicationDelegate> {
public:

    virtual ~ApplicationDelegate() = default;

    virtual void applicationWillFinishLaunching(Application *application) {}

    virtual void applicationDidFinishLaunching(Application *application) {}

    virtual bool applicationShouldTerminateAfterLastWindowClosed(Application *application) { return false; }

    virtual void applicationWillTerminate(Application *application) {}

    virtual bool gameShouldPauseWhenNotActive(class Game &game) { return true; }

    /// override this handy method to perform game setup in the main thread, before game start
    virtual void gameSetup(class Game &game) {}

    virtual void gameStart(class Game &game) {}

    virtual void gameStop(class Game &game) {}
};

enum ModalResponse {
    kModalResponseCancel,
    kModalResponseOk,
};

enum MessageBoxStyleFlagBits {
    kMessageBoxStyleDefault = 0,
    kMessageBoxStyleError   = 1 << 0
};

typedef ANFlags MessageBoxStyleFlag;

enum DarkMode {
    kLightMode,
    kDarkMode,
    kAutoDarkMode
};

class AN_API Application : private NonCopyable {

protected:

    std::string appName;
    std::string appVersion;

    RefCountedPtr<ApplicationDelegate> _appDelegate;

    virtual ~Application() = default;

    friend class Window;
public:

    static Application &GetSharedApplication();

    Application();

    void setName(const char *aName);
    void setVersion(const char *aVersion);

    const char *getName();
    const char *getVersion();

    virtual Window *getMainWindow() = 0;

    virtual bool pollEvent() = 0;

    virtual void messageBox(const char *title, const char *message, MessageBoxStyleFlag flags, Window *window = nullptr) = 0;

    virtual void run();

    virtual void run(int argc, const char *argv[]);

    /// \AnyActor
    virtual void terminate() = 0;

    virtual void setDarkMode(DarkMode mode) {}

    virtual DarkMode getDarkMode() const { return kLightMode; }

    virtual void showAboutWindow() {}

    ApplicationDelegate *getDelegate() const { return _appDelegate.get(); }

    void setDelegate(ApplicationDelegate *delegate) { _appDelegate = delegate; }

    /// for now command line args api is for internal use
    template<typename T>
    T getCommandLineArg(const char *name) {
        return std::any_cast<T>(getCommandLineArg(name));
    }

    std::any getCommandLineArg(const char *name);
};

extern AN_API Application *App;

class Window;

typedef void (*OpenPanelCallback)(ModalResponse, const char *, void *userdata);


class AN_API OpenPanel : public RefCounted<OpenPanel> {

public:

    static OpenPanel *Alloc();

    virtual ~OpenPanel() = default;

    virtual bool init() = 0;

    virtual void setAllowOtherTypes(bool val) = 0;

    virtual void addAllowContentExtension(const char *description, const char *extension) = 0;

    virtual void setTitle(const char *title) = 0;

    virtual void setFileName(const char *name) = 0;

    virtual const char *getFileName() = 0;

    virtual void setDefaultExtension(const char *extension) = 0;

    virtual const char *getExtension() = 0;

    virtual void beginSheetModal(Window *window, OpenPanelCallback completionHandler, void *userdata) = 0;

    template<typename Func>
    void beginSheetModal(Window *window, Func &&func) {
        beginSheetModal(window, [](ModalResponse res, const char *path, void *userdata) {
                    Func *f = (Func *)userdata;
                    (*f)(res, path);
                }, &func);
    }
};

class AN_API SavePanel : public OpenPanel {
public:

    static SavePanel *Alloc();

};



}

#endif //OJOIE_APP_HPP
