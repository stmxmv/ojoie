//
// Created by aojoie on 6/24/2023.
//

#include "MainIMGUI.hpp"
#include "AppDelegate.hpp"

#include "Panels/ViewportPanel.hpp"
#include "Panels/ConsolePanel.hpp"
#include "Panels/InspectorPanel.hpp"
#include "Panels/ProjectPanel.hpp"
#include "Panels/HierarchyPanel.hpp"

#include <ojoie/Threads/Dispatch.hpp>
#include <ojoie/Core/Game.hpp>
#include <ojoie/Core/App.hpp>
#include <ojoie/Core/Event.hpp>
#include <ojoie/Core/DragAndDrop.hpp>
#include <ojoie/Render/TextureLoader.hpp>
#include <ojoie/Input/InputManager.hpp>
#include <ojoie/IMGUI/IconsFontAwesome6Brands.h>
#include <imgui_internal.h>

#include <ojoie/Physics/PhysicsManager.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace AN::Editor {

IMPLEMENT_AN_CLASS(MainIMGUI)
LOAD_AN_CLASS(MainIMGUI)

MainIMGUI::MainIMGUI(ObjectCreationMode mode) : Super(mode), play(), pause() {}
MainIMGUI::~MainIMGUI() {}


bool ButtonCenteredOnLine(const char* label, const ImVec2 &size, float alignment = 0.5f) {
    ImGuiStyle& style = ImGui::GetStyle();

    float width = std::max(ImGui::CalcTextSize(label).x, size.x) + style.FramePadding.x * 2.0f;
    float avail = ImGui::GetContentRegionAvail().x;

    float off = (avail - width) * alignment;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

    return ImGui::Button(label, size);
}

static void AlignForWidth(float width, float alignment = 0.5f) {
    ImGuiStyle& style = ImGui::GetStyle();
    float avail = ImGui::GetContentRegionMax().x;
    float off = (avail - width) * alignment;
    if (off > 0.0f)
        ImGui::SetCursorPosX(off);
}

static void PushButtonState(bool state) {
    if (state) {
        ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
        ImGui::PushStyleColor(ImGuiCol_Button, col);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    }
}

static void PopButtonState() {
    ImGui::PopStyleColor(2);
}

static void DockBuilderFocusWindow(const char *window) {
    auto asset = ImGui::FindWindowByName(window);
    ImGui::FocusWindow(asset);
}

class DemoPanel : public Panel {

public:

    DemoPanel() {
        setOpen(false);
    }

    virtual void onGUI() override {
        ImGui::ShowDemoWindow(getOpenPtr());
    }
};

class InfoPanel : public Panel {

    float f = 0.0f;
    int counter = 0;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    Texture2D *tex;
    bool dragAndDropUpdating;

    bool *demoPanelOpenP{};

public:

    InfoPanel() : dragAndDropUpdating() {
        tex = NewObject<Texture2D>();
        TextureDescriptor descriptor{};
        descriptor.width = 255;
        descriptor.height = 255;
        descriptor.mipmapLevel = 1;
        descriptor.pixelFormat = kPixelFormatRGBA8Unorm_sRGB;

        std::vector<UInt8> data(255 * 255 * 4, 255); // set a white image

        SamplerDescriptor samplerDescriptor = Texture::DefaultSamplerDescriptor();
        samplerDescriptor.filter = AN::kSamplerFilterNearest;

        ANAssert(tex->init(descriptor, samplerDescriptor));
        tex->setPixelData(data.data());
        tex->setReadable(true);
        tex->uploadToGPU(false);
    }

    virtual void onGUI() override {

        if (demoPanelOpenP == nullptr) {
            DemoPanel *demoPanel = GetMainIMGUI().findPanelOfType<DemoPanel>();
            if (demoPanel) {
                demoPanelOpenP = demoPanel->getOpenPtr();
            }
        }
        ImGui::Begin("Debug");// Create a window called "Hello, world!" and append into it.

        if (ImGui::Button("Test Log")) {
            AN_LOG(Debug, "%s", std::format("Log Test {}", rand()).c_str());
        }

        ImGui::Text("This is some useful text.");         // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", demoPanelOpenP);// Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", demoPanelOpenP);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float *) &clear_color);// Edit 3 floats representing a color

        if (ImGui::Button("Button"))// Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f * GetGame().deltaTime, 1.f / GetGame().deltaTime);

        Mouse &mouse = GetInputManager().getMouse();
        Vector2f mouseDelta = mouse.getMouseDelta().getValue();
        Vector2f scrollDelta = mouse.getMouseScroll().getValue();
        ImGui::Text("Mouse delta X: %f Y: %f ", mouseDelta.x, mouseDelta.y);
        ImGui::Text("Mouse Scroll delta X: %f Y: %f", scrollDelta.x, scrollDelta.y);
        ImGui::Text("Mouse Left Button: %d Middle Button: %d Right Button: %d",
                    mouse.getMouseLeft().isPress(), mouse.getMouseMiddle().isPress(), mouse.getMouseRight().isPress());

        if (ImGui::TreeNode("Keyboard")) {

            struct Key {
                const char* lib = nullptr;
                unsigned int order;
                unsigned int keyCode = 0;
                float offset = 0;
                float width = 40;
            };

            static const Key Keys[6][18] = {
                { { "ESC", 4, kInputKeyEsc, 18, 40 },  { "F1", 5, kInputKey_F1, 18, 40 },  { "F2", 6, kInputKey_F2, 0, 40 },  { "F3", 7, kInputKey_F3, 0, 40 },  { "F4", 8, kInputKey_F4, 0, 40 },  { "F5", 9, kInputKey_F5, 24, 40 },  { "F6", 10, kInputKey_F6, 0, 40 },  { "F7", 11, kInputKey_F7, 0, 40 },  { "F8", 12, kInputKey_F8, 0, 40 },  { "F9", 13, kInputKey_F9, 24, 40 },  { "F10", 14, kInputKey_F10, 0, 40 },  { "F11", 15, kInputKey_F11, 0, 40 },  { "F12", 16, kInputKey_F12, 0, 40 },  { "PrSn", 17, 0, 24, 40 },  { "ScLk", 18, 0, 0, 40 },  { "Brk", 19, 0, 0, 40 }, },
                { { "~", 20, 0, 0, 40 },  { "1", 21, 49, 0, 40 },  { "2", 22, 50, 0, 40 },  { "3", 23, 51, 0, 40 },  { "4", 24, 52, 0, 40 },  { "5", 25, 53, 0, 40 },  { "6", 26, 54, 0, 40 },  { "7", 27, 55, 0, 40 },  { "8", 28, 56, 0, 40 },  { "9", 29, 57, 0, 40 },  { "0", 30, 48, 0, 40 },  { "-", 31, 0, 0, 40 },  { "+", 32, 0, 0, 40 },  { ICON_FA_DELETE_LEFT, 33, 0, 0, 80 },  { "Ins", 34, 0, 24, 40 },  { "Hom", 35, 0, 0, 40 },  { "PgU", 36, 0, 0, 40 }, },
                { { "TAB", 3, kInputKeyTab, 0, 60 },  { "Q", 37, 113, 0, 40 },  { "W", 38, 119, 0, 40 },  { "E", 39, 101, 0, 40 },  { "R", 40, 114, 0, 40 },  { "T", 41, 116, 0, 40 },  { "Y", 42, 121, 0, 40 },  { "U", 43, 117, 0, 40 },  { "I", 44, 105, 0, 40 },  { "O", 45, 111, 0, 40 },  { "P", 46, 112, 0, 40 },  { "[", 47, 0, 0, 40 },  { "]", 48, 0, 0, 40 },  { "|", 49, 0, 0, 60 },  { "Del", 50, 0, 24, 40 },  { "End", 51, 0, 0, 40 },  { "PgD", 52, 0, 0, 40 }, },
                { { "CAPS", 53, kInputKeyCaps, 0, 80 },  { "A", 54, 97, 0, 40 },  { "S", 55, 115, 0, 40 },  { "D", 56, 100, 0, 40 },  { "F", 57, 102, 0, 40 },  { "G", 58, 103, 0, 40 },  { "H", 59, 104, 0, 40 },  { "J", 60, 106, 0, 40 },  { "K", 61, 107, 0, 40 },  { "L", 62, 108, 0, 40 },  { ";", 63, 0, 0, 40 },  { "'", 64, 0, 0, 40 },  { "ENTER", 65, kInputKeyEnter, 0, 84 }, },
                { { "SHIFT", 2, kInputKeyLeftShift, 0, 104 },  { "Z", 66, 122, 0, 40 },  { "X", 67, 120, 0, 40 },  { "C", 68, 99, 0, 40 },  { "V", 69, 118, 0, 40 },  { "B", 70, 98, 0, 40 },  { "N", 71, 110, 0, 40 },  { "M", 72, 109, 0, 40 },  { ",", 73, 0, 0, 40 },  { ".", 74, 0, 0, 40 },  { "/", 75, 0, 0, 40 },  { "SHIFT", 2, kInputKeyRightShift, 0, 104 },  { ICON_FA_ARROW_UP, 76, 0, 68, 40 }, },
                { { "CTRL", 0, kInputKeyLeftControl, 0, 60 }, { ICON_FA_WINDOWS, 0, kInputKeyLeftSuper, 0, 60 }, { "ALT", 1, kInputKeyLeftAlt, 0, 60 },  { "__", 77, kInputKeySpace, 0, 260 },  { "ALT", 1, kInputKeyRightAlt, 0, 60 }, { ICON_FA_WINDOWS, 0, kInputKeyRightSuper, 0, 60 },  { "CTRL", 0, kInputKeyRightControl, 0, 60 },  { ICON_FA_ARROW_LEFT, 78, 0, 24, 40 },  { ICON_FA_ARROW_DOWN, 79, 0, 0, 40 },  { ICON_FA_ARROW_RIGHT, 80, 0, 0, 40 }, }
            };

            static int editingHotkey = -1;
            static bool keyDown[512] = {};


            ImGui::BeginGroup();

            for (int i = 0; i < 512; i++) {
                ButtonControl *control = GetInputManager().getKeyboard().getButtonControl(i);
                if (control) {
                    keyDown[i] = control->isPress();
                }
            }

            for (unsigned int y = 0; y < 6; y++) {
                int x = 0;
                ImGui::BeginGroup();
                while (Keys[y][x].lib) {
                    const Key  &key = Keys[y][x];
                    const float ofs = key.offset + (x ? 4.f : 0.f);

                    const float width = key.width;
                    if (x) {
                        ImGui::SameLine(0.f, ofs);
                    } else {
                        if (ofs >= 1.f) {
                            ImGui::Indent(ofs);
                        }
                    }

                    bool &butSwtch = keyDown[key.keyCode];

                    ImGui::PushStyleColor(ImGuiCol_Button, butSwtch ? 0xFF1040FF : 0x80000000);
                    if (ImGui::Button(Keys[y][x].lib, ImVec2(width, 40))) {
                        //                        butSwtch = !butSwtch;
                    }
                    ImGui::PopStyleColor();
                    x++;
                }
                ImGui::EndGroup();
            }

            ImGui::EndGroup();
            ImGui::TreePop();
        }

        ImGui::Separator();

        if (ImGui::TreeNode("Image Test")) {
            if (ImGui::Button("OpenFile")) {
                OpenPanel *openPanel = OpenPanel::Alloc();

                if (!openPanel->init()) {
                    AN_LOG(Error, "init openPannel fail");
                } else {
                    openPanel->setAllowOtherTypes(true);
                    openPanel->addAllowContentExtension("JPG Image", "jpg");
                    openPanel->addAllowContentExtension("PNG Image", "png");
                    openPanel->setDefaultExtension("png");

                    openPanel->beginSheetModal(GetAppDelegate().getMainWindow(), [this](ModalResponse res, const char *path) {
                        if (res == AN::kModalResponseOk && path) {
                            AN_LOG(Log, "Load image %s", path);
                            auto result = TextureLoader::LoadTexture(path);
                            if (result.isValid()) {
                                tex->resize(result.getWidth(), result.getHeight());
                                tex->setPixelData(result.getData());
                                tex->uploadToGPU(false);
                            }
                        }
                    });
                }
                openPanel->release();
            }

            ImVec2 size = ImGui::GetContentRegionAvail();

            float imageRatio = (float)tex->getDataWidth() / (float)tex->getDataHeight();
            float imageHeight = size.y;
            float imageWidth = imageRatio * imageHeight;
            if (imageWidth > size.x) {
                imageWidth = size.x;
                imageHeight = imageWidth / imageRatio;
            }

            ImVec2 imageSize{ imageWidth, imageHeight };
            ImVec2 imageCursorPos = ImGui::GetCursorPos();
            ImVec2 imageBlockBegin = imageCursorPos + (size - imageSize) * 0.5f;
            ImGui::SetCursorPos(imageBlockBegin);

            Camera *camera = GetMainIMGUI().getComponent<Camera>();
            if (camera) {
                ImGui::Image(camera->getShadowMap(), imageSize);
            }


            if (Event::Current().getType() == AN::kDragExited) {
                dragAndDropUpdating = false;
            }

            if (dragAndDropUpdating) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 startPos = imageBlockBegin + ImGui::GetWindowPos(); // Starting position of the rectangle
                ImVec2 endPos = startPos + imageSize; // Ending position of the rectangle
                ImU32 borderColor = IM_COL32(55, 142, 240, 255); // Border color (red in this example)
                float borderWidth = 2.0f; // Border width in pixels

                drawList->AddRect(startPos, endPos, borderColor, 0.0f, ImDrawCornerFlags_All, borderWidth);
            }

            // Handle mouse input for dragging the image
            if (ImGui::IsItemHovered()) {
                /// drag and drop
                if (Event::Current().getType() == AN::kDragUpdated) {
                    if (GetDragAndDrop().getPaths().size() == 1) {
                        GetDragAndDrop().setVisualMode(AN::kDragOperationCopy);

                        dragAndDropUpdating = true;
                    }

                } else if (Event::Current().getType() == kDragPerform) {
                    if (GetDragAndDrop().getPaths().size() == 1) {
                        AN_LOG(Debug, "%s", GetDragAndDrop().getPaths()[0].c_str());
                    }

                    dragAndDropUpdating = false;
                }

            } else {
                dragAndDropUpdating = false;
            }

            ImGui::TreePop();
        }

        ImGui::End();
    }
};

bool MainIMGUI::init() {
    if (!Super::init()) return false;

    addPanel<ViewportPanel>();

    addPanel<HierarchyPanel>();

    addPanel<DemoPanel>();

    addPanel<InspectorPanel>();
    addPanel<InfoPanel>();

    addPanel<ProjectPanel>();
    addPanel<ConsolePanel>();

    GetPhysicsManager().pause(true);

    return true;
}

void MainIMGUI::dealloc() {
    _editorPanels.clear();
    Super::dealloc();
}

void MainIMGUI::onGUI() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags host_window_flags = 0;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    //            host_window_flags |= ImGuiWindowFlags_MenuBar;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("MainDockSpaceViewport", nullptr, host_window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    static bool dockFirstTime = true;
    if (dockFirstTime) {
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        ImGuiID dock_id_left;
        ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.3f, nullptr, &dock_id_left);
        ImGui::DockBuilderDockWindow("Debug", dock_id_right);
        ImGui::DockBuilderDockWindow("Inspector", dock_id_right);


        /// focus window on next frame
        Dispatch::async(Dispatch::Main, [] {
            DockBuilderFocusWindow("Inspector");
        });


        ImGuiID dock_id_up;
        ImGuiID dock_id_down = ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Down, 0.3f, nullptr, &dock_id_up);

        ImGui::DockBuilderDockWindow("Console", dock_id_down);
        ImGui::DockBuilderDockWindow("Project", dock_id_down);

        Dispatch::async(Dispatch::Main, [] {
            DockBuilderFocusWindow("Project");
        });

        dock_id_right = ImGui::DockBuilderSplitNode(dock_id_up, ImGuiDir_Right, 0.7f, nullptr, &dock_id_left);

        ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left);
        ImGui::DockBuilderDockWindow("Scene", dock_id_right);

//        ImGui::Focus
        ImGui::DockBuilderFinish(dockspace_id);
    }



    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                                        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        float height = ImGui::GetFrameHeight();

        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4 oldMenuBarBg = style.Colors[ImGuiCol_MenuBarBg];
        style.Colors[ImGuiCol_MenuBarBg] = { 0.1f, 0.1f, 0.1f, 1.f };

        /// top status bar
        if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height, window_flags)) {
            if (ImGui::BeginMenuBar()) {

                ImGui::Button(ICON_FA_USER, { 40.f, 0.f });

                ImGuiStyle& style = ImGui::GetStyle();
                float width = 0.0f;
                width += 45.f;
                width += style.ItemSpacing.x;
                width += 45.f;
                width += style.ItemSpacing.x;
                width += 45.f; /// subtract for user above

                AlignForWidth(width);


                PushButtonState(play);
                if (ImGui::Button(" " ICON_FA_PLAY " ", { 45.f, 0.f })) {
                    play = !play;
                    GetPhysicsManager().pause(!play);
                    if (play) {
                        GetPhysicsManager().clearState();
                    }
                }
                PopButtonState();

                ImGui::SameLine();

                PushButtonState(pause);
                if (ImGui::Button(" " ICON_FA_PAUSE " ", { 45.f, 0.f })) {
                    pause = !pause;
                }
                PopButtonState();

                ImGui::SameLine();
                if (ImGui::Button(" " ICON_FA_FORWARD_STEP " ", { 45.f, 0.f })) {

                }

                AlignForWidth(40.f, 1.f);
                ImGui::Button(ICON_FA_MAGNIFYING_GLASS, { 40.f, 0.f });

                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        /// bottom status bar

        if (ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, height, window_flags)) {
            if (ImGui::BeginMenuBar()) {
                ImGui::TextColored({ 0.14f, 0.66f, 0.95f, 1.f }, ICON_FA_CIRCLE_INFO " bottom bar info");
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        style.Colors[ImGuiCol_MenuBarBg] = oldMenuBarBg;
    }

    for (auto &panel : _editorPanels) {
        if (!panel->isCloseable() || panel->isOpened()) {
            panel->onGUI();
        }
    }

    ImGui::End();// MainDockSpace

    if (dockFirstTime) {
        dockFirstTime = false;
    }
}

MainIMGUI &GetMainIMGUI() {
    static MainIMGUI *mainIMGUI;
    if (mainIMGUI == nullptr) {
        mainIMGUI = Object::FindObjectOfType<MainIMGUI>();
    }
    return *mainIMGUI;
}

}