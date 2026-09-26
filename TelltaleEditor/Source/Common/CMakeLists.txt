set(TARGET_NAME Common)

file(GLOB_RECURSE COMMON_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp) # Recursively get all headers
file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp) # Recursively get all sources
file(GLOB_RECURSE COMMON_INLINES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.inl) # Recursively get all inlines
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${COMMON_HEADERS} ${COMMON_SOURCES} ${COMMON_INLINES})

add_library(${TARGET_NAME} STATIC ${COMMON_HEADERS} ${COMMON_SOURCES} ${COMMON_INLINES})

target_link_libraries(${TARGET_NAME} PUBLIC
    ToolLibrary
    SDL3::SDL3-static
)

# Include D3D libraries
if(WIN32)
    target_link_libraries(${TARGET_NAME} PRIVATE d3d12.lib d3dcompiler.lib)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
    if(BUILD_TYPE_UPPER STREQUAL "DEBUG")
        target_link_libraries(${TARGET_NAME} PRIVATE "${THIRD_PARTY_DIR}/WinPIXEventRuntime/Bin/WinPixEventRuntime.lib")
    endif()
    target_include_directories(${TARGET_NAME} PUBLIC "${THIRD_PARTY_DIR}/WinPIXEventRuntime/Include")
elseif(UNIX AND NOT APPLE)
    # TODO GLSLANG
elseif(APPLE)
    # TODO METAL COMP
    find_library(METAL_FRAMEWORK Metal)
    find_library(METALKIT_FRAMEWORK MetalKit)
    find_library(FOUNDATION_FRAMEWORK Foundation)

    if(NOT METAL_FRAMEWORK)
        message(FATAL_ERROR "Cannot build Telltale Editor: Metal framework not found. Please ensure the Metal SDK is installed, and make sure MetalKit is as well!")
    endif()

    if(NOT METALKIT_FRAMEWORK)
        message(FATAL_ERROR "Cannot build Telltale Editor: MetalKit framework not found! Please ensure this SDK is installed!")
    endif()

    target_link_libraries(${TARGET_NAME} PRIVATE
        ${METAL_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
    )
endif()

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "TelltaleEditor")

if(WIN32)
    add_copy_cmd_debug("WinPixEventRuntime.dll" "${THIRD_PARTY_DIR}/WinPIXEventRuntime/Bin" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
endif()

# Enable platform specific defines
set_build_type(${TARGET_NAME})
set_platform_type(${TARGET_NAME})

ignore_warnings(${TARGET_NAME} 1)
