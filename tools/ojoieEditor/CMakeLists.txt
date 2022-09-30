add_an_tool(ojoieEditor WIN32 main.cpp)

target_link_libraries(ojoieEditor PRIVATE ojoieEditorRuntime)

if(VLD_MEM_CHECK)
    find_package(VLD)
    if(${VLD_FOUND})
        message("found vld at ${VLD_ROOT_DIR}")
        target_include_directories(ojoieEditor PRIVATE ${VLD_INCLUDE_DIRS})
        target_link_libraries(ojoieEditor PRIVATE ${VLD_LIBRARIES})
    endif()
endif()
