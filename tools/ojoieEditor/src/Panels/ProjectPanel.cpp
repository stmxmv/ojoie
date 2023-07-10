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
}

void ProjectPanel::drawProjectTreeRecursive(const std::filesystem::path &currentPath) {
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

    if (ImGui::IsItemClicked())
    {
        mSelectedDirectory = currentPath;
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

            drawProjectTreeRecursive(path);
        }
        ImGui::TreePop();
    }
}


void ProjectPanel::drawProjectTree() {
    drawProjectTreeRecursive(GetCurrentDirectory());
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

    ImGui::Text("Final Text");

    ImGui::EndTable();
//    ImGui::Columns(1);

    //ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
    //ImGui::SliderFloat("Padding", &padding, 0, 32);
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
        drawProjectTree();
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    if (ImGui::BeginChild("CONTENT_BROWSER_CONTENT")) {
        drawFolderContent();
    }
    ImGui::EndChild();

    ImGui::Columns(1);

    ImGui::End();
}
}// namespace AN::Editor