//
// Created by Aleudillonam on 7/26/2022.
//

#ifndef OJOIE_WINDOW_HPP
#define OJOIE_WINDOW_HPP

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

enum class CursorState {
    Normal,
    Hidden,
    Disabled
};


namespace Cursor {

/// get and set the cursor's state of the front window

CursorState getState();

void setState(CursorState state);

}

class Window : NonCopyable {
    struct Impl;
    Impl *impl;

    CursorState _cursorState{ CursorState::Normal };

    void _didResize(const Rect &frame);

    friend class Application;
public:
    /// \brief construct a null like window
    Window();

    virtual ~Window();

    virtual bool init(const Rect &frame);

    void setTitle(const char *title);

    void orderOut();

    void orderFront();

    void makeKey();

    /// \brief make current context in the render queue, only this method can
    ///        be called on any thread, others must be called on main thread
    void makeCurrentContext();

    void setFrame(const Rect &frame);

    Rect getFrame();

    void makeKeyAndOrderFront() {
        makeKey();
        orderFront();
    }

    void center();

    void *getUnderlyingWindow();

    CursorState getCursorState() const {
        return _cursorState;
    }

    void setCursorState(CursorState state);

    Delegate<void(const Rect &frame)> didResize;

};

}
#endif//OJOIE_WINDOW_HPP
