//
// Created by aojoie on 5/17/2023.
//

#include "IMGUI/IMGUI.hpp"
#include "IMGUI/IMGUIManager.hpp"
#include "Core/Actor.hpp"

namespace AN {

IMPLEMENT_AN_CLASS_HAS_INIT_ONLY(IMGUI)
LOAD_AN_CLASS(IMGUI)

IMGUI::~IMGUI() {}

void IMGUI::InitializeClass() {
    GetClassStatic()->registerMessageCallback(kDidAddComponentMessage, OnAddComponentMessage);
}

void IMGUI::OnAddComponentMessage(void *receiver, Message &message) {
    IMGUI *self = (IMGUI *)receiver;
    if (message.getData<IMGUI *>() == self) {
        GetIMGUIManager().addIMGUIComponent(self->_listNode);
    }
}

}