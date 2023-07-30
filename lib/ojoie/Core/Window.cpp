//
// Created by Aleudillonam on 7/26/2022.
//

#include "Core/Window.hpp"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <ojoie/Core/private/win32/Window.hpp>
#endif

namespace AN {


Window *Window::Alloc() {
    return new WIN::Window();
}

CursorState Cursor::getState() {
    return App->getMainWindow()->getCursorState();
}

void Cursor::setState(CursorState state) {
    App->getMainWindow()->setCursorState(state);
}

void Cursor::setShape(CursorShape shape) {
    App->getMainWindow()->setCursorShape(shape);
}

void Cursor::setShape(const char *name) {
    App->getMainWindow()->setCursorShape(name);
}

CursorShape Cursor::getShape() {
    return App->getMainWindow()->getCursorShape();
}

}