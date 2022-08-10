//
// Created by Aleudillonam on 7/26/2022.
//

#include <glad/glad.h>
#include "Core/Window.hpp"
#include "Core/Dispatch.hpp"
#include "Core/Game.hpp"
#include "Core/private/App.hpp"


#include "Render/RenderQueue.hpp"
#include "Render/Renderer.hpp"

#include "Input/InputManager.hpp"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
#include <GLFW/glfw3.h>

namespace AN {


void glfwSetWindowCenter(GLFWwindow* window) {
    // Get window position and size
    int window_x, window_y;
    glfwGetWindowPos(window, &window_x, &window_y);

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    // Halve the window size and use it to adjust the window position to the center of the window
    window_width *= 0.5;
    window_height *= 0.5;

    window_x += window_width;
    window_y += window_height;

    // Get the list of monitors
    int monitors_length;
    GLFWmonitor **monitors = glfwGetMonitors(&monitors_length);

    if(monitors == nullptr) {
        // Got no monitors back
        return;
    }

    // Figure out which monitor the window is in
    GLFWmonitor *owner = nullptr;
    int owner_x, owner_y, owner_width, owner_height;

    for(int i = 0; i < monitors_length; i++) {
        // Get the monitor position
        int monitor_x, monitor_y;
        glfwGetMonitorPos(monitors[i], &monitor_x, &monitor_y);

        // Get the monitor size from its video mode
        int monitor_width, monitor_height;
        GLFWvidmode *monitor_vidmode = (GLFWvidmode*) glfwGetVideoMode(monitors[i]);

        if(monitor_vidmode == nullptr) {
            // Video mode is required for width and height, so skip this monitor
            continue;
        }
        monitor_width = monitor_vidmode->width;
        monitor_height = monitor_vidmode->height;


        // Set the owner to this monitor if the center of the window is within its bounding box
        if((window_x > monitor_x && window_x < (monitor_x + monitor_width)) && (window_y > monitor_y && window_y < (monitor_y + monitor_height))) {
            owner = monitors[i];

            owner_x = monitor_x;
            owner_y = monitor_y;

            owner_width = monitor_width;
            owner_height = monitor_height;
        }
    }

    if(owner != nullptr) {
        // Set the window position to the center of the owner monitor
        glfwSetWindowPos(window, owner_x + (owner_width * 0.5) - window_width, owner_y + (owner_height * 0.5) - window_height);
    }
}

struct Window::Impl {
    GLFWwindow *glfwWindow;
};

Window::Window() : impl(new Impl()) {}

Window::~Window() {
    if (GetRenderQueue().isRunning()) {
        if (GetRenderer().currentWindow == this) {
            GetRenderQueue().enqueue([this] {
                Window *expected = this;
                GetRenderer().currentWindow.compare_exchange_strong(expected, nullptr, std::memory_order_relaxed);
            });
            RenderFence fence;
            fence.wait();
        }
    }

    /// input
    if (GetInputManager().getCurrentWindow() == this) {
        GetInputManager().setCurrentWindow(nullptr);
    }


    if (impl->glfwWindow) {
        glfwDestroyWindow(impl->glfwWindow);
    }
    delete impl;
    std::erase(App->impl->windows, this);
}


bool Window::init(const Rect &rect) {
    App->impl->windows.push_back(this);
    impl->glfwWindow = glfwCreateWindow((int)rect.width(), (int)rect.height(), "", nullptr, nullptr);
    if (!impl->glfwWindow) {
        return false;
    }
    glfwSetWindowPos(impl->glfwWindow, (int)rect.x(), (int)rect.y());
    glfwSetWindowUserPointer(impl->glfwWindow, this);
    glfwSetWindowCloseCallback(impl->glfwWindow, [](GLFWwindow* window) {
        Window *self = (Window *)glfwGetWindowUserPointer(window);
        self->orderOut();
    });
    glfwSetFramebufferSizeCallback(impl->glfwWindow, [](GLFWwindow* window, int width, int height) {
        Window *self = (Window *)glfwGetWindowUserPointer(window);
        int x, y;
        glfwGetWindowPos(window, &x, &y);
        self->_didResize({ (double)x, (double)y, (double)width, (double)height });
    });

    return true;
}

void Window::_didResize(const Rect &frame) {
    if (didResize) {
        didResize(frame);
    }
    if (GetRenderer().currentWindow == this) {
        int frameWidth, frameHeight;
        glfwGetFramebufferSize(impl->glfwWindow, &frameWidth, &frameHeight);
#ifdef _WIN32
        HWND hWnd = glfwGetWin32Window(impl->glfwWindow);
        HDC hdc     = GetDC(hWnd);
        float dpiScaleX = (float)GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
        float dpiScaleY = (float)GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
        ReleaseDC(hWnd, hdc);
#endif
        Dispatch::async(Dispatch::Game, [=, this] {
            GetRenderQueue().enqueue([=, this] {
                if (GetRenderer().currentWindow == this) {
//                    GetRenderer().currentWindow.store(this, std::memory_order_relaxed);
                    GetRenderer().renderContext.frameWidth = (float)frameWidth;
                    GetRenderer().renderContext.frameHeight = (float)frameHeight;
                    GetRenderer().renderContext.windowWidth = (float)frame.width();
                    GetRenderer().renderContext.windowHeight = (float)frame.height();

                    GetRenderer().renderContext.dpiScaleX = dpiScaleX;
                    GetRenderer().renderContext.dpiScaleY = dpiScaleY;
                }
            });
        });
    }
}

void Window::setFrame(const Rect &frame) {
    glfwSetWindowPos(impl->glfwWindow, (int)frame.x(), (int)frame.y());
    glfwSetWindowSize(impl->glfwWindow, (int)frame.width(), (int)frame.height());
}

Rect Window::getFrame() {
    int x, y, width, height;
    glfwGetWindowPos(impl->glfwWindow, &x, &y);
    glfwGetWindowSize(impl->glfwWindow, &width, &height);
    return { (double)x, (double)y, (double)width, (double)height };
}

void Window::setTitle(const char *title) {
    glfwSetWindowTitle(impl->glfwWindow, title);
}

void Window::orderOut() {
    glfwHideWindow(impl->glfwWindow);
    --App->impl->numOfVisibleWindows;
}

void Window::orderFront() {
    glfwShowWindow(impl->glfwWindow);
    ++App->impl->numOfVisibleWindows;
    App->impl->frontWindow = this;
}

void Window::makeKey() {
    glfwFocusWindow(impl->glfwWindow);
}

void Window::center() {
    glfwSetWindowCenter(impl->glfwWindow);
}

void Window::makeCurrentContext() {

    auto task = [this] {
        Rect frame = getFrame();
        int frameWidth, frameHeight;
        glfwGetFramebufferSize(impl->glfwWindow, &frameWidth, &frameHeight);

#ifdef _WIN32
        HWND hWnd = glfwGetWin32Window(impl->glfwWindow);
        HDC hdc     = GetDC(hWnd);
        float dpiScaleX = (float)GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
        float dpiScaleY = (float)GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
        ReleaseDC(hWnd, hdc);
#endif

        auto renderQueueTask = [=, cursorState = _cursorState] {
            glfwMakeContextCurrent(impl->glfwWindow);
            glfwSwapInterval(0);
            if (!gladLoadGL()) {
                Dispatch::async(Dispatch::Main, []{
                    throw Exception("GLAD Load fail");
                });
            }
            GetRenderer().currentWindow.store(this, std::memory_order_relaxed);
            GetRenderer().renderContext.frameWidth = (float)frameWidth;
            GetRenderer().renderContext.frameHeight = (float)frameHeight;
            GetRenderer().renderContext.windowWidth = (float)frame.width();
            GetRenderer().renderContext.windowHeight = (float)frame.height();
            GetRenderer().renderContext.cursorState = cursorState;


            GetRenderer().renderContext.dpiScaleX = dpiScaleX;
            GetRenderer().renderContext.dpiScaleY = dpiScaleY;

        };

        auto gameTask = [=] {
            GetGame().width = frameWidth;
            GetGame().height = frameHeight;
        };


        if (std::this_thread::get_id() == Dispatch::GetThreadID(Dispatch::Game) ||
            !Dispatch::isRunning(Dispatch::Game)) {

            GetRenderQueue().enqueue(renderQueueTask);
            gameTask();

        } else {
            Dispatch::async(Dispatch::Game, [=, this] {
                GetRenderQueue().enqueue(renderQueueTask);
                gameTask();
            });
        }


        /// input
        GetInputManager().setCurrentWindow(this);
    };

   if (std::this_thread::get_id() == Dispatch::GetThreadID(Dispatch::Main)) {
       task();
   } else {
       Dispatch::async(Dispatch::Main, task);
   }

}

void *Window::getUnderlyingWindow() {
    return impl->glfwWindow;
}


void Window::setCursorState(CursorState state) {
    _cursorState = state;

    if (GetRenderer().currentWindow == this) {
        Dispatch::async(Dispatch::Game, [state, this] {
            GetRenderQueue().enqueue([state, this] {
                if (GetRenderer().currentWindow == this) {
                    GetRenderer().renderContext.cursorState = state;
                }
            });
        });
    }

    switch (state) {
        case CursorState::Normal:
            glfwSetInputMode(impl->glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            break;
        case CursorState::Hidden:
            glfwSetInputMode(impl->glfwWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            break;
        case CursorState::Disabled:
            glfwSetInputMode(impl->glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwRawMouseMotionSupported()) {
                glfwSetInputMode(impl->glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            }
            break;
    }
}

CursorState Cursor::getState() {
    return App->getFrontWindow()->getCursorState();
}

void Cursor::setState(CursorState state) {
    App->getFrontWindow()->setCursorState(state);
}


}