//
// Created by Aleudillonam on 7/26/2022.
//

#ifndef OJOIE_WINDOW_HPP
#define OJOIE_WINDOW_HPP

#include <ojoie/Core/CGTypes.hpp>
#include <ojoie/Object/Object.hpp>
#include <ojoie/Template/RC.hpp>
#include <ojoie/Core/App.hpp>
#include <ojoie/Core/Menu.hpp>
#include <ojoie/App/Views/View.hpp>

namespace AN {

enum CursorState {
    kCursorStateNormal,
    kCursorStateHidden,
    kCursorStateDisabled
};

enum class CursorShape {
    Arrow,
    IBeam,
    CrossHair,
    Hand,
    HResize,
    VResize,
    Custom
};

namespace Cursor {

/// get and set the cursor's state of the main window

AN_API CursorState getState();

AN_API void setState(CursorState state);

AN_API void setShape(CursorShape shape);

AN_API void setShape(const char *name);

AN_API CursorShape getShape();

AN_API Point getPosition();

AN_API void setPosition(const Point &point);

}

class AN_API Window : public RefCounted<Window> {

protected:

    Menu *_menu = nullptr;

    CursorState _cursorState{ kCursorStateNormal };

    friend class Application;

public:

    virtual ~Window() {
        if (_menu) {
            _menu->release();
        }
    }

    /// allocate the window instance
    static Window *Alloc();

    /// setting wantsLayer to true to make it a game window, which can be rendered to,
    /// currently only support one game window
    virtual bool init(const Rect &frame, bool wantsLayer = true) = 0;

    virtual void setTitle(const char *title) = 0;

    virtual void orderOut() = 0;

    virtual void orderFront() = 0;

    virtual void makeKey() = 0;

    virtual bool isVisible() { return true; }

    virtual void setFrame(const Rect &frame) = 0;

    virtual Rect getFrame() = 0;

    void makeKeyAndOrderFront() {
        makeKey();
        orderFront();
    }

    virtual void center() = 0;

    CursorState getCursorState() const { return _cursorState; }

    virtual CursorShape getCursorShape() const = 0;

    virtual void setCursorState(CursorState state) = 0;

    virtual void setCursorShape(CursorShape shape) = 0;

    /// set cursor using resource name
    virtual void setCursorShape(const char *name) = 0;
    virtual Point getCursorPosition() = 0;
    virtual  void setCursorPosition(const Point &point) = 0;

    virtual float getDPIScaleX() { return 1.f; }
    virtual float getDPIScaleY() { return 1.f; }

    virtual void setBorderLessStyle() = 0;
    virtual void setFullScreen(bool fullScreen) = 0;
    virtual bool isFullScreen() = 0;

    virtual bool isZoomed() const = 0;
    virtual void zoom() = 0;

    virtual void setMenu(Menu *menu) {
        _menu = menu;
        if (_menu) {
            _menu->retain();
        }
    }

    virtual void setIMEInput(bool visible, UInt32 x, UInt32 y) = 0;

    virtual void addSubView(View *view) = 0;
};

}
#endif//OJOIE_WINDOW_HPP
