add_subdirectory(imgui)
add_subdirectory(glad)
add_subdirectory(glfw)
add_subdirectory(FreeType)
add_subdirectory(fmod)

if (OJOIE_USE_VULKAN)
    add_subdirectory(volk)
    add_subdirectory(SpvReflect)
endif()