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
    if (App->getMainWindow() == nullptr) return kCursorStateNormal;
    return App->getMainWindow()->getCursorState();
}

void Cursor::setState(CursorState state) {
    if (App->getMainWindow() == nullptr) return;
    App->getMainWindow()->setCursorState(state);
}

void Cursor::setShape(CursorShape shape) {
    if (App->getMainWindow() == nullptr) return;
    App->getMainWindow()->setCursorShape(shape);
}

void Cursor::setShape(const char *name) {
    if (App->getMainWindow() == nullptr) return;
    App->getMainWindow()->setCursorShape(name);
}

CursorShape Cursor::getShape() {
    if (App->getMainWindow() == nullptr) return CursorShape::Arrow;
    return App->getMainWindow()->getCursorShape();
}

Point Cursor::getPosition() {
    if (App->getMainWindow() == nullptr) return {};
    return App->getMainWindow()->getCursorPosition();
}

void Cursor::setPosition(const Point &point) {
    if (App->getMainWindow() == nullptr) return;
    return App->getMainWindow()->setCursorPosition(point);
}

}