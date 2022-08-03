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

class Window : NonCopyable {
    struct Impl;
    Impl *impl;
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

    /// \brief make current context in the render queue
    void makeCurrentContext();

    void setFrame(const Rect &frame);

    void makeKeyAndOrderFront() {
        makeKey();
        orderFront();
    }

    void center();

    void *getUnderlyingWindow();

    Delegate<void(const Rect &frame)> didResize;

};

}
#endif//OJOIE_WINDOW_HPP
