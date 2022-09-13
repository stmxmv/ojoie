//
// Created by Aleudillonam on 9/10/2022.
//

#include "Core/Dispatch.hpp"
#include "Core/Game.hpp"
#include "UI/Imgui.hpp"
#include "UI/ImguiStyles.hpp"
#include "Render/Renderer.hpp"
#include "Render/Buffer.hpp"

#include <GLFW/glfw3.h>
#include <Imgui/imgui.h>
#include <UI/imgui_impl_glfw.h>
#include <thread>

namespace AN::UI {

#ifdef AN_DEBUG
#define CHECK_ON_RENDER_THREAD()  do { \
        if (std::this_thread::get_id() != Dispatch::GetThreadID(Dispatch::Render)) { \
            throw Exception("Contexts must execute on render thread!");\
        }\
    } while (0)
#else
#define CHECK_ON_RENDER_THREAD() (void)0
#endif

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

    io.IniFilename = nullptr; // disable save ini file

    // Setup Dear ImGui style
    ImGui::StyleColorsSpectrum();
    //        ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
}

struct InputContext {
    GLFWwindowfocusfun preWindowFocusFun;
    GLFWcursorenterfun preCursorEnterFun;
    GLFWcursorposfun preCursorPosFun;
    GLFWmousebuttonfun preMouseButtonFun;
    GLFWscrollfun preScrollFun;
    GLFWkeyfun preKeyFun;
    GLFWcharfun preCharFun;
    GLFWmonitorfun preMonitorFun;
};

static InputContext staticInputContext{};

static void restoreInputCallbacks(GLFWwindow *glfWwindow, InputContext &inputContext) {
    glfwSetWindowFocusCallback(glfWwindow, inputContext.preWindowFocusFun);
    glfwSetCursorPosCallback(glfWwindow, inputContext.preCursorPosFun);
    glfwSetCursorEnterCallback(glfWwindow, inputContext.preCursorEnterFun);
    glfwSetMouseButtonCallback(glfWwindow, inputContext.preMouseButtonFun);
    glfwSetScrollCallback(glfWwindow, inputContext.preScrollFun);
    glfwSetKeyCallback(glfWwindow, inputContext.preKeyFun);
    glfwSetCharCallback(glfWwindow, inputContext.preCharFun);

    glfwSetMonitorCallback(inputContext.preMonitorFun);
}

static void installInputCallbacks(GLFWwindow *glfWwindow, InputContext &inputContext) {

    /// all glfw callbacks calls on main thread

    inputContext.preWindowFocusFun = glfwSetWindowFocusCallback(glfWwindow, [](GLFWwindow* window, int focused) {
        if (staticInputContext.preWindowFocusFun) {
            staticInputContext.preWindowFocusFun(window, focused);
        }
        Dispatch::async(Dispatch::Game, [=]{
            Dispatch::async(Dispatch::Render, [=]{
                ImGui_ImplGlfw_WindowFocusCallback(window, focused);
            });
        });
    });

    inputContext.preCursorEnterFun = glfwSetCursorEnterCallback(glfWwindow, [](GLFWwindow *window, int entered) {
        if (staticInputContext.preCursorEnterFun) {
            staticInputContext.preCursorEnterFun(window, entered);
        }
        Dispatch::async(Dispatch::Game, [=]{
            Dispatch::async(Dispatch::Render, [=]{
                ImGui_ImplGlfw_CursorEnterCallback(window, entered);
            });
        });
    });

    inputContext.preCursorPosFun = glfwSetCursorPosCallback(glfWwindow, [](GLFWwindow* window, double xpos, double ypos) {
        if (staticInputContext.preCursorPosFun) {
            staticInputContext.preCursorPosFun(window, xpos, ypos);
        }
        Dispatch::async(Dispatch::Game, [=]{
            Dispatch::async(Dispatch::Render, [=]{
                CHECK_ON_RENDER_THREAD();
                ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
            });
        });
    });

    inputContext.preMouseButtonFun = glfwSetMouseButtonCallback(glfWwindow, [](GLFWwindow* window, int button, int action, int mods) {
        if (staticInputContext.preMouseButtonFun) {
            staticInputContext.preMouseButtonFun(window, button, action, mods);
        }
        Dispatch::async(Dispatch::Game, [=]{
            Dispatch::async(Dispatch::Render, [=]{
                ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
            });
        });
    });

    inputContext.preScrollFun = glfwSetScrollCallback(glfWwindow, [](GLFWwindow* window, double xoffset, double yoffset) {
        if (staticInputContext.preScrollFun) {
            staticInputContext.preScrollFun(window, xoffset, yoffset);
        }
        Dispatch::async(Dispatch::Game, [=]{
            Dispatch::async(Dispatch::Render, [=]{
                ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
            });
        });
    });

    inputContext.preKeyFun = glfwSetKeyCallback(glfWwindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (staticInputContext.preKeyFun) {
            staticInputContext.preKeyFun(window, key, scancode, action, mods);
        }
        Dispatch::async(Dispatch::Game, [=]{
            Dispatch::async(Dispatch::Render, [=]{
                ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
            });
        });
    });

    inputContext.preCharFun = glfwSetCharCallback(glfWwindow, [](GLFWwindow* window, unsigned int codepoint){
        if (staticInputContext.preCharFun) {
            staticInputContext.preCharFun(window, codepoint);
        }
        Dispatch::async(Dispatch::Game, [=]{
            Dispatch::async(Dispatch::Render, [=]{
                ImGui_ImplGlfw_CharCallback(window, codepoint);
            });
        });
    });


    inputContext.preMonitorFun = glfwSetMonitorCallback([](GLFWmonitor* monitor, int event) {
        if (staticInputContext.preMonitorFun) {
            staticInputContext.preMonitorFun(monitor, event);
        }
        Dispatch::async(Dispatch::Game, [=]{
            Dispatch::async(Dispatch::Render, [=]{
                ImGui_ImplGlfw_MonitorCallback(monitor, event);
            });
        });
    });

}

static void deinitImgui() {
    CHECK_ON_RENDER_THREAD();
    GetRenderer().resourceFence();
    ImGui::DestroyContext();
}

bool Imgui::init() {
    initializeImgui();
    RC::SamplerDescriptor samplerDescriptor = RC::SamplerDescriptor::Default();
    samplerDescriptor.mipFilter = RC::SamplerMipFilter::Nearest;
    samplerDescriptor.addressModeU = RC::SamplerAddressMode::ClampToEdge;
    samplerDescriptor.addressModeV = RC::SamplerAddressMode::ClampToEdge;
    samplerDescriptor.addressModeW = RC::SamplerAddressMode::ClampToEdge;
    samplerDescriptor.borderColor = RC::SamplerBorderColor::OpaqueWhite;

    return sampler.init(samplerDescriptor);
}

void Imgui::deinit() {
    sampler.deinit();
    fontTexture.deinit();
    renderPipeline.deinit();
    deinitImgui();
}

void Imgui::render(const AN::RenderContext &context) {

    static GLFWwindow *lastWindow = nullptr;
    static float dpiScaleX = 0.f;
    GLFWwindow *currentWindow = (GLFWwindow *)context.window->getUnderlyingWindow();
    if (lastWindow != currentWindow) {
        if (lastWindow) {
            ImGui_ImplGlfw_Shutdown();
        } else {
            GetGame().registerCleanupTask([]{
                Dispatch::async(Dispatch::Render, [=]{
                    ImGui_ImplGlfw_Shutdown();
                });
            });
        }
        /// it seems not installing callback can be called instead of main thread
        ImGui_ImplGlfw_InitForOpenGL(currentWindow, false);

        Dispatch::async(Dispatch::Main, [=, lastWindow = lastWindow]{
            if (lastWindow) {
                restoreInputCallbacks(lastWindow, staticInputContext);
            }
            installInputCallbacks(currentWindow, staticInputContext);
        });

        lastWindow = currentWindow;
    }

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

        RC::BufferBlock bufferBlock = context.stageBufferPool.bufferBlock(upload_size);
        RC::BufferAllocation stageBufferAllocation = bufferBlock.allocate(upload_size);


        memcpy(stageBufferAllocation.map(), font_data, upload_size);
        stageBufferAllocation.getBuffer().flush();

        RC::TextureDescriptor textureDescriptor{};
        textureDescriptor.width = tex_width;
        textureDescriptor.height = tex_height;
        textureDescriptor.mipmapLevel = 1;
        textureDescriptor.pixelFormat = RC::PixelFormat::RGBA8Unorm;

        if (!fontTexture.init(textureDescriptor)) {
            ANLog("Fail to init imgui font texture");
            return;
        }

        RC::BlitCommandEncoder &blitCommandEncoder = context.blitCommandEncoder;

        blitCommandEncoder.copyBufferToTexture(stageBufferAllocation.getBuffer(),
                                               stageBufferAllocation.getOffset(), 0, 0, fontTexture, 0, 0, 0, tex_width, tex_height);

    }

    if (!renderPipelineInited) {

        RC::ShaderLibrary vertexLibrary, fragmentLibrary;

        if (!vertexLibrary.init(RC::ShaderLibraryType::Vertex, "imgui.vert.spv")) {
            return;
        }

        if (!fragmentLibrary.init(RC::ShaderLibraryType::Fragment, "imgui.frag.spv")) {
            return;
        }

        RC::VertexDescriptor vertexDescriptor{};
        vertexDescriptor.attributes[0].format = RC::VertexFormat::Float2;
        vertexDescriptor.attributes[0].binding = 0;
        vertexDescriptor.attributes[0].offset = offsetof(ImDrawVert, pos);
        vertexDescriptor.attributes[0].location = 0;

        vertexDescriptor.attributes[1].format = RC::VertexFormat::Float2;
        vertexDescriptor.attributes[1].binding = 0;
        vertexDescriptor.attributes[1].offset = offsetof(ImDrawVert, uv);
        vertexDescriptor.attributes[1].location = 1;

        vertexDescriptor.attributes[2].format = RC::VertexFormat::UChar4;
        vertexDescriptor.attributes[2].binding = 0;
        vertexDescriptor.attributes[2].offset = offsetof(ImDrawVert, col);
        vertexDescriptor.attributes[2].location = 2;

        vertexDescriptor.layouts[0].stepFunction = RC::VertexStepFunction::PerVertex;
        vertexDescriptor.layouts[0].stride = sizeof(ImDrawVert);

        RC::DepthStencilDescriptor depthStencilDescriptor{};
        depthStencilDescriptor.depthTestEnabled = false;
        depthStencilDescriptor.depthWriteEnabled = false;
        depthStencilDescriptor.depthCompareFunction = RC::CompareFunction::Never;

        RC::RenderPipelineDescriptor renderPipelineDescriptor{};
        renderPipelineDescriptor.vertexFunction = { .name = "main", .library = &vertexLibrary };
        renderPipelineDescriptor.fragmentFunction = { .name = "main", .library = &fragmentLibrary };

        renderPipelineDescriptor.colorAttachments[0].writeMask = RC::ColorWriteMask::All;
        renderPipelineDescriptor.colorAttachments[0].blendingEnabled = true;

        renderPipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = RC::BlendFactor::SourceAlpha;
        renderPipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = RC::BlendFactor::OneMinusSourceAlpha;
        renderPipelineDescriptor.colorAttachments[0].rgbBlendOperation = RC::BlendOperation::Add;

        renderPipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = RC::BlendFactor::One;
        renderPipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = RC::BlendFactor::OneMinusSourceAlpha;
        renderPipelineDescriptor.colorAttachments[0].alphaBlendOperation = RC::BlendOperation::Add;


        renderPipelineDescriptor.vertexDescriptor = vertexDescriptor;
        renderPipelineDescriptor.depthStencilDescriptor = depthStencilDescriptor;

        renderPipelineDescriptor.bindings[0] = RC::BindingType::Sampler;
        renderPipelineDescriptor.bindings[1] = RC::BindingType::Texture;

        renderPipelineDescriptor.pushConstantEnabled = true;
        renderPipelineDescriptor.pushConstantDescriptor.offset = 0;
        renderPipelineDescriptor.pushConstantDescriptor.size = sizeof(Math::mat4);
        renderPipelineDescriptor.pushConstantDescriptor.stageFlag = RC::ShaderStageFlag::Vertex;

        renderPipelineDescriptor.rasterSampleCount = context.msaaSamples;
        renderPipelineDescriptor.alphaToOneEnabled = false;
        renderPipelineDescriptor.alphaToCoverageEnabled = false;

        renderPipelineDescriptor.cullMode = RC::CullMode::None;

        if (!renderPipeline.init(renderPipelineDescriptor)) {
            return;
        }

        vertexLibrary.deinit();
        fragmentLibrary.deinit();

        renderPipelineInited = true;
    }

}

void Imgui::newFrame(const RenderContext &context) {
    ImGui_ImplGlfw_NewFrame(context);
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


    RC::BufferBlock bufferBlock = context.vertexBufferPool.bufferBlock(vertex_buffer_size);
    RC::BufferAllocation vertexBufferAllocation = bufferBlock.allocate(vertex_buffer_size);
    memcpy(vertexBufferAllocation.map(), vertex_data.data(), vertex_buffer_size);

    vertexBufferAllocation.getBuffer().flush();

    renderCommandEncoder.bindVertexBuffer(0, vertexBufferAllocation.getOffset(), vertexBufferAllocation.getBuffer());



    RC::BufferBlock indexBufferBlock = context.indexBufferPool.bufferBlock(index_buffer_size);
    RC::BufferAllocation indexBufferAllocation = indexBufferBlock.allocate(index_buffer_size);
    memcpy(indexBufferAllocation.map(), index_data.data(), index_buffer_size);
    indexBufferAllocation.getBuffer().flush();

    renderCommandEncoder.bindIndexBuffer(RC::IndexType::UInt16, indexBufferAllocation.getOffset(), indexBufferAllocation.getBuffer());

}
void Imgui::endFrame(const RenderContext &context) {
    ImGui::Render();

    RC::RenderCommandEncoder &renderCommandEncoder = context.renderCommandEncoder;

//    renderCommandEncoder.setCullMode(RC::CullMode::None);
    renderCommandEncoder.bindRenderPipeline(renderPipeline);

    renderCommandEncoder.bindSampler(0, sampler);
    renderCommandEncoder.bindTexture(1, fontTexture);

    // Pre-rotation
    auto &io             = ImGui::GetIO();
    auto  push_transform = Math::mat4(1.0f);

    // GUI coordinate space to screen space
    push_transform = Math::translate(push_transform, Math::vec3(-1.0f, -1.0f, 0.0f));
    push_transform = Math::scale(push_transform, Math::vec3(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, 0.0f));

    renderCommandEncoder.pushConstants(RC::ShaderStageFlag::Vertex, 0, sizeof push_transform, &push_transform);

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
                RC::ScissorRect scissor_rect{ .x = std::max(static_cast<int32_t>(cmd->ClipRect.x), 0),
                                              .y = std::max(static_cast<int32_t>(cmd->ClipRect.y), 0),
                                              .width = static_cast<int32_t>(cmd->ClipRect.z - cmd->ClipRect.x),
                                              .height = static_cast<int32_t>(cmd->ClipRect.w - cmd->ClipRect.y) };

                renderCommandEncoder.setScissor(scissor_rect);
                renderCommandEncoder.drawIndexed(cmd->ElemCount, index_offset, vertex_offset);
                index_offset += cmd->ElemCount;
            }
            vertex_offset += cmd_list->VtxBuffer.Size;
        }
    }

    RC::ScissorRect scissor_rect{ .x = 0, .y = 0,
                                  .width = (int32_t)context.frameWidth,
                                  .height = (int32_t)context.frameHeight };
    renderCommandEncoder.setScissor(scissor_rect);
}

}