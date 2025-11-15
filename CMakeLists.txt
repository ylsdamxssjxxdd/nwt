cmake_minimum_required(VERSION 3.16)

project(nwt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets Network)

set(SOURCES
    src/main.cpp
    src/core/DiscoveryService.cpp
    src/core/PeerDirectory.cpp
    src/core/MessageRouter.cpp
    src/core/ChatController.cpp
    src/ui/MainWindow.cpp
    src/ui/SettingsDialog.cpp
    src/ui/RoleSelectionDialog.cpp
    src/ui/ShareCenterDialog.cpp
)

add_executable(nwt ${SOURCES})

target_include_directories(nwt PRIVATE src src/core src/ui)

target_link_libraries(nwt PRIVATE Qt5::Core Qt5::Widgets Qt5::Network)
