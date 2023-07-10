include(GoogleTest)
add_an_test(shader_compile_test shader_compile_test.cpp)
target_link_libraries(shader_compile_test PRIVATE ojoie)

add_an_test(shader_test shader_test.cpp)
target_link_libraries(shader_test PRIVATE ojoie)