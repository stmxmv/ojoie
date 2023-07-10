add_an_tool(fmoddTest main.cpp)

target_link_libraries(fmoddTest PRIVATE ojoie fmod)

if (TARGET fmoddTest)

#    add_custom_command(
#            TARGET fmoddTest POST_BUILD
#            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
#            "$<TARGET_FILE:fmod>" "$<TARGET_FILE_DIR:fmoddTest>"
#            COMMENT "Copying fmod.dll for fmoddTest"
#    )

endif()