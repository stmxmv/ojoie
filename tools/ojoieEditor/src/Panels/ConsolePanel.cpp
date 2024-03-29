//
// Created by aojoie on 6/24/2023.
//

#include "Panels/ConsolePanel.hpp"
#include <ojoie/Threads/Dispatch.hpp>


#include <imgui_internal.h>

namespace AN::Editor {


static void ANLogCallback(const char *log, size_t size, void *userdata) {
    /// this can be call in any thread
    auto task = [log = std::string(log), userdata] {
        ConsolePanel *self = (ConsolePanel *) userdata;
        self->addLog("%s", log.c_str());
        fprintf(stderr, "%s", log.c_str());
        fflush(stderr);
    };

    if (GetCurrentThreadID() != Dispatch::GetThreadID(Dispatch::Game)) {
        Dispatch::async(Dispatch::Game, task);
    } else {
        task();
    }
}

ConsolePanel::ConsolePanel() {
    AutoScroll = true;
    clear();

    ANLogSetCallback(ANLogCallback, this);
}

ConsolePanel::~ConsolePanel() {
    ANLogResetCallback();
}

void ConsolePanel::contextMenu(int n, const char *log) {
    // <-- use last item id as popup id
    if (ImGui::BeginPopupContextItem()) {
        selected = n;
        if (ImGui::MenuItem("Copy")) {
            ImGui::CloseCurrentPopup();
            ImGui::SetClipboardText(log);
        }
        ImGui::EndPopup();
    }
}

void ConsolePanel::clear() {
    Buf.clear();
    LineOffsets.clear();
    LineOffsets.push_back(0);
}

void ConsolePanel::addLog(const char *fmt, ...) {
    int     old_size = Buf.size();
    va_list args;
    va_start(args, fmt);
    Buf.appendfv(fmt, args);
    va_end(args);
    for (int new_size = Buf.size(); old_size < new_size; old_size++)
        if (Buf[old_size] == '\n')
            LineOffsets.push_back(old_size + 1);
}

void ConsolePanel::onGUI() {
    if (!ImGui::Begin("Console", getOpenPtr())) {
        ImGui::End();
        return;
    }

    // Options menu
    if (ImGui::BeginPopup("Options")) {
        ImGui::Checkbox("Auto-scroll", &AutoScroll);
        ImGui::EndPopup();
    }

    // Main window
    if (ImGui::Button("Options"))
        ImGui::OpenPopup("Options");
    ImGui::SameLine();
    bool shouldClear = ImGui::Button("Clear");
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");
    ImGui::SameLine();
    ImGui::Text("   Filter");
    ImGui::SameLine();
    Filter.Draw("##_Filter", -100.0f);

    ImGui::Separator();

    if (ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {
        if (shouldClear)
            clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        if (!Buf.empty()) {
            const char *buf        = Buf.begin();
            const char *buf_end    = Buf.end();
            int         lineNumber = 0;
            if (Filter.IsActive()) {
                // In this example we don't use the clipper when Filter is enabled.
                // This is because we don't have random access to the result of our filter.
                // A real application processing logs with ten of thousands of entries may want to store the result of
                // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
                for (int line_no = 0; line_no < LineOffsets.Size; line_no++) {
                    const char *line_start = buf + LineOffsets[line_no];
                    char       *line_end   = (char *) ((line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end);
                    if (Filter.PassFilter(line_start, line_end)) {
                        // ImGui::TextUnformatted(line_start, line_end);

                        if (line_start[0] == 0 || line_start[0] == '\n') continue;

                        char old    = *line_end;
                        line_end[0] = 0;

                        ImGui::PushID(line_start);
                        ImVec2 pos      = ImGui::GetCurrentWindow()->DC.CursorPos;
                        ImVec2 textSize = ImGui::CalcTextSize(line_start);
                        float  width    = std::max(textSize.x + 10.f, ImGui::GetContentRegionAvail().x);
                        if (ImGui::Selectable("", lineNumber == selected,
                                              ImGuiSelectableFlags_AllowDoubleClick, ImVec2(width, 50.f))) {
                            if (ImGui::IsMouseDoubleClicked(0)) {
                                // Double-click action, if desired
                            }
                            selected = lineNumber;
                        }

                        ImGui::RenderText(pos + ImVec2(5.f, 25.f - textSize.y / 2.f), line_start);

                        contextMenu(lineNumber, line_start);

                        line_end[0] = old;
                        ++lineNumber;

                        ImGui::PopID();
                    }
                }
            } else {
                // The simplest and easy way to display the entire buffer:
                //   ImGui::TextUnformatted(buf_begin, buf_end);
                // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
                // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
                // within the visible area.
                // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
                // on your side is recommended. Using ImGuiListClipper requires
                // - A) random access into your data
                // - B) items all being the  same height,
                // both of which we can handle since we have an array pointing to the beginning of each line of text.
                // When using the filter (in the block of code above) we don't have random access into the data to display
                // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
                // it possible (and would be recommended if you want to search through tens of thousands of entries).
                ImGuiListClipper clipper;
                clipper.Begin(LineOffsets.Size);

                while (clipper.Step()) {
                    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
                        const char *line_start = buf + LineOffsets[line_no];
                        char       *line_end   = (char *) ((line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end);
                        //                        ImGui::TextUnformatted(line_start, line_end);

                        if (line_start[0] == 0 || line_start[0] == '\n') continue;

                        char old    = *line_end;
                        line_end[0] = 0;

                        ImGui::PushID(line_start);

                        ImVec2 pos      = ImGui::GetCurrentWindow()->DC.CursorPos;
                        ImVec2 textSize = ImGui::CalcTextSize(line_start);
                        float  width    = std::max(textSize.x + 10.f, ImGui::GetContentRegionAvail().x);
                        if (ImGui::Selectable("", lineNumber == selected,
                                              ImGuiSelectableFlags_AllowDoubleClick, ImVec2(width, 50.f))) {
                            if (ImGui::IsMouseDoubleClicked(0)) {
                                // Double-click action, if desired
                            }
                            selected = lineNumber;
                        }

                        ImGui::RenderText(pos + ImVec2(5.f, 25.f - textSize.y / 2.f), line_start);

                        contextMenu(lineNumber, line_start);
                        line_end[0] = old;
                        ++lineNumber;

                        ImGui::PopID();
                    }
                }
                clipper.End();
            }
        }

        ImGui::PopStyleVar();

        // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
        // Using a scrollbar or mouse-wheel will take away from the bottom edge.
        if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    ImGui::End();
}
}// namespace AN::Editor