

add_library(glfw INTERFACE)

if (WIN32)

    set(GLFW_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/win32/glfw/include)

    set(GLFW_STATIC_LIB ${CMAKE_SOURCE_DIR}/thirdparty/win32/glfw/lib-vc2022/glfw3.lib)

endif()

target_include_directories(glfw INTERFACE ${GLFW_INCLUDE_DIR})

target_link_libraries(glfw INTERFACE ${GLFW_STATIC_LIB})

