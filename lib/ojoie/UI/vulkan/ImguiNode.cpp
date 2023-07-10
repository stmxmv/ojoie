//
// Created by Aleudillonam on 7/28/2022.
//

#include "UI/ImguiNode.hpp"
#include "Threads/Dispatch.hpp"
#include "Core/Game.hpp"
#include "Core/Window.hpp"

#include "Render/RenderQueue.hpp"

#include <imgui/imgui.h>

namespace AN {

#ifdef AN_DEBUG
#define CHECK_ON_RENDER_THREAD()  do { \
        if (GetCurrentThreadID() != Dispatch::GetThreadID(Dispatch::Render)) { \
            throw Exception("Contexts must execute on render thread!");\
        }\
    } while (0)
#else
#define CHECK_ON_RENDER_THREAD() (void)0
#endif

static UI::Imgui imguiInstance;

bool ImguiNode::init() {
    return Node::init();
}


UI::Imgui &ImguiNode::GetImGuiInstance() {
    return imguiInstance;
}

void ImguiNode::ImguiNodeSceneProxy::newFrame(const RenderContext &context) {
    CHECK_ON_RENDER_THREAD();
    // Start the Dear ImGui frame
    imguiInstance.newFrame(context);

}
void ImguiNode::ImguiNodeSceneProxy::endFrame(const RenderContext &context) {
    // Rendering
    imguiInstance.endFrame(context);
}


void ImguiNode::ImguiNodeSceneProxy::postRender(const RenderContext &context) {
    Super::SceneProxyType::postRender(context);

    static bool isImguiInited = false;
    if (!isImguiInited) {
        if (!imguiInstance.init()) {
            return;
        }
        GetRenderQueue().registerCleanupTask([] {
            imguiInstance.deinit();
        });
        isImguiInited = true;
    }

    imguiInstance.render(context);

}


void TestImguiNode::TestImguiNodeSceneProxy::postRender(const RenderContext &context) {
    Super::SceneProxyType::postRender(context);

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