//
// Created by aojoie on 5/15/2023.
//

#ifndef OJOIE_INPUTCOMPONENT_HPP
#define OJOIE_INPUTCOMPONENT_HPP

#include <ojoie/Core/Component.hpp>
#include <ojoie/Input/InputAction.hpp>
#include <ojoie/Input/InputControlPath.hpp>
#include <ojoie/Template/LinkedList.hpp>

namespace AN {

class InputComponent;
typedef ListNode<InputComponent> InputComponentListNode;

class AN_API InputComponent : public Component {

    InputActionMap *_actionMap;
    InputComponentListNode _listNode{ this };
    DECLARE_DERIVED_AN_CLASS(InputComponent, Component)

public:

    explicit InputComponent(ObjectCreationMode mode) : Super(mode) {}

    static void InitializeClass();

    virtual bool init() override;

    void setActionMap(InputActionMap *actionMap) { _actionMap = actionMap; }

    void update();

};

}

#endif//OJOIE_INPUTCOMPONENT_HPP
