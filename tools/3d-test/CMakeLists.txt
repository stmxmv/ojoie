add_an_tool(3d-test WIN32 main.cpp)

target_link_libraries(3d-test PRIVATE ojoie)

if(VLD_MEM_CHECK)
    find_package(VLD)
    if(${VLD_FOUND})
        message("found vld at ${VLD_ROOT_DIR}")
        target_include_directories(3d-test PRIVATE ${VLD_INCLUDE_DIRS})
        target_link_libraries(3d-test PRIVATE ${VLD_LIBRARIES})
    endif()
endif()
