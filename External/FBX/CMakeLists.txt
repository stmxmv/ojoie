add_library(fbx INTERFACE)

target_include_directories(fbx INTERFACE 2020.2.1/include)

if (WIN32)
    set(FBX_LIB_DEBUG_PREFIX "${CMAKE_CURRENT_LIST_DIR}/2020.2.1/lib/vs2019/x64/debug")
    set(
            FBX_LIBS_DEBUG
            ${FBX_LIB_DEBUG_PREFIX}/libfbxsdk-md.lib
            ${FBX_LIB_DEBUG_PREFIX}/libxml2-md.lib
            ${FBX_LIB_DEBUG_PREFIX}/zlib-md.lib
    )

    set(FBX_LIB_PREFIX "${CMAKE_CURRENT_LIST_DIR}/2020.2.1/lib/vs2019/x64/release")
    set(
            FBX_LIBS
            ${FBX_LIB_PREFIX}/libfbxsdk-md.lib
    )
    target_link_libraries(
            fbx INTERFACE
            $<$<CONFIG:Debug>:${FBX_LIBS_DEBUG}>
            $<$<CONFIG:Release>:${FBX_LIBS}>
            $<$<CONFIG:RelWithDebInfo>:${FBX_LIBS}>
    )
endif()