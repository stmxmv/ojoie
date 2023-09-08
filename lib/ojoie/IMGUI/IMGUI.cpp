//
// Created by aojoie on 5/17/2023.
//

#include "IMGUI/IMGUI.hpp"
#include "Core/Actor.hpp"
#include "IMGUI/IMGUIManager.hpp"

#include <imgui_internal.h>

namespace AN {

IMPLEMENT_AN_CLASS_INIT(IMGUI)
LOAD_AN_CLASS(IMGUI)

IMGUI::~IMGUI() {}

void IMGUI::InitializeClass() {
    GetClassStatic()->registerMessageCallback(kDidAddComponentMessage, OnAddComponentMessage);
}

void IMGUI::OnAddComponentMessage(void *receiver, Message &message) {
    IMGUI *self = (IMGUI *) receiver;
    if (message.getData<IMGUI *>() == self) {
        GetIMGUIManager().addIMGUIComponent(self->_listNode);
    }
}

void ItemLabel(const std::string &title, ItemLabelFlag flags) {
    ImGuiWindow      *window    = ImGui::GetCurrentWindow();
    const ImVec2      lineStart = ImGui::GetCursorScreenPos();
    const ImGuiStyle &style     = ImGui::GetStyle();
    float             fullWidth = ImGui::GetContentRegionAvail().x;
    float             itemWidth = ImGui::CalcItemWidth() + style.ItemSpacing.x * 16.f; // mul a value to get a widen space
    ImVec2            textSize  = ImGui::CalcTextSize(title.c_str());
    ImRect            textRect;
    textRect.Min = ImGui::GetCursorScreenPos();
    if (flags & kItemLabelRight)
        textRect.Min.x = textRect.Min.x + itemWidth;
    else
        textRect.Min.x += style.ItemSpacing.x * 8.f; // mul a value to get a widen space
    textRect.Max = textRect.Min;
    textRect.Max.x += fullWidth - itemWidth;
    textRect.Max.y += textSize.y;

    ImGui::SetCursorScreenPos(textRect.Min);

    ImGui::AlignTextToFramePadding();
    // Adjust text rect manually because we render it directly into a drawlist instead of using public functions.
    textRect.Min.y += window->DC.CurrLineTextBaseOffset;
    textRect.Max.y += window->DC.CurrLineTextBaseOffset;

    ImGui::ItemSize(textRect);
    if (ImGui::ItemAdd(textRect, window->GetID(title.data(), title.data() + title.size()))) {
        ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), textRect.Min, textRect.Max, textRect.Max.x,
                                  textRect.Max.x, title.data(), title.data() + title.size(), &textSize);

        if (textRect.GetWidth() < textSize.x && ImGui::IsItemHovered())
            ImGui::SetTooltip("%.*s", (int) title.size(), title.data());
    }
    if (flags & kItemLabelLeft) {
        ImGui::SetCursorScreenPos(textRect.Max - ImVec2{ 0, textSize.y + window->DC.CurrLineTextBaseOffset });
        ImGui::SameLine();
    } else if (flags & kItemLabelRight)
        ImGui::SetCursorScreenPos(lineStart);
}

void AlignForWidth(float width, float alignment) {
    ImGuiStyle &style = ImGui::GetStyle();
    float       avail = ImGui::GetContentRegionMax().x;
    float       off   = (avail - width) * alignment;
    if (off > 0.0f)
        ImGui::SetCursorPosX(off);
}

}// namespace AN