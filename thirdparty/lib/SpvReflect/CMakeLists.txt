add_library(SpvReflect OBJECT spirv_reflect.c)

target_include_directories(SpvReflect PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty/include/SpvReflect)

target_link_libraries(SpvReflect PRIVATE volk)