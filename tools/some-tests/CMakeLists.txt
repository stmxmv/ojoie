add_an_tool(fbx_test fbx_test.cpp)

target_link_libraries(fbx_test PRIVATE ojoie)

add_an_tool(yaml_test yaml_test.cpp)
target_link_libraries(yaml_test PRIVATE ojoie)

add_an_tool(mono_test mono_test.cpp)

target_link_libraries(mono_test PRIVATE ojoie MonoLoader)

add_an_tool(mono_func_check mono_func_check.cpp)

target_link_libraries(mono_func_check PRIVATE ojoie MonoLoader)

if(VLD_MEM_CHECK AND VLD_FOUND)
    target_include_directories(fbx_test PRIVATE ${VLD_INCLUDE_DIRS})
    target_link_libraries(fbx_test PRIVATE ${VLD_LIBRARIES})

    target_include_directories(yaml_test PRIVATE ${VLD_INCLUDE_DIRS})
    target_link_libraries(yaml_test PRIVATE ${VLD_LIBRARIES})

    target_include_directories(mono_test PRIVATE ${VLD_INCLUDE_DIRS})
    target_link_libraries(mono_test PRIVATE ${VLD_LIBRARIES})
endif()
