//
// Created by aojoie on 6/24/2023.
//

#include "Panels/ProjectPanel.hpp"
#include "Project/Project.hpp"
#include "imgui.h"
#include "ojoie/Misc/ResourceManager.hpp"
#include "ojoie/Render/Texture2D.hpp"

#include <filesystem>
#include <ojoie/Misc/ResourceManager.hpp>
#include <ojoie/HAL/File.hpp>
#include <ojoie/Editor/Selection.hpp>
#include <ojoie/Render/Mesh/Mesh.hpp>
#include <ojoie/Render/Material.hpp>
#include <imgui_stdlib.h>

namespace AN::Editor {


static bool HaveDirectoryMember(const std::filesystem::path &currentPath) {
    for (auto &directoryEntry : std::filesystem::directory_iterator(currentPath)) {
        if (directoryEntry.is_directory())
            return true;
    }
    return false;
}

static bool IsDirectoryEmpty(const std::filesystem::path &path) {
    for (auto &directoryEntry : std::filesystem::directory_iterator(path)) {
        return false;
    }
    return true;
}

bool ProjectPanel::drawSingleLineLabelWithEllipsis(const char* label, float maxWidth, bool selected, bool *showFull) {
    if (selected && showFull && *showFull) {
        ImGui::PushItemWidth(maxWidth);
        if (ImGui::InputText("##input", &inputTextBuffer)) {
            Object *object = Selection::GetActiveObject();
            if (object) {
                NamedObject *namedObject = object->as<NamedObject>();
                if (namedObject) {
                    namedObject->setName(inputTextBuffer.c_str());
                }
            }
        }
        ImGui::PopItemWidth();

        if (ImGui::IsItemDeactivated()) {
            *showFull = false;
        }
        return false;
    }

    ImVec2 textSize = ImGui::CalcTextSize(label);

    if (textSize.x > maxWidth) {
        ImVec2 ellipsisSize   = ImGui::CalcTextSize("...");
        float  availableWidth = maxWidth - ellipsisSize.x;

        if (availableWidth > 0.0f) {
            std::string wrappedText;
            float       textWidth = 0.0f;

            for (const char *p = label; *p; p++) {
                float charWidth = ImGui::GetFont()->GetCharAdvance(*p);
                if (textWidth + charWidth > availableWidth) {
                    wrappedText += "...";
                    break;
                }

                wrappedText += *p;
                textWidth += charWidth;
            }

            ImGui::Text("%s", wrappedText.c_str());
        }
    } else {
        ImGui::Text("%s", label);
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
        if (selected && showFull) {
            *showFull = true;
            inputTextBuffer = label;
        }
        return true;
    }

    return false;
}

ProjectPanel::ProjectPanel() : labelFullText() {
    fileImage        = GetResourceManager().getResource(Texture2D::GetClassNameStatic(), "FileIconTex")->as<Texture2D>();
    folderImage      = GetResourceManager().getResource(Texture2D::GetClassNameStatic(), "FolderIconTex")->as<Texture2D>();
    folderEmptyImage = GetResourceManager().getResource(Texture2D::GetClassNameStatic(), "FolderEmptyIconTex")->as<Texture2D>();

    Collections textureCollection{};
    textureCollection.name = "Texture";
    textureCollection.onDrawContent.bind(this, &ProjectPanel::drawTextures);
    m_Collections.push_back(textureCollection);

    Collections meshCollection{};
    meshCollection.name = "Mesh";
    meshCollection.onDrawContent.bind(this, &ProjectPanel::drawMeshes);
    m_Collections.push_back(meshCollection);

    Collections materialCollection{};
    materialCollection.name = "Material";
    materialCollection.onDrawContent.bind(this, &ProjectPanel::drawMaterials);
    m_Collections.push_back(materialCollection);

    Collections shaderCollectiohn{};
    shaderCollectiohn.name = "Shader";
    shaderCollectiohn.onDrawContent.bind(this, &ProjectPanel::drawShaders);
    m_Collections.push_back(shaderCollectiohn);
}

bool ProjectPanel::drawProjectTreeRecursive(const std::filesystem::path &currentPath) {
    const ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                         ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

    ImGuiTreeNodeFlags nodeFlags = baseFlags;

    if (mSelectedDirectory && *mSelectedDirectory == currentPath)
    {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    bool bNeedOpen = true;
    if (!HaveDirectoryMember(currentPath))
    {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        bNeedOpen = false;
    }

    std::string label    = "##" + currentPath.filename().string();
    bool        nodeOpen = ImGui::TreeNodeEx(label.c_str(), nodeFlags);

    bool action = false;
    if (ImGui::IsItemClicked())
    {
        mSelectedDirectory = currentPath;
        action = true;
    }

    ImGui::SameLine();
    const char *icon = nodeOpen && bNeedOpen ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER;
    ImGui::Text(" %s %s", icon, currentPath.filename().string().c_str());

    if (nodeOpen && bNeedOpen)
    {
        for (auto &p : std::filesystem::directory_iterator(currentPath))
        {
            const auto &path = p.path();
            if (!std::filesystem::is_directory(path))
            {
                continue;
            }

            action |= drawProjectTreeRecursive(path);
        }
        ImGui::TreePop();
    }
    return action;
}


bool ProjectPanel::drawProjectTree() {
    return drawProjectTreeRecursive(GetCurrentDirectory());
}

template<typename T, typename GetImage>
void ProjectPanel::drawCollections(const std::vector<T *> &collections, GetImage &&getImage, const char *DragDropName) {
    float panelWidth  = ImGui::GetContentRegionAvail().x;
    float panelHeight = ImGui::GetContentRegionAvail().y;
    static float padding       = 20.0f;
    static float thumbnailSize = 96.0f;
    float        cellSize      = thumbnailSize + padding;

    bool clearSelected = true;

    auto pos = ImGui::GetCursorPos();
    int  columnCount = (int) (panelWidth * 0.8f / cellSize);
    if (columnCount < 1)
        columnCount = 1;

    ImGui::SetCursorPos({ pos.x + panelWidth * 0.03f, pos.y + panelHeight * 0.1f });
    ImGui::BeginTable("##ContentTable", columnCount);

    for (int i = 0; i < collections.size(); i++) {

        ImGui::TableNextColumn();

        ImGui::BeginGroup();

        ImGui::PushID(collections[i]);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        Texture2D *image = getImage(collections[i]);

        bool selected = Selection::GetActiveObject() && Selection::GetActiveObject()->getInstanceID() == collections[i]->getInstanceID();
        pos = ImGui::GetCursorPos();
        if (ImGui::Selectable(std::format("##thumbnail{}", i).c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick, { thumbnailSize, thumbnailSize })) {
            labelFullText = false;
            selected = true;
            Selection::SetActiveObject(collections[i]);
            clearSelected = false;
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {

            }
        }

        if (ImGui::BeginDragDropSource()) {
            clearSelected = false;
            ImGui::SetDragDropPayload(DragDropName, &collections[i], sizeof(void*), ImGuiCond_Once);
            ImGui::EndDragDropSource();
        }

        ImGui::SetCursorPos({ pos.x + thumbnailSize * 0.05f, pos.y + thumbnailSize * 0.05f});
        ImGui::Image(image, { thumbnailSize * 0.9f, thumbnailSize * 0.9f });
        ImGui::PopStyleColor();

        //        ImVec2 text_size = ImGui::CalcTextSize(filenameString.c_str());
        //        pos              = ImGui::GetCursorPos();
        //        pos.x += (thumbnailSize - text_size.x) * 0.5f;
        //        ImGui::SetCursorPos(pos);
        if (drawSingleLineLabelWithEllipsis(collections[i]->getName().c_str(), thumbnailSize, selected, &labelFullText)) {
            if (!selected) {
                Selection::SetActiveObject(collections[i]);
            }
            clearSelected = false;
        }

        ImGui::EndGroup();
        ImGui::PopID();
    }

    if (ImGui::IsMouseClicked(1) && ImGui::IsWindowHovered()) {
        ImGui::OpenPopup("PROJECT_CONTEXT");
    }

    if (clearSelected && ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered(ImGuiWindowFlags_ChildWindow)) {
        Selection::SetActiveObject(nullptr);
    }

    Object *selectedObject = Selection::GetActiveObject();
    bool deleteAction = false;
    if (ImGui::BeginPopup("PROJECT_CONTEXT")) {
        if (ImGui::MenuItem("Delete", 0, false, selectedObject != nullptr)) {
            ImGui::CloseCurrentPopup();
            deleteAction = true;
        }
        ImGui::EndPopup();
    }

    if (deleteAction) {
        ImGui::OpenPopup("Delete Selected Asset##DELETE_ALERT");
    }

    if (ImGui::BeginPopupModal("Delete Selected Asset##DELETE_ALERT", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to delete %s", selectedObject->as<NamedObject>()->getName().c_str());

        if (ImGui::Button("Delete", ImVec2(240, 0))) {
            DestroyObject(selectedObject);
            Selection::SetActiveObject(nullptr);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(240, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::EndTable();
}

void ProjectPanel::drawTextures() {
    std::vector<Texture2D *> tex2Ds =  Object::FindObjectsOfType<Texture2D>();
    drawCollections(tex2Ds, [](Texture2D *tex) { return tex; }, "PROJECT_TEXTURE");
}

void ProjectPanel::drawMeshes() {
    std::vector<Mesh *> meshs =  Object::FindObjectsOfType<Mesh>();
    drawCollections(meshs, [this](Mesh *mesh) { return fileImage; }, "PROJECT_MESH");
}

void ProjectPanel::drawMaterials() {
    std::vector<Material *> meshs =  Object::FindObjectsOfType<Material>();
    drawCollections(meshs, [this](Material *mesh) { return fileImage; }, "PROJECT_MATERIAL");
}

void ProjectPanel::drawShaders() {
    std::vector<Shader *> meshs =  Object::FindObjectsOfType<Shader>();
    drawCollections(meshs, [this](Shader *mesh) { return fileImage; }, "PROJECT_SHADER");
}

void ProjectPanel::drawFolderContent() {
    /// all imported items
    if (!mSelectedDirectory) {
        return;
    }

    mCurrentDirectory = *mSelectedDirectory;

    static float padding       = 20.0f;
    static float thumbnailSize = 96.0f;
    float        cellSize      = thumbnailSize + padding;

    float panelWidth  = ImGui::GetContentRegionAvail().x;
    float panelHeight = ImGui::GetContentRegionAvail().y;
    int  columnCount = (int) (panelWidth * 0.8f / cellSize);
    if (columnCount < 1)
        columnCount = 1;

    bool clearSelected = true;

    auto pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos({ pos.x + panelWidth * 0.03f, pos.y + panelHeight * 0.1f });

//    ImGui::Columns(columnCount, nullptr, false);

    ImGui::BeginTable("##ContentTable", columnCount);

    std::vector<std::filesystem::path> sortedDirectory;
    int                                directoryEndIndex = -1;
    for (auto &directoryEntry : std::filesystem::directory_iterator(mCurrentDirectory)) {
        if (!directoryEntry.is_directory()) {
            sortedDirectory.push_back(directoryEntry.path());
        } else {
            sortedDirectory.insert(sortedDirectory.begin(), directoryEntry.path());
            directoryEndIndex++;
        }
    }

    for (int i = 0; i < sortedDirectory.size(); i++) {

        ImGui::TableNextColumn();

        ImGui::BeginGroup();

        const auto &path           = sortedDirectory[i];
        auto        relativePath   = std::filesystem::relative(path, GetAssetFolder());
        std::string filenameString = relativePath.filename().string();

        ImGui::PushID(filenameString.c_str());
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        Texture2D *image;
        if (is_directory(path)) {
            if (IsDirectoryEmpty(path)) {
                image = folderEmptyImage;
            } else {
                image = folderImage;
            }
        } else {
            image = fileImage;
        }

        bool selected = selectedPath == path;
        pos = ImGui::GetCursorPos();
        if (ImGui::Selectable(std::format("##thumbnail{}", i).c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick, { thumbnailSize, thumbnailSize })) {
            selectedPath = path.string();
            labelFullText = false;
            selected = true;
            clearSelected = false;

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                if (i <= directoryEndIndex) {
                    mCurrentDirectory /= path.filename();
                    mSelectedDirectory = mCurrentDirectory;
                    labelFullText = false;
                }
            }
        }

        if (ImGui::BeginDragDropSource()) {
            const wchar_t *itemPath = relativePath.c_str();
            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t), ImGuiCond_Once);
            ImGui::EndDragDropSource();

            clearSelected = false;
        }

        ImGui::SetCursorPos({ pos.x + thumbnailSize * 0.05f, pos.y + thumbnailSize * 0.05f});
        ImGui::Image(image, { thumbnailSize * 0.9f, thumbnailSize * 0.9f }, {}, { 1, 1 }, { 0.76f, 0.76f, 0.76f, 1.f });
        ImGui::PopStyleColor();


//        ImVec2 text_size = ImGui::CalcTextSize(filenameString.c_str());
//        pos              = ImGui::GetCursorPos();
//        pos.x += (thumbnailSize - text_size.x) * 0.5f;
//        ImGui::SetCursorPos(pos);
        if (drawSingleLineLabelWithEllipsis(filenameString.c_str(), thumbnailSize, selected, &labelFullText)) {
            if (!selected) {
                selectedPath = path.string();
            }

            clearSelected = false;
        }

        ImGui::EndGroup();

//        ImGui::NextColumn();

        ImGui::PopID();
    }

    if (clearSelected && ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered(ImGuiWindowFlags_ChildWindow)) {
        selectedPath.clear();
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

//    ImGui::Text("Final Text");

    ImGui::EndTable();
//    ImGui::Columns(1);

    //ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
    //ImGui::SliderFloat("Padding", &padding, 0, 32);
}


bool ProjectPanel::Collections::drawNode() {
    ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                   ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth |
                                   ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (open) {
        baseFlags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::TreeNodeEx(std::format("{}##Project", name).c_str(), baseFlags);

    if (ImGui::IsItemClicked()) {
        open = true;
        return true;
    }
    return false;
}

bool ProjectPanel::Collections::drawContent() {
    if (open) {
        onDrawContent();
    }
    return false;
}

void ProjectPanel::onGUI() {
    ImGui::Begin("Project", getOpenPtr());
    ImGui::Columns(2);

    static bool init = true;
    if (init) {
        ImGui::SetColumnWidth(0, 240.0f);
        init = false;
    }


    if (ImGui::BeginChild("CONTENT_BROWSER_TREE")) {
        if (drawProjectTree()) {
            for (Collections &co : m_Collections) {
                co.reset();
            }
        }

        ImGui::Separator();

        Collections *selected = nullptr;
        for (Collections &co : m_Collections) {
            if (co.drawNode()) {
                mSelectedDirectory = "";
                selected = &co;
            }
        }

        if (selected) {
            for (Collections &co : m_Collections) {
                if (&co != selected) {
                    co.reset();
                }
            }
        }
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    if (ImGui::BeginChild("CONTENT_BROWSER_CONTENT")) {
        bool collectionDrawn = false;
        for (Collections &co : m_Collections) {
            if (co.open) {
                collectionDrawn = true;
                co.drawContent();
            }
        }
        if (!collectionDrawn) {
            drawFolderContent();
        }
    }

    ImGui::EndChild();

    ImGui::Columns(1);

    ImGui::End();
}
}// namespace AN::Editor