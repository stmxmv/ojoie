//
// Created by aojoie on 5/15/2023.
//

#ifndef OJOIE_INPUTCONTROLPATH_HPP
#define OJOIE_INPUTCONTROLPATH_HPP

namespace AN {


constexpr static int kInputControlPathMaxNum = 512;

enum InputControlPath {
    kPointerX     = 430,
    kPointerY     = 431,
    kPointerDelta = 432,
    kMouseScroll  = 433,

    kMouseLeftButton   = 434,
    kMouseMiddleButton = 435,
    kMouseRightButton  = 436,
};

/// Some keys have been cleverly chosen to map to ASCII
/// characters map to lowercase
enum InputKey {
    kInputKeyUnknown           = 0,

    kInputKeyLeftShift         = 201,
    kInputKeyLeftControl       = 202,
    kInputKeyLeftAlt           = 203,
    kInputKeyLeftSuper         = 204,
    kInputKeyRightShift        = 205,
    kInputKeyRightControl      = 206,
    kInputKeyRightAlt          = 207,
    kInputKeyRightSuper        = 208,
    kInputKeyCaps              = 209,
    kInputKeyNumLock           = 210,
    kInputKeyKeypadEnter       = 211,

    kInputKey_F1 = 282,
    kInputKey_F2 = 283,
    kInputKey_F3 = 284,
    kInputKey_F4 = 285,
    kInputKey_F5 = 286,
    kInputKey_F6 = 287,
    kInputKey_F7 = 288,
    kInputKey_F8 = 289,
    kInputKey_F9 = 290,
    kInputKey_F10 = 291,
    kInputKey_F11 = 292,
    kInputKey_F12 = 293,

    /// begin map to ASCII
    kInputKeyTab               = 9,
    kInputKeyEsc               = 27,
    kInputKeyEnter             = 13,
    kInputKeySpace             = 32,

    kInputKey_0    = 48,
    kInputKey_1    = 49,
    kInputKey_2    = 50,
    kInputKey_3    = 51,
    kInputKey_4    = 52,
    kInputKey_5    = 53,
    kInputKey_6    = 54,
    kInputKey_7    = 55,
    kInputKey_8    = 56,
    kInputKey_9    = 57,

    kInputKey_A    = 97,
    kInputKey_B    = 98,
    kInputKey_C    = 99,
    kInputKey_D    = 100,
    kInputKey_E    = 101,
    kInputKey_F    = 102,
    kInputKey_G    = 103,
    kInputKey_H    = 104,
    kInputKey_I    = 105,
    kInputKey_J    = 106,
    kInputKey_K    = 107,
    kInputKey_L    = 108,
    kInputKey_M    = 109,
    kInputKey_N    = 110,
    kInputKey_O    = 111,
    kInputKey_P    = 112,
    kInputKey_Q    = 113,
    kInputKey_R    = 114,
    kInputKey_S    = 115,
    kInputKey_T    = 116,
    kInputKey_U    = 117,
    kInputKey_V    = 118,
    kInputKey_W    = 119,
    kInputKey_X    = 120,
    kInputKey_Y    = 121,
    kInputKey_Z    = 122,

    /// end map to ASCII
};




}

#endif//OJOIE_INPUTCONTROLPATH_HPP
