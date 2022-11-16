//
// Created by aojoie on 10/6/2022.
//

#ifndef OJOIE_MENUBAR_HPP
#define OJOIE_MENUBAR_HPP

#include "EditorPanel.hpp"

namespace AN::Editor {

inline static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

class DockSpace : public Panel {
    typedef DockSpace Self;
    typedef Panel Super;

public:

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }


    virtual bool init() override {
        return Super::init();
    }

    virtual void draw(const RenderContext &context) override {
        Super::draw(context);

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_window_flags = 0;
        host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
        host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        host_window_flags |= ImGuiWindowFlags_MenuBar;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("MainDockSpaceViewport", NULL, host_window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                if (ImGui::MenuItem("Open", NULL)) {
                    AN::Dispatch::async(AN::Dispatch::Main, [this, window = context.window]{
                        AN::OpenPanel openPanel;
                        openPanel.init();
                        openPanel.allowOtherTypes = true;
                        static const char *audioDescription = "Supported Audio File";
                        openPanel.addAllowContentExtension(audioDescription, "wav");
                        openPanel.addAllowContentExtension(audioDescription, "mp3");
                        openPanel.addAllowContentExtension(audioDescription, "flac");
                        openPanel.setTitle("Choose Wav File");
                        openPanel.beginSheetModal(window, [this](AN::Application::ModalResponse response, const char *filePath) {
                            if (response == AN::Application::ModalResponse::Ok && filePath) {


                            }
                        });
                    });
                }
                ImGui::Separator();

                ImGui::EndMenu();
            }
            HelpMarker(
                    "When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n"
                    "- Drag from window title bar or their tab to dock/undock." "\n"
                    "- Drag from window menu button (upper-left button) to undock an entire node (all windows)." "\n"
                    "- Hold SHIFT to disable docking (if io.ConfigDockingWithShift == false, default)" "\n"
                    "- Hold SHIFT to enable docking (if io.ConfigDockingWithShift == true)" "\n"
                    "This demo app has nothing to do with enabling docking!" "\n\n"
                    "This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window." "\n\n"
                    "Read comments in ShowExampleAppDockSpace() for more details.");

            ImGui::EndMenuBar();
        }
        ImGui::End();
    }
};


}

#endif//OJOIE_MENUBAR_HPP
