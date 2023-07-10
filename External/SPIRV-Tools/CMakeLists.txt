add_library(spirv-tools INTERFACE)

target_include_directories(spirv-tools INTERFACE include)

if (WIN32)
    set(SPIRV_TOOLS_LIB_DEBUG_PREFIX "${CMAKE_CURRENT_LIST_DIR}/win32/debug")
    set(
            SPIRV_TOOLS_LIBS_DEBUG
            ${SPIRV_TOOLS_LIB_DEBUG_PREFIX}/SPIRV-Tools.lib
            ${SPIRV_TOOLS_LIB_DEBUG_PREFIX}/SPIRV-Tools-diff.lib
            ${SPIRV_TOOLS_LIB_DEBUG_PREFIX}/SPIRV-Tools-link.lib
            ${SPIRV_TOOLS_LIB_DEBUG_PREFIX}/SPIRV-Tools-lint.lib
            ${SPIRV_TOOLS_LIB_DEBUG_PREFIX}/SPIRV-Tools-reduce.lib
            ${SPIRV_TOOLS_LIB_DEBUG_PREFIX}/SPIRV-Tools-opt.lib
    )

    set(SPIRV_TOOLS_LIB_PREFIX "${CMAKE_CURRENT_LIST_DIR}/win32/release")
    set(
            SPIRV_TOOLS_LIBS
            ${SPIRV_TOOLS_LIB_PREFIX}/SPIRV-Tools.lib
            ${SPIRV_TOOLS_LIB_PREFIX}/SPIRV-Tools-diff.lib
            ${SPIRV_TOOLS_LIB_PREFIX}/SPIRV-Tools-link.lib
            ${SPIRV_TOOLS_LIB_PREFIX}/SPIRV-Tools-lint.lib
            ${SPIRV_TOOLS_LIB_PREFIX}/SPIRV-Tools-reduce.lib
            ${SPIRV_TOOLS_LIB_PREFIX}/SPIRV-Tools-opt.lib
    )
    target_link_libraries(
            spirv-tools INTERFACE
            $<$<CONFIG:Debug>:${SPIRV_TOOLS_LIBS_DEBUG}>
            $<$<CONFIG:Release>:${SPIRV_TOOLS_LIBS}>
            $<$<CONFIG:RelWithDebInfo>:${SPIRV_TOOLS_LIBS}>
    )
endif()