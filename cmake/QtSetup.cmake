# QtSetup.cmake - Qt discovery and automoc/rcc/uic for nwt

include_guard(GLOBAL)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(_NWT_QT_COMPONENTS Core Widgets Network Sql)
if (NWT_ENABLE_QT_TTS)
    list(APPEND _NWT_QT_COMPONENTS TextToSpeech)
endif()

find_package(Qt5 5.15 REQUIRED COMPONENTS ${_NWT_QT_COMPONENTS})

# Derive Qt bin and lib directories from Qt5_DIR for packaging tools.
if (DEFINED Qt5_DIR)
    get_filename_component(Qt5_BIN_DIR "${Qt5_DIR}/../../../bin" ABSOLUTE)
    get_filename_component(Qt5_LIB_DIR "${Qt5_DIR}/../.." ABSOLUTE)
else()
    # Fallback: try qmake on PATH to locate the Qt bin dir.
    find_program(_qmake_executable qmake HINTS ENV QTDIR PATH_SUFFIXES bin)
    if (_qmake_executable)
        get_filename_component(Qt5_BIN_DIR "${_qmake_executable}" DIRECTORY)
    endif()
    unset(_qmake_executable CACHE)
endif()
