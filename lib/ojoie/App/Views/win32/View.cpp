//
// Created by aojoie on 6/18/2023.
//

#include "App/Views/win32/View.hpp"
#include "../../../Core/win32/Resources/resource.h"

#include "Core/private/win32/App.hpp"
#include "Utility/Log.h"

extern HINSTANCE gOJOIEHInstance;
extern HINSTANCE gHInstance;

namespace AN::WIN {


View::View() : hWnd() {}

static const wchar_t *kViewClassName = L"ojoie.view";


static LRESULT CALLBACK WindowProcStub(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    View *const view = (View *) (GetWindowLongPtr(hWnd, GWLP_USERDATA));
    return view->handleMessageInternal(hWnd, msg, wParam, lParam);
}

static LRESULT CALLBACK WindowProcSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        const CREATESTRUCT *const create = (CREATESTRUCT *) (lParam);
        View *const view           = static_cast<View *>(create->lpCreateParams);

        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) (view));
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) (WindowProcStub));

        return view->handleMessageInternal(hWnd, msg, wParam, lParam);
    }

    //handle msg pass before WM_NCCREATE
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void View::RegisterWindowClass() {
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProcSetup;
    wc.hInstance = gHInstance;
    wc.hIcon = LoadIcon(gOJOIEHInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = kViewClassName;
    RegisterClass(&wc);
}

void View::UnregisterWindowClass() {
    if (!UnregisterClass(kViewClassName, gHInstance)) {
        AN_LOG(Error, "cannot unregister window class : %s", GetLastErrorString().c_str());
    }
}

bool View::init(const Rect &frame) {

    hWnd = CreateWindowExW(WS_EX_LAYERED,
                           kViewClassName,
                           nullptr,
                           WS_POPUP,
                           frame.x(), frame.y(), frame.width(), frame.height(),
                           nullptr, nullptr, gHInstance, this);

    if (hWnd == nullptr) {
        MessageBoxW(nullptr, L"Fail to create window", L"ojoie", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    return true;
}

LRESULT View::handleMessageInternal(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

Rect View::getFrame() {
    RECT rect{};
    GetClientRect(hWnd, &rect);

    Rect ret{};
    ret.size.width  = rect.right - rect.left;
    ret.size.height = rect.bottom - rect.top;

    return ret;
}

View::~View() {
    if (hWnd) {
        DestroyWindow(hWnd);
        hWnd = nullptr;
    }
}

}