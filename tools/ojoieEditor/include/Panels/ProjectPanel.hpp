//
// Created by aojoie on 6/24/2023.
//

#pragma once

#include "EditorPanel.hpp"

#include <ojoie/Render/Texture2D.hpp>

#include <filesystem>
#include <optional>

namespace AN::Editor {

class ProjectPanel : public Panel {

    std::filesystem::path mCurrentDirectory;
    std::optional<std::filesystem::path> mSelectedDirectory;

    Texture2D *fileImage, *folderImage, *folderEmptyImage;

    std::string selectedPath;

    bool labelFullText;
    std::string inputTextBuffer;

    bool drawSingleLineLabelWithEllipsis(const char* label, float maxWidth, bool selected, bool *showFull);

    void drawProjectTreeRecursive(const std::filesystem::path& currentPath);

    void drawProjectTree();

    void drawFolderContent();

public:

    ProjectPanel();

    virtual void onGUI() override;

};

}
