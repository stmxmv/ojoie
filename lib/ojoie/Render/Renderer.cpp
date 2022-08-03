//
// Created by Aleudillonam on 7/28/2022.
//

#include <glad/glad.h>

#include "Core/Log.h"
#include "Core/Game.hpp"

#include "Render/Renderer.hpp"
#include "Render/RenderQueue.hpp"

#include <GLFW/glfw3.h>
namespace AN {


Renderer &Renderer::GetSharedRenderer() {
    static Renderer renderer;
    return renderer;
}

void Renderer::renderOnce() {
//    timer.mark();
    renderContext.deltaTime = GetGame().deltaTime;
    renderContext.elapsedTime = GetGame().elapsedTime;

    GLFWwindow *glfWwindow = glfwGetCurrentContext();
    Window *window = (Window *)glfwGetWindowUserPointer(glfWwindow);
    renderContext.window = window;

    int display_w, display_h;
    glfwGetFramebufferSize(glfWwindow, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(cosf(renderContext.elapsedTime), sinf(renderContext.elapsedTime), 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (auto &node : nodesToRender) {
        node->render(renderContext);
    }


    glfwSwapBuffers(glfWwindow);

    if (completionHandler) [[likely]] {
        completionHandler();
    }
}

void Renderer::renderNodes(const std::vector<std::shared_ptr<Node>> &nodes) {
    static bool firstTime = true;
    if (firstTime) [[unlikely]] {
        firstTime = false;
        GetRenderQueue().enqueue([]{
            GLint major, minor;
            glGetIntegerv(GL_MAJOR_VERSION, &major);
            glGetIntegerv(GL_MINOR_VERSION, &minor);

            ANLog("OpenGL Version %s (%d.%d)", glGetString(GL_VERSION), major, minor);
            ANLog("GLSL Version %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
            ANLog("Device %s", glGetString(GL_RENDERER));

        });
    }

    GetRenderQueue().enqueue([this, nodes = std::vector<std::shared_ptr<Node>>(nodes)]() mutable  {
        nodesToRender = std::move(nodes);
    });
}
void Renderer::render() {
    GetRenderQueue().enqueue([this] {
        renderOnce();
    });
}


}