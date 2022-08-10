

add_an_library(ojoie SHARED
        Core/App.cpp
        Core/Window.cpp
        Core/Game.cpp
        Core/Node.cpp
        Core/DispatchQueue.cpp
        Core/Log.cpp
        Core/Exception.cpp
        Core/Event.cpp

        Render/RenderQueue.cpp
        Render/Renderer.cpp
        Render/RenderPipeline.cpp
        Render/Mesh.cpp
        Render/Model.cpp
        Render/Font.cpp

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


        Utility/STBTextureLoader.cpp
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

target_link_libraries(ojoie PRIVATE glfw glad imgui FreeType)


find_program(GLSLANG_VALIDATOR glslangValidator)

if (GLSLANG_VALIDATOR)
     message("glslangValidator found at ${GLSLANG_VALIDATOR}")
else()
     message(FATAL_ERROR "glslangValidator not found")
endif()




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
