# EnvInfo.cmake - compute output name and environment string (trimmed from eva)

include_guard(GLOBAL)

# Default target name to the project name if not provided.
if (NOT DEFINED NWT_TARGET)
    set(NWT_TARGET "${PROJECT_NAME}")
endif()

# Default version tag for the binary name (can be overridden by cache/CLI).
if (NOT DEFINED version)
    set(version "dev")
endif()

# Normalize architecture display string.
set(ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
if (ARCHITECTURE MATCHES "x86_64|AMD64|i[3-6]86")
    set(ARCHITECTURE "x86")
elseif (ARCHITECTURE MATCHES "aarch64|arm")
    set(ARCHITECTURE "arm")
endif()

# Compose versioned output name (kept as eva_OUTPUT_NAME for parity with eva).
set(eva_OUTPUT_NAME "${NWT_TARGET}-${version}-${ARCHITECTURE}")

# OS version string.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    execute_process(
        COMMAND lsb_release -d
        OUTPUT_VARIABLE OS_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    execute_process(
        COMMAND sw_vers -productVersion
        OUTPUT_VARIABLE OS_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    execute_process(
        COMMAND cmd.exe /C ver
        OUTPUT_VARIABLE OS_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    string(REGEX REPLACE "\r" "" OS_VERSION "${OS_VERSION}")
    string(STRIP "${OS_VERSION}" OS_VERSION)
else()
    set(OS_VERSION "unknown OS")
endif()

set(PROCESSOR_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
string(CONCAT eva_ENVIRONMENT "${OS_VERSION} CPU: ${PROCESSOR_ARCHITECTURE}")
string(CONCAT eva_ENVIRONMENT "${eva_ENVIRONMENT}, GPU: unknown")

set(NWT_ENVIRONMENT "${eva_ENVIRONMENT}")
set(NWT_VERSION "${eva_OUTPUT_NAME}")

set(COMPILE_VERSION "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if (DEFINED Qt5Core_VERSION)
    set(QT_VERSION_ "${Qt5Core_VERSION}")
elseif(DEFINED Qt5Widgets_VERSION)
    set(QT_VERSION_ "${Qt5Widgets_VERSION}")
endif()

if (NOT DEFINED NWT_PRODUCT_TIME)
    string(TIMESTAMP NWT_PRODUCT_TIME "%Y-%m-%d %H:%M:%S")
    set(NWT_PRODUCT_TIME "${NWT_PRODUCT_TIME}" CACHE STRING "Product build time (frozen across re-configures)")
endif()
