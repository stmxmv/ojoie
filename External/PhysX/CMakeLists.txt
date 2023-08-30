add_library(PhysX SHARED IMPORTED GLOBAL)

if (WIN32)

    message("PhysX use md version")
    set(PhysX_BIN_DIR ${CMAKE_CURRENT_LIST_DIR}/bin/win.x86_64.vc142.md)

    set(PhysX_COMPONENTS Common Cooking Foundation)

    foreach (component ${PhysX_COMPONENTS})
        add_library(PhysX::${component} SHARED IMPORTED GLOBAL)
        set_property(TARGET PhysX::${component} APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
        set_property(TARGET PhysX::${component} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(
                PhysX::${component} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/include"
                IMPORTED_IMPLIB_DEBUG "${PhysX_BIN_DIR}/checked/PhysX${component}_64.lib"
                IMPORTED_LOCATION_DEBUG "${PhysX_BIN_DIR}/checked/PhysX${component}_64.dll"
                IMPORTED_IMPLIB_RELEASE "${PhysX_BIN_DIR}/release/PhysX${component}_64.lib"
                IMPORTED_LOCATION_RELEASE "${PhysX_BIN_DIR}/release/PhysX${component}_64.dll"
        )
        list(APPEND PhysX_COMPONENT_TARGETS PhysX::${component})
    endforeach()

    set(PhysX_STATIC_COMPONENTS Extensions Task PvdSDK CharacterKinematic)
    foreach (component ${PhysX_STATIC_COMPONENTS})
        add_library(PhysX::${component} STATIC IMPORTED GLOBAL)
        set_property(TARGET PhysX::${component} APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
        set_property(TARGET PhysX::${component} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(
                PhysX::${component} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/include"
                IMPORTED_LOCATION_DEBUG "${PhysX_BIN_DIR}/debug/PhysX${component}_static_64.lib"
                IMPORTED_LOCATION_RELEASE "${PhysX_BIN_DIR}/release/PhysX${component}_static_64.lib"
        )
        list(APPEND PhysX_COMPONENT_TARGETS PhysX::${component})
    endforeach()


    add_library(PVDRuntime SHARED IMPORTED GLOBAL)
    set_property(TARGET PVDRuntime APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_property(TARGET PVDRuntime APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(
            PVDRuntime PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_LIST_DIR}/include;${CMAKE_CURRENT_LIST_DIR}/pvdruntime/include"
            IMPORTED_IMPLIB_DEBUG "${PhysX_BIN_DIR}/checked/PVDRuntime_64.lib"
            IMPORTED_LOCATION_DEBUG "${PhysX_BIN_DIR}/checked/PVDRuntime_64.dll"
            IMPORTED_IMPLIB_RELEASE "${PhysX_BIN_DIR}/release/PVDRuntime_64.lib"
            IMPORTED_LOCATION_RELEASE "${PhysX_BIN_DIR}/release/PVDRuntime_64.dll"
    )
    list(APPEND PhysX_COMPONENT_TARGETS PVDRuntime)

    set_property(TARGET PhysX APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_property(TARGET PhysX APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(
            PhysX PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/include"
            IMPORTED_IMPLIB_DEBUG "${PhysX_BIN_DIR}/checked/PhysX_64.lib"
            IMPORTED_LOCATION_DEBUG "${PhysX_BIN_DIR}/checked/PhysX_64.dll"
            IMPORTED_IMPLIB_RELEASE "${PhysX_BIN_DIR}/release/PhysX_64.lib"
            IMPORTED_LOCATION_RELEASE "${PhysX_BIN_DIR}/release/PhysX_64.dll"
            INTERFACE_LINK_LIBRARIES "${PhysX_COMPONENT_TARGETS}"
    )

elseif (APPLE)
    message(FATAL_ERROR "PhysX dose not have apple compiled binary")
    #    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    #        set_target_properties(
    #                PhysX PROPERTIES
    #                INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/thirdparty/macos/glfw/include"
    #                INTERFACE_LINK_LIBRARIES "-framework Cocoa -framework IOKit"
    #                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/thirdparty/macos/lib-universal/libglfw3.a"
    #        )
    #    endif()

endif ()

