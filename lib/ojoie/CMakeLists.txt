

macro(add_glsl_src target)

     find_package(Vulkan REQUIRED)

     if (NOT Vulkan_FOUND)
         message(FATAL_ERROR "Vulkan SDK is not found!")
     endif()

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

        Allocator/MemoryManager.cpp

        Object/Class.cpp
        Object/Object.cpp
        Object/NamedObject.cpp

        Core/App.cpp
        Core/Window.cpp
        Core/Game.cpp
        Core/Exception.cpp
        Core/Configuration.cpp
        Core/Actor.cpp
        Core/Component.cpp
        Core/Behavior.cpp
        Core/Name.cpp
        Core/Screen.cpp
        Core/Event.cpp
        Core/Menu.cpp
        Core/UUID.cpp

        App/Views/View.cpp
        App/Views/ImageVIew.cpp

        Modules/Dylib.cpp

        Asset/FBXImporter.cpp

        Serialize/Coder/YamlEncoder.cpp
        Serialize/Coder/YamlDecoder.cpp
        Serialize/Coder/YAMLNode.cpp
        Serialize/FloatStringConversion.cpp

        Components/TransformComponent.cpp

        Threads/Event.cpp
        Threads/DispatchQueue.cpp
        Threads/Threads.cpp

        HAL/File.cpp

        Misc/ResourceManager.cpp


        Geometry/Cube.cpp
        Geometry/Sphere.cpp
        Geometry/Plane.cpp

        Utility/Log.cpp
        Utility/Assert.cpp
        Utility/SourceFile.cpp
        Utility/String.cpp
        Utility/Path.cpp

        ShaderLab/Token.cpp
        ShaderLab/Lexer.cpp
        ShaderLab/Parser.cpp

        Render/RenderQueue.cpp
        Render/Renderer.cpp
        Render/RenderPipelineState.cpp
        # Render/Font.cpp
        Render/VertexBuffer.cpp
        Render/Buffer.cpp
        Render/Texture.cpp
        Render/TextureLoader.cpp
        Render/CommandBuffer.cpp
        Render/VertexData.cpp
        Render/ShaderFunction.cpp
        Render/PipelineReflection.cpp
        Render/CommandPool.cpp
        Render/RenderContext.cpp
        Render/Texture2D.cpp
        Render/RenderTarget.cpp
        Render/RenderPass.cpp
        Render/Material.cpp
        Render/Mesh/Mesh.cpp
        Render/Mesh/MeshRenderer.cpp
        Render/UniformBuffers.cpp

        Render/RenderManager.cpp
        Render/RenderLoop/ForwardRenderLoop.cpp

        Render/Shader/ShaderCompiler.cpp
        Render/Shader/Shader.cpp
        Render/QualitySettings.cpp
        Render/TextureManager.cpp
        Render/Layer.cpp
        Render/TextureCube.cpp
        Render/Image.cpp
        Render/LayerManager.cpp
        Render/Light.cpp

        Camera/Camera.cpp

       # UI/ImguiNode.cpp
       # UI/Imgui.cpp


        Audio/Sound.cpp
        Audio/WavFile.cpp
        Audio/Mp3File.cpp


        Input/InputManager.cpp
        Input/InputAction.cpp
        Input/InputBinding.cpp
        Input/InputControl.cpp
        Input/InputComponent.cpp

        IMGUI/IMGUIManager.cpp
        IMGUI/IMGUI.cpp
        IMGUI/ComboFilter.cpp
)

if (WIN32)
    set(
            OJOIE_SRCS
            ${OJOIE_SRCS}
            Core/win32/Application.cpp
            Core/win32/Window.cpp
            Core/win32/Screen.cpp
            Core/win32/DragAndDrop.cpp
            Core/win32/DragAndDropImpl.cpp
            Core/win32/Menu.cpp
            Core/win32/UAHMenuBar.cpp
            Core/win32/DarkMode.cpp
            Core/win32/Resources/resources.rc

            App/Views/win32/View.cpp
            App/Views/win32/ImageVIew.cpp

            Input/win32/InputManager.cpp

            Render/D3D11/Device.cpp
            Render/D3D11/Layer.cpp
            Render/D3D11/RenderTarget.cpp
            Render/D3D11/RenderTypes.cpp
            Render/D3D11/TextureManager.cpp
            Render/D3D11/VertexBuffer.cpp
            Render/D3D11/UniformBuffers.cpp
            Render/D3D11/CommandBuffer.cpp
            Render/D3D11/RenderPipelineState.cpp
            Render/D3D11/ShaderFunction.cpp
            Render/D3D11/VertexInputLayouts.cpp
            Render/D3D11/D3D11Hash.cpp
    )

endif()
if (OJOIE_USE_VULKAN)
    set(VULKAN_SRC_DIR Render/vulkan)
    set(
            OJOIE_SRCS
            ${OJOIE_SRCS}
            ${VULKAN_SRC_DIR}/vulkan.cpp
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
            ${VULKAN_SRC_DIR}/PipelineLayout.cpp
            ${VULKAN_SRC_DIR}/RenderPipeline.cpp
            ${VULKAN_SRC_DIR}/RenderTypes.cpp
            ${VULKAN_SRC_DIR}/hash.cpp
            ${VULKAN_SRC_DIR}/ShaderFunction.cpp
            ${VULKAN_SRC_DIR}/RenderResourceCache.cpp
            ${VULKAN_SRC_DIR}/Semaphore.cpp
            ${VULKAN_SRC_DIR}/TextureManager.cpp
            ${VULKAN_SRC_DIR}/RenderTarget.cpp
            ${VULKAN_SRC_DIR}/UniformBuffers.cpp
    )
    set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} volk SpvReflect)

    set(SHADER_DIR Shaders/vulkan)

    set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} dxc)
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



add_an_library(
        ojoie
        ${OJOIE_SRCS}
)

#add_glsl_src(
#        ojoie
#        ${GLSL_SRCS}
#)

target_compile_definitions(ojoie PRIVATE AN_BUILD_OJOIE)

if(VLD_MEM_CHECK AND VLD_FOUND)
    set(OJOIE_PRIVATE_INCLUDE_DIR ${OJOIE_PRIVATE_INCLUDE_DIR} ${VLD_INCLUDE_DIRS})
    set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} ${VLD_LIBRARIES})
endif()


option(OJOIE_USE_ASSIMP "Compile ojoie with assimp" FALSE)
option(OJOIE_USE_FREETYPE "Compile ojoie with FreeType" FALSE)
option(OJOIE_USE_SPIRV "Compile ojoie with SpirvTool" FALSE)
option(OJOIE_USE_FLAC "Compile ojoie with libFLAC" TRUE)

if (OJOIE_USE_SPIRV)
    set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} spirv-tools)
endif ()

if (OJOIE_USE_ASSIMP)
    find_package(assimp)

    if (assimp_FOUND)
        message("Found assimp at ${assimp_DIR}")
        set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} assimp::assimp)

        add_custom_command(
                TARGET ojoie POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different $<TARGET_FILE:assimp::assimp> $<TARGET_FILE_DIR:ojoie>
        )

    else()
        message("assimp not found, not build with assimp")
    endif()
endif ()

if (OJOIE_USE_FREETYPE)
    set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} FreeType)
    add_custom_command(
            TARGET ojoie POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different $<TARGET_FILE:FreeType> $<TARGET_FILE_DIR:ojoie>
    )
endif ()

if (OJOIE_USE_FLAC)
    find_package(FLAC)

    if (FLAC_FOUND)
        message("Found FLAC at ${FLAC_DIR}")
        set(OJOIE_SRCS ${OJOIE_SRCS} Audio/FlacFile.cpp)
        set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} FLAC::FLAC++)
    else()
        message("FLAC not found, not build with FLAC")
    endif()
endif ()

find_package(directx-headers)
find_package(directxmath)
find_package(directxtex CONFIG REQUIRED)
set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} Microsoft::DirectXTex)

find_package(TBB REQUIRED)
set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} TBB::tbb TBB::tbbmalloc)

set(OJOIE_PRIVATE_DEFINITIONS ${OJOIE_PRIVATE_DEFINITIONS} WITH_TBB)
target_compile_definitions(ojoie PRIVATE ${OJOIE_PRIVATE_DEFINITIONS})

set(OJOIE_PRIVATE_LINK_LIBS ${OJOIE_PRIVATE_LINK_LIBS} detours yaml imgui ImGuizmo)

set(
        OJOIE_INTERFACE_INCLUDE_DIR
        ${OJOIE_INTERFACE_INCLUDE_DIR}
        $<TARGET_PROPERTY:yaml,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:imgui,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ImGuizmo,INCLUDE_DIRECTORIES>
)

target_include_directories(
        ojoie
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        INTERFACE
        ${OJOIE_INTERFACE_INCLUDE_DIR}
        PRIVATE
        ${OJOIE_PRIVATE_INCLUDE_DIR}
)

target_link_libraries(ojoie PRIVATE ${OJOIE_PRIVATE_LINK_LIBS})

## By default, symbols provided by the sublibs that are not used by mainlib (which is all of them in this case)
## are not used. This changes that.
## https://stackoverflow.com/questions/11429055/cmake-how-create-a-single-shared-library-from-all-static-libraries-of-subprojec
#if (MSVC)
#    set_target_properties(ojoie PROPERTIES
#            LINK_FLAGS "/WHOLEARCHIVE"
#            )
#elseif (APPLE)
#    set_target_properties(ojoie PROPERTIES
#            LINK_FLAGS "-Wl,-all_load"
#            )
#else ()
#    set_target_properties(ojoie PROPERTIES
#            LINK_FLAGS "-Wl,--whole-archive"
#            )
#endif ()



# we are not installing

#install_an_library(ojoie)
#
#install(
#        DIRECTORY ${CMAKE_SOURCE_DIR}/ojoie
#        COMPONENT ojoie
#        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/ojoie
#        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
#     )

# ojoie editor runtime
add_an_library(
        ojoieEditorRuntime SHARED
        ${OJOIE_SRCS}
        Editor/Selection.cpp
)

target_compile_definitions(ojoieEditorRuntime PRIVATE AN_BUILD_OJOIE ${OJOIE_PRIVATE_DEFINITIONS} PUBLIC OJOIE_WITH_EDITOR)

target_include_directories(
        ojoieEditorRuntime
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        INTERFACE
        ${OJOIE_INTERFACE_INCLUDE_DIR}
        PRIVATE
        ${OJOIE_PRIVATE_INCLUDE_DIR}
)

target_link_libraries(ojoieEditorRuntime PRIVATE ${OJOIE_PRIVATE_LINK_LIBS})

add_dependencies(ojoieEditorRuntime ojoie)