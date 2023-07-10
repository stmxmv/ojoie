//
// Created by Aleudillonam on 7/26/2022.
//

#ifndef OJOIE_WINDOW_HPP
#define OJOIE_WINDOW_HPP

#include <ojoie/BaseClasses/Object.hpp>
#include <ojoie/Template/RC.hpp>
#include <ojoie/Core/App.hpp>

namespace AN {

struct Size {
    double width;
    double height;
};

struct Point {
    double x;
    double y;
};

struct Rect {
    Point origin;
    Size size;
    double width() const { return size.width; }
    double height() const { return size.height; }
    double x() const { return origin.x; }
    double y() const { return origin.y; }
};

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
    VResize
};

namespace Cursor {

/// get and set the cursor's state of the front window



CursorState getState();

void setState(CursorState state);

void setShape(CursorShape shape);

}


Size GetDefaultScreenSize();

class Window : public RefCounted<Window> {
    struct Impl;
    Impl *impl;

protected:

    CursorState _cursorState{ kCursorStateNormal };

    void _didResize(const Rect &frame);

    friend class Application;

public:
    /// \brief construct a null like window
    Window();

    virtual ~Window();

    /// allocate the window instance
    static Window *Alloc();

    virtual bool init(const Rect &frame);

    virtual void setTitle(const char *title);

    virtual void orderOut();

    virtual void orderFront();

    virtual void makeKey();

    virtual bool isVisible() { return true; }

    /// \brief make current context in the render queue, only this method can
    ///        be called on any thread, others must be called on main thread
    void makeCurrentContext();

    virtual void setFrame(const Rect &frame);

    virtual Rect getFrame();

    void makeKeyAndOrderFront() {
        makeKey();
        orderFront();
    }

    virtual void center();

    void *getUnderlyingWindow();

    CursorState getCursorState() const {
        return _cursorState;
    }

    virtual void setCursorState(CursorState state);

    virtual void setCursorShape(CursorShape shape);

    virtual float getDPIScaleX() { return 1.f; }
    virtual float getDPIScaleY() { return 1.f; }

    Delegate<void(const Rect &frame)> didResize;

};

}
#endif//OJOIE_WINDOW_HPP
