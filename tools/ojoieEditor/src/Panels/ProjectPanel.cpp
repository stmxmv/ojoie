//
// Created by aojoie on 6/24/2023.
//

#include "Panels/ProjectPanel.hpp"
#include "Project/Project.hpp"
#include "imgui.h"
#include "ojoie/Misc/ResourceManager.hpp"
#include "ojoie/Render/Texture2D.hpp"

#include <fstream>
#include <filesystem>
#include <ojoie/Misc/ResourceManager.hpp>
#include <ojoie/HAL/File.hpp>
#include <ojoie/Editor/Selection.hpp>
#include <ojoie/Render/Mesh/Mesh.hpp>
#include <ojoie/Render/Material.hpp>
#include <ojoie/Core/DragAndDrop.hpp>
#include <ojoie/Core/Event.hpp>
#include <ojoie/Serialize/SerializeManager.hpp>
#include <ojoie/Render/TextureLoader.hpp>
#include <imgui_stdlib.h>

#include <ojoie/Asset/FBXImporter.hpp>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#undef GetCurrentDirectory
#endif

namespace AN::Editor {

static const char *s_DefaultShaderString = R"(Shader "Custom/Default"
{
    Properties
    {
        _MainTex ("Main Tex", 2D) = "white" {}
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque" "RenderPipeline" = "UniversalRenderPipeline" }

        HLSLINCLUDE

        #include "Core.hlsl"
        #include "Lighting.hlsl"

        CBUFFER_START(ANPerMaterial)
            AN_DECLARE_TEX2D(_MainTex);
        CBUFFER_END

        ENDHLSL

        Pass 
        {
            Tags { "LightMode" = "Forward" }

            HLSLPROGRAM

            struct appdata 
            {   
                float3 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };
            struct v2f
            {
                float4 vertexOut : SV_POSITION;
                float2 uv : TEXCOORD0;
            };
            v2f vertex_main(appdata v)
            {
                v2f o;
                o.vertexOut = TransformObjectToHClip(v.vertex.xyz);
                o.uv = v.uv;
                return o;
            }
            half4 fragment_main(v2f i) : SV_TARGET 
            {
                float3 texColor = AN_SAMPLE_TEX2D(_MainTex, i.uv).rgb;
                return half4(texColor, 1.0);
            }
            ENDHLSL
        }
    }
}
)";

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

static std::filesystem::path GetNewFileName(std::filesystem::path newFileName, const char *ext) {
    int count = 1;
    std::string name = newFileName.stem().string();
    while (exists(newFileName)) {
        newFileName.remove_filename();
        if (ext) {
            newFileName.append(std::format("{}{}.{}", name, count, ext));
        } else {
            newFileName.append(std::format("{}{}", name, count));
        }

        ++count;
    }
    return newFileName;
}

void ProjectPanel::ContextMenu() {
    /// context menu
    bool deleteAction = false;
    Object *selectedObject = Selection::GetActiveObject();

    bool selectedIsDirectory = is_directory(selectedPath);

    if (ImGui::BeginPopup("PROJECT_CONTEXT")) {

        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Directory", 0, false, !mCurrentDirectory.empty())) {
                std::filesystem::path newFolderName = mCurrentDirectory.append("NewFolder");
                std::filesystem::create_directory(GetNewFileName(newFolderName, nullptr));
            }

            if (ImGui::MenuItem("Shader", 0, false, !mCurrentDirectory.empty())) {
                std::filesystem::path newShaderName = GetNewFileName(mCurrentDirectory.append("CustomShader.shader"), "shader");
                std::ofstream file(newShaderName);
                if (!file.is_open()) {
                    AN_LOG(Error, "Cannot Create shader at directory");
                } else {
                    file << s_DefaultShaderString;
                    file.close();
                    Shader *shader = NewObject<Shader>();
                    if (shader->initWithScript(newShaderName.string())) {
                        std::filesystem::path assetPath = newShaderName.replace_extension("asset");
                        GetSerializeManager().serializeObjectAtPath(shader, assetPath.string().c_str());

                        shader->createGPUObject();

                        GetResourceManager().resetResourcePath(shader, assetPath.string().c_str());
                    } else {
                        DestroyObject(shader);
                    }
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Show in Explorer", 0, false, !mCurrentDirectory.empty())) {
            ShellExecuteW(NULL, L"open", mCurrentDirectory.c_str(), NULL, NULL, SW_SHOWDEFAULT);
        }

        if (ImGui::MenuItem("Delete", 0, false, selectedObject != nullptr || selectedIsDirectory)) {
            ImGui::CloseCurrentPopup();
            deleteAction = true;
        }
        ImGui::EndPopup();
    }

    if (deleteAction) {
        ImGui::OpenPopup("Delete Selected Asset##DELETE_ALERT");
    }

    if (ImGui::BeginPopupModal("Delete Selected Asset##DELETE_ALERT", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::string deleteItemName;
        if (selectedIsDirectory) {
            deleteItemName = selectedPath.string();
        } else {
            deleteItemName = selectedObject->as<NamedObject>()->getName().c_str();
        }
        std::string displayText = std::format("Are you sure you want to delete {}?", deleteItemName);
        for (int i = 0; i < displayText.size(); i += 50) {
            displayText.insert(i, "\r\n");
        }
        ImGui::TextUnformatted(displayText.data());
        if (ImGui::Button("Delete", ImVec2(240, 0))) {
            if (selectedIsDirectory) {
                std::filesystem::remove_all(selectedPath);
            } else {
                /// TODO this should be delay destroy
                DestroyObject(selectedObject);
                Selection::SetActiveObject(nullptr);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(240, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

bool ProjectPanel::drawSingleLineLabelWithEllipsis(const char* label, float maxWidth, bool selected, bool *showFull) {
    if (selected && showFull && *showFull) {
        ImGui::PushItemWidth(maxWidth);
        if (ImGui::InputText("##input", &inputTextBuffer)) {

        }

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Object *object = Selection::GetActiveObject();
            if (object) {
                NamedObject *namedObject = object->as<NamedObject>();
                if (object->is<Shader>()) {
                    /// also rename shader file, and re serialize since shader has script path record
                    ANAssert(selectedPath.extension() == ".asset");
                    std::filesystem::remove(selectedPath);

                    selectedPath.replace_extension("shader");
                    auto oldShaderPath = selectedPath;
                    selectedPath.replace_filename(inputTextBuffer);
                    std::filesystem::rename(oldShaderPath, selectedPath);

                    object->as<Shader>()->setTextAssetPath(selectedPath.string());

                    selectedPath.replace_extension("asset");
                    GetSerializeManager().serializeObjectAtPath(object, selectedPath.string().c_str());
                    GetResourceManager().resetResourcePath(object, selectedPath.string().c_str());
                } else {
                    if (namedObject) {
                        namedObject->setName(inputTextBuffer.c_str());
                    }
                }
            }

            bool selectedIsDirectory = is_directory(selectedPath);
            if (selectedIsDirectory) {
                auto oldPath = selectedPath;
                selectedPath.replace_filename(inputTextBuffer);
                std::filesystem::rename(oldPath, selectedPath);
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
        ImGui::OpenPopup("PROJECT_CONTEXT", ImGuiPopupFlags_AnyPopupLevel);
    }

    if (clearSelected && ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered(ImGuiWindowFlags_ChildWindow)) {
        Selection::SetActiveObject(nullptr);
    }

    ContextMenu();

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

    std::sort(sortedDirectory.begin(), sortedDirectory.end());

    for (int i = 0; i < sortedDirectory.size(); i++) {

        const auto &path           = sortedDirectory[i];
        auto        relativePath   = std::filesystem::relative(path, GetAssetFolder());

        if (relativePath.extension() != ".asset" && !is_directory(relativePath)) {
            continue;
        }

        std::string filenameString = relativePath.filename().string();


        ImGui::TableNextColumn();
        ImGui::BeginGroup();

        ImGui::PushID(filenameString.c_str());
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        Texture2D *image;
        Object *resource = nullptr;
        if (is_directory(path)) {
            if (IsDirectoryEmpty(path)) {
                image = folderEmptyImage;
            } else {
                image = folderImage;
            }
        } else {
            image = fileImage;
            resource = GetResourceManager().getResourceExact(path.string().c_str());
            if (resource) {
                Texture2D *tex2D = resource->as<Texture2D>();
                if (tex2D) {
                    image = tex2D;
                }
            }
        }

        bool selected = selectedPath == path;
        if (resource) {
            if (Selection::GetActiveObject()) {
                selected &= Selection::GetActiveObject()->getInstanceID() == resource->getInstanceID();
            }
        }
        pos = ImGui::GetCursorPos();
        if (ImGui::Selectable(std::format("##thumbnail{}", i).c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick, { thumbnailSize, thumbnailSize })) {
            selectedPath = path;
            labelFullText = false;
            selected = true;
            clearSelected = false;

            if (resource) {
                Selection::SetActiveObject(resource);
            }

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
        std::string displayName = filenameString;
        /// script asset use real file name
        if (resource) {
            if (resource->isKindOf<TextAsset>()) {
                displayName = std::filesystem::path(resource->as<TextAsset>()->getTextAssetPath()).filename().string();
            } else {
                displayName = resource->as<NamedObject>()->getName().string_view();
            }
        }
        if (drawSingleLineLabelWithEllipsis(displayName.c_str(), thumbnailSize, selected, &labelFullText)) {
            if (!selected) {
                selectedPath = path.string();
                if (resource) {
                    Selection::SetActiveObject(resource);
                }
            }

            clearSelected = false;
        }

        ImGui::EndGroup();

//        ImGui::NextColumn();

        ImGui::PopID();
    }

    if (ImGui::IsMouseClicked(1) && ImGui::IsWindowHovered()) {
        ImGui::OpenPopup("PROJECT_CONTEXT", ImGuiPopupFlags_AnyPopupLevel);
    }

    if (clearSelected && ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered(ImGuiWindowFlags_ChildWindow)) {
        selectedPath.clear();
        Selection::SetActiveObject(nullptr);
    }

    ContextMenu();

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
                mCurrentDirectory.clear();
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

    if (Event::Current().getType() == AN::kDragExited) {
        dragAndDropUpdating = false;
    }

    bool bMouseHover = ImGui::IsWindowHovered();

    if (dragAndDropUpdating) {
        ImDrawList *drawList    = ImGui::GetWindowDrawList();
        ImVec2      startPos    = ImGui::GetWindowPos();// Starting position of the rectangle
        ImVec2      endPos      = startPos + ImGui::GetWindowSize();                   // Ending position of the rectangle
        ImU32       borderColor = IM_COL32(55, 142, 240, 255);            // Border color (red in this example)
        float       borderWidth = 2.0f;                                   // Border width in pixels

        drawList->AddRect(startPos, endPos, borderColor, 0.0f, ImDrawCornerFlags_All, borderWidth);
    }

    // Handle mouse input for dragging the image
    if (ImGui::IsWindowHovered()) {

        bMouseHover = true;

        /// drag and drop
        if (Event::Current().getType() == AN::kDragUpdated) {
            if (GetDragAndDrop().getPaths().size() == 1 && !mCurrentDirectory.empty()) {
                GetDragAndDrop().setVisualMode(AN::kDragOperationCopy);

                dragAndDropUpdating = true;
            }

        } else if (Event::Current().getType() == kDragPerform && !mCurrentDirectory.empty()) {
            if (GetDragAndDrop().getPaths().size() == 1) {
                AN_LOG(Debug, "%s", GetDragAndDrop().getPaths()[0].c_str());
            }

            std::filesystem::path path = GetDragAndDrop().getPaths()[0];
            if (path.extension() == ".fbx") {
                FBXImporter importer;

                if (importer.isValid()) {
                    if (importer.loadScene(path.string().c_str(), nullptr)) {
                        if (importer.importMesh(nullptr)) {
                            auto meshes = importer.getImportMeshes();

                            ANAssert(meshes.size() > 0);
                            const ImportMesh &importMesh = meshes[0];

                            Mesh *mesh = NewObject<Mesh>();
                            mesh->init();
                            mesh->setName(path.stem().string().c_str());
                            mesh->resizeVertices(importMesh.positions.size(), kShaderChannelVertex | kShaderChannelTexCoord0 |
                                                                                      kShaderChannelNormal | kShaderChannelTangent);

                            mesh->setVertices(importMesh.positions.data(), importMesh.positions.size());
                            mesh->setUV(0, importMesh.texcoords[0].data(), importMesh.texcoords[0].size());
                            mesh->setNormals(importMesh.normals.data(), importMesh.normals.size());
                            mesh->setTangents(importMesh.tangents.data(), importMesh.tangents.size());

                            mesh->setSubMeshCount(importMesh.subMeshes.size());
                            for (int i = 0; i < importMesh.subMeshes.size(); ++i) {
                                mesh->setIndices(importMesh.subMeshes[i].indices.data(), importMesh.subMeshes[i].indices.size(), i);
                            }

                            std::filesystem::path assetPath(mCurrentDirectory);
                            assetPath.append(path.filename().string());
                            assetPath.replace_extension("asset");
                            GetSerializeManager().serializeObjectAtPath(mesh, assetPath.string().c_str());
                            GetResourceManager().resetResourcePath(mesh, assetPath.string().c_str());
                            mesh->createVertexBuffer();
                        }
                    }
                }
            }
            else {
                auto result = TextureLoader::LoadTexture(path.string().c_str(), true);
                if (result.isValid()) {
                    Texture2D *texture = NewObject<Texture2D>();
                    TextureDescriptor textureDescriptor;
                    textureDescriptor.width = result.getWidth();
                    textureDescriptor.height = result.getHeight();
                    textureDescriptor.pixelFormat = result.getPixelFormat();
                    textureDescriptor.mipmapLevel = result.getMipmapLevel();

                    ANAssert(texture->init(textureDescriptor));
                    texture->setPixelData(result.getData());
                    texture->setName(path.stem().string().c_str());

                    std::filesystem::path assetPath(mCurrentDirectory);
                    assetPath.append(path.filename().string());
                    assetPath.replace_extension("asset");

                    GetSerializeManager().serializeObjectAtPath(texture, assetPath.string().c_str());
                    GetResourceManager().resetResourcePath(texture, assetPath.string().c_str());

                    texture->uploadToGPU();
                }
            }
            dragAndDropUpdating = false;
        }

    } else {
        bMouseHover = false;
        dragAndDropUpdating = false;
    }

    ImGui::EndChild();

    ImGui::Columns(1);

    ImGui::End();




}
}// namespace AN::Editor