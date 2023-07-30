
set(PROJECT_DEBUG_MACRO AN_DEBUG)

if (WIN32)
    set(CMAKE_DEBUG_POSTFIX d)
    set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb")
else()
    set(CMAKE_DEBUG_POSTFIX )
endif()

function(add_an_tool name)
    cmake_parse_arguments(ARG "WIN32" "" "" ${ARGN})
    if (ARG_WIN32 AND WIN32)
        add_executable(${name} WIN32 ${ARG_UNPARSED_ARGUMENTS})
    else()
        add_executable(${name} ${ARG_UNPARSED_ARGUMENTS})
    endif()

    target_compile_options(${name} PRIVATE ${AN_COMPILE_OPTIONS})

    set_target_properties(${name} PROPERTIES DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}")
    set_target_properties(${name}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            )
    if (APPLE)
        get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
        if (multi_config)
            set_target_properties(${name}
                    PROPERTIES
                    INSTALL_RPATH "@executable_path;@executable_path/../lib;@executable_path/../../lib/$<CONFIG>"
                    )
        else()
            set_target_properties(${name}
                    PROPERTIES
                    INSTALL_RPATH "@executable_path;@executable_path/../lib"
                    )
        endif()

    elseif (WIN32)
        if (ARG_WIN32)
            target_link_options(${name} PRIVATE /ENTRY:mainCRTStartup)
        endif()
    endif()
endfunction(add_an_tool)


function(install_an_tool name)
    if(TARGET ${name})
        install(TARGETS ${name}
                LIBRARY DESTINATION bin
                ARCHIVE DESTINATION bin
                RUNTIME DESTINATION bin
        )
    else()
        add_custom_target(${name} ${ARGN})
    endif()
endfunction(install_an_tool)


# install public headers by setting the target PUBLIC_HEADER property
# you can also keep headers structure like as follows:
#     install(
#        DIRECTORY ${src_header_dir}
#        COMPONENT ${target_name}
#        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/...
#        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
#     )
function(add_an_library name)
#    cmake_parse_arguments(ARG "NOT_INSTALL" "" "" ${ARGN})
    add_library(${name} ${ARGN})
    target_compile_options(${name} PRIVATE ${AN_COMPILE_OPTIONS})
    set_target_properties(${name} PROPERTIES DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}")
    if (WIN32)
        set_target_properties(${name}
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
                )
    else()
        set_target_properties(${name}
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
                )
    endif()
    set_target_properties(${name}
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            )

    if (APPLE)
           set_target_properties(${name}
                    PROPERTIES
                    INSTALL_RPATH "@executable_path;@executable_path/../lib;@loader_path"
           )
    endif()

endfunction(add_an_library)


function(install_an_library name)
    if(TARGET ${name})
        # may link here
        if (WIN32)
            install(TARGETS ${name} EXPORT ${name}-targets
                    COMPONENT ${name}
                    LIBRARY DESTINATION lib/${PROJECT_NAME}
                    ARCHIVE DESTINATION lib/${PROJECT_NAME}
                    RUNTIME DESTINATION bin
                    PUBLIC_HEADER DESTINATION include
            )
        else()
            install(TARGETS ${name} EXPORT ${name}-targets
                    COMPONENT ${name}
                    LIBRARY DESTINATION lib/${PROJECT_NAME}
                    ARCHIVE DESTINATION lib/${PROJECT_NAME}
                    RUNTIME DESTINATION lib/${PROJECT_NAME}
                    PUBLIC_HEADER DESTINATION include/${name}
            )
        endif(WIN32)

        install(EXPORT ${name}-targets
                FILE "${PROJECT_NAME}-${name}-targets.cmake"
                NAMESPACE ${PROJECT_NAME}::
                DESTINATION lib/cmake/${PROJECT_NAME}
                COMPONENT ${component}
        )

    else()
        add_custom_target(${name} ${ARGN})
    endif()
endfunction(install_an_library)

include_directories(include)
add_compile_definitions($<$<CONFIG:Debug>:${PROJECT_DEBUG_MACRO}>)


