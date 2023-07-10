add_library(Mono INTERFACE)

if (WIN32)
    set(MONO_DIR "${CMAKE_SOURCE_DIR}/thirdparty/Mono/win32")
    set(MONO_LIB "${MONO_DIR}/lib/mono-2.0-sgen.lib")
endif()


target_include_directories(Mono INTERFACE "${MONO_DIR}/include/mono-2.0")
target_link_libraries(Mono INTERFACE ${MONO_LIB})