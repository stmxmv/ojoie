//
// Created by aojoie on 6/24/2023.
//

#include "Panels/ViewportPanel.hpp"
#include "MainIMGUI.hpp"

#include <ojoie/Core/Actor.hpp>
#include <ojoie/Camera/Camera.hpp>
#include <ojoie/Core/Event.hpp>
#include <ojoie/Render/TextureLoader.hpp>
#include <ojoie/Core/DragAndDrop.hpp>

namespace AN::Editor {

ViewportPanel::ViewportPanel() : sceneTarget() {
    sceneTarget = MakeObjectPtr<RenderTarget>();
    
    AttachmentDescriptor attachmentDescriptor{};
    attachmentDescriptor.width = 1920;
    attachmentDescriptor.height = 1080;
    attachmentDescriptor.samples = 1;
    attachmentDescriptor.format = kRTFormatDefault;

    ANAssert(sceneTarget->init(attachmentDescriptor));

    Camera *camera = GetMainIMGUI().addComponent<Camera>();
    camera->setMatchLayerRatio(false);
    camera->setRenderTarget(sceneTarget.get());
}

void ViewportPanel::onGUI() {
    {
        ImGui::Begin("Scene");
        ImVec2 size = ImGui::GetContentRegionAvail();

        bool shouldAdjustSceneTarget = false;
        if (sceneTarget == nullptr) {
            shouldAdjustSceneTarget = true;
        }
        if (sceneTarget) {
            Size targetSize = sceneTarget->getSize();
            if (targetSize.width != (UInt32)size.x || targetSize.height != (UInt32)size.y) {
                shouldAdjustSceneTarget = true;
            }
        }

        if (shouldAdjustSceneTarget) {
            Camera *camera = GetMainIMGUI().getComponent<Camera>();
            if (camera) {
                camera->setViewportRatio(size.x / size.y);
            }
        }


        ImVec2 imageCursorPos  = ImGui::GetCursorPos();
        ImVec2 imageBlockBegin = imageCursorPos;
        ImGui::Image(sceneTarget.get(), size);

        if (Event::Current().getType() == AN::kDragExited) {
            dragAndDropUpdating = false;
        }

        if (dragAndDropUpdating) {
            ImDrawList *drawList    = ImGui::GetWindowDrawList();
            ImVec2      startPos    = imageBlockBegin + ImGui::GetWindowPos();// Starting position of the rectangle
            ImVec2      endPos      = startPos + size;                   // Ending position of the rectangle
            ImU32       borderColor = IM_COL32(55, 142, 240, 255);            // Border color (red in this example)
            float       borderWidth = 2.0f;                                   // Border width in pixels

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
        ImGui::End();
    }
}

}