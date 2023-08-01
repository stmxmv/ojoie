//
// Created by aojoie on 5/17/2023.
//

#ifndef OJOIE_IMGUI_HPP
#define OJOIE_IMGUI_HPP

#include <ojoie/Core/Component.hpp>
#include <ojoie/Template/LinkedList.hpp>

#include <ojoie/IMGUI/IconsFontAwesome6.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace AN {

class IMGUI;
typedef ListNode<IMGUI> IMGUIListNode;
typedef List<IMGUIListNode> IMGUIList;

class AN_API IMGUI : public Component {

    IMGUIListNode _listNode{ this };

    DECLARE_DERIVED_AN_CLASS(IMGUI, Component)

    static void OnAddComponentMessage(void *receive, Message &message);

public:

    explicit IMGUI(ObjectCreationMode mode) : Super(mode) {}

    static void InitializeClass();

    virtual void onGUI() {}

};

enum ItemLabelFlag {
    kItemLabelLeft = 1u << 0u,
    kItemLabelRight = 1u << 1u,
    kItemLabelDefault = kItemLabelLeft,
};

AN_API void ItemLabel(const std::string &title, ItemLabelFlag flags);

AN_API void AlignForWidth(float width, float alignment = 0.5f);

}

#endif//OJOIE_IMGUI_HPP
