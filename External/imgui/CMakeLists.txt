aux_source_directory(lib IMGUI_SRCS)
add_an_library(imgui OBJECT ${IMGUI_SRCS})
target_include_directories(imgui PUBLIC include)
target_compile_definitions(imgui PRIVATE IMGUI_DECLARE_EXPORT INTERFACE IMGUI_DECLARE_STATIC)