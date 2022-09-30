

macro(add_glsl_src target)

     find_package(Vulkan REQUIRED FATAL_ERROR)

     if(Vulkan_GLSLC_EXECUTABLE)

          foreach(SHADER_SRC ${ARGN})

               get_filename_component(SHADER_SRC_NAME ${SHADER_SRC} NAME)
               set(SHADER_SPV "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_SRC_NAME}.spv")
               if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/${SHADER_SRC})
                    set(SHADER_SRC "${CMAKE_CURRENT_LIST_DIR}/${SHADER_SRC}")
               endif()
               add_custom_command(
                       OUTPUT ${SHADER_SPV}
                       DEPENDS ${SHADER_SRC}
                       WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                       #                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
                       COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER_SRC} -o ${SHADER_SPV}
                       COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target}>/Resources/Shaders
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_SPV} $<TARGET_FILE_DIR:${target}>/Resources/Shaders
                       COMMENT "Compile Vulkan shader source ${SHADER_SRC_NAME} ..."
               )
               list(APPEND SHADER_SPVS ${SHADER_SPV})
          endforeach()

         target_sources(${target} PRIVATE ${SHADER_SPVS})

     else()
          message( FATAL_ERROR "vulkan glslc not found!" )
     endif()

endmacro()

set(
        OJOIE_SRCS
        ${OJOIE_SRCS}
        Core/App.cpp
        Core/Window.cpp
        Core/Game.cpp
        Core/Node.cpp
        Core/DispatchQueue.cpp
        Core/Log.cpp
        Core/Exception.cpp
        Core/Event.cpp
        Core/Configuration.cpp

        Render/RenderQueue.cpp
        Render/Renderer.cpp
        Render/RenderPipelineState.cpp
        Render/Mesh.cpp
        Render/Model.cpp
        Render/Font.cpp
        Render/UniformBuffer.cpp
        Render/VertexBuffer.cpp
        Render/Buffer.cpp
        Render/IndexBuffer.cpp
        Render/Texture.cpp
        Render/Sampler.cpp
        Render/TextureLoader.cpp
        Render/RenderCommandEncoder.cpp
        Render/BlitCommandEncoder.cpp
        Render/CommandBuffer.cpp
        Render/CommandQueue.cpp
        Render/BufferPool.cpp
        Render/BufferManager.cpp
        Render/Buffer.cpp

        UI/ImguiNode.cpp
        UI/imgui_impl_glfw.cpp
        UI/Imgui.cpp


        Audio/Sound.cpp
        Audio/WavFile.cpp
        Audio/Mp3File.cpp


        Node/StaticMeshNode.cpp
        Node/CameraNode.cpp
        Node/StaticModelNode.cpp
        Node/TextNode.cpp
        Node/Node2D.cpp


        Input/InputManager.cpp
)


if (OJOIE_USE_VULKAN)
    set(VULKAN_SRC_DIR Render/vulkan)
    set(
            OJOIE_SRCS
            ${OJOIE_SRCS}
            ${VULKAN_SRC_DIR}/Instance.cpp
            ${VULKAN_SRC_DIR}/Device.cpp
            ${VULKAN_SRC_DIR}/Image.cpp
            ${VULKAN_SRC_DIR}/FrameBuffer.cpp
            ${VULKAN_SRC_DIR}/RenderPass.cpp
            ${VULKAN_SRC_DIR}/SwapChain.cpp
            ${VULKAN_SRC_DIR}/RenderFrame.cpp
            ${VULKAN_SRC_DIR}/Layer.cpp
            ${VULKAN_SRC_DIR}/SemaphorePool.cpp
            ${VULKAN_SRC_DIR}/DescriptorSetManager.cpp
            ${VULKAN_SRC_DIR}/DescriptorSetLayout.cpp
            ${VULKAN_SRC_DIR}/CommandPool.cpp
            ${VULKAN_SRC_DIR}/Queue.cpp
            ${VULKAN_SRC_DIR}/PipelineReflection.cpp
            ${VULKAN_SRC_DIR}/ShaderLibrary.cpp
            ${VULKAN_SRC_DIR}/ShaderProgram.cpp
            ${VULKAN_SRC_DIR}/PipelineLayout.cpp
            ${VULKAN_SRC_DIR}/RenderPipeline.cpp
    )
    set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} volk SpvReflect)

    set(SHADER_DIR Shaders/vulkan)
endif()


set(
        GLSL_SRCS
        ${GLSL_SRCS}
        ${SHADER_DIR}/MeshNode.vert
        ${SHADER_DIR}/MeshNode.frag
        ${SHADER_DIR}/MeshNodeTextured.frag
        ${SHADER_DIR}/text.vert
        ${SHADER_DIR}/text.frag
        ${SHADER_DIR}/imgui.vert
        ${SHADER_DIR}/imgui.frag
        ${SHADER_DIR}/deferred/geometry.vert
        ${SHADER_DIR}/deferred/geometry.frag
        ${SHADER_DIR}/deferred/geometryTextured.frag
        ${SHADER_DIR}/deferred/lighting.vert
        ${SHADER_DIR}/deferred/lighting.frag
        ${SHADER_DIR}/deferred/geometryTAA.vert
        ${SHADER_DIR}/deferred/geometryTAA.frag
        ${SHADER_DIR}/deferred/geometryTexturedTAA.frag
        ${SHADER_DIR}/deferred/TAA.vert
        ${SHADER_DIR}/deferred/TAA.frag
)




set(
        OJOIE_PRIVATE_INCLUDE_DIR
        ${OJOIE_PRIVATE_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/include/ojoie
        ${CMAKE_BINARY_DIR}/include/ojoie
)



find_package(FLAC)

if (FLAC_FOUND)
     message("Found FLAC at ${FLAC_DIR}")
     set(OJOIE_SRCS ${OJOIE_SRCS} Audio/FlacFile.cpp)
     set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} FLAC::FLAC++)
else()
     message("FLAC not found, not build with FLAC")
endif()

find_package(assimp)

if (assimp_FOUND)
     message("Found assimp at ${assimp_DIR}")
     set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} assimp::assimp)
else()
     message("assimp not found, not build with assimp")
endif()

if(VLD_MEM_CHECK)
    find_package(VLD)
    if(${VLD_FOUND})
        message("found vld at ${VLD_ROOT_DIR}")
        set(OJOIE_PRIVATE_INCLUDE_DIR ${OJOIE_PRIVATE_INCLUDE_DIR} ${VLD_INCLUDE_DIRS})
        set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} ${VLD_LIBRARIES})
    endif()
endif()


set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} glfw glad imgui FreeType)



add_an_library(
        ojoie SHARED
        ${OJOIE_SRCS}
)

add_glsl_src(
        ojoie
        ${GLSL_SRCS}
)

target_compile_definitions(ojoie PRIVATE AN_BUILD_OJOIE)

target_include_directories(
        ojoie
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${OJOIE_PRIVATE_INCLUDE_DIR}
)

target_link_libraries(ojoie PRIVATE ${OJOIE_PRIVATE_LINK_LIBS})

add_custom_command(
        TARGET ojoie POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different $<TARGET_FILE:FreeType> $<TARGET_FILE_DIR:ojoie>
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different $<TARGET_FILE:assimp::assimp> $<TARGET_FILE_DIR:ojoie>
)


install_an_library(ojoie)

install(
        DIRECTORY ${CMAKE_SOURCE_DIR}/ojoie
        COMPONENT ojoie
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/ojoie
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
     )

# ojoie editor runtime
add_an_library(
        ojoieEditorRuntime SHARED
        ${OJOIE_SRCS}
)

target_compile_definitions(ojoieEditorRuntime PRIVATE AN_BUILD_OJOIE OJOIE_WITH_EDITOR)

target_include_directories(
        ojoieEditorRuntime
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${OJOIE_PRIVATE_INCLUDE_DIR}
)

target_link_libraries(ojoieEditorRuntime PRIVATE ${OJOIE_PRIVATE_LINK_LIBS})

add_dependencies(ojoieEditorRuntime ojoie)