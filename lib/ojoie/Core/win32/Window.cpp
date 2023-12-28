//
// Created by aojoie on 4/22/2023.
//

#include "Core/private/win32/Window.hpp"
#include "Core/private/win32/App.hpp"
#include "Core/private/win32/Menu.hpp"
#include "App/Views/win32/View.hpp"
#include "Render/RenderContext.hpp"
#include "Render/LayerManager.hpp"
#include "Render/RenderManager.hpp"

#include "Core/DragAndDrop.hpp"
#include "Core/Game.hpp"
#include "Input/InputManager.hpp"
#include "Utility/win32/Unicode.hpp"

#include "IMGUI/IMGUIManager.hpp"

#include "DarkMode.h"
#include "UAHMenuBar.h"

#include "Render/private/D3D11/Device.hpp"

#include "Resources/resource.h"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/Layer.hpp"
#endif

#include <windowsx.h>

extern HINSTANCE gOJOIEHInstance;
extern HINSTANCE gHInstance;

namespace AN::WIN {

#define WM_WINDOW_HIDE (WM_USER + 0x0001)

static UINT_PTR modalTimerID = 2783923;


static void CALLBACK ModalTimerProc(HWND handle, UINT message, UINT timerID,
                             DWORD time) {
    App->pollEvent();
    GetGame().performMainLoop();
}

void Win32_EnableAlphaCompositing(HWND hwnd);


static HWND g_MouseCaptureWindow = NULL;
static int  g_MouseCaptureCount  = 0;

static void ResetMouseCapture(void) {
    g_MouseCaptureWindow = NULL;
    g_MouseCaptureCount  = 0;
}

static void SetMouseCapture(HWND window) {
    if (window != g_MouseCaptureWindow) {
        ReleaseCapture();
        ResetMouseCapture();
    }

    if (g_MouseCaptureCount > 0) {
        ++g_MouseCaptureCount;
    } else {
        g_MouseCaptureWindow = window;
        g_MouseCaptureCount  = 1;

        SetCapture(window);
    }
}

static void ReleaseMouseCapture(void) {
    if (0 == --g_MouseCaptureCount) {
        ReleaseCapture();
        ResetMouseCapture();
    }
}

Window::Window() : dropData(), bBridge() {}

static LRESULT CALLBACK WindowProcStub(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window *const window = (Window *) (GetWindowLongPtr(hWnd, GWLP_USERDATA));
    return window->handleMessageInternal(hWnd, msg, wParam, lParam);
}

static LRESULT CALLBACK WindowProcSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        const CREATESTRUCT *const create = (CREATESTRUCT *) (lParam);
        Window *const window           = static_cast<Window *>(create->lpCreateParams);

        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) (window));

        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) (WindowProcStub));

        return window->handleMessageInternal(hWnd, msg, wParam, lParam);
    }

    //handle msg pass before WM_NCCREATE
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static const wchar_t *kWindowClassName = L"ojoie.window";

void Window::RegisterWindowClass() {
    HINSTANCE hInstance = gHInstance;
    /// register window class
    WNDCLASSEXW wcex;
    wcex.cbSize     = sizeof(WNDCLASSEX);
    wcex.style      = CS_OWNDC;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;

    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);//NOLINT
    wcex.hbrBackground = nullptr;

    wcex.hIcon   = LoadIcon(gOJOIEHInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    wcex.hIconSm = LoadIcon(gOJOIEHInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));

    wcex.lpszClassName = kWindowClassName;
    wcex.lpszMenuName  = nullptr;

    wcex.hInstance = hInstance;
    wcex.lpfnWndProc = WindowProcSetup;

    if (!RegisterClassExW(&wcex)) {
        MessageBoxW(nullptr, L"cannot register window class", L"ojoie", MB_ICONERROR | MB_OK);
        ExitProcess(-1);
    }
}

void Window::UnregisterWindowClass() {
    if (!UnregisterClass(kWindowClassName, gHInstance)) {
        AN_LOG(Error, "cannot unregister window class : %s", TranslateErrorCode(GetLastError()).c_str());
    }
}

bool Window::init(const AN::Rect &frame, bool wantsLayer) {
    RECT rect;
    rect.left   = 0;
    rect.right  = (long) frame.width();
    rect.top    = 0;
    rect.bottom = (long) frame.height();

    style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    style |= WS_MAXIMIZEBOX | WS_THICKFRAME; // resizable

    if (AdjustWindowRect(&rect, style, FALSE) <= 0) {
        MessageBoxW(nullptr, TranslateErrorCodeW((HRESULT) GetLastError()).c_str(), L"ojoie", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    hWnd = CreateWindowW(
            L"ojoie.window",
            nullptr,
            style,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr, nullptr, gHInstance, this);// set create param to this

    if (hWnd == nullptr) {
        MessageBoxW(nullptr, L"Fail to create window", L"ojoie", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    retrieveDPIScale();

    /// wantsLayer means this is a game window, currenly only support one
    if (wantsLayer) {
        /// if use vulkan, create win32 vulkan layer
        auto layer = std::unique_ptr<Layer>(Layer::Alloc());

        if (layer) {
            if (!layer->init(this)) {
                return false;
            }

            _layer = std::move(layer);
            /// render layer index can't change during runtime anyway
            GetLayerManager().addLayerInternal(_layer.get());


        } else {
            AN_LOG(Error, "Fail to create window layer");
        }
    }

    dropData = GetDragAndDrop().registerWindowForDrop(this);

    /// disable IME when created
    setIMEInput(false, 0, 0);

//    Win32_EnableAlphaCompositing(hWnd);

    AllowDarkModeForWindow(hWnd, true);
    RefreshTitleBarThemeColor(hWnd);

    return true;
}

void Window::bridge(HWND ahWnd) {
    bBridge = true;
    hWnd = ahWnd;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) this);

    dropData = GetDragAndDrop().registerWindowForDrop(this);

    retrieveDPIScale();

    /// disable IME when created
    setIMEInput(false, 0, 0);
//    Win32_EnableAlphaCompositing(hWnd);

    AllowDarkModeForWindow(hWnd, true);
    RefreshTitleBarThemeColor(hWnd);
    RedrawWindow(hWnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASENOW);
}

void Window::bridge1(HWND ahWnd) {
    bBridge = true;
    hWnd = ahWnd;
    retrieveDPIScale();

    /// disable IME when created
    setIMEInput(false, 0, 0);

    AllowDarkModeForWindow(hWnd, true);
    RefreshTitleBarThemeColor(hWnd);
    RedrawWindow(hWnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASENOW);
}

Window::~Window() {
    if (hWnd) {
        if (dropData) {
            GetDragAndDrop().unregisterWindowForDrop(this, dropData);
        }

        if (_menuAccelerators) {
            DestroyAcceleratorTable(_menuAccelerators);
        }

        if (_layer) {
            GetLayerManager().removeLayerInternal(_layer.get());
            _layer.reset();
        }

        if (!bBridge) {

            /// release all sub views
            EnumChildWindows(hWnd, [](HWND childHwnd, LPARAM lParam) -> BOOL {
                        WIN::View *view = (WIN::View *)GetWindowLongPtr(childHwnd, GWLP_USERDATA);
                        view->release();
                        return TRUE;  // Return TRUE to continue enumerating child windows
            }, 0);

            DestroyWindow(hWnd);
        }

        hWnd = nullptr;
    }
}

void Window::retrieveDPIScale() {
    HDC hdc    = GetDC(hWnd);
    _DPIScaleX = (float) GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    _DPIScaleY = (float) GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
    ReleaseDC(hWnd, hdc);
}

void Window::setTitle(const char *title) {
    std::wstring wTitle = Utf8ToWide(title);
    SetWindowTextW(hWnd, wTitle.c_str());
}

void Window::orderOut() {
    ShowWindow(hWnd, SW_HIDE);
}


static void ShowWindowAndChildren(HWND hWnd, int nCmdShow) {
    // Show the window
    ShowWindow(hWnd, nCmdShow);
    InvalidateRect(hWnd, nullptr, true);
    UpdateWindow(hWnd);

    // Show each child window recursively
    HWND hWndChild = GetTopWindow(hWnd);
    while (hWndChild != NULL) {
        ShowWindowAndChildren(hWndChild, nCmdShow);
        hWndChild = GetNextWindow(hWndChild, GW_HWNDNEXT);
    }
}

void Window::orderFront() {
    ShowWindowAndChildren(hWnd, SW_SHOWNA);
}

void Window::makeKey() {
    SetActiveWindow(hWnd);
}

WNDPROC Window::setWNDPROCCallback(WNDPROC proc) {
    WNDPROC old = _procCallback;
    _procCallback = proc;
    return old;
}

LRESULT Window::handleMessageInternal(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT hr;
    if (IsDarkModeEnabled() && UAHWndProc(hWnd, msg, wParam, lParam, &hr)) {
        return hr;
    }

    if (_procCallback && _procCallback(hWnd, msg, wParam, lParam) == TRUE) {
        return TRUE;
    }

    switch (msg) {
        case WM_COMMAND:
        {
            if (_menu) {
                WIN::Menu *wMenu = (WIN::Menu *)_menu;
                if (wMenu->executeMenuItemWithID(LOWORD(wParam))) {
                    return 0;
                }
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }

        case WM_SIZE:
        {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);

            if (_layer) {
                Size size{ width, height };
                _layer->resize(size);
                GetRenderManager().onLayerSizeChange(_layer->getIndex(), size);
            }
        }
            break;

        case WM_ENTERMENULOOP:
        case WM_ENTERSIZEMOVE:
            SetTimer(hWnd, modalTimerID, USER_TIMER_MINIMUM, (TIMERPROC)ModalTimerProc);
            break;

        case WM_EXITMENULOOP:
        case WM_EXITSIZEMOVE:
            KillTimer(hWnd, modalTimerID);
            break;

        /// not add WM_SIZE, which may cause dead lock
        case WM_PAINT:
        {
            GetGame().performMainLoop();
            ValidateRect(hWnd, nullptr);
        }
            break;

        case WM_ERASEBKGND:
            // do not erase background
            return 1;

        case WM_GETMINMAXINFO:
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 128;
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 128;
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        {
            SetMouseCapture(hWnd);

            if (bActiveInNonClientArea) {
                if (_cursorState == kCursorStateHidden) {
                    showCursor(false);
                }
                if (_cursorState == kCursorStateDisabled) {
                    confineCursor(true);
                    showCursor(false);
                }
                bActiveInNonClientArea = false;
            }
        }
        break;

        case WM_LBUTTONUP:
        {
            ReleaseMouseCapture();
        }
        break;

        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        {
            SetMouseCapture(hWnd);
        }
        break;

        case WM_RBUTTONUP:
        {
            ReleaseMouseCapture();
        }
        break;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        {
            SetMouseCapture(hWnd);
        }
        break;

        case WM_MBUTTONUP:
        {
            ReleaseMouseCapture();
        }
        break;

        case WM_MOUSEMOVE:
        {
            // Get mouse pos directly from the event itself
            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);

            Vector2f position{ p.x, p.y };
            GetInputManager().getMouse().getCurrentFrameState().mousePos = position;
        }
        break;

        case WM_MOUSEWHEEL:
            return 0;

        case WM_CAPTURECHANGED:
        {
            ResetMouseCapture();
        }
        break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            /// we handle in raw keyboard
        }
            break;

        case WM_CLOSE:
            ShowWindow(hWnd, SW_HIDE);
            PostMessage(nullptr, WM_WINDOW_HIDE, 0, 0);
            return 0;

            //        case WM_SETFOCUS:
            //            Cursor::SetCursorVisibility(Cursor::IsCursorVisible());
            //            break;
            //

        case WM_DPICHANGED:
        {
            retrieveDPIScale();
            // Use the suggested new window size.
            RECT *const prcNewWindow  = (RECT *) (lParam);// NOLINT
            int         iWindowX      = prcNewWindow->left;
            int         iWindowY      = prcNewWindow->top;
            int         iWindowWidth  = prcNewWindow->right - prcNewWindow->left;
            int         iWindowHeight = prcNewWindow->bottom - prcNewWindow->top;
            SetWindowPos(hWnd, nullptr, iWindowX, iWindowY, iWindowWidth, iWindowHeight, SWP_NOZORDER | SWP_NOACTIVATE);
            //            Size newSize = getFrameBufferSize();
            //            didResize(newSize);

        }
            break;

        case WM_ACTIVATEAPP:
            /// this message is sent inside PeekMessage or GetMessage, we can't get it right before DispatchMessage
            ((WIN::Application *)App)->setActiveInternal(wParam == TRUE);
            GetInputManager().resetState();
            break;

        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                /// show the cursor when inactive
                if (_cursorState == kCursorStateDisabled || _cursorState == kCursorStateHidden) {
                    showCursor(true);
                }

            } else {

                /// WA_ACTIVE | WA_CLICKACTIVE
                RECT rect;
                POINT cursorPos;

                if (GetClientRect(hWnd, &rect) && GetCursorPos(&cursorPos)) {
                    /// only hide or disable when cursor is in the client rect
                    ScreenToClient(hWnd, &cursorPos);
                    if (PtInRect(&rect, cursorPos)) {
                        if (_cursorState == kCursorStateHidden) {
                            showCursor(false);
                        }
                        if (_cursorState == kCursorStateDisabled) {
                            confineCursor(true);
                            showCursor(false);
                        }
                        break;
                    }
                }

                bActiveInNonClientArea = true;
            }

            return DefWindowProc(hWnd, msg, wParam, lParam);
            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

void Window::setFrame(const Rect &frame) {
    RECT rect{};
    rect.left = frame.x();
    rect.top = frame.y();
    rect.right  = (long) frame.x() + frame.width();
    rect.bottom = (long) frame.y() + frame.height();
    if (AdjustWindowRect(&rect, style, _menu != nullptr) <= 0) {
        MessageBoxW(nullptr, TranslateErrorCodeW((HRESULT) GetLastError()).c_str(), L"Window", MB_OK | MB_ICONEXCLAMATION);
        exit(-1);
    }
    int width  = rect.right - rect.left;
    int height = rect.bottom - rect.top;
//    GetWindowRect(hWnd, &rect);
    SetWindowPos(hWnd, nullptr, (int) rect.left, (int) rect.top, width, height, 0);
}

Rect Window::getFrame() {
    RECT rect{};
    GetClientRect(hWnd, &rect);

    Rect ret{};
    ret.size.width  = rect.right - rect.left;
    ret.size.height = rect.bottom - rect.top;

    POINT pt{ .x = rect.left, .y = rect.top }; // those are zero anyway
    ClientToScreen(hWnd, &pt);
    ret.origin.x    = pt.x;
    ret.origin.y    = pt.y;

    return ret;
}

void Window::center() {
    int  screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int  screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    if (AdjustWindowRect(&clientRect, style, _menu != nullptr) <= 0) {
        MessageBoxW(nullptr, TranslateErrorCodeW((HRESULT) GetLastError()).c_str(), L"ojoie", MB_OK | MB_ICONEXCLAMATION);
        return;
    }
    int clientWidth  = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;
    SetWindowPos(hWnd, nullptr,
                 screenWidth / 2 - clientWidth / 2,
                 screenHeight / 2 - clientHeight / 2,
                 clientWidth, clientHeight, 0);
}

void Window::showCursor(bool show) {
    if (show) {
        while (::ShowCursor(TRUE) < 0)
            ;
    } else {
        while (::ShowCursor(FALSE) >= 0)
            ;
    }
}

void Window::confineCursor(bool confine) {
    if (confine) {
        RECT r;
        GetWindowRect(hWnd, &r );
        int cx = (r.right + r.left)/2;
        int cy = (r.top + r.bottom)/2;
        SetCursorPos(cx, cy);
        r.left = cx;
        r.right = cx;
        r.top = cy;
        r.bottom = cy;
        ClipCursor(&r);
    } else {
        ClipCursor(nullptr);
    }
}

void Window::setCursorState(CursorState state) {
    if (_cursorState == state) return;

    if (_cursorState == kCursorStateNormal) {
        showCursor(false);
        if (state == kCursorStateDisabled) {
            confineCursor(true);
        }
    } else if (state == kCursorStateNormal) {
        showCursor(true);
        if (_cursorState == kCursorStateDisabled) {
            confineCursor(false);
        }
    } else if (_cursorState == kCursorStateHidden) {
        /// state === disabled
        confineCursor(true);
    } else {
        /// _cursorState == disabled
        /// state === hidden
        confineCursor(false);
    }

    _cursorState = state;
}

static HCURSOR s_CursorArrow = LoadCursor(nullptr, IDC_ARROW);
static HCURSOR s_CursorBeam = LoadCursor(nullptr, IDC_IBEAM);
static HCURSOR s_CursorCrossHair = LoadCursor(nullptr, IDC_CROSS);
static HCURSOR s_CursorHand = LoadCursor(nullptr, IDC_HAND);
static HCURSOR s_CursorHResize = LoadCursor(nullptr, IDC_SIZEWE);
static HCURSOR s_CursorVResize = LoadCursor(nullptr, IDC_SIZENS);


void Window::setCursorShape(CursorShape shape) {
    switch (shape) {
        case CursorShape::Arrow:
            ::SetCursor(s_CursorArrow);
            break;
        case CursorShape::IBeam:
            ::SetCursor(s_CursorBeam);
            break;
        case CursorShape::CrossHair:
            ::SetCursor(s_CursorCrossHair);
            break;
        case CursorShape::Hand:
            ::SetCursor(s_CursorHand);
            break;
        case CursorShape::HResize:
            ::SetCursor(s_CursorHResize);
            break;
        case CursorShape::VResize:
            ::SetCursor(s_CursorVResize);
            break;
    }
}

CursorShape Window::getCursorShape() const {
    HCURSOR hCursor = GetCursor();
    if (hCursor == s_CursorArrow) {
        return CursorShape::Arrow;
    }
    if (hCursor == s_CursorBeam) {
        return CursorShape::IBeam;
    }
    if (hCursor == s_CursorCrossHair) {
        return CursorShape::CrossHair;
    }
    if (hCursor == s_CursorHand) {
        return CursorShape::Hand;
    }
    if (hCursor == s_CursorHResize) {
        return CursorShape::HResize;
    }
    if (hCursor == s_CursorVResize) {
        return CursorShape::VResize;
    }
    return CursorShape::Custom;
}

void Window::setCursorShape(const char *name) {
    std::wstring wName = Utf8ToWide(name);
    ::SetCursor(LoadCursorW(gHInstance, wName.c_str()));
}

Point Window::getCursorPosition() {
    POINT point;
    ::GetCursorPos(&point);
    return { point.x, point.y };
}

void Window::setCursorPosition(const Point &point) {
    ::SetCursorPos(point.x, point.y);
}

bool Window::isVisible() {
    return IsWindowVisible(hWnd);
}

void Window::setFullScreen(bool fullScreen) {
    if (bIsFullScreen == fullScreen) return;
    bIsFullScreen = fullScreen;
    if (fullScreen) {
        POINT Point = {0};
        HMONITOR Monitor = MonitorFromPoint(Point, MONITOR_DEFAULTTONEAREST);
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        if (GetMonitorInfo(Monitor, &MonitorInfo)) {
            _oldZoom = IsZoomed(hWnd);

            WINDOWPLACEMENT windowPlacement;
            GetWindowPlacement(hWnd, &windowPlacement);
            _oldRect = windowPlacement.rcNormalPosition;

            DWORD Style = WS_POPUP | WS_VISIBLE;
            SetWindowLongPtr(hWnd, GWL_STYLE, Style);
            SetWindowPos(hWnd, nullptr,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW);

            // we set fullScreen windowed mode to avoid delay

            // _layer->setFullScreen(true);
        }
    } else {
        // _layer->setFullScreen(false);
        SetWindowLongPtr(hWnd, GWL_STYLE, style);
        SetWindowPos(hWnd, nullptr,
                     _oldRect.left, _oldRect.top, _oldRect.right - _oldRect.left, _oldRect.bottom - _oldRect.top,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        if (_oldZoom) {
            ShowWindow(hWnd, SW_MAXIMIZE);
        }
    }
}

void Window::setMenu(AN::Menu *menu) {
    AN::Window::setMenu(menu);

    if (_menu) {
        WIN::Menu *wMenu = (WIN::Menu *)menu;
        wMenu->rebuildMenu();
        SetMenu(hWnd, wMenu->getHMENU());
        DrawMenuBar(hWnd);

        if (!wMenu->getAccelerators().empty()) {
            _menuAccelerators = CreateAcceleratorTable((LPACCEL)wMenu->getAccelerators().data(),
                                                       wMenu->getAccelerators().size());
        }
    }
}

void Window::dragEvent(DWORD keyState, const POINTL &pt, EventType eventType) {


    if (GetIMGUIManager().viewportEnabled()) {
        GetIMGUIManager().dragEvent(pt.x, pt.y);
    } else {
        POINT p{ .x = pt.x, .y = pt.y };
        ScreenToClient(hWnd, &p);
        GetIMGUIManager().dragEvent(p.x, p.y);
    }

//    if (eventType == kDragUpdated) {
//        FORWARD_WM_MOUSEMOVE(hWnd, p.x, p.y, keyState, handleMessageInternal);
//    } else if (eventType == kDragEnter) {
//        FORWARD_WM_LBUTTONDOWN(hWnd, false, p.x, p.y, keyState, handleMessageInternal);
//    } else if (eventType == kDragExited) {
//        FORWARD_WM_LBUTTONUP(hWnd, p.x, p.y, keyState, handleMessageInternal);
//    }

}

void Window::setIMEInput(bool visible, UInt32 x, UInt32 y) {
    // Notify OS Input Method Editor of text input position

    ::ImmAssociateContextEx(hWnd, NULL, visible ? IACE_DEFAULT : 0);
    if (HIMC himc = ::ImmGetContext(hWnd)) {
        COMPOSITIONFORM composition_form = {};
        composition_form.ptCurrentPos.x = (LONG)x;
        composition_form.ptCurrentPos.y = (LONG)y;
        composition_form.dwStyle = CFS_FORCE_POSITION;
        ::ImmSetCompositionWindow(himc, &composition_form);
        CANDIDATEFORM candidate_form = {};
        candidate_form.dwStyle = CFS_CANDIDATEPOS;
        candidate_form.ptCurrentPos.x = (LONG)x;
        candidate_form.ptCurrentPos.y = (LONG)y;
        ::ImmSetCandidateWindow(himc, &candidate_form);
        ::ImmReleaseContext(hWnd, himc);
    }
}

void Window::addSubView(AN::View *view) {
    view->retain();
    WIN::View *winView = dynamic_cast<WIN::View *>(view); /// use dyn cast due to virtual base
    HWND childHWND = winView->getHWND();
    SetParent(childHWND, hWnd);
    ShowWindow(childHWND, SW_SHOWNORMAL);
    InvalidateRect(childHWND, nullptr, true);
    UpdateWindow(childHWND);
}

void Window::setBorderLessStyle() {
    style = GetWindowLong(hWnd, GWL_STYLE);

    // Remove the window frame and caption styles
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

    // Set the modified style to the window
    SetWindowLong(hWnd, GWL_STYLE, style);

    // Update the window's non-client area
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

bool Window::isZoomed() const {
    return IsZoomed(hWnd) == TRUE;
}

void Window::zoom() {
    if (!isZoomed()) {
        ShowWindow(hWnd, SW_MAXIMIZE);
    } else {
        ShowWindow(hWnd, SW_RESTORE);
    }
}


}// namespace AN::WIN