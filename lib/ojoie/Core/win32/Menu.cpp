//
// Created by aojoie on 5/20/2023.
//


#include "Core/private/win32/Menu.hpp"
#include "Utility/Log.h"
#include "Utility/String.hpp"
#include "Utility/win32/Unicode.hpp"

#include <format>

namespace AN::WIN {

static constexpr int kMaxMenuTitleLength = 200;
static constexpr int kStartMenuItemIDs = 50000;

static bool FindMenuItemRecurse(const ULONG_PTR item, HMENU startMenu, HMENU &outMenu, int &outIndex) {
    MENUITEMINFOW info;
    info.cbSize = sizeof(info);
    info.fMask  = MIIM_DATA | MIIM_SUBMENU;

    int count = GetMenuItemCount(startMenu);
    for (int i = 0; i < count; ++i) {
        if (GetMenuItemInfoW(startMenu, i, TRUE, &info)) {
            if (info.hSubMenu == nullptr) {
                // check if this item is the one
                if (info.dwItemData == item) {
                    outMenu  = startMenu;
                    outIndex = i;
                    return true;
                }
            } else {
                // recurse into submenu
                bool found = FindMenuItemRecurse(item, info.hSubMenu, outMenu, outIndex);
                if (found)
                    return true;
            }
        }
    }

    // didn't find it here
    return false;
}
static bool FindMenuItemInMainMenu( const MenuItem* item, HMENU startMenu, HMENU& outMenu, int& outIndex) {
    return FindMenuItemRecurse( reinterpret_cast<ULONG_PTR>(item), startMenu, outMenu, outIndex );
}

static bool EqualMenuNames(const char *a, const char *b) {
    if (*a == '&')
        ++a;
    if (*b == '&')
        ++b;

    while (*a && (*a == *b)) {
        ++a;
        ++b;
        if (*a == '&')
            ++a;
        if (*b == '&')
            ++b;
    }

    return *a == 0 && *b == 0;
}

static HMENU GetSubMenuByName(HMENU menu, const char *name, int *outIndex, int *outID, ULONG_PTR *outItemData) {
    char          buffer[kMaxMenuTitleLength];
    MENUITEMINFOA item;
    item.cbSize     = sizeof(item);
    item.fMask      = MIIM_STRING | MIIM_ID | MIIM_DATA;
    item.dwTypeData = buffer;
    item.cch        = 199;

    int count = GetMenuItemCount(menu);
    for (int i = 0; i < count; ++i) {
        item.cch = 199;
        if (!GetMenuItemInfoA(menu, i, TRUE, &item)) {
            AN_LOG(Error, "Failed to get menu item info");
            continue;
        }

        if (item.cch == 0)
            continue;

        if (EqualMenuNames(name, buffer)) {
            *outIndex = i;
            if (outID)
                *outID = item.wID;
            if (outItemData)
                *outItemData = item.dwItemData;
            return GetSubMenu(menu, i);
        }
    }

    *outIndex = -1;
    if (outID)
        *outID = 0;
    if (outItemData)
        *outItemData = NULL;
    return NULL;
}

struct SpecialMenuKey {
    const char *name;
    short       vkey;
    const char *display;
};

static SpecialMenuKey s_SpecialKeys[] = {
    { "LEFT", VK_LEFT, "\xE2\x86\x90" },// UTF-8 sequences for arrows here
    { "RIGHT", VK_RIGHT, "\xE2\x86\x92" },
    { "UP", VK_UP, "\xE2\x86\x91" },
    { "DOWN", VK_DOWN, "\xE2\x86\x93" },
    { "F1", VK_F1, "F1" },
    { "F2", VK_F2, "F2" },
    { "F3", VK_F3, "F3" },
    { "F4", VK_F4, "F4" },
    { "F5", VK_F5, "F5" },
    { "F6", VK_F6, "F6" },
    { "F7", VK_F7, "F7" },
    { "F8", VK_F8, "F8" },
    { "F9", VK_F9, "F9" },
    { "F10", VK_F10, "F10" },
    { "F11", VK_F11, "F11" },
    { "F12", VK_F12, "F12" },
    { "HOME", VK_HOME, "Home" },
    { "END", VK_END, "End" },
    { "PGUP", VK_PRIOR, "PgUp" },
    { "PGDN", VK_NEXT, "PgDown" },
};

static short GetVirtualKey (char c) {
    HKL keybardLayoutHandle = GetKeyboardLayout(0);
    int virtualKey = VkKeyScanExW (ToLower(c), keybardLayoutHandle);
    if (virtualKey == -1)
        AN_LOG(Error, "GetVirtualKey: Could not map char: %c (%d) to any virtual key", c, c);
    return virtualKey;
}

const char *Menu::menuNameToWinMenuName(const std::string_view &name, int commandID, bool addShortcut) {
    static std::string res;
    res.clear();
    res.reserve(name.size());

    bool  inShortcut = false;
    ACCEL shortcut;
    shortcut.cmd   = commandID;
    shortcut.key   = 0;
    shortcut.fVirt = 0;

    if (!addShortcut) {
        res = name;
    } else {
        bool wasSpace = false;
        for (size_t i = 0; i < name.size(); ++i) {
            char c = name[i];
            if (wasSpace && !inShortcut && (c == '%' || c == '#' || c == '&' || c == '_')) {
                res.push_back('\t');
                inShortcut = true;
            }
            wasSpace = IsSpace(c);

            if (inShortcut) {
                if (c == '%') {
                    res += "Ctrl+";
                    shortcut.fVirt |= FCONTROL | FVIRTKEY;
                } else if (c == '#') {
                    res += "Shift+";
                    shortcut.fVirt |= FSHIFT | FVIRTKEY;
                } else if (c == '&') {
                    res += "Alt+";
                    shortcut.fVirt |= FALT | FVIRTKEY;
                } else if (c == '_') {
                    // nothing!
                } else {
                    // check if that's one of the special keys like "LEFT"
                    bool specialKey = false;
                    for (int j = 0; j < std::size(s_SpecialKeys); ++j) {
                        if (name.substr(i) == s_SpecialKeys[j].name) {
                            shortcut.key = s_SpecialKeys[j].vkey;
                            res += s_SpecialKeys[j].display;
                            specialKey = true;
                            break;
                        }
                    }

                    // otherwise it's a simple one letter key
                    if (!specialKey) {
                        shortcut.key = GetVirtualKey(c);
                        res += ToUpper(c);
                    } else {
                        // got a special key shortcut, stop processing further
                        break;
                    }
                }
            } else {
                res.push_back(c);
            }
        }
    }

    if (addShortcut && shortcut.key && shortcut.cmd) {
        _MenuShortcuts.push_back(shortcut);
    }

    return res.c_str();
}

bool Menu::init(MenuStyle style) {
    if (!Super::init(style)) return false;
    if (style == kMenuTopLevel) {
        hMenu = CreateMenu();
    } else {
        hMenu = CreatePopupMenu();
    }
    return hMenu != nullptr;
}

Menu::~Menu() {
    if (hMenu) {
        DestroyMenu(hMenu);
        hMenu = nullptr;
    }
}

void Menu::doInsertMenu(HMENU menu, MenuItem *menuItem, int index, UINT mask, HMENU submenu) {
    MENUITEMINFOW item;
    memset(&item, 0, sizeof(item));
    item.cbSize = sizeof(item);

    item.fMask    = mask;
    item.wID      = _MenuIDToItem.size() + kStartMenuItemIDs;
    item.hSubMenu = submenu;

    std::wstring wideStr = Utf8ToWide(menuNameToWinMenuName(menuItem->m_Name, item.wID, true));
    if (wideStr.length() > kMaxMenuTitleLength) {
        wideStr.resize(kMaxMenuTitleLength - 1);
    }

    item.dwTypeData = wideStr.data();

    item.fState     = (menuItem->enabled ? MFS_ENABLED : MFS_DISABLED) | (menuItem->checked ? MFS_CHECKED : 0);
    item.dwItemData = (ULONG_PTR) menuItem;
    if (InsertMenuItemW(menu, index, TRUE, &item)) {
        _MenuIDToItem.push_back(menuItem);
    } else {
        AN_LOG(Error, "Failed to insert item. Name: %s, Command: %s", menuItem->m_Name.c_str(), menuItem->m_Command.c_str());
    }
}

static void AppendSeparator(HMENU menu) {
    MENUITEMINFO item;
    item.cbSize     = sizeof(item);
    item.fMask      = MIIM_FTYPE | MIIM_DATA;
    item.fType      = MFT_SEPARATOR;
    item.dwItemData = (ULONG_PTR) -1;
    if (!InsertMenuItem(menu, GetMenuItemCount(menu), TRUE, &item)) {
        AN_LOG(Error, "Failed to add separator");
    }
}

void Menu::addSubMenuItems(HMENU menu, std::list<MenuItem> &items) {
    int pos = -1;
    for (MenuItem &item : items) {
        // If we have a jump in position, it means we have insertion from 2 different places, so put a separator in
        if (pos != -1 && item.m_Position > pos + 10)
            AppendSeparator(menu);

        pos = item.m_Position;

        if (!item.m_Name.empty()) {
            if (item.m_Submenu != nullptr) {
                WIN::Menu *submenu = (WIN::Menu *) item.m_Submenu;
                doInsertMenu(menu, &item, GetMenuItemCount(menu), MIIM_STRING | MIIM_SUBMENU | MIIM_ID | MIIM_DATA, submenu->hMenu);
                addSubMenuItems(submenu->hMenu, submenu->_menus);
            } else {
                doInsertMenu(menu, &item, GetMenuItemCount(menu), MIIM_STRING | MIIM_STATE | MIIM_ID | MIIM_DATA, 0);
            }

        } else {
            AppendSeparator(menu);
        }
    }
}

void Menu::rebuildMenu() {

    // delete items in menu that are created by us earlier
    MENUITEMINFO itemInfo;
    itemInfo.cbSize = sizeof(itemInfo);
    itemInfo.fMask  = MIIM_DATA;
    for (int j = 0; j < GetMenuItemCount(hMenu); /**/) {
        if (GetMenuItemInfo(hMenu, j, TRUE, &itemInfo)) {
            if (itemInfo.dwItemData != NULL) {
                DeleteMenu(hMenu, j, MF_BYPOSITION);
                continue;
            }
        }
        ++j;
    }

    addSubMenuItems(hMenu, _menus);
}

void Menu::setChecked(const std::string_view &menuName, bool checked) {
    MenuItem *item = findItem(menuName);
    if (item == nullptr) {
        AN_LOG(Warning, "%s", std::format("Item {} cannot be checked because it doesn't exist", menuName).c_str());
        return;
    }

    item->checked = checked;
    HMENU menu;
    int   itemIndex;
    if (FindMenuItemInMainMenu(item, hMenu, menu, itemIndex)) {
        CheckMenuItem(menu, itemIndex, MF_BYPOSITION | (checked ? MF_CHECKED : MF_UNCHECKED));
    }
}

void Menu::setEnabled(const std::string_view &menuName, bool enabled) {
    MenuItem *item = findItem(menuName);
    if (item == nullptr) {
        AN_LOG(Warning, "%s", std::format("Item {} cannot be checked because it doesn't exist", menuName).c_str());
        return;
    }

    item->enabled = enabled;

    HMENU menu;
    int   itemIndex;
    if (FindMenuItemInMainMenu(item, hMenu, menu, itemIndex)) {
        EnableMenuItem(menu, itemIndex, MF_BYPOSITION | (enabled ? MF_ENABLED : MF_GRAYED));
    }
}

bool Menu::executeMenuItemWithID( int id ) {

    int index = id - kStartMenuItemIDs;
    int numIDs = _MenuIDToItem.size();
    if( index < 0 || index >= numIDs )
        return false;

    MenuItem* item = _MenuIDToItem[index];

    return executeMenuItem(*item);
}

}