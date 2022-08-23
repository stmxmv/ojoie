//
// Created by Aleudillonam on 7/28/2022.
//

#include "UI/ImguiNode.hpp"
#include "Core/Dispatch.hpp"
#include "Core/Game.hpp"
#include "Core/Window.hpp"
#include "Render/Renderer.hpp"
#include "Render/RenderPipeline.hpp"
#include "Render/private/vulkan.hpp"

#include <imgui/imgui.h>
#include <UI/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include <GLFW/glfw3.h>

#include "UI/ImguiStyles.hpp"

namespace AN {

#ifdef AN_DEBUG
#define CHECK_ON_RENDER_THREAD()  do { \
        if (std::this_thread::get_id() != Dispatch::GetThreadID(Dispatch::Render)) { \
            throw Exception("Contexts must execute on render thread!");\
        }\
    } while (0)
#else
#define CHECK_ON_RENDER_THREAD() (void)0
#endif

static VkDescriptorPool descriptorPool;
static VkDevice device;

static void check_vk_result(VkResult result) {
    if (result != 0) {
        ANLog("Vulkan Error result code %d", result);
    }
}

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

    const RenderContext &renderContext = GetRenderer().getRenderContext();
    device = renderContext.graphicContext->logicalDevice;

    /// create descriptor pool
    VkDescriptorPoolSize pool_sizes[] =
            {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    if (VK_SUCCESS != vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool)) {
        ANLog("vkCreateDescriptorPool fail");
        return;
    }


    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = renderContext.graphicContext->vkInstance;
    init_info.PhysicalDevice = renderContext.graphicContext->physicalDevice;
    init_info.Device = renderContext.graphicContext->logicalDevice;
    init_info.QueueFamily = renderContext.graphicContext->graphicsQueueFamily;
    init_info.Queue = renderContext.graphicContext->graphicQueue;
    init_info.PipelineCache = nullptr;
    init_info.DescriptorPool = descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = renderContext.maxFrameInFlight;
    init_info.ImageCount = renderContext.maxFrameInFlight;
    init_info.MSAASamples = (VkSampleCountFlagBits)renderContext.msaaSamples;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = check_vk_result;

    ImGui_ImplVulkan_Init(&init_info, renderContext.graphicContext->renderPass);

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
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    ImGui::DestroyContext();
}

bool ImguiNode::init() {
    Node::init();

    static bool isImguiInited = false;
    if (!isImguiInited) {
        Dispatch::async(Dispatch::Game, [=]{
            Dispatch::async(Dispatch::Render, [=]{
                initializeImgui();
            });
        });

        GetGame().registerCleanupTask([]{
            /// called on game thread
            Dispatch::async(Dispatch::Render, [=]{
                deinitImgui();
            });
        });
    }



    return true;
}

namespace RC {

extern RC::RenderPipeline *CurrentPipeline;

}


void ImguiNode::render(const RenderContext &context) {
    Node::render(context);

    RC::CurrentPipeline = nullptr;

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

        // Upload Fonts
        {
            // Use any command queue
            VkCommandBuffer command_buffer = beginSingleTimeCommands(context);

            ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

            endSingleTimeCommands(context, command_buffer);

            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
    }

}

void ImguiNode::newFrame(const RenderContext &context) {
    CHECK_ON_RENDER_THREAD();
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame(context);
    ImGui::NewFrame();
}

void ImguiNode::endFrame(const RenderContext &context) {
    // Rendering
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), context.graphicContext->commandBuffer);
}

void TestImguiNode::render(const RenderContext &context) {
    ImguiNode::render(context);

    newFrame(context);

    static bool show_demo_window = true, show_another_window = true;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f * context.deltaTime, 1.f / context.deltaTime);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    endFrame(context);
}


}