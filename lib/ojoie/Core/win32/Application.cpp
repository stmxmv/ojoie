//
// Created by aojoie on 4/22/2023.
//

#include "Core/private/win32/App.hpp"
#include "Core/private/win32/Window.hpp"
#include "Threads/Dispatch.hpp"

#include "Core/DragAndDrop.hpp"
#include "Core/Game.hpp"
#include "Render/RenderContext.hpp"
#include "Utility/win32/Unicode.hpp"

#include "App/Views/win32/View.hpp"

#include "Resources/resource.h"

#include "Modules/Dylib.hpp"
#include "../../tools/CrashHandler/CrashHandler.hpp"

#include "DarkMode.h"
#include <fcntl.h>
#pragma comment(lib, "windowsapp")

#include <hidusage.h>
#include <dwmapi.h>
#include <Uxtheme.h>

#include <algorithm>
#include <ranges>
#include <string>
#include <tchar.h>
#include <vector>

HINSTANCE gOJOIEHInstance = nullptr; /// the ojoie dll instance
HINSTANCE gHInstance = nullptr; /// the exe excutable instance

BOOL WINAPI DllMain(
        HINSTANCE hinstDLL,  // handle to DLL module
        DWORD fdwReason,     // reason for calling function
        LPVOID lpvReserved )  // reserved
{
    // Perform actions based on the reason for calling.
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Initialize once for each new process.
            // Return FALSE to fail DLL load.
            gOJOIEHInstance = hinstDLL;
            gHInstance = GetModuleHandle(nullptr);
            break;

        case DLL_THREAD_ATTACH:
            // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
            // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:

            if (lpvReserved != nullptr)
            {
                break; // do not do cleanup if process termination scenario
            }

            // Perform any necessary cleanup.
            break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

namespace AN {

extern AN_API AN::Application *App;

#ifdef OJOIE_WITH_EDITOR
void LoadRenderDoc();
#endif//OJOIE_WITH_EDITOR

}

namespace AN::WIN {

typedef void (*ApplicationMainLoopCallback)(void *info);


// Perform our own check with RtlVerifyVersionInfo() instead of using functions from <VersionHelpers.h> as they
// require a manifest to be functional for checks above 8.1. See https://github.com/ocornut/imgui/issues/4200
static BOOL _IsWindowsVersionOrGreater(WORD major, WORD minor, WORD) {
    typedef LONG(WINAPI * PFN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW *, ULONG, ULONGLONG);
    static PFN_RtlVerifyVersionInfo RtlVerifyVersionInfoFn = nullptr;
    if (RtlVerifyVersionInfoFn == nullptr) {
        if (HMODULE ntdllModule = ::GetModuleHandleA("ntdll.dll")) {
            RtlVerifyVersionInfoFn = (PFN_RtlVerifyVersionInfo) GetProcAddress(ntdllModule, "RtlVerifyVersionInfo");
        }
    }
    if (RtlVerifyVersionInfoFn == nullptr) {
        return FALSE;
    }

    RTL_OSVERSIONINFOEXW versionInfo = {};
    ULONGLONG conditionMask          = 0;
    versionInfo.dwOSVersionInfoSize  = sizeof(RTL_OSVERSIONINFOEXW);
    versionInfo.dwMajorVersion       = major;
    versionInfo.dwMinorVersion       = minor;
    VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
    return (RtlVerifyVersionInfoFn(&versionInfo, VER_MAJORVERSION | VER_MINORVERSION, conditionMask) == 0) ? TRUE : FALSE;
}


#define _IsWindowsVistaOrGreater()   _IsWindowsVersionOrGreater(HIBYTE(0x0600), LOBYTE(0x0600), 0)// _WIN32_WINNT_VISTA
#define _IsWindows8OrGreater()       _IsWindowsVersionOrGreater(HIBYTE(0x0602), LOBYTE(0x0602), 0)// _WIN32_WINNT_WIN8
#define _IsWindows8Point1OrGreater() _IsWindowsVersionOrGreater(HIBYTE(0x0603), LOBYTE(0x0603), 0)// _WIN32_WINNT_WINBLUE
#define _IsWindows10OrGreater()      _IsWindowsVersionOrGreater(HIBYTE(0x0A00), LOBYTE(0x0A00), 0)// _WIN32_WINNT_WINTHRESHOLD / _WIN32_WINNT_WIN10

typedef enum { PROCESS_DPI_UNAWARE           = 0,
               PROCESS_SYSTEM_DPI_AWARE      = 1,
               PROCESS_PER_MONITOR_DPI_AWARE = 2 } PROCESS_DPI_AWARENESS;
typedef enum { MDT_EFFECTIVE_DPI = 0,
               MDT_ANGULAR_DPI   = 1,
               MDT_RAW_DPI       = 2,
               MDT_DEFAULT       = MDT_EFFECTIVE_DPI } MONITOR_DPI_TYPE;

typedef HRESULT(WINAPI *PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);                    // Shcore.lib + dll, Windows 8.1+
typedef HRESULT(WINAPI *PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT *, UINT *);     // Shcore.lib + dll, Windows 8.1+
typedef DPI_AWARENESS_CONTEXT(WINAPI *PFN_SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);// User32.lib + dll, Windows 10 v1607+ (Creators Update)

#ifndef _DPI_AWARENESS_CONTEXTS_
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE (DPI_AWARENESS_CONTEXT) - 3
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (DPI_AWARENESS_CONTEXT) - 4
#endif

// Helper function to enable DPI awareness without setting up a manifest
void Win32_EnableDpiAwareness() {
    //    if (_IsWindows10OrGreater()) {
    //        static HINSTANCE user32_dll = ::LoadLibraryA("user32.dll");// Reference counted per-process
    //        if (PFN_SetThreadDpiAwarenessContext SetThreadDpiAwarenessContextFn = (PFN_SetThreadDpiAwarenessContext)::GetProcAddress(user32_dll, "SetThreadDpiAwarenessContext")) {
    //            SetThreadDpiAwarenessContextFn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2); //NOLINT
    //            return;
    //        }
    //    }
    /// use above seems it will cause vkAcquireNextImage always return OUT_OF_DATE, so just comment it out, *** if possible just use a manifest file to compile
    if (_IsWindows8Point1OrGreater()) {
        static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll");// Reference counted per-process
        if (PFN_SetProcessDpiAwareness SetProcessDpiAwarenessFn = (PFN_SetProcessDpiAwareness)::GetProcAddress(shcore_dll, "SetProcessDpiAwareness")) {
            SetProcessDpiAwarenessFn(PROCESS_PER_MONITOR_DPI_AWARE);
            return;
        }
    }
#if _WIN32_WINNT >= 0x0600
    ::SetProcessDPIAware();
#endif
}


void Win32_EnableAlphaCompositing(HWND hwnd) {
    if (!_IsWindowsVistaOrGreater())
        return;

    BOOL composition;
    if (FAILED(::DwmIsCompositionEnabled(&composition)) || !composition)
        return;

    BOOL  opaque;
    DWORD color;
    if (_IsWindows8OrGreater() || (SUCCEEDED(::DwmGetColorizationColor(&color, &opaque)) && !opaque)) {
        HRGN           region = ::CreateRectRgn(0, 0, -1, -1);
        DWM_BLURBEHIND bb     = {};
        bb.dwFlags            = DWM_BB_ENABLE | DWM_BB_BLURREGION;
        bb.hRgnBlur           = region;
        bb.fEnable            = TRUE;
        ::DwmEnableBlurBehindWindow(hwnd, &bb);
        ::DeleteObject(region);
    } else {
        DWM_BLURBEHIND bb = {};
        bb.dwFlags        = DWM_BB_ENABLE;
        ::DwmEnableBlurBehindWindow(hwnd, &bb);
    }
}


#define WM_WINDOW_HIDE   (WM_USER + 0x0001)
#define WM_DISPATCH_TASK (WM_USER + 0x0002)

static void CreateDevConsole() {
    if (!AllocConsole()) {
        AN_LOG(Error, "Fail to create console %s", TranslateErrorCode(GetLastError()).c_str());
        return;
    }
    SetConsoleTitle(TEXT("Dev Console"));
    EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE , MF_GRAYED);
    DrawMenuBar(GetConsoleWindow());
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONOUT$", "r", stdin);

    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    // std::wcout, std::wclog, std::wcerr, std::wcin
    HANDLE hConOut = CreateFile(TEXT("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hConIn = CreateFile(TEXT("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
    SetStdHandle(STD_ERROR_HANDLE, hConOut);
    SetStdHandle(STD_INPUT_HANDLE, hConIn);
    std::wcout.clear();
    std::wclog.clear();
    std::wcerr.clear();
    std::wcin.clear();
}

inline static bool IsColorLight(winrt::Windows::UI::Color& clr) {
    return (((5 * clr.G) + (2 * clr.R) + clr.B) > (8 * 128));
}

bool Application::isDarkMode() {
    using namespace winrt::Windows::UI::ViewManagement;
    auto foreground = uiSettings.GetColorValue(UIColorType::Foreground);
    return IsColorLight(foreground);
}

UInt64 gMainThreadID;

static void RefreshDarkModeState() {
    EnumThreadWindows(gMainThreadID,
            [](HWND hWnd, LPARAM lparam) -> BOOL {
                RefreshTitleBarThemeColor(hWnd);
                DrawMenuBar(hWnd);
                RedrawWindow(hWnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASENOW);
                return TRUE;
            }, 0);
}

STICKYKEYS g_StartupStickyKeys = {sizeof(STICKYKEYS), 0};
TOGGLEKEYS g_StartupToggleKeys = {sizeof(TOGGLEKEYS), 0};
FILTERKEYS g_StartupFilterKeys = {sizeof(FILTERKEYS), 0};

void AllowAccessibilityShortcutKeys(bool bAllowKeys) {
    if (bAllowKeys) {
        // Restore StickyKeys/etc to original state and enable Windows key
        STICKYKEYS sk = g_StartupStickyKeys;
        TOGGLEKEYS tk = g_StartupToggleKeys;
        FILTERKEYS fk = g_StartupFilterKeys;

        SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
        SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
        SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);
    } else {
        // Disable StickyKeys/etc shortcuts but if the accessibility feature is on,
        // then leave the settings alone as its probably being usefully used

        STICKYKEYS skOff = g_StartupStickyKeys;
        if ((skOff.dwFlags & SKF_STICKYKEYSON) == 0) {
            // Disable the hotkey and the confirmation
            skOff.dwFlags &= ~SKF_HOTKEYACTIVE;
            skOff.dwFlags &= ~SKF_CONFIRMHOTKEY;

            SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &skOff, 0);
        }

        TOGGLEKEYS tkOff = g_StartupToggleKeys;
        if ((tkOff.dwFlags & TKF_TOGGLEKEYSON) == 0) {
            // Disable the hotkey and the confirmation
            tkOff.dwFlags &= ~TKF_HOTKEYACTIVE;
            tkOff.dwFlags &= ~TKF_CONFIRMHOTKEY;

            SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &tkOff, 0);
        }

        FILTERKEYS fkOff = g_StartupFilterKeys;
        if ((fkOff.dwFlags & FKF_FILTERKEYSON) == 0) {
            // Disable the hotkey and the confirmation
            fkOff.dwFlags &= ~FKF_HOTKEYACTIVE;
            fkOff.dwFlags &= ~FKF_CONFIRMHOTKEY;

            SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &fkOff, 0);
        }
    }
}

Application::Application() : bActive(true), _running(), _darkMode((DarkMode)-1), uiSettings(nullptr) {
    if (!App) {
        App = this;
        Dispatch::SetThreadID(Dispatch::Main, GetCurrentThreadID());

        gMainThreadID = ::GetCurrentThreadId();

        Dispatch::GetDelegate()[Dispatch::Main] = [] (TaskInterface task) {
            if (task) {
                TaskInterface *heapTask = new TaskInterface(std::move(task));
                PostThreadMessageW(gMainThreadID, WM_DISPATCH_TASK, 0, (LPARAM)heapTask);
            }
        };

        /// application should use manifest file to enable dpi awareness
//        Win32_EnableDpiAwareness();

        MSG msg{};
        PeekMessageW(&msg, nullptr, 0, 0, 0); /// force to create message queue

    } else {
        throw Exception("Executable can only has one AN::Application instance!");
    }
}

static BOOL CALLBACK EnumWindowCallback(HWND hWnd, LPARAM lparam) {

    int *outVisiableWindowNum = (int *)lparam;

    long style = GetWindowLong(hWnd, GWL_EXSTYLE);

    // List visible windows and not utility window
    if (IsWindowVisible(hWnd) && !(style & WS_EX_TOOLWINDOW)) {
        ++(*outVisiableWindowNum);
    }

    return TRUE;
}


bool Application::pollEvent() {
    MSG msg{};
    BOOL result = PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE);

    if (result) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return result == TRUE;
}

void Application::run(int argc, const char *argv[]) {

    /// super
    AN::Application::run(argc, argv);

#ifdef AN_DEBUG
    /// show console when debug build and with no debugger
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        CreateDevConsole();
    }

#endif//AN_DEBUG

    // Save the current sticky/toggle/filter key settings so they can be restored them later
    SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
    SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
    SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);

    /// we don't want the sticky key ...
    AllowAccessibilityShortcutKeys(false);

    /// init com
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        AN_LOG(Error, "CoInitializeEx fail");
        return;
    }

    InitDarkMode();

    uiSettings = winrt::Windows::UI::ViewManagement::UISettings();

    Window::RegisterWindowClass();
    View::RegisterWindowClass();

    /// applicationWillFinishLaunching may create splash window
    if (_appDelegate) {
        _appDelegate->applicationWillFinishLaunching(this);
    }

    while (pollEvent()) {}

    if ((int)_darkMode == -1) {
        _darkMode = kLightMode;
    }

    uiSettings.ColorValuesChanged(
            [](auto &&settings, auto &&inspectable) {
                auto foregroundRevoker = settings.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Foreground);
                bool isDarkModeRevoker = static_cast<bool>(IsColorLight(foregroundRevoker));

                Dispatch::async(Dispatch::Main, [isDarkModeRevoker] {
                    if (((Application *)App)->_darkMode == kAutoDarkMode) {
                        if (isDarkModeRevoker) {
                            SetDarkMode(true, false);
                        } else {
                            SetDarkMode(false, false);
                        }
                        RefreshDarkModeState();
                    }
                });
            });


    PFN_ANCreateCrashHandler ANCreateCrashHandler;
    PFN_ANDeleteCrashHandler ANDeleteCrashHandler;

    LoadAndLookupSymbols("CrashHandler",
                         "ANCreateCrashHandler", &ANCreateCrashHandler,
                         "ANDeleteCrashHandler", &ANDeleteCrashHandler,
                         nullptr);

    CrashHandlerInterface *crashHandler = nullptr;
    if (ANCreateCrashHandler) {
        crashHandler = ANCreateCrashHandler("./ANCrashHandler.exe", getName(), getVersion(), "Data/Dump");
        crashHandler->install();
    } else {
        AN_LOG(Warning, "not found CrashHandler");
    }


    ANAssert(GetDragAndDrop().init());


#ifdef OJOIE_WITH_EDITOR
    /// only load renderDoc in editor runtime

    bool enableRenderDoc = getCommandLineArg<bool>("--enable-render-doc");
    if (enableRenderDoc) {
        LoadRenderDoc();
    }

#endif

    /// init render context and game instance
    InitializeRenderContext(kGraphicsAPID3D11);
    ANAssert(GetGame().init());

    if (_appDelegate) {
        _appDelegate->applicationDidFinishLaunching(this);
        _appDelegate->gameSetup(GetGame());
    }

    _running = true;

    GetGame().start();

    if (_appDelegate) {
        _appDelegate->gameStart(GetGame());
    }


    for (;;) {
        Event::Current().setTypeInternal(kEventNone);

        MSG msg{};
        BOOL result;

        bool dontWaitForMessages = bActive || !_running;
        if (dontWaitForMessages) {
            result = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE); // block to use GetMessage, instead of peek PM_REMOVE
        } else {
            result = GetMessage(&msg, nullptr, 0, 0);
        }

        if (result) {

            WIN::Window *window = (WIN::Window *)getMainWindow();

            if (window && window->getMenuAccelerators()) {
                if (TranslateAccelerator(window->getHWND(), window->getMenuAccelerators(), &msg)) {
                    continue;
                }
            }

            TranslateMessage(&msg); /// translate virtual key into char

            switch (msg.message) {
                case WM_WINDOW_HIDE:
                {
                    int visiableWindowNum = 0;
                    EnumThreadWindows(::GetCurrentThreadId(), EnumWindowCallback, (LPARAM) &visiableWindowNum);

                    if (visiableWindowNum == 0) {
                        if (_appDelegate) {
                            if (_appDelegate->applicationShouldTerminateAfterLastWindowClosed(this)) {
                                terminate();
                            }
                        }
                    }

                }
                break;

                case WM_DISPATCH_TASK:
                {
                    TaskInterface *task = (TaskInterface *)msg.lParam;
                    // task is valid when post
                    task->run();
                    delete task;
                }
                break;

                default:
                    DispatchMessage(&msg);
                    break;
            }
        } else {
            if (!_running) {
                break;
            }
            GetGame().performMainLoop();
        }
    }

    /// perform clean up
    GetGame().deinit();

    if (_appDelegate) {
        _appDelegate->applicationWillTerminate(this);
    }

    _appDelegate.reset();

    View::UnregisterWindowClass();
    Window::UnregisterWindowClass();

    /// destroy all remaining objects
    Object::DestroyAllObjects();

    DeallocRenderContext();
    GetDragAndDrop().deinit();

    uiSettings = nullptr;
    CoUninitialize();

    AllowAccessibilityShortcutKeys(true);

    if (ANDeleteCrashHandler && crashHandler) {
        ANDeleteCrashHandler(crashHandler);
    }
}

void Application::terminate() {
    _running = false;
}

AN::Window *Application::getMainWindow() {
    HWND hWnd = GetActiveWindow();
    if (hWnd) {
        AN::Window *window = (AN::Window *) (GetWindowLongPtr(hWnd, GWLP_USERDATA));
        return window;
    }
    return nullptr;
}

static COLORREF GetBrushColorValue(HBRUSH hBrush) {
    LOGBRUSH logBrush;
    GetObject(hBrush, sizeof(LOGBRUSH), &logBrush);
    return logBrush.lbColor;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);

    switch (message) {
        case WM_INITDIALOG:
        {

            // Get the owner window and dialog box rectangles.
            HWND hwndOwner;
            RECT rcOwner, rcDlg, rc;
            if ((hwndOwner = GetParent(hDlg)) == NULL)
            {
                hwndOwner = GetDesktopWindow();
            }

            GetWindowRect(hwndOwner, &rcOwner);
            GetWindowRect(hDlg, &rcDlg);
            CopyRect(&rc, &rcOwner);

            // Offset the owner and dialog box rectangles so that right and bottom
            // values represent the width and height, and then offset the owner again
            // to discard space taken up by the dialog box.

            OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
            OffsetRect(&rc, -rc.left, -rc.top);
            OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

            // The new position is the sum of half the remaining space and the owner's
            // original position.

            SetWindowPos(hDlg,
                         HWND_TOP,
                         rcOwner.left + (rc.right / 2),
                         rcOwner.top + (rc.bottom / 2),
                         0, 0,          // Ignores size arguments.
                         SWP_NOSIZE);

            AllowDarkModeForWindow(hDlg, true);
            RefreshTitleBarThemeColor(hDlg);

            EnumChildWindows(hDlg, [](HWND childHWnd, LPARAM lParam) -> BOOL {
                        const int MAX_CLASSNAME = 256;
                        TCHAR className[MAX_CLASSNAME];
                        if (GetClassName(childHWnd, className, MAX_CLASSNAME) != 0) {
                            if (_tcscmp(className, TEXT("Button")) == 0) {
                                // The window class belongs to a button control
                                SetWindowTheme(childHWnd, L"Explorer", NULL);
                                AllowDarkModeForWindow(childHWnd, true);
                                SendMessageW(childHWnd, WM_THEMECHANGED, 0, 0);
                            }
                        }
                        return TRUE;
                    }, 0);

            if (GetDlgCtrlID((HWND) wParam) != IDOK)
            {
                SetFocus(GetDlgItem(hDlg, IDOK));
                return FALSE;
            }
            return TRUE;
        }

        case WM_ERASEBKGND:
        {
            if (IsDarkModeEnabled()) {
                RECT rc;
                HDC hdc = (HDC)wParam;
                GetClientRect(hDlg, &rc);
                FillRect(hdc, &rc, (HBRUSH)GetStockObject(DKGRAY_BRUSH));
                return TRUE;
            }
        }
        break;

        case WM_CTLCOLORBTN:
            if (IsDarkModeEnabled()) {
                return (INT_PTR)GetStockObject(DKGRAY_BRUSH);
            }
            break;

        case WM_CTLCOLORSTATIC:
        {
            if (IsDarkModeEnabled()) {
                HBRUSH hBrush = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
                COLORREF brushColor = GetBrushColorValue(hBrush);
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, RGB(230, 230, 230));
                SetBkColor(hdc, brushColor);
                return (INT_PTR)hBrush;
            }
        }
        break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR) TRUE;
            }
            break;
    }
    return (INT_PTR) FALSE;
}

void Application::showAboutWindow() {
    DialogBox(gOJOIEHInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), GetActiveWindow(), About);
}


void Application::setDarkMode(DarkMode mode) {
    if (_darkMode == mode) {
        return;
    }

    bool oldMode = _darkMode;
    _darkMode = mode;

    switch (mode) {
        case kLightMode:
            SetDarkMode(false, false);
            RefreshDarkModeState();
            break;
        case kDarkMode:
            SetDarkMode(true, false);
            RefreshDarkModeState();
            break;
        case kAutoDarkMode:
        {
            bool darkMode = isDarkMode();
            if (oldMode == kDarkMode != darkMode) {
                SetDarkMode(darkMode, false);
                RefreshDarkModeState();
            }
        }
        break;
    }
}

void Application::messageBox(const char *title, const char *message, MessageBoxStyleFlag flags, AN::Window *window) {
    std::wstring wTitle = Utf8ToWide(title);
    std::wstring wMessage = Utf8ToWide(message);

    HWND hWnd = nullptr;
    if (window) {
        hWnd = ((WIN::Window *)window)->getHWND();
    }
    UINT types = MB_OK;

    if (flags & kMessageBoxStyleError) {
        types |= MB_ICONERROR;
    }
    MessageBoxW(hWnd, wMessage.c_str(), wTitle.c_str(), types);
}


bool OpenPanel::init() {

    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC,
                                  IID_IFileOpenDialog, &pFileDialog);

    if (FAILED(hr)) {
        AN_LOG(Error, "%s", TranslateErrorCode(hr).c_str());
        return false;
    }

    // Set dialog options
    DWORD dwFlags;
    pFileDialog->GetOptions(&dwFlags);
    pFileDialog->SetOptions(dwFlags | FOS_FORCEFILESYSTEM); // Force file system dialog

    return true;
}

void OpenPanel::processFilter() {
    if (!allowOtherTypes && allowContentExtension.empty()) {
        return;
    }

    fileTypes.clear();

    if (allowOtherTypes) {
        fileTypes.push_back({ L"All Files", L"*.*" });
    }

    std::vector<std::wstring> tempStrings;
    tempStrings.reserve(allowContentExtension.size() * 2);
    for (auto &&[desc, extensions] : allowContentExtension) {

        COMDLG_FILTERSPEC type;
        tempStrings.push_back(Utf8ToWide(desc));
        type.pszName = tempStrings.back().c_str();

        std::string filter;
        for (int i = 0; i < (int)extensions.size(); ++i) {
            filter.append("*.");
            filter.append(extensions[i]);
            if (i != (int)extensions.size() - 1) {
                filter.push_back(';');
            }
        }

        extensionList.push_back(filter);

        tempStrings.push_back(Utf8ToWide(filter));
        type.pszSpec = tempStrings.back().c_str();

        fileTypes.push_back(type);
    }

    pFileDialog->SetFileTypes(fileTypes.size(), fileTypes.data());

    if (allowContentExtension.empty()) {
        pFileDialog->SetFileTypeIndex(1);
    } else {

        if (!defaultExtension.empty()) {
            for (int i = 0; i < extensionList.size(); ++i) {
                if (extensionList[i].find(defaultExtension) != std::string::npos) {
                    if (allowOtherTypes) {
                        pFileDialog->SetFileTypeIndex(i + 2);
                    } else {
                        pFileDialog->SetFileTypeIndex(i + 1);
                    }

                    return;
                }
            }
        }
        if (allowOtherTypes) {
            pFileDialog->SetFileTypeIndex(2);
        } else {
            pFileDialog->SetFileTypeIndex(1);
        }
    }
}

void OpenPanel::beginSheetModal(AN::Window *window, OpenPanelCallback completionHandler, void *userdata) {
    processFilter();

    HWND owner = nullptr;
    /// note: you need to init com as COINIT_APARTMENTTHREADED, otherwise it will hang forever!!
    if (window) {
        owner = ((Window *)window)->getHWND();
    }

    HRESULT hr = pFileDialog->Show(owner);

    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        completionHandler(kModalResponseCancel, nullptr, userdata);
        return;
    } else if (FAILED(hr)){
        AN_LOG(Error, "OpenPanel Show Fail %s", TranslateErrorCode(hr).c_str());
        return;
    }

    ComPtr<IShellItem> pItem;
    hr = pFileDialog->GetResult(&pItem);

    if (FAILED(hr)) {
        AN_LOG(Error, "OpenPanel Get Result Fail %s", TranslateErrorCode(hr).c_str());
        return;
    }

    PWSTR pszFilePath;
    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

    if (FAILED(hr)) {
        AN_LOG(Error, "OpenPanel Get Display Name Fail %s", TranslateErrorCode(hr).c_str());
        return;
    }

    std::string path = WideToUtf8(pszFilePath);
    completionHandler(kModalResponseOk, path.c_str(), userdata);

    CoTaskMemFree(pszFilePath);
}

void OpenPanel::setTitle(const char *title) {
    std::wstring wTitle = Utf8ToWide(title);
    pFileDialog->SetTitle(wTitle.c_str());
}

void OpenPanel::setFileName(const char *name) {
    std::wstring wName = Utf8ToWide(name);
    pFileDialog->SetFileName(wName.c_str());
}

const char *OpenPanel::getFileName() {
    PWSTR wFileName;
    pFileDialog->GetFileName(&wFileName);
    fileName = WideToUtf8(wFileName);
    CoTaskMemFree(wFileName);
    return fileName.c_str();
}

const char *OpenPanel::getExtension() {
    if (allowContentExtension.empty()) {
        return "";
    }

    UINT index;
    pFileDialog->GetFileTypeIndex(&index);
    --index;
    if (allowOtherTypes) {
        --index;
    }
    return extensionList[index].c_str() + 2; // the extra ".*"
}

bool SavePanel::init() {
    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC,
                                  IID_IFileSaveDialog, &pFileDialog);

    if (FAILED(hr)) {
        AN_LOG(Error, "%s", TranslateErrorCode(hr).c_str());
        return false;
    }
    return true;
}


std::wstring TranslateErrorCodeW(HRESULT hr) noexcept {
    wchar_t *msgBuf = nullptr;
    DWORD msgLen  = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            msgBuf,
            0,
            nullptr);

    if (msgLen == 0) {
        return std::format(L"Unidentified error code {}", hr);
    }

    std::wstring errorString(msgBuf);
    LocalFree(msgBuf);
    return errorString;
}

std::string TranslateErrorCode(HRESULT hr) noexcept {
    std::wstring wText = TranslateErrorCodeW(hr);
    std::string text;
    text.resize(WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, nullptr, 0, nullptr, nullptr));
    WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, text.data(), (int)text.size(), nullptr, nullptr);
    return text;
}

std::string GetLastErrorString() noexcept {
    return TranslateErrorCode(GetLastError());
}

std::wstring GetLastErrorStringW() noexcept {
    return TranslateErrorCodeW(GetLastError());
}

}