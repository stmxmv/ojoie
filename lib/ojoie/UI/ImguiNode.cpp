//
// Created by Aleudillonam on 7/28/2022.
//

#include "UI/ImguiNode.hpp"
#include "Core/DispatchQueue.hpp"
#include "Core/Game.hpp"
#include "Core/Window.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

#include "UI/ImguiStyles.hpp"

namespace AN {

#ifdef AN_DEBUG
#define CHECK_ON_RENDER_THREAD()  do { \
        if (std::this_thread::get_id() != DispatchQueue::GetThreadID(DispatchQueue::Render)) { \
            throw Exception("Contexts must execute on render thread!");\
        }\
    } while (0)
#else
#define CHECK_ON_RENDER_THREAD() (void)0
#endif

static void initializeImgui() {
    CHECK_ON_RENDER_THREAD();
    const char* glsl_version = "#version 430 core";
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsSpectrum();
//    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    ImGui_ImplOpenGL3_Init(glsl_version);

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
        DispatchQueue::async(DispatchQueue::Game, [=]{
            DispatchQueue::async(DispatchQueue::Render, [=]{
                ImGui_ImplGlfw_WindowFocusCallback(window, focused);
            });
        });
    });

    inputContext.preCursorEnterFun = glfwSetCursorEnterCallback(glfWwindow, [](GLFWwindow *window, int entered) {
        if (staticInputContext.preCursorEnterFun) {
            staticInputContext.preCursorEnterFun(window, entered);
        }
        DispatchQueue::async(DispatchQueue::Game, [=]{
            DispatchQueue::async(DispatchQueue::Render, [=]{
                ImGui_ImplGlfw_CursorEnterCallback(window, entered);
            });
        });
    });

    inputContext.preCursorPosFun = glfwSetCursorPosCallback(glfWwindow, [](GLFWwindow* window, double xpos, double ypos) {
        if (staticInputContext.preCursorPosFun) {
            staticInputContext.preCursorPosFun(window, xpos, ypos);
        }
        DispatchQueue::async(DispatchQueue::Game, [=]{
            DispatchQueue::async(DispatchQueue::Render, [=]{
                CHECK_ON_RENDER_THREAD();
                ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
            });
        });
    });

    inputContext.preMouseButtonFun = glfwSetMouseButtonCallback(glfWwindow, [](GLFWwindow* window, int button, int action, int mods) {
        if (staticInputContext.preMouseButtonFun) {
            staticInputContext.preMouseButtonFun(window, button, action, mods);
        }
        DispatchQueue::async(DispatchQueue::Game, [=]{
            DispatchQueue::async(DispatchQueue::Render, [=]{
                ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
            });
        });
    });

    inputContext.preScrollFun = glfwSetScrollCallback(glfWwindow, [](GLFWwindow* window, double xoffset, double yoffset) {
        if (staticInputContext.preScrollFun) {
            staticInputContext.preScrollFun(window, xoffset, yoffset);
        }
        DispatchQueue::async(DispatchQueue::Game, [=]{
            DispatchQueue::async(DispatchQueue::Render, [=]{
                ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
            });
        });
    });

    inputContext.preKeyFun = glfwSetKeyCallback(glfWwindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (staticInputContext.preKeyFun) {
            staticInputContext.preKeyFun(window, key, scancode, action, mods);
        }
        DispatchQueue::async(DispatchQueue::Game, [=]{
            DispatchQueue::async(DispatchQueue::Render, [=]{
                ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
            });
        });
    });

    inputContext.preCharFun = glfwSetCharCallback(glfWwindow, [](GLFWwindow* window, unsigned int codepoint){
        if (staticInputContext.preCharFun) {
            staticInputContext.preCharFun(window, codepoint);
        }
        DispatchQueue::async(DispatchQueue::Game, [=]{
            DispatchQueue::async(DispatchQueue::Render, [=]{
                ImGui_ImplGlfw_CharCallback(window, codepoint);
            });
        });
    });


    inputContext.preMonitorFun = glfwSetMonitorCallback([](GLFWmonitor* monitor, int event) {
        if (staticInputContext.preMonitorFun) {
            staticInputContext.preMonitorFun(monitor, event);
        }
        DispatchQueue::async(DispatchQueue::Game, [=]{
            DispatchQueue::async(DispatchQueue::Render, [=]{
                ImGui_ImplGlfw_MonitorCallback(monitor, event);
            });
        });
    });

}

static void deinitImgui() {
    CHECK_ON_RENDER_THREAD();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool ImguiNode::init() {
    Node::init();

    static bool isImguiInited = false;
    if (!isImguiInited) {
        DispatchQueue::async(DispatchQueue::Game, [=]{
            DispatchQueue::async(DispatchQueue::Render, [=]{
                initializeImgui();
            });
        });

        GetGame().registerCleanupTask([]{
            /// called on game thread
            DispatchQueue::async(DispatchQueue::Render, [=]{
                deinitImgui();
            });
        });
    }



    return true;
}

void ImguiNode::render(const RenderContext &context) {
    Node::render(context);

    static GLFWwindow *lastWindow = nullptr;
    GLFWwindow *currentWindow = (GLFWwindow *)context.window->getUnderlyingWindow();
    if (lastWindow != currentWindow) {
        if (lastWindow) {
            ImGui_ImplGlfw_Shutdown();
        }
        ImGui_ImplGlfw_InitForOpenGL(currentWindow, false);

#ifdef _WIN32
        ImGuiIO& io = ImGui::GetIO();
        HWND hWnd = glfwGetWin32Window(currentWindow);
        HDC hdc     = GetDC(hWnd);
        float g_DPIScaleX = (float)GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
//        float g_DPIScaleY = GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
        ReleaseDC(hWnd, hdc);
        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuil.ttf", 16.f * g_DPIScaleX);

#elif defined(__APPLE__)

        /// TODO macos font

#endif


        DispatchQueue::async(DispatchQueue::Main, [=]{
            if (lastWindow) {
                restoreInputCallbacks(lastWindow, staticInputContext);
            }
            installInputCallbacks(currentWindow, staticInputContext);
        });

        lastWindow = currentWindow;
    }
}

void ImguiNode::newFrame() {
    CHECK_ON_RENDER_THREAD();
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImguiNode::endFrame() {
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void TestImguiNode::render(const RenderContext &context) {
    ImguiNode::render(context);

    newFrame();

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

    endFrame();
}


}