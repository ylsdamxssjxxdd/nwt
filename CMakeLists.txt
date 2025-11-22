cmake_minimum_required(VERSION 3.16)

project(nwt LANGUAGES CXX)

# Place all runtime outputs under <build>/bin for all configs (parity with eva).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(ProjectOptions)  # build options, static flags, packaging toggles
include(QtSetup)         # Qt discovery and AUTOMOC/AUTORCC/AUTOUIC
include(EnvInfo)         # eva_OUTPUT_NAME, NWT_ENVIRONMENT, etc.

enable_testing()

set(SOURCES
    src/main.cpp
    src/core/DiscoveryService.cpp
    src/core/PeerDirectory.cpp
    src/core/MessageRouter.cpp
    src/core/ShareManager.cpp
    src/core/ChatController.cpp
    src/core/LanguageManager.cpp
    src/core/SettingsTypes.h
    src/core/StorageManager.cpp
    src/ui/MainWindow.cpp
    src/ui/SettingsDialog.cpp
    src/ui/ShareCenterDialog.cpp
    src/ui/ProfileDialog.cpp
    src/ui/StyleHelper.cpp
    src/ui/EmojiImageHandler.cpp
    src/ui/PeerListModel.cpp
    src/ui/RecentChatListModel.cpp
    src/ui/ContactsSidebar.cpp
    src/ui/EmotionPicker.cpp
    src/ui/ChatPanel.cpp
    src/ui/AvatarHelper.cpp
    src/ui/ui_resources.qrc
)

add_executable(
    ${NWT_TARGET}
    ${BODY_PACK_EXE}
    ${SOURCES}
)

if (UNIX)
    # Linux: keep binary name as plain "nwt" for runtime/AppDir consistency.
    set_target_properties(${NWT_TARGET} PROPERTIES OUTPUT_NAME "nwt")
else()
    # Other platforms: include version/arch in the binary name.
    set_target_properties(${NWT_TARGET} PROPERTIES OUTPUT_NAME "${eva_OUTPUT_NAME}")
endif()

target_include_directories(${NWT_TARGET} PRIVATE src src/core src/ui)

target_link_libraries(${NWT_TARGET} PRIVATE
    ${extra_LIBS}
    Qt5::Core Qt5::Widgets Qt5::Network Qt5::Sql
)

target_compile_features(${NWT_TARGET} PRIVATE cxx_std_17)

# Apply MinGW-specific compile/link options collected in ProjectOptions.cmake.
if (MINGW)
    if (DEFINED NWT_COMPILE_OPTIONS)
        target_compile_options(${NWT_TARGET} PRIVATE ${NWT_COMPILE_OPTIONS})
        message(STATUS "NWT_COMPILE_OPTIONS: ${NWT_COMPILE_OPTIONS}")
    endif()
    if (DEFINED NWT_LINK_OPTIONS)
        target_link_options(${NWT_TARGET} PRIVATE ${NWT_LINK_OPTIONS})
        message(STATUS "NWT_LINK_OPTIONS: ${NWT_LINK_OPTIONS}")
    endif()
endif()

include(PostBuild)

# Optional coverage aggregation target (gcovr), mirroring eva.
if (NWT_ENABLE_COVERAGE)
    find_program(GCOVR_EXECUTABLE gcovr)
    if (GCOVR_EXECUTABLE)
        set(NWT_COVERAGE_DIR ${CMAKE_BINARY_DIR}/reports/coverage)
        set(NWT_COVERAGE_HTML ${NWT_COVERAGE_DIR}/index.html)
        set(NWT_COVERAGE_XML ${NWT_COVERAGE_DIR}/coverage.xml)
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E make_directory ${NWT_COVERAGE_DIR}
            COMMAND ${GCOVR_EXECUTABLE}
                    --root ${CMAKE_SOURCE_DIR}
                    --object-directory ${CMAKE_BINARY_DIR}
                    --print-summary
                    --html-details ${NWT_COVERAGE_HTML}
                    --xml ${NWT_COVERAGE_XML}
                    --filter ${CMAKE_SOURCE_DIR}/src
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating gcovr coverage report (reports/coverage/index.html + coverage.xml)")
    else()
        message(WARNING "gcovr executable not found; 'coverage' target will not be available.")
    endif()
endif()
