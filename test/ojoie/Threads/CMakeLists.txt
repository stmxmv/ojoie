include(GoogleTest)
add_an_test(task_test task_test.cpp)
target_link_libraries(task_test PRIVATE ojoie)

if (VLD_MEM_CHECK AND VLD_FOUND)
    target_include_directories(task_test PRIVATE ${VLD_INCLUDE_DIRS})
    target_link_libraries(task_test PRIVATE ${VLD_LIBRARIES})
endif ()