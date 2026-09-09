set(TARGET_NAME EditorQt)

file(GLOB_RECURSE EDITORQT_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp) # Recursively get all headers
file(GLOB_RECURSE EDITORQT_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp) # Recursively get all sources
file(GLOB_RECURSE EDITORQT_INLINES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.inl) # Recursively get all inlines
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITORQT_HEADERS} ${EDITORQT_SOURCES} ${EDITORQT_INLINES})

add_executable(${TARGET_NAME} ${EDITORQT_HEADERS} ${EDITORQT_SOURCES} ${EDITORQT_INLINES})

find_package(Qt6 QUIET COMPONENTS Core Gui Widgets)

if(Qt6_FOUND)
    message(STATUS "Found Qt6 - using installation")
else()
    message(FATAL_ERROR "No Qt6 found on your system. Please see the README and install it first or build from source and pass it to CMake")
endif()

qt_standard_project_setup()

target_link_libraries(${TARGET_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Common
    nfd
)

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "TelltaleEditorQt")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "TelltaleEditor")
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TELLTALE_EDITOR_ROOT_DIR}/Bin/Debug")
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TELLTALE_EDITOR_ROOT_DIR}/Bin/Release")

# Windows debugger directory
if(WIN32)
set_target_properties(${TARGET_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${TELLTALE_EDITOR_ROOT_DIR}/Bin/Debug")
endif()
# Xcode debugger dir
if(APPLE)
set_target_properties(${TARGET_NAME} PROPERTIES XCODE_SCHEME_WORKING_DIRECTORY "${TELLTALE_EDITOR_ROOT_DIR}/Bin/Debug")
endif()

# Enable platform specific defines
set_build_type(${TARGET_NAME})
set_platform_type(${TARGET_NAME})

ignore_warnings(${TARGET_NAME} 1)

# Qt

if (WIN32)
    find_program(QT_DEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${Qt6_DIR}/../../../bin"
    )

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${QT_DEPLOYQT_EXECUTABLE}"
                "$<TARGET_FILE:${TARGET_NAME}>"
                --no-translations --release
        COMMENT "Deploying Qt dependencies..."
    )

elseif (APPLE)
    find_program(QT_DEPLOYQT_EXECUTABLE
        NAMES macdeployqt
        HINTS "${Qt6_DIR}/../../../bin"
    )

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${QT_DEPLOYQT_EXECUTABLE}"
                "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>"
                --release
        COMMENT "Deploying Qt dependencies..."
    )
else()
    message(WARNING "Unknown platform / linux. You will need to ensure you place the shared libraries in the executable directory.")
endif()
