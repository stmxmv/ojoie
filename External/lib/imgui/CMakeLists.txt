
add_library(
        imgui OBJECT
        imgui.cpp
        imgui_demo.cpp
        imgui_draw.cpp
#        imgui_impl_opengl3.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
)

find_package(Vulkan)

if (Vulkan_FOUND)
    target_sources(imgui PRIVATE imgui_impl_vulkan.cpp)
    target_link_libraries(imgui PRIVATE volk)
endif()

#target_link_libraries(imgui PRIVATE glfw)

target_include_directories(imgui PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty/include/imgui)