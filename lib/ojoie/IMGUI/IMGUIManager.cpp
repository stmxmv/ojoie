//
// Created by aojoie on 5/17/2023.
//

#include "IMGUI/IMGUIManager.hpp"
#include "IMGUI/ImguiStyles.hpp"
#include "Render/RenderContext.hpp"
#include "Render/LayerManager.hpp"
#include "Configuration/platform.h"
#include "Render/RenderManager.hpp"
#include "Misc/ResourceManager.hpp"
#include <ojoie/Render/CommandPool.hpp>
#include <ojoie/Threads/Dispatch.hpp>
#include <ojoie/Core/Game.hpp>
#include <ojoie/HAL/File.hpp>
#include <ojoie/IMGUI/IconsFontAwesome6Brands.h>

#if AN_WIN
#include "Core/private/win32/Window.hpp"
#include "Render/private/D3D11/Layer.hpp"
#include "Math/Math.hpp"
#include "Math/Float.hpp"
#include <imgui_impl_win32.h>
#endif

#include <DirectXTex.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include <filesystem>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace AN {

#define SETTINGS_DIR "./Data/Settings"
#define IMGUI_INI_NAME "imgui.ini"

#ifdef AN_WIN
static WNDPROC s_oldWndProc;
#endif

struct ImGui_ImplDX11_ViewportData {
    WIN::Window *window;
    D3D11::Layer *layer;

    RenderPass   renderPass;
};

static void ImGui_ImplDX11_CreateWindow(ImGuiViewport* viewport) {
    ImGui_ImplDX11_ViewportData* vd = IM_NEW(ImGui_ImplDX11_ViewportData)();
    viewport->RendererUserData = vd;

#if AN_WIN
    HWND hwnd = viewport->PlatformHandleRaw ? (HWND)viewport->PlatformHandleRaw : (HWND)viewport->PlatformHandle;

    vd->window = (WIN::Window *)Window::Alloc();
    vd->window->bridge(hwnd);

    vd->layer = new D3D11::Layer();
#endif

    vd->layer->init(vd->window);

    Size renderArea = vd->layer->getSize();
    /// create render pass
    SubpassInfo   subpass_infos[1] = {};
    LoadStoreInfo load_store[1]    = {};

    // render target
    load_store[0].loadOp  = kAttachmentLoadOpClear;
    load_store[0].storeOp = kAttachmentStoreOpStore;

    subpass_infos[0].colorAttachments       = { 0 };

    RenderPassDescriptor renderPassDescriptor{};

    renderPassDescriptor.loadStoreInfos.assign(std::begin(load_store), std::end(load_store));
    renderPassDescriptor.subpasses.assign(std::begin(subpass_infos), std::end(subpass_infos));

    RenderTargetDescriptor targetAttachment{};
    targetAttachment.format  = kRTFormatDefault;
    targetAttachment.width = renderArea.width;
    targetAttachment.height = renderArea.height;
    targetAttachment.samples = 1;

//    renderPassDescriptor.attachments.push_back(targetAttachment);

    ANAssert(vd->renderPass.init(renderPassDescriptor));
}

static void ImGui_Impl_DestroyWindow(ImGuiViewport* viewport) {
    // The main viewport (owned by the application) will always have RendererUserData == nullptr since we didn't create the data for it.
    if (ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData) {
        delete vd->layer;
        vd->window->release();

        vd->layer = nullptr;
        vd->window = nullptr;

        vd->renderPass.deinit();

        IM_DELETE(vd);
    }
    viewport->RendererUserData = nullptr;
}

static void ImGui_Impl_SetWindowSize(ImGuiViewport* viewport, ImVec2 size) {
    ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData;
    vd->layer->resize({ (UInt32)size.x, (UInt32)size.y });
}


void IMGUIManager::renderDrawData(RenderContext &context, ImDrawData* draw_data, bool main) {

    size_t vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    size_t index_buffer_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

    if ((vertex_buffer_size == 0) || (index_buffer_size == 0)) {
        return;
    }


    // Upload data
    void *VB, *IB;
    context.commandBuffer->getDynamicVertexBuffer().getChunk(channelInfo,
                                                             draw_data->TotalVtxCount, draw_data->TotalIdxCount,
                                                             kDrawIndexedTriangles, &VB, &IB);

    ImDrawVert *vtx_dst = (ImDrawVert *) VB;
    ImDrawIdx  *idx_dst = (ImDrawIdx *) IB;

    for (int i = 0; i < draw_data->CmdListsCount; i++) {
        const ImDrawList *cmd_list = draw_data->CmdLists[i];
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }

    context.commandBuffer->getDynamicVertexBuffer().releaseChunk(draw_data->TotalVtxCount, draw_data->TotalIdxCount);

    /// TODO bind uniform buffer
    // Pre-rotation
    auto &io             = ImGui::GetIO();
    auto  push_transform = Math::mat4(1.0f);

    // GUI coordinate space to screen space
    push_transform = Math::translate(push_transform, Math::vec3(-1.0f, -1.0f, 0.0f));
    push_transform = Math::scale(push_transform, Math::vec3(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, 0.0f));

    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    float mvp[4][4] =
            {
                { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
                { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
                { 0.0f,         0.0f,           0.5f,       0.0f },
                { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
            };

    material->setMatrix("_IMGUITransform", (Matrix4x4f &)mvp);
    /// add draw commands to render thread to render

    int32_t     vertex_offset = 0;
    uint32_t    index_offset  = 0;

    ImVec2 clip_off = draw_data->DisplayPos;
    ImVec2 displaySize = draw_data->DisplaySize;

    if (draw_data->CmdListsCount > 0) {
        for (int32_t i = 0; i < draw_data->CmdListsCount; i++) {
            const ImDrawList *cmd_list = draw_data->CmdLists[i];
            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
                const ImDrawCmd *cmd = &cmd_list->CmdBuffer[j];
                ImTextureID texId = cmd->GetTexID();
                ScissorRectInfo  scissor_rect{ .x      = std::max((cmd->ClipRect.x - clip_off.x) / displaySize.x, 0.f),
                                               .y      = std::max((cmd->ClipRect.y - clip_off.y) / displaySize.y, 0.f),
                                               .width  = ((cmd->ClipRect.z - cmd->ClipRect.x) / displaySize.x),
                                               .height = ((cmd->ClipRect.w - cmd->ClipRect.y) / displaySize.y) };

                if (!Math::isFinite(scissor_rect.x) || !Math::isFinite(scissor_rect.y) ||
                    !Math::isFinite(scissor_rect.width) || !Math::isFinite(scissor_rect.height) ||
                    CompareApproximately(scissor_rect.width, 0.f) || CompareApproximately(scissor_rect.height, 0.f)) {
                    continue;
                }

                DrawCommand command{ .vertexOffset = (UInt32) vertex_offset + cmd->VtxOffset,
                                     .indexOffset  = index_offset + cmd->IdxOffset,
                                     .indexCount   = cmd->ElemCount,
                                     .scissor      = scissor_rect,
                                     .tex        = (Texture *) texId };



                if (main) {
                    ScissorRect scissorScaled{ .x      = std::max((int32_t) (command.scissor.x * context.frameWidth), 0),
                                               .y      = std::max((int32_t) (command.scissor.y * context.frameHeight), 0),
                                               .width  = (int32_t) (command.scissor.width * context.frameWidth),
                                               .height = (int32_t) (command.scissor.height * context.frameHeight) };
                    context.commandBuffer->setScissor(scissorScaled);
                } else {
                    ScissorRect scissorScaled{ .x      = std::max((int32_t) (command.scissor.x * displaySize.x), 0),
                                               .y      = std::max((int32_t) (command.scissor.y * displaySize.y), 0),
                                               .width  = (int32_t) (command.scissor.width * displaySize.x),
                                               .height = (int32_t) (command.scissor.height * displaySize.y) };
                    context.commandBuffer->setScissor(scissorScaled);
                }



                material->setTexture("_IMGUITexture", command.tex);
                material->applyMaterial(context.commandBuffer, 0U);

                context.commandBuffer->getDynamicVertexBuffer().drawChunk(context.commandBuffer,
                                                                          command.indexCount,
                                                                          command.indexOffset,
                                                                          command.vertexOffset);

                //                vertexBuffer.drawIndexed(context.commandBuffer,
                //                                         command.indexCount,
                //                                         command.indexOffset,
                //                                         command.vertexOffset);
            }

            index_offset += cmd_list->IdxBuffer.Size;
            vertex_offset += cmd_list->VtxBuffer.Size;
        }
    }
}

static void initializeImgui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    /// Since 1.87 imgui_glfw uses the io.AddKeyEvent() to support keyboard
    /// now we are not supporting gamepad
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;     // disable Keyboard Controls
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;      // disable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigDockingAlwaysTabBar = false;
    io.ConfigWindowsResizeFromEdges = true;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigViewportsNoTaskBarIcon = true;
//    io.ConfigViewportsNoAutoMerge = true;
    io.ConfigDockingTransparentPayload = true;
    io.IniFilename = nullptr; // we manually save the ini file

    io.BackendRendererName = "OJOIE-D3D11";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_CreateWindow = ImGui_ImplDX11_CreateWindow;
    platform_io.Renderer_DestroyWindow = ImGui_Impl_DestroyWindow;
    platform_io.Renderer_SetWindowSize = ImGui_Impl_SetWindowSize;

    platform_io.Renderer_Tick = [] {
        GetGame().performMainLoop();
    };

    // Setup Dear ImGui style
    ImGui::StyleColorsSpectrum();

    auto &style = ImGui::GetStyle();

    style.WindowMinSize = { 128.f, 128.f };

//    ImGui::GetStyle().ChildRounding = 8.0f;

//    ImGui::GetStyle().GrabRounding = 8.0f;
//    ImGui::GetStyle().PopupRounding = 8.0f;
//    ImGui::GetStyle().ScrollbarRounding = 8.0f;

//            ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    if (std::filesystem::exists(std::format("{}/" SETTINGS_DIR "/" IMGUI_INI_NAME, GetApplicationFolder()).c_str())) {
        ImGui::LoadIniSettingsFromDisk(std::format("{}/" SETTINGS_DIR "/" IMGUI_INI_NAME, GetApplicationFolder()).c_str());
    }
}

bool IMGUIManager::init() {
    window = nullptr;
    initializeImgui();

#if AN_WIN
    /// if no window, just return
    if (!GetLayerManager().hasLayer(0)) {
        return true;
    }

    window = (WIN::Window *)GetLayerManager().getLayerAt(0)->getWindow();
    ImGui_ImplWin32_Init(((WIN::Window *)window)->getHWND());

    s_oldWndProc = ((WIN::Window *)window)->setWNDPROCCallback(
            [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        if (s_oldWndProc && s_oldWndProc(hWnd, msg, wParam, lParam) == TRUE) {
            return TRUE;
        }
        return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    });

    ImGuiIO& io = ImGui::GetIO();

    io.SetPlatformImeDataFn = [](ImGuiViewport* viewport, ImGuiPlatformImeData* data) {
        UInt32 x = data->InputPos.x - viewport->Pos.x;
        UInt32 y = data->InputPos.y - viewport->Pos.y;
        if (viewport == ImGui::GetMainViewport()) {
            ((WIN::Window *)GetIMGUIManager().window)->setIMEInput(data->WantVisible, x, y);
        } else {
            if (ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData) {
                vd->window->setIMEInput(data->WantVisible, x, y);
            }
        }
    };

#else
#error "not implement"
#endif

//    if (!vertexBuffer.init()) return false;
//
//    vertexBuffer.setVertexStreamMode(0, kStreamModeDynamic);
//    vertexBuffer.setIndicesDynamic(true);

    /// init shader
    shader = (Shader *)GetResourceManager().getResource(Shader::GetClassNameStatic(), "IMGUI");
    ANAssert(shader);

    /// init material
    material = MakeObjectPtr<Material>();
    material->init(shader, "IMGUIMat");

    channelInfo[kShaderChannelVertex].offset = offsetof(ImDrawVert, pos);
    channelInfo[kShaderChannelVertex].format = kChannelFormatFloat;
    channelInfo[kShaderChannelVertex].dimension = 2;

    channelInfo[kShaderChannelTexCoord0].offset = offsetof(ImDrawVert, uv);
    channelInfo[kShaderChannelTexCoord0].format = kChannelFormatFloat;
    channelInfo[kShaderChannelTexCoord0].dimension = 2;

    channelInfo[kShaderChannelColor].offset = offsetof(ImDrawVert, col);
    channelInfo[kShaderChannelColor].format = kChannelFormatColor;
    channelInfo[kShaderChannelColor].dimension = 1;

    _dpiScaleX = window->getDPIScaleX();
    loadFont();

    return true;
}


void IMGUIManager::deinit() {

    ImGui_ImplWin32_Shutdown();

    if (!std::filesystem::exists(std::format("{}/" SETTINGS_DIR, GetApplicationFolder()).c_str()) || !std::filesystem::is_directory(std::format("{}/" SETTINGS_DIR, GetApplicationFolder()).c_str())) {
        std::filesystem::create_directories(std::format("{}/" SETTINGS_DIR, GetApplicationFolder()).c_str());
    }

    ImGui::SaveIniSettingsToDisk(std::format("{}/" SETTINGS_DIR "/" IMGUI_INI_NAME, GetApplicationFolder()).c_str());

    ImGui::DestroyContext();

    material.reset();
    fontTexture.reset();
//    vertexBuffer.deinit();
}


void IMGUIManager::loadFont() {
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig config{};

#ifdef _WIN32
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(std::format("{}/Data/Fonts/Inter-Light.ttf", GetApplicationFolder()).c_str(), 15.f * _dpiScaleX, &config);

    static const ImWchar ranges[] = {
        ICON_MIN_FA, ICON_MAX_FA,
        0,
    };

    float iconFontSize = 13.f * _dpiScaleX; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
    config.MergeMode = true;
    config.GlyphMinAdvanceX = iconFontSize; // Use if you want to make the icon monospaced
    config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF(std::format("{}/Data/Fonts/" FONT_ICON_FILE_NAME_FAR, GetApplicationFolder()).c_str(), iconFontSize, &config, ranges);
    io.Fonts->AddFontFromFileTTF(std::format("{}/Data/Fonts/" FONT_ICON_FILE_NAME_FAS, GetApplicationFolder()).c_str(), iconFontSize, &config, ranges);
    io.Fonts->AddFontFromFileTTF(std::format("{}/Data/Fonts/" FONT_ICON_FILE_NAME_FAB, GetApplicationFolder()).c_str(), iconFontSize, &config, ranges);
    /// OJOIE uncomment to support chinese
    //        ImFontConfig config{};
    //        config.MergeMode = true;
    //        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyhl.ttc", 16.f * dpiScaleX,
    //                                     &config, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    ANAssert(io.Fonts->Build());
    /// chinese font
//        io.Fonts->AddFontFromFileTTF("Data/Fonts/SourceHanSansCN-Regular.otf", 16.f * dpiScaleX,
//                                     nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
#elif defined(__APPLE__)

    /// TODO macos font

#endif

    /// create font texture
    unsigned char *font_data;
    int tex_width, tex_height;
    io.Fonts->GetTexDataAsRGBA32(&font_data, &tex_width, &tex_height);
    size_t upload_size = (size_t) tex_width * tex_height * 4 * sizeof(char);

    PixelFormat pixelFormat = kPixelFormatRGBA8Unorm;

    DirectX::Image image;
    image.width = tex_width;
    image.height = tex_height;
    image.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    image.rowPitch = tex_width * 4 * sizeof(char);
    image.slicePitch = tex_width * tex_height * 4 * sizeof(char);
    image.pixels = font_data;

    DirectX::ScratchImage srcImage;
    HRESULT hr = srcImage.InitializeFromImage(image);
    ANAssert(SUCCEEDED(hr));

    DirectX::ScratchImage bcImage;
    hr = Compress(
            srcImage.GetImages(), srcImage.GetImageCount(),
            srcImage.GetMetadata(), DXGI_FORMAT_BC3_UNORM,
            DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT,
            bcImage);

    if (FAILED(hr)) {
        AN_LOG(Error, "Fail to compress font texture");
    } else {
        pixelFormat = kPixelFormatBC3_RGBA;
        font_data = bcImage.GetPixels();
        AN_LOG(Info, "Compress IMGUI font texture to BC3 success");
    }

    TextureDescriptor textureDescriptor{};
    textureDescriptor.width = tex_width;
    textureDescriptor.height = tex_height;
    textureDescriptor.mipmapLevel = 1;
    textureDescriptor.pixelFormat = pixelFormat;

    fontTexture = MakeObjectPtr<Texture2D>();

    SamplerDescriptor samplerDescriptor = Texture::DefaultSamplerDescriptor();
    samplerDescriptor.filter = kSamplerFilterBilinear;
    samplerDescriptor.addressModeU = kSamplerAddressModeClampToEdge;
    samplerDescriptor.addressModeV = kSamplerAddressModeClampToEdge;
    samplerDescriptor.addressModeW = kSamplerAddressModeClampToEdge;
    samplerDescriptor.borderColor = kSamplerBorderColorOpaqueWhite;

    if (!fontTexture->init(textureDescriptor, samplerDescriptor)) {
        AN_LOG(Error, "cannot init IMGUI font texture");
        fontTexture.reset();
        return;
    }

    fontTexture->setPixelData(font_data);
    fontTexture->uploadToGPU(false);

    io.Fonts->SetTexID(fontTexture.get());

    material->setTexture("_IMGUITexture", fontTexture.get()); // set as default texture


    io.Fonts->ClearTexData();
}


void IMGUIManager::onNewFrame() {
    if (window == nullptr) return;

    if (_dpiScaleX != window->getDPIScaleX()) {
        _dpiScaleX = window->getDPIScaleX();
        loadFont();
    }

    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuizmo::BeginFrame();
}


void IMGUIManager::update(UInt32 frameVersion) {
    if (window == nullptr) return;
    ImGui::EndFrame();
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
    }
}

void IMGUIManager::render(RenderContext &context) {
    if (window == nullptr) return;

    UInt32 frameVersion = context.frameVersion;

    ImGui::Render();

    /// render main window
    renderDrawData(context, ImGui::GetDrawData(), true);

    ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
    for (int i = 1; i < platform_io.Viewports.Size; i++) {
        ImGuiViewport* viewport = platform_io.Viewports[i];
        ImGui_ImplDX11_ViewportData* vd = (ImGui_ImplDX11_ViewportData*)viewport->RendererUserData;
        Presentable *presentable = vd->layer->nextPresentable();
        CommandBuffer *commandBuffer = GetCommandPool().newCommandBuffer();

        ClearValue clearValue[] = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        const RenderTarget *renderTargets[] = { presentable->getRenderTarget() };
        Size renderArea = presentable->getRenderTarget()->getSize();
        commandBuffer->beginRenderPass(renderArea.width, renderArea.height,
                                            vd->renderPass,
                                            renderTargets,
                                            clearValue);

        commandBuffer->setViewport({ .originX = 0.f, .originY = 0.f, .width = (float) renderArea.width, .height = (float) renderArea.height, .znear = 0.f, .zfar = 1.f });

        commandBuffer->setScissor({ .x = 0, .y = 0, .width = (int) renderArea.width, .height = (int) renderArea.height });

        renderDrawData(context, viewport->DrawData, false);

        commandBuffer->endRenderPass();
        commandBuffer->present(*presentable);
        commandBuffer->submit();
    }
}


void IMGUIManager::dragEvent(float x, float y) {
    auto &io = ImGui::GetIO();
    io.AddMousePosEvent(x, y);
}

bool IMGUIManager::viewportEnabled() {
    auto &io = ImGui::GetIO();
    return io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable;
}

    void IMGUIManager::addIMGUIComponent(IMGUIListNode &node) {
    _IMGUIList.push_back(node);
}

void IMGUIManager::removeIMGUIComponent(IMGUIListNode &node) {
    node.removeFromList();
}

void IMGUIManager::updateIMGUIComponents() {
    for (auto &node : _IMGUIList) {
        IMGUI &imgui = *node;
        imgui.onGUI();
    }
}


IMGUIManager &GetIMGUIManager() {
    static IMGUIManager imguiManager;
    return imguiManager;
}

}
