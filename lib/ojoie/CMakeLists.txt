

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
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_SPV} $<TARGET_FILE_DIR:${target}>
                       COMMENT "Compile Vulkan shader source ${SHADER_SRC_NAME} ..."
               )
               list(APPEND SHADER_SPVS ${SHADER_SPV})
          endforeach()

         target_sources(${target} PRIVATE ${SHADER_SPVS})

     else()
          message( FATAL_ERROR "vulkan glslc not found!" )
     endif()

endmacro()

add_an_library(ojoie SHARED
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
        Render/RenderPipeline.cpp
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

        UI/ImguiNode.cpp
        UI/imgui_impl_glfw.cpp


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
    target_sources(
            ojoie PRIVATE
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
    )
    target_link_libraries(ojoie PRIVATE volk)
endif()

set(SHADER_DIR Node/shaders/vulkan)

add_glsl_src(
        ojoie
        ${SHADER_DIR}/MeshNode.vert
        ${SHADER_DIR}/MeshNode.frag
        ${SHADER_DIR}/MeshNodeTextured.frag
        ${SHADER_DIR}/text.vert
        ${SHADER_DIR}/text.frag
)


target_compile_definitions(ojoie PRIVATE AN_BUILD_OJOIE)

target_include_directories(ojoie
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/ojoie>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include/ojoie>
        )

find_package(FLAC)

if (FLAC_FOUND)
     message("Found FLAC at ${FLAC_DIR}")
     target_sources(ojoie PRIVATE Audio/FlacFile.cpp)
     target_link_libraries(ojoie PRIVATE FLAC::FLAC++)
else()
     message("FLAC not found, not build with FLAC")
endif()

find_package(assimp)

if (assimp_FOUND)
     message("Found assimp at ${assimp_DIR}")
     target_link_libraries(ojoie PRIVATE assimp::assimp)
else()
     message("assimp not found, not build with assimp")
endif()


#if (NOT APPLE)
#     find_package(Vulkan REQUIRED FATAL_ERROR)
#     if (Vulkan_FOUND)
#          message("Found Vulkan at ${Vulkan_LIBRARY} version ${Vulkan_VERSION}")
#          target_link_libraries(ojoie PRIVATE Vulkan::Vulkan)
#     else()
#          message(FATAL_ERROR "Vulkan not found")
#     endif()
#
#endif()

target_link_libraries(ojoie PRIVATE glfw glad imgui FreeType)


#find_program(GLSLANG_VALIDATOR glslangValidator)
#
#if (GLSLANG_VALIDATOR)
#     message("glslangValidator found at ${GLSLANG_VALIDATOR}")
#else()
#     message(FATAL_ERROR "glslangValidator not found")
#endif()




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
