add_subdirectory(lib)
add_subdirectory(yaml)
add_subdirectory(SPIRV-Tools)
add_subdirectory(Mono)
add_subdirectory(imgui)
add_subdirectory(ImGuizmo)
add_subdirectory(PhysX)

if (WIN32)
    add_subdirectory(detours)
    add_subdirectory(FBX)
endif ()