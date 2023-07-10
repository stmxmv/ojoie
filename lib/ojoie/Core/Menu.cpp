//
// Created by aojoie on 5/20/2023.
//

#include "Core/Menu.hpp"
#include "Utility/Log.h"
#ifdef _WIN32
#include "Core/private/win32/Menu.hpp"
#endif

namespace AN {

MenuItem::MenuItem(int pos, std::string_view name, std::string_view cmd, MenuInterface *target, Menu *submenu) {
    m_Position = pos;
    m_Name = name;
    m_Command = cmd;
    m_Target = target;
    m_Submenu = submenu;
    contextUserData = 0;
    checked = false;
    enabled = true;
    m_Parent = nullptr;
}

MenuItem::~MenuItem() {}

MenuItem MenuItem::CreateItem(int pos, std::string_view name, std::string_view cmd, MenuInterface *target) {
    return MenuItem (pos, name, cmd, target, nullptr);
}
MenuItem MenuItem::CreateSubmenu(int pos, Menu *submenu, std::string_view name) {
    return MenuItem (pos, name, "", nullptr, submenu);
}
MenuItem MenuItem::CreateSeparator(int pos) {
    return MenuItem(pos, "", "", nullptr, nullptr);
}

Menu *Menu::Alloc() {
#ifdef _WIN32
    return new WIN::Menu();
#else
#error "not implement"
#endif
}

Menu::~Menu() {
    for (MenuItem &item : _menus) {
        if (item.m_Target) {
            item.m_Target->release();
        }
        if (item.m_Submenu) {
            item.m_Submenu->release();
        }
    }
    _menus.clear();
}

void Menu::insertItem(const MenuItem &item) {
    for (auto it = _menus.begin(); it != _menus.end(); ++it) {
        if (it->m_Position > item.m_Position) {
            _menus.insert(it, item);
            return;
        }
    }
    _menus.push_back(item);
}

void Menu::addMenuItem(const std::string_view &menuName, const std::string_view &command, MenuInterface *obj, int position) {
    if (obj) {
        obj->retain();
    }
    insertItem(MenuItem::CreateItem(position, menuName, command, obj));
}

void Menu::addSubMenu(const std::string_view &menuName, Menu *submenu, int position) {
    submenu->retain();
    insertItem(MenuItem::CreateSubmenu(position, submenu, menuName));
}

void Menu::addSeparator(const std::string_view &menuName, int position) {
    insertItem(MenuItem::CreateSeparator(position));
}

MenuItem *Menu::findItem(const std::string_view &menuName) {
    for (MenuItem &item : _menus) {
        if (item.m_Name == menuName) {
            return &item;
        }
    }
    return nullptr;
}

bool Menu::removeMenuItem(const std::string_view &menuName) {
    for (auto it = _menus.begin(); it != _menus.end(); ++it) {
        if (it->m_Name == menuName) {
            if (it->m_Target) {
                it->m_Target->release();
            }
            if (it->m_Submenu) {
                it->m_Submenu->release();
            }
            _menus.erase(it);
            return true;
        }
    }
    return false;
}

bool Menu::getChecked(const std::string_view &menuName) {
    for (MenuItem &item : _menus) {
        if (item.m_Name == menuName) {
            return item.checked;
        }
    }
    return false;
}
bool Menu::getEnabled(const std::string_view &menuName) {
    for (MenuItem &item : _menus) {
        if (item.m_Name == menuName) {
            return item.enabled;
        }
    }
    return false;
}

bool Menu::executeMenuItem(MenuItem &item) {
    if (item.m_Target == nullptr) {
        AN_LOG(Error, "ExecuteMenuItem target for %s does not exist", item.m_Name.c_str());
        return false;
    }

//    if (gExecuteMenuItemCallback)
//        gExecuteMenuItemCallback(item);

    item.m_Target->execute(item);

    return true;
}


}