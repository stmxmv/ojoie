add_an_tool(test WIN32 main.cpp)

target_link_libraries(test PRIVATE ojoie)

target_link_options(test PRIVATE /ENTRY:mainCRTStartup)