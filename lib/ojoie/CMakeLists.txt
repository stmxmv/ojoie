

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

        UI/ImguiNode.cpp


        Audio/Sound.cpp
        Audio/WavFile.cpp
        Audio/Mp3File.cpp
        )


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
     target_sources(ojoie PRIVATE Audio/FlacFile.cpp)
     target_link_libraries(ojoie PRIVATE FLAC::FLAC++)
endif()

target_link_libraries(ojoie PRIVATE glfw glad imgui FLAC::FLAC++)

install_an_library(ojoie)

install(
        DIRECTORY ${CMAKE_SOURCE_DIR}/ojoie
        COMPONENT ojoie
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/ojoie
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
     )
