//
// Created by aojoie on 5/20/2023.
//

#ifndef OJOIE_WIN_MENU_HPP
#define OJOIE_WIN_MENU_HPP

#include <ojoie/Core/Menu.hpp>
#include <Windows.h>

#include <vector>

namespace AN::WIN {


class Menu : public AN::Menu {

    typedef AN::Menu Super;

    HMENU hMenu;

    typedef std::vector<MenuItem*>	MenuIDToItem;
    MenuIDToItem	_MenuIDToItem;

    typedef std::vector<ACCEL>	ShortcutVector;
    ShortcutVector	_MenuShortcuts;

    const char*  menuNameToWinMenuName(const std::string_view& name, int commandID, bool addShortcut );
    void doInsertMenu(HMENU menu, MenuItem *menuItem, int index, UINT mask, HMENU submenu);
    void addSubMenuItems( HMENU menu, std::list<MenuItem> &items );
public:

    Menu() : hMenu() {}

    virtual bool init(MenuStyle style) override;

    virtual ~Menu() override;

    void rebuildMenu();

    virtual void setChecked(const std::string_view &menuName, bool checked) override;
    virtual void setEnabled(const std::string_view &menuName, bool enabled) override;

    bool executeMenuItemWithID(int id);

    HMENU getHMENU() const { return hMenu; }

    const ShortcutVector &getAccelerators() const { return _MenuShortcuts; }
};

}


#endif//OJOIE_WIN_MENU_HPP
