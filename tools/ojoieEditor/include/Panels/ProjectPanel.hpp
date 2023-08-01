//
// Created by aojoie on 6/24/2023.
//

#pragma once

#include "EditorPanel.hpp"

#include <ojoie/Render/Texture2D.hpp>
#include <ojoie/Template/delegate.hpp>
#include <filesystem>
#include <optional>

namespace AN::Editor {

class ProjectPanel : public Panel {


    struct Collections {
        bool open = false;
        const char *name;

        Delegate<void()> onDrawContent;

        bool drawNode();
        bool drawContent();
        void reset() {
            open = false;
        }
    };


    std::filesystem::path mCurrentDirectory;
    std::optional<std::filesystem::path> mSelectedDirectory;

    Texture2D *fileImage, *folderImage, *folderEmptyImage;

    std::string selectedPath;

    bool labelFullText;
    std::string inputTextBuffer;

    std::vector<Collections> m_Collections;


    bool drawSingleLineLabelWithEllipsis(const char* label, float maxWidth, bool selected, bool *showFull);

    bool drawProjectTreeRecursive(const std::filesystem::path& currentPath);

    bool drawProjectTree();

    template<typename T, typename GetImage>
    void drawCollections(const std::vector<T *> &collections, GetImage &&getImage, const char *DragDropName);

    void drawTextures();
    void drawMeshes();
    void drawMaterials();
    void drawShaders();

    void drawFolderContent();
public:

    ProjectPanel();

    virtual void onGUI() override;

};

}
