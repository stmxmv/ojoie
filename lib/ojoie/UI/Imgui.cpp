//
// Created by Aleudillonam on 9/10/2022.
//

#include "Threads/Dispatch.hpp"
#include "Core/Game.hpp"
#include "UI/Imgui.hpp"
#include "UI/ImguiStyles.hpp"
#include "Render/Renderer.hpp"
#include "Render/Buffer.hpp"

#include <imgui/imgui.h>
#include <thread>
#include <filesystem>

namespace AN::UI {

#ifdef AN_DEBUG
#define CHECK_ON_RENDER_THREAD()  do { \
        if (GetCurrentThreadID() != Dispatch::GetThreadID(Dispatch::Render)) { \
            throw Exception("Contexts must execute on render thread!");\
        }\
    } while (0)
#else
#define CHECK_ON_RENDER_THREAD() (void)0
#endif

#define SETTINGS_DIR "./Settings"
#define IMGUI_INI_NAME "imgui.ini"

static void initializeImgui() {
    CHECK_ON_RENDER_THREAD();
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    /// Since 1.87 imgui_glfw uses the io.AddKeyEvent() to support keyboard
    /// now we are not supporting gamepad
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.IniFilename = nullptr; // we manually save the ini file

    // Setup Dear ImGui style
    ImGui::StyleColorsSpectrum();
    //        ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    if (std::filesystem::exists(SETTINGS_DIR "/" IMGUI_INI_NAME)) {
        ImGui::LoadIniSettingsFromDisk(SETTINGS_DIR "/" IMGUI_INI_NAME);
    }
}

//struct InputContext {
//    GLFWwindowfocusfun preWindowFocusFun;
//    GLFWcursorenterfun preCursorEnterFun;
//    GLFWcursorposfun preCursorPosFun;
//    GLFWmousebuttonfun preMouseButtonFun;
//    GLFWscrollfun preScrollFun;
//    GLFWkeyfun preKeyFun;
//    GLFWcharfun preCharFun;
//    GLFWmonitorfun preMonitorFun;
//};
//
//static InputContext staticInputContext{};
//
//static void restoreInputCallbacks(GLFWwindow *glfWwindow, InputContext &inputContext) {
//    glfwSetWindowFocusCallback(glfWwindow, inputContext.preWindowFocusFun);
//    glfwSetCursorPosCallback(glfWwindow, inputContext.preCursorPosFun);
//    glfwSetCursorEnterCallback(glfWwindow, inputContext.preCursorEnterFun);
//    glfwSetMouseButtonCallback(glfWwindow, inputContext.preMouseButtonFun);
//    glfwSetScrollCallback(glfWwindow, inputContext.preScrollFun);
//    glfwSetKeyCallback(glfWwindow, inputContext.preKeyFun);
//    glfwSetCharCallback(glfWwindow, inputContext.preCharFun);
//
//    glfwSetMonitorCallback(inputContext.preMonitorFun);
//}
//
//static void installInputCallbacks(GLFWwindow *glfWwindow, InputContext &inputContext) {
//
//    /// all glfw callbacks calls on main thread
//
//    inputContext.preWindowFocusFun = glfwSetWindowFocusCallback(glfWwindow, [](GLFWwindow* window, int focused) {
//        if (staticInputContext.preWindowFocusFun) {
//            staticInputContext.preWindowFocusFun(window, focused);
//        }
//        Dispatch::async(Dispatch::Game, [=]{
//            Dispatch::async(Dispatch::Render, [=]{
//                ImGui_ImplGlfw_WindowFocusCallback(window, focused);
//            });
//        });
//    });
//
//    inputContext.preCursorEnterFun = glfwSetCursorEnterCallback(glfWwindow, [](GLFWwindow *window, int entered) {
//        if (staticInputContext.preCursorEnterFun) {
//            staticInputContext.preCursorEnterFun(window, entered);
//        }
//        Dispatch::async(Dispatch::Game, [=]{
//            Dispatch::async(Dispatch::Render, [=]{
//                ImGui_ImplGlfw_CursorEnterCallback(window, entered);
//            });
//        });
//    });
//
//    inputContext.preCursorPosFun = glfwSetCursorPosCallback(glfWwindow, [](GLFWwindow* window, double xpos, double ypos) {
//        if (staticInputContext.preCursorPosFun) {
//            staticInputContext.preCursorPosFun(window, xpos, ypos);
//        }
//        Dispatch::async(Dispatch::Game, [=]{
//            Dispatch::async(Dispatch::Render, [=]{
//                CHECK_ON_RENDER_THREAD();
//                ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
//            });
//        });
//    });
//
//    inputContext.preMouseButtonFun = glfwSetMouseButtonCallback(glfWwindow, [](GLFWwindow* window, int button, int action, int mods) {
//        if (staticInputContext.preMouseButtonFun) {
//            staticInputContext.preMouseButtonFun(window, button, action, mods);
//        }
//        Dispatch::async(Dispatch::Game, [=]{
//            Dispatch::async(Dispatch::Render, [=]{
//                ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
//            });
//        });
//    });
//
//    inputContext.preScrollFun = glfwSetScrollCallback(glfWwindow, [](GLFWwindow* window, double xoffset, double yoffset) {
//        if (staticInputContext.preScrollFun) {
//            staticInputContext.preScrollFun(window, xoffset, yoffset);
//        }
//        Dispatch::async(Dispatch::Game, [=]{
//            Dispatch::async(Dispatch::Render, [=]{
//                ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
//            });
//        });
//    });
//
//    inputContext.preKeyFun = glfwSetKeyCallback(glfWwindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
//        if (staticInputContext.preKeyFun) {
//            staticInputContext.preKeyFun(window, key, scancode, action, mods);
//        }
//        Dispatch::async(Dispatch::Game, [=]{
//            Dispatch::async(Dispatch::Render, [=]{
//                ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
//            });
//        });
//    });
//
//    inputContext.preCharFun = glfwSetCharCallback(glfWwindow, [](GLFWwindow* window, unsigned int codepoint){
//        if (staticInputContext.preCharFun) {
//            staticInputContext.preCharFun(window, codepoint);
//        }
//        Dispatch::async(Dispatch::Game, [=]{
//            Dispatch::async(Dispatch::Render, [=]{
//                ImGui_ImplGlfw_CharCallback(window, codepoint);
//            });
//        });
//    });
//
//
//    inputContext.preMonitorFun = glfwSetMonitorCallback([](GLFWmonitor* monitor, int event) {
//        if (staticInputContext.preMonitorFun) {
//            staticInputContext.preMonitorFun(monitor, event);
//        }
//        Dispatch::async(Dispatch::Game, [=]{
//            Dispatch::async(Dispatch::Render, [=]{
//                ImGui_ImplGlfw_MonitorCallback(monitor, event);
//            });
//        });
//    });
//
//}

static void deinitImgui() {
    CHECK_ON_RENDER_THREAD();
    GetRenderer().resourceFence();

    if (!std::filesystem::exists(SETTINGS_DIR) || !std::filesystem::is_directory(SETTINGS_DIR)) {
        std::filesystem::create_directories(SETTINGS_DIR);
    }

    ImGui::SaveIniSettingsToDisk(SETTINGS_DIR "/" IMGUI_INI_NAME);

    ImGui::DestroyContext();
}

bool Imgui::init() {
    initializeImgui();
//    RC::SamplerDescriptor samplerDescriptor = RC::SamplerDescriptor::Default();
//    samplerDescriptor.mipFilter = RC::SamplerMipFilter::Nearest;
//    samplerDescriptor.addressModeU = RC::SamplerAddressMode::ClampToEdge;
//    samplerDescriptor.addressModeV = RC::SamplerAddressMode::ClampToEdge;
//    samplerDescriptor.addressModeW = RC::SamplerAddressMode::ClampToEdge;
//    samplerDescriptor.borderColor = RC::SamplerBorderColor::OpaqueWhite;

//    if (!sampler.init(samplerDescriptor)) {
//        return false;
//    }

    VertexDescriptor vertexDescriptor{};
    vertexDescriptor.attributes[0].format = kChannelFormatFloat;
    vertexDescriptor.attributes[0].dimension = 2;
    vertexDescriptor.attributes[0].binding = 0;
    vertexDescriptor.attributes[0].offset = offsetof(ImDrawVert, pos);
    vertexDescriptor.attributes[0].location = 0;

    vertexDescriptor.attributes[1].format = kChannelFormatFloat;
    vertexDescriptor.attributes[1].dimension = 2;
    vertexDescriptor.attributes[1].binding = 0;
    vertexDescriptor.attributes[1].offset = offsetof(ImDrawVert, uv);
    vertexDescriptor.attributes[1].location = 1;

    vertexDescriptor.attributes[2].format = kChannelFormatByte;
    vertexDescriptor.attributes[2].dimension = 4;
    vertexDescriptor.attributes[2].binding = 0;
    vertexDescriptor.attributes[2].offset = offsetof(ImDrawVert, col);
    vertexDescriptor.attributes[2].location = 2;

    vertexDescriptor.layouts[0].stepFunction = kVertexStepFunctionPerVertex;
    vertexDescriptor.layouts[0].stride = sizeof(ImDrawVert);

    DepthStencilDescriptor depthStencilDescriptor{};
    depthStencilDescriptor.depthTestEnabled = false;
    depthStencilDescriptor.depthWriteEnabled = false;
    depthStencilDescriptor.depthCompareFunction = kCompareFunctionNever;

    RenderPipelineStateDescriptor renderPipelineStateDescriptor{};
    throw std::runtime_error("not implement");
//    renderPipelineStateDescriptor.vertexFunction = { .name = "main", .library = "imgui.vert.spv" };
//    renderPipelineStateDescriptor.fragmentFunction = { .name = "main", .library = "imgui.frag.spv" };

    renderPipelineStateDescriptor.colorAttachments[0].writeMask = kColorWriteMaskAll;
    renderPipelineStateDescriptor.colorAttachments[0].blendingEnabled = true;

    renderPipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = kBlendFactorSourceAlpha;
    renderPipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = kBlendFactorOneMinusSourceAlpha;
    renderPipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = kBlendOperationAdd;

    renderPipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = kBlendFactorZero;
    renderPipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = kBlendFactorOne;
    renderPipelineStateDescriptor.colorAttachments[0].alphaBlendOperation = kBlendOperationAdd;


//    renderPipelineStateDescriptor.vertexDescriptor = vertexDescriptor;
    renderPipelineStateDescriptor.depthStencilDescriptor = depthStencilDescriptor;

    renderPipelineStateDescriptor.rasterSampleCount = GetRenderer().getRenderContext().msaaSamples;
    renderPipelineStateDescriptor.alphaToOneEnabled = false;
    renderPipelineStateDescriptor.alphaToCoverageEnabled = false;

    renderPipelineStateDescriptor.cullMode = kCullModeNone;

//    if (!renderPipelineState.init(std::move(renderPipelineStateDescriptor))) {
//        return false;
//    }

    return true;
}

void Imgui::deinit() {
//    sampler.deinit();
//    fontTexture.deinit();
    renderPipelineState.deinit();
    deinitImgui();
}

void Imgui::render(const AN::RenderContext &context) {

//    static GLFWwindow *lastWindow = nullptr;
    static float dpiScaleX = 0.f;
//    GLFWwindow *currentWindow/* = (GLFWwindow *)context.window->getUnderlyingWindow()*/;
//    if (lastWindow != currentWindow) {
//        if (lastWindow) {
//            ImGui_ImplGlfw_Shutdown();
//        } else {
//            GetGame().registerCleanupTask([]{
//                Dispatch::async(Dispatch::Render, [=]{
//                    ImGui_ImplGlfw_Shutdown();
//                });
//            });
//        }
        /// it seems not installing callback can be called instead of main thread
#ifdef OJOIE_USE_VULKAN
//        ImGui_ImplGlfw_InitForVulkan(currentWindow, false);
#elif defined(OJOIE_USE_OPENGL)
        ImGui_ImplGlfw_InitForOpenGL(currentWindow, false);
#else
#error "not implement"
#endif


//        Dispatch::async(Dispatch::Main, [=, lastWindow = lastWindow]{
//            if (lastWindow) {
//                restoreInputCallbacks(lastWindow, staticInputContext);
//            }
//            installInputCallbacks(currentWindow, staticInputContext);
//        });
//
//        lastWindow = currentWindow;
//    }

    if (dpiScaleX != context.dpiScaleX) {
        dpiScaleX = context.dpiScaleX;
        ImGuiIO& io = ImGui::GetIO();
#ifdef _WIN32
        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuil.ttf", 16.f * context.dpiScaleX);
#elif defined(__APPLE__)

        /// TODO macos font

#endif

        /// create font texture
        unsigned char *font_data;
        int tex_width, tex_height;
        io.Fonts->GetTexDataAsRGBA32(&font_data, &tex_width, &tex_height);
        size_t upload_size = (size_t) tex_width * tex_height * 4 * sizeof(char);

        RC::BufferAllocation stageBufferAllocation = context.bufferManager.buffer(RC::BufferUsageFlag::TransferSource, upload_size);


        memcpy(stageBufferAllocation.map(), font_data, upload_size);
        stageBufferAllocation.getBuffer().flush();

//        RC::TextureDescriptor textureDescriptor{};
//        textureDescriptor.width = tex_width;
//        textureDescriptor.height = tex_height;
//        textureDescriptor.mipmapLevel = 1;
//        textureDescriptor.pixelFormat = RC::PixelFormat::RGBA8Unorm;
//
//        if (!fontTexture.init(textureDescriptor)) {
//            ANLog("Fail to init imgui font texture");
//            return;
//        }

//        io.Fonts->SetTexID((void *)&fontTexture);

        RC::BlitCommandEncoder &blitCommandEncoder = context.blitCommandEncoder;

//        blitCommandEncoder.copyBufferToTexture(stageBufferAllocation.getBuffer(),
//                                               stageBufferAllocation.getOffset(), 0, 0, fontTexture, 0, 0, 0, tex_width, tex_height);

    }
}

void Imgui::newFrame(const RenderContext &context) {
//    ImGui_ImplGlfw_NewFrame(context);
    ImGui::NewFrame();
}


void Imgui::updateBuffer(const RenderContext &context, RC::RenderCommandEncoder &renderCommandEncoder) {
    ImDrawData *draw_data = ImGui::GetDrawData();

    size_t vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    size_t index_buffer_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

    if ((vertex_buffer_size == 0) || (index_buffer_size == 0)) {
        return;
    }

    vertex_data.resize(vertex_buffer_size);
    index_data.resize(index_buffer_size);

    // Upload data
    ImDrawVert *vtx_dst = (ImDrawVert *) vertex_data.data();
    ImDrawIdx  *idx_dst = (ImDrawIdx *) index_data.data();

    for (int i = 0; i < draw_data->CmdListsCount; i++) {
        const ImDrawList *cmd_list = draw_data->CmdLists[i];
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }


    RC::BufferAllocation vertexBufferAllocation = context.bufferManager.buffer(RC::BufferUsageFlag::VertexBuffer, vertex_buffer_size);
    memcpy(vertexBufferAllocation.map(), vertex_data.data(), vertex_buffer_size);

    vertexBufferAllocation.getBuffer().flush();

    renderCommandEncoder.bindVertexBuffer(0, vertexBufferAllocation.getOffset(), vertexBufferAllocation.getBuffer());


    RC::BufferAllocation indexBufferAllocation = context.bufferManager.buffer(RC::BufferUsageFlag::IndexBuffer, index_buffer_size);
    memcpy(indexBufferAllocation.map(), index_data.data(), index_buffer_size);
    indexBufferAllocation.getBuffer().flush();

    renderCommandEncoder.bindIndexBuffer(RC::IndexType::UInt16, indexBufferAllocation.getOffset(), indexBufferAllocation.getBuffer());

}
void Imgui::endFrame(const RenderContext &context) {
    ImGui::Render();

    RC::RenderCommandEncoder &renderCommandEncoder = context.renderCommandEncoder;

//    renderCommandEncoder.setCullMode(RC::CullMode::None);
    renderCommandEncoder.setRenderPipelineState(renderPipelineState);

//    renderCommandEncoder.bindSampler(0, sampler);

    RC::Texture *texture = nullptr;

    // Pre-rotation
    auto &io             = ImGui::GetIO();
    auto  push_transform = Math::mat4(1.0f);

    // GUI coordinate space to screen space
    push_transform = Math::translate(push_transform, Math::vec3(-1.0f, -1.0f, 0.0f));
    push_transform = Math::scale(push_transform, Math::vec3(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, 0.0f));

    renderCommandEncoder.pushConstants(0, sizeof push_transform, &push_transform);

    updateBuffer(context, renderCommandEncoder);


    // Render commands
    ImDrawData *draw_data     = ImGui::GetDrawData();
    int32_t     vertex_offset = 0;
    uint32_t    index_offset  = 0;

    if (draw_data->CmdListsCount > 0) {
        for (int32_t i = 0; i < draw_data->CmdListsCount; i++) {
            const ImDrawList *cmd_list = draw_data->CmdLists[i];
            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
                const ImDrawCmd *cmd = &cmd_list->CmdBuffer[j];
//                RC::ScissorRect scissor_rect{ .x = std::max(static_cast<int32_t>(cmd->ClipRect.x), 0),
//                                              .y = std::max(static_cast<int32_t>(cmd->ClipRect.y), 0),
//                                              .width = static_cast<int32_t>(cmd->ClipRect.z - cmd->ClipRect.x),
//                                              .height = static_cast<int32_t>(cmd->ClipRect.w - cmd->ClipRect.y) };

                RC::Texture *currentTexture = (RC::Texture *)cmd->GetTexID();
                if (currentTexture != texture) {
                    texture = currentTexture;
                    renderCommandEncoder.bindTexture(2, *texture);
                }

//                renderCommandEncoder.setScissor(scissor_rect);
                renderCommandEncoder.drawIndexed(cmd->ElemCount, index_offset, vertex_offset);
                index_offset += cmd->ElemCount;
            }
            vertex_offset += cmd_list->VtxBuffer.Size;
        }
    }

//    RC::ScissorRect scissor_rect{ .x = 0, .y = 0,
//                                  .width = (int32_t)context.frameWidth,
//                                  .height = (int32_t)context.frameHeight };
//    renderCommandEncoder.setScissor(scissor_rect);
}

}