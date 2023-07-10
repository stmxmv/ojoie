
# need to specify the version, eg. project(anyName VERSION 1.0)

message("install prefix is ${CMAKE_INSTALL_PREFIX}")

# make uninstall
if(NOT TARGET uninstall)
    configure_file(
            "cmake/modules/uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
            IMMEDIATE @ONLY)

    add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake)
endif()


# Version Information
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        cmake/modules/${PROJECT_NAME}-config-version.cmake
        VERSION ${PACKAGE_VERSION}
        COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(cmake/modules/${PROJECT_NAME}-config.cmake.in
        "${CMAKE_BINARY_DIR}/cmake/modules/${PROJECT_NAME}-config.cmake"
        INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}")

install(
        FILES
        "${CMAKE_BINARY_DIR}/cmake/modules/${PROJECT_NAME}-config.cmake"
        "${CMAKE_BINARY_DIR}/cmake/modules/${PROJECT_NAME}-config-version.cmake"
        DESTINATION lib/cmake/${PROJECT_NAME}
)


## install headers, apple will use framework
#if(NOT APPLE)
#    install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/" # source directory
#            DESTINATION "include" # target directory
#            FILES_MATCHING
#            PATTERN "*.h"
#            PATTERN "*.hpp"
#            PATTERN "*.inl"
#            )
#
#endif()