//
// Created by aojoie on 6/24/2023.
//

#pragma once
#include <ojoie/Core/App.hpp>
#include <ojoie/Core/Window.hpp>
#include <ojoie/Core/Menu.hpp>
#include <ojoie/Template/RC.hpp>
#include <ojoie/Utility/Timer.hpp>

namespace AN::Editor {

class AppDelegate : public AN::ApplicationDelegate {

    AN::RefCountedPtr<AN::Window> mainWindow;
    AN::RefCountedPtr<AN::Menu> mainMenu;
    AN::RefCountedPtr<AN::Menu> appMenu;
    AN::Size size;

    AN::Window *splashWindow;

    Timer splashTimer;

public:

    virtual bool applicationShouldTerminateAfterLastWindowClosed(AN::Application *application) override {
        return true;
    }

    virtual void applicationWillFinishLaunching(Application *application) override;

    virtual void applicationDidFinishLaunching(AN::Application *application) override;

    virtual void gameStart(Game &game) override;

    virtual void gameSetup(AN::Game &game) override;

    virtual void applicationWillTerminate(AN::Application *application) override;

    AN::Window *getMainWindow() const { return mainWindow.get(); }
};


inline AppDelegate &GetAppDelegate() {
    return (AppDelegate &)*App->getDelegate();
}

inline AN::Window *GetMainWindow() {
    return GetAppDelegate().getMainWindow();
}

}
