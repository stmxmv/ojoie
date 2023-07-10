//
// Created by aojoie on 6/24/2023.
//

#pragma once

#include "EditorPanel.hpp"

namespace AN::Editor {

class ConsolePanel : public Panel {

    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool                AutoScroll;  // Keep scrolling if already at the bottom.

    int selected = 0;

    void contextMenu(int n, const char *log);

public:

    ConsolePanel();

    ~ConsolePanel() override;

    void clear();

    void addLog(const char* fmt, ...) IM_FMTARGS(2);

    virtual void onGUI() override;
};

}
