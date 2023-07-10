add_an_tool(ecs-test main.cpp)

target_link_libraries(ecs-test PRIVATE ojoie)

if(VLD_MEM_CHECK)
    target_include_directories(ecs-test PRIVATE ${VLD_INCLUDE_DIRS})
    target_link_libraries(ecs-test PRIVATE ${VLD_LIBRARIES})
endif()