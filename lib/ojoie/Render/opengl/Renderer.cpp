//
// Created by Aleudillonam on 7/28/2022.
//

#include <glad/glad.h>

#include "Core/Dispatch.hpp"
#include "Core/Game.hpp"
#include "Core/Log.h"
#include "Core/Window.hpp"
#include "Render/Renderer.hpp"

#include <GLFW/glfw3.h>
namespace AN {


static void APIENTRY glDebugOutput(GLenum source,
                                   GLenum type,
                                   unsigned int id,
                                   GLenum severity,
                                   GLsizei length,
                                   const char *message,
                                   const void *userParam);


Renderer &Renderer::GetSharedRenderer() {
    static Renderer renderer;
    return renderer;
}

void Renderer::renderOnce() {
    //    timer.mark();
    renderContext.deltaTime = GetGame().deltaTime;
    renderContext.elapsedTime = GetGame().elapsedTime;

    renderContext.window = currentWindow;

    ++renderContext.frameCount;

    /// TODO ask inputManager to get cursor state to context


    glViewport(0, 0, (int)renderContext.frameWidth, (int)renderContext.frameHeight);
    glClearColor(0.f, 0.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto &node : nodesToRender) {
        if (node->r_needsRender) {
            node->render(renderContext);
        }
    }


    glfwSwapBuffers((GLFWwindow *)currentWindow.load()->getUnderlyingWindow());

    if (completionHandler) [[likely]] {
        completionHandler();
    }
}

void Renderer::renderNodes(const std::vector<std::shared_ptr<Node>> &nodes) {
    static Window *lastWindow = nullptr;
    if (lastWindow != currentWindow.load(std::memory_order_relaxed)) [[unlikely]] {
        lastWindow = currentWindow.load(std::memory_order_relaxed);
        GetRenderQueue().enqueue([]{
            GLint major, minor;
            glGetIntegerv(GL_MAJOR_VERSION, &major);
            glGetIntegerv(GL_MINOR_VERSION, &minor);

            ANLog("OpenGL Version %s (%d.%d)", glGetString(GL_VERSION), major, minor);
            ANLog("GLSL Version %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
            ANLog("Device %s", glGetString(GL_RENDERER));

            int nrAttributes;
            glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
            ANLog("Maximum nr of vertex attributes supported: %d", nrAttributes);

#ifdef AN_DEBUG
            int flags;
            glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
            if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
                ANLog("Initialize OpenGL Debug Context");
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(glDebugOutput, nullptr);
                glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            }
#endif

            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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


static void APIENTRY glDebugOutput(GLenum source,
                                   GLenum type,
                                   unsigned int id,
                                   GLenum severity,
                                   GLsizei length,
                                   const char *message,
                                   const void *userParam) {
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
        return;
    }

    ANLog("Debug message (%u): %s", id, message);

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            ANLog("Source: API");
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            ANLog("Source: Window System");;
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            ANLog("Source: Shader Compiler");
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            ANLog("Source: Third Party");
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            ANLog("Source: Application");
            break;
        case GL_DEBUG_SOURCE_OTHER:
            ANLog("Source: Other");
            break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            ANLog("Type: Error");
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            ANLog("Type: Deprecated Behaviour");
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            ANLog("Type: Undefined Behaviour");
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            ANLog("Type: Portability");
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            ANLog("Type: Performance");
            break;
        case GL_DEBUG_TYPE_MARKER:
            ANLog("Type: Marker");
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            ANLog("Type: Push Group");
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            ANLog("Type: Pop Group");
            break;
        case GL_DEBUG_TYPE_OTHER:
            ANLog("Type: Other");
            break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            ANLog("Severity: high");
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            ANLog("Severity: medium");
            break;
        case GL_DEBUG_SEVERITY_LOW:
            ANLog("Severity: low");
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            ANLog("Severity: notification");
            break;
    }
}

}