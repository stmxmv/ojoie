//
// Created by aojoie on 4/23/2023.
//

#include "Core/private/win32/Screen.hpp"
#include "Core/Screen.hpp"



namespace AN::WIN {

static HMONITOR GetPrimaryMonitorHandle() {
    const POINT ptZero = { 0, 0 };
    return MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
}

Size Screen::getSize() {
//    MONITORINFO info{};
//    GetMonitorInfoW(hMonitor, &info);
//    return { .width = (double)info.rcMonitor.right - info.rcMonitor.left,
//             .height = (double)info.rcMonitor.bottom - info.rcMonitor.top };

    DEVMODEW dm;
    dm.dmSize = sizeof(dm);
    EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &dm);
    return { .width = dm.dmPelsWidth, .height = dm.dmPelsHeight };
}

int Screen::getRefreshRate() {
    DEVMODEW dm;
    dm.dmSize = sizeof(dm);
    EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &dm);
    return (int) dm.dmDisplayFrequency;
}

}