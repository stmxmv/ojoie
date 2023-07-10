include(GoogleTest)
add_an_test(enumerate_test enumerate_test.cpp)
add_an_test(ref_count_test ref_count_test.cpp)
target_link_libraries(ref_count_test PRIVATE ojoie)