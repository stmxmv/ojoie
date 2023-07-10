//
// Created by aojoie on 5/20/2023.
//

#ifndef OJOIE_MENU_HPP
#define OJOIE_MENU_HPP

#include <ojoie/Template/RC.hpp>
#include <string>
#include <list>

namespace AN {

class MenuItem;

class MenuInterface : public RefCounted<MenuInterface> {
public:
    virtual ~MenuInterface() = default;

    virtual void execute(const MenuItem &menuItem) = 0;
};

class Menu;

struct AN_API MenuItem {
    int m_Position;				///< Integer position (this is what we sort by)
    std::string m_Name;			///< Name of the command (without prefix)
    std::string m_Command;		///< Command name. If empty, item is a separator
    MenuInterface *m_Target;	///< the target for the event.
    Menu* m_Submenu;		///< Pointer to a submenu.
    MenuItem* m_Parent;
    int     contextUserData;
    bool	checked;
    bool	enabled;

    MenuItem (int pos, std::string_view name, std::string_view cmd,  MenuInterface *target, Menu *submenu);
    ~MenuItem ();

    /// create a standard menu item.
    static MenuItem CreateItem(int pos, std::string_view name, std::string_view cmd, MenuInterface *target);
    static MenuItem CreateSubmenu(int pos, Menu *submenu, std::string_view name);
    static MenuItem CreateSeparator(int pos);

};

enum MenuStyle {
    kMenuTopLevel,
    kMenuPopup // submenu will always be this style
};

class AN_API Menu : public RefCounted<Menu> {

protected:

    std::list<MenuItem> _menus;

    void insertItem(const MenuItem &item);

public:

    static Menu *Alloc();

    virtual bool init(MenuStyle style) { return true; }

    virtual ~Menu();

    void addMenuItem(const std::string_view &menuName,
                     const std::string_view &command,
                     MenuInterface *obj = nullptr,
                     int position = 100);

    void addSubMenu(const std::string_view &menuName, Menu *submenu, int position = 100);

    void addSeparator(const std::string_view &menuName, int position = 100);

    virtual void setChecked(const std::string_view &menuName, bool checked) = 0;

    bool getChecked(const std::string_view &menuName);

    virtual void setEnabled(const std::string_view &menuName, bool enabled) = 0;

    bool getEnabled(const std::string_view &menuName);

    MenuItem *findItem(const std::string_view &menuName);

    bool removeMenuItem(const std::string_view &menuName);

    bool executeMenuItem(MenuItem& menuItem);

};


}// namespace AN

#endif//OJOIE_MENU_HPP
