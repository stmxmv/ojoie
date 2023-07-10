//
// Created by aojoie on 7/3/2023.
//

#include "Input/win32/InputManager.hpp"
#include "Core/private/win32/App.hpp"
#include <hidusage.h>

extern HINSTANCE gHInstance;

namespace AN::WIN {

/// seams win32 messing this typedef
typedef unsigned __int64 QWORD;

#if 0
int kVirtKeyToKeyCode[256];
static void InputInitVKTable() {
    static bool kVirtKeyToKeyCodeInitialized = false;
    if (!kVirtKeyToKeyCodeInitialized) {
        for (int i = 0; i < _countof(kVirtKeyToKeyCode); ++i) {
            kVirtKeyToKeyCode[i] = kInputKeyUnknown;
        }

        kVirtKeyToKeyCode[VK_ESCAPE] = kInputKeyEsc;
        kVirtKeyToKeyCode['1'] = kInputKey_1;
        kVirtKeyToKeyCode['2'] = kInputKey_2;
        kVirtKeyToKeyCode['3'] = kInputKey_3;
        kVirtKeyToKeyCode['4'] = kInputKey_4;
        kVirtKeyToKeyCode['5'] = kInputKey_5;
        kVirtKeyToKeyCode['6'] = kInputKey_6;
        kVirtKeyToKeyCode['7'] = kInputKey_7;
        kVirtKeyToKeyCode['8'] = kInputKey_8;
        kVirtKeyToKeyCode['9'] = kInputKey_9;
        kVirtKeyToKeyCode['0'] = kInputKey_0;
//        kVirtKeyToKeyCode[VK_OEM_MINUS] = SDLK_MINUS;
//        kVirtKeyToKeyCode[VK_OEM_PLUS] = SDLK_EQUALS;
//        kVirtKeyToKeyCode[VK_BACK] = SDLK_BACKSPACE;
//        kVirtKeyToKeyCode[VK_TAB] = SDLK_TAB;
        kVirtKeyToKeyCode['Q'] = kInputKey_Q;
        kVirtKeyToKeyCode['W'] = kInputKey_W;
        kVirtKeyToKeyCode['E'] = kInputKey_E;
        kVirtKeyToKeyCode['R'] = kInputKey_R;
        kVirtKeyToKeyCode['T'] = kInputKey_T;
        kVirtKeyToKeyCode['Y'] = kInputKey_Y;
        kVirtKeyToKeyCode['U'] = kInputKey_U;
        kVirtKeyToKeyCode['I'] = kInputKey_I;
        kVirtKeyToKeyCode['O'] = kInputKey_O;
        kVirtKeyToKeyCode['P'] = kInputKey_P;
//        kVirtKeyToKeyCode[VK_OEM_4] = SDLK_LEFTBRACKET; // TODO
//        kVirtKeyToKeyCode[VK_OEM_6] = SDLK_RIGHTBRACKET; // TODO
        kVirtKeyToKeyCode[VK_RETURN] = kInputKeyEnter;
        kVirtKeyToKeyCode[VK_LCONTROL] = kInputKeyLeftControl;
        kVirtKeyToKeyCode[VK_CONTROL] = kInputKeyLeftControl;
        kVirtKeyToKeyCode['A'] = kInputKey_A;
        kVirtKeyToKeyCode['S'] = kInputKey_S;;
        kVirtKeyToKeyCode['D'] = kInputKey_D;;
        kVirtKeyToKeyCode['F'] = kInputKey_F;;
        kVirtKeyToKeyCode['G'] = kInputKey_G;;
        kVirtKeyToKeyCode['H'] = kInputKey_H;;
        kVirtKeyToKeyCode['J'] = kInputKey_J;;
        kVirtKeyToKeyCode['K'] = kInputKey_K;;
        kVirtKeyToKeyCode['L'] = kInputKey_L;;
//        kVirtKeyToKeyCode[VK_OEM_1] = SDLK_SEMICOLON; // TODO
//        kVirtKeyToKeyCode[VK_OEM_7] = SDLK_QUOTE; // TODO
//        kVirtKeyToKeyCode[VK_OEM_3] = SDLK_BACKQUOTE; // TODO
//        kVirtKeyToKeyCode[VK_OEM_8] = SDLK_BACKQUOTE; // TODO
        kVirtKeyToKeyCode[VK_LSHIFT] = kInputKeyLeftShift;
//        kVirtKeyToKeyCode[VK_OEM_5] = SDLK_BACKSLASH; // TODO
//        kVirtKeyToKeyCode[VK_OEM_102] = SDLK_BACKSLASH; // TODO
        kVirtKeyToKeyCode['Z'] = kInputKey_Z;
        kVirtKeyToKeyCode['X'] = kInputKey_X;
        kVirtKeyToKeyCode['C'] = kInputKey_C;
        kVirtKeyToKeyCode['V'] = kInputKey_V;
        kVirtKeyToKeyCode['B'] = kInputKey_B;
        kVirtKeyToKeyCode['N'] = kInputKey_N;
        kVirtKeyToKeyCode['M'] = kInputKey_M;
//        kVirtKeyToKeyCode[VK_OEM_COMMA] = SDLK_COMMA;
//        kVirtKeyToKeyCode[VK_OEM_PERIOD] = SDLK_PERIOD;
//        kVirtKeyToKeyCode[VK_OEM_2] = SDLK_SLASH;
        kVirtKeyToKeyCode[VK_RSHIFT] = kInputKeyRightShift;
//        kVirtKeyToKeyCode[VK_MULTIPLY] = SDLK_KP_MULTIPLY;
        kVirtKeyToKeyCode[VK_LMENU] = kInputKeyLeftAlt;
        kVirtKeyToKeyCode[VK_SPACE] = kInputKeySpace;
        kVirtKeyToKeyCode[VK_CAPITAL] = kInputKeyCaps;
//        kVirtKeyToKeyCode[VK_F1] = SDLK_F1;
//        kVirtKeyToKeyCode[VK_F2] = SDLK_F2;
//        kVirtKeyToKeyCode[VK_F3] = SDLK_F3;
//        kVirtKeyToKeyCode[VK_F4] = SDLK_F4;
//        kVirtKeyToKeyCode[VK_F5] = SDLK_F5;
//        kVirtKeyToKeyCode[VK_F6] = SDLK_F6;
//        kVirtKeyToKeyCode[VK_F7] = SDLK_F7;
//        kVirtKeyToKeyCode[VK_F8] = SDLK_F8;
//        kVirtKeyToKeyCode[VK_F9] = SDLK_F9;
//        kVirtKeyToKeyCode[VK_F10] = SDLK_F10;
        kVirtKeyToKeyCode[VK_NUMLOCK] = kInputKeyNumLock;
//        kVirtKeyToKeyCode[VK_SCROLL] = SDLK_SCROLLOCK;
//        kVirtKeyToKeyCode[VK_NUMPAD7] = SDLK_KP7;
//        kVirtKeyToKeyCode[VK_NUMPAD8] = SDLK_KP8;
//        kVirtKeyToKeyCode[VK_NUMPAD9] = SDLK_KP9;
//        kVirtKeyToKeyCode[VK_SUBTRACT] = SDLK_KP_MINUS;
//        kVirtKeyToKeyCode[VK_NUMPAD4] = SDLK_KP4;
//        kVirtKeyToKeyCode[VK_NUMPAD5] = SDLK_KP5;
//        kVirtKeyToKeyCode[VK_NUMPAD6] = SDLK_KP6;
//        kVirtKeyToKeyCode[VK_ADD] = SDLK_KP_PLUS;
//        kVirtKeyToKeyCode[VK_NUMPAD1] = SDLK_KP1;
//        kVirtKeyToKeyCode[VK_NUMPAD2] = SDLK_KP2;
//        kVirtKeyToKeyCode[VK_NUMPAD3] = SDLK_KP3;
//        kVirtKeyToKeyCode[VK_NUMPAD0] = SDLK_KP0;
//        kVirtKeyToKeyCode[VK_DECIMAL] = SDLK_KP_PERIOD;
//        kVirtKeyToKeyCode[VK_F11] = SDLK_F11;
//        kVirtKeyToKeyCode[VK_F12] = SDLK_F12;

//        kVirtKeyToKeyCode[VK_F13] = SDLK_F13;
//        kVirtKeyToKeyCode[VK_F14] = SDLK_F14;
//        kVirtKeyToKeyCode[VK_F15] = SDLK_F15;

        kVirtKeyToKeyCode[VK_RCONTROL] = kInputKeyRightControl;
//        kVirtKeyToKeyCode[VK_DIVIDE] = SDLK_KP_DIVIDE;
//        kVirtKeyToKeyCode[VK_SNAPSHOT] = SDLK_SYSREQ; // PrintSrc/SysReq
        kVirtKeyToKeyCode[VK_RMENU] = kInputKeyRightAlt;
//        kVirtKeyToKeyCode[VK_PAUSE] = SDLK_PAUSE;
//        kVirtKeyToKeyCode[VK_HOME] = SDLK_HOME;
//        kVirtKeyToKeyCode[VK_UP] = SDLK_UP;
//        kVirtKeyToKeyCode[VK_PRIOR] = SDLK_PAGEUP;
//        kVirtKeyToKeyCode[VK_LEFT] = SDLK_LEFT;
//        kVirtKeyToKeyCode[VK_RIGHT] = SDLK_RIGHT;
//        kVirtKeyToKeyCode[VK_END] = SDLK_END;
//        kVirtKeyToKeyCode[VK_DOWN] = SDLK_DOWN;
//        kVirtKeyToKeyCode[VK_NEXT] = SDLK_PAGEDOWN;
//        kVirtKeyToKeyCode[VK_INSERT] = SDLK_INSERT;
//        kVirtKeyToKeyCode[VK_DELETE] = SDLK_DELETE;
        kVirtKeyToKeyCode[VK_LWIN] = kInputKeyLeftSuper;
        kVirtKeyToKeyCode[VK_RWIN] = kInputKeyRightSuper;
//        kVirtKeyToKeyCode[VK_APPS] = SDLK_MENU;

        kVirtKeyToKeyCodeInitialized = true;
    }
}
#endif

int kScanCodeToKeyCode[256];
int kScanCodeToKeyCodeE0[256];

static void InputInitScanCodeTable() {
    static bool Initialized = false;
    if (!Initialized) {
        for (int i = 0; i < _countof(kScanCodeToKeyCode); ++i) {
            kScanCodeToKeyCode[i] = kInputKeyUnknown;
            kScanCodeToKeyCodeE0[i] = kInputKeyUnknown;
        }

        kScanCodeToKeyCode[0x1E] = kInputKey_A;
        kScanCodeToKeyCode[0x30] = kInputKey_B;
        kScanCodeToKeyCode[0x2E] = kInputKey_C;
        kScanCodeToKeyCode[0x20] = kInputKey_D;
        kScanCodeToKeyCode[0x12] = kInputKey_E;
        kScanCodeToKeyCode[0x21] = kInputKey_F;
        kScanCodeToKeyCode[0x22] = kInputKey_G;
        kScanCodeToKeyCode[0x23] = kInputKey_H;
        kScanCodeToKeyCode[0x17] = kInputKey_I;
        kScanCodeToKeyCode[0x24] = kInputKey_J;
        kScanCodeToKeyCode[0x25] = kInputKey_K;
        kScanCodeToKeyCode[0x26] = kInputKey_L;
        kScanCodeToKeyCode[0x32] = kInputKey_M;
        kScanCodeToKeyCode[0x31] = kInputKey_N;
        kScanCodeToKeyCode[0x18] = kInputKey_O;
        kScanCodeToKeyCode[0x19] = kInputKey_P;
        kScanCodeToKeyCode[0x10] = kInputKey_Q;
        kScanCodeToKeyCode[0x13] = kInputKey_R;
        kScanCodeToKeyCode[0x1F] = kInputKey_S;
        kScanCodeToKeyCode[0x14] = kInputKey_T;
        kScanCodeToKeyCode[0x16] = kInputKey_U;
        kScanCodeToKeyCode[0x2F] = kInputKey_V;
        kScanCodeToKeyCode[0x11] = kInputKey_W;
        kScanCodeToKeyCode[0x2D] = kInputKey_X;
        kScanCodeToKeyCode[0x15] = kInputKey_Y;
        kScanCodeToKeyCode[0x2C] = kInputKey_Z;

        kScanCodeToKeyCode[0x02] = kInputKey_1;
        kScanCodeToKeyCode[0x03] = kInputKey_2;
        kScanCodeToKeyCode[0x04] = kInputKey_3;
        kScanCodeToKeyCode[0x05] = kInputKey_4;
        kScanCodeToKeyCode[0x06] = kInputKey_5;
        kScanCodeToKeyCode[0x07] = kInputKey_6;
        kScanCodeToKeyCode[0x08] = kInputKey_7;
        kScanCodeToKeyCode[0x09] = kInputKey_8;
        kScanCodeToKeyCode[0x0A] = kInputKey_9;
        kScanCodeToKeyCode[0x0B] = kInputKey_0;

        kScanCodeToKeyCode[0x0F] = kInputKeyTab;
        kScanCodeToKeyCode[0x01] = kInputKeyEsc;
        kScanCodeToKeyCode[0x1C] = kInputKeyEnter;
        kScanCodeToKeyCode[0x39] = kInputKeySpace;

        kScanCodeToKeyCode[0x3B] = kInputKey_F1;
        kScanCodeToKeyCode[0x3C] = kInputKey_F2;
        kScanCodeToKeyCode[0x3D] = kInputKey_F3;
        kScanCodeToKeyCode[0x3E] = kInputKey_F4;
        kScanCodeToKeyCode[0x3F] = kInputKey_F5;
        kScanCodeToKeyCode[0x40] = kInputKey_F6;
        kScanCodeToKeyCode[0x41] = kInputKey_F7;
        kScanCodeToKeyCode[0x42] = kInputKey_F8;
        kScanCodeToKeyCode[0x43] = kInputKey_F9;
        kScanCodeToKeyCode[0x44] = kInputKey_F10;
        kScanCodeToKeyCode[0x57] = kInputKey_F11;
        kScanCodeToKeyCode[0x58] = kInputKey_F12;


        kScanCodeToKeyCode[0x3A] = kInputKeyCaps;
        kScanCodeToKeyCode[0x45] = kInputKeyNumLock;
        kScanCodeToKeyCodeE0[0x45] = kInputKeyNumLock;
        kScanCodeToKeyCodeE0[0x1C] = kInputKeyKeypadEnter;

        kScanCodeToKeyCode[0x1D] = kInputKeyLeftControl;
        kScanCodeToKeyCode[0x2A] = kInputKeyLeftShift;
        kScanCodeToKeyCode[0x38] = kInputKeyLeftAlt;
        kScanCodeToKeyCodeE0[0x5B] = kInputKeyLeftSuper;

        kScanCodeToKeyCodeE0[0x1D] = kInputKeyRightControl;
        kScanCodeToKeyCode[0x36] = kInputKeyRightShift;
        kScanCodeToKeyCodeE0[0x38] = kInputKeyRightAlt;
        kScanCodeToKeyCodeE0[0x5C] = kInputKeyRightSuper;

        Initialized = true;
    }
}

static ANArray<UInt8> s_RawInputBuffer(8192);

static void ProcessMouseRawInput(PRAWINPUT input, Vector2f &mouseDelta, Vector2f &scrollDelta) {
    if (MOUSE_MOVE_RELATIVE == input->data.mouse.usFlags) {
        mouseDelta.x += (float) input->data.mouse.lLastX;
        mouseDelta.y += (float) -input->data.mouse.lLastY;
    }

    if (input->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
        GetInputManager().getMouse().queueButtonState(0, true);
    }
    if (input->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
        GetInputManager().getMouse().queueButtonState(0, false);
    }
    if (input->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
        GetInputManager().getMouse().queueButtonState(2, true);
    }
    if (input->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
        GetInputManager().getMouse().queueButtonState(2, false);
    }
    if (input->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
        GetInputManager().getMouse().queueButtonState(1, true);
    }
    if (input->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) {
        GetInputManager().getMouse().queueButtonState(1, false);
    }

    // update wheel

    if (input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
        scrollDelta.y += (float) input->data.mouse.usButtonData;
    } else if (input->data.mouse.usButtonData & RI_MOUSE_HWHEEL) {
        scrollDelta.x += (float) input->data.mouse.usButtonData;
    }
}

static void ProcessKeyboardRawInput(PRAWINPUT input) {
    RAWKEYBOARD *keyboard = &input->data.keyboard;

    /// we don't process E1
    if (keyboard->Flags & RI_KEY_E1) {
        return;
    }

    int path;
    if (keyboard->Flags & RI_KEY_E0) {
        path = kScanCodeToKeyCodeE0[keyboard->MakeCode & 0x00FF];
    } else {
        path = kScanCodeToKeyCode[keyboard->MakeCode];
    }

    if (path == kInputKeyUnknown) {
        return;
    }

    if (keyboard->Flags & RI_KEY_BREAK) {
        // release

        GetInputManager().getKeyboard().queueButtonState(path, false);
    } else {
        // press

        GetInputManager().getKeyboard().queueButtonState(path, true);
    }
}


static LRESULT CALLBACK InputManagerWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    switch (msg) {
        case WM_INPUT: {

            PRAWINPUT rawInputBuffer = (PRAWINPUT) s_RawInputBuffer.data();
            UINT      size           = s_RawInputBuffer.size();

            Vector2f mouseDelta{};
            Vector2f scrollDelta{};

            // First, process the message we received
            for (int i = 0; i < 1000; ++i) {
                if (-1 != GetRawInputData((HRAWINPUT) lParam,
                                          RID_INPUT,
                                          rawInputBuffer,
                                          &size,
                                          sizeof(RAWINPUTHEADER))) {
                    // only mouse events seems to be delivered when running in low integrity process

                    if (RIM_TYPEMOUSE == rawInputBuffer->header.dwType) {
                        /// mouse input
                        ProcessMouseRawInput(rawInputBuffer, mouseDelta, scrollDelta);
                    } else if (RIM_TYPEKEYBOARD == rawInputBuffer->header.dwType) {
                        ProcessKeyboardRawInput(rawInputBuffer);
                    }
                    break;
                }
                DWORD error = GetLastError();

                if (ERROR_INSUFFICIENT_BUFFER != error) {
                    AN_LOG(Error, "Failed to get input data: %s", GetLastErrorString().c_str());
                    break;
                }

                s_RawInputBuffer.resize(size);
                rawInputBuffer = reinterpret_cast<PRAWINPUT>(s_RawInputBuffer.data());
            }

            // Next, drain the raw input message queue so they don't keep getting
            // pumped one at a time through PeekMessage/DispatchMessage as that is
            // too slow with high input polling rate devices
            for (;;) {
                UINT rawInputCount = GetRawInputBuffer(rawInputBuffer, &size, sizeof(RAWINPUTHEADER));
                if (rawInputCount == 0)
                    break;

                if (rawInputCount == -1) {
                    DWORD error = GetLastError();

                    if (ERROR_INSUFFICIENT_BUFFER != error) {
                        AN_LOG(Error, "Failed to get raw input buffer: %s", GetLastErrorString().c_str());
                        break;
                    }

                    s_RawInputBuffer.resize(size);
                    rawInputBuffer = reinterpret_cast<PRAWINPUT>(s_RawInputBuffer.data());
                    continue;
                }

                PRAWINPUT input = rawInputBuffer;
                for (int j = 0; j < rawInputCount; ++j) {
                    if (input->header.dwType == RIM_TYPEMOUSE) {
                        ProcessMouseRawInput(input, mouseDelta, scrollDelta);
                    } else if (RIM_TYPEKEYBOARD == input->header.dwType) {
                        ProcessKeyboardRawInput(input);
                    }
                    input = NEXTRAWINPUTBLOCK(input);
                }
            }

            GetInputManager().getMouse().queueDelta(mouseDelta * 0.05f, scrollDelta * 0.05f);// TODO may be move this constant to Input Sensitivity
        }
            return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static const wchar_t *kWindowClassName = L"ojoie.input.window";

bool InputManager::init() {
    if (!AN::InputManager::init()) return false;

    InputInitScanCodeTable();

    WNDCLASS wc{};
    wc.lpfnWndProc = InputManagerWindowProc;
    wc.hInstance = gHInstance;
    wc.lpszClassName = kWindowClassName;
    
    if (!RegisterClass(&wc)) {
        AN_LOG(Error, "Failed to register InputManager win32 window");
        return false;
    }

    hWnd = CreateWindow(kWindowClassName, TEXT("InputManager"), 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, gHInstance, 0);

    if (!hWnd) {
        AN_LOG(Error, "Failed to create InputManager win32 window: %s", GetLastErrorString().c_str());
        return false;
    }
    /// note that Only one window per raw input device class may be registered to receive raw input within a process
    /// register raw input
    RAWINPUTDEVICE rawInputDevices[] = {
        { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, 0, hWnd },	// mouse
        { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, 0, hWnd } // keyboard
    };

    if (!RegisterRawInputDevices(rawInputDevices, ARRAYSIZE(rawInputDevices), sizeof(RAWINPUTDEVICE))) {
        AN_LOG(Error, "Failed to register devices: %s", GetLastErrorString().c_str());
        return false;
    }

    return true;
}

void InputManager::deinit() {
    DestroyWindow(hWnd);

    if (!UnregisterClass(kWindowClassName, gHInstance)) {
        AN_LOG(Error, "cannot unregister window class : %s", GetLastErrorString().c_str());
    }

    AN::InputManager::deinit();
}

}