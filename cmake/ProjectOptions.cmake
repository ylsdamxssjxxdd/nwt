# ProjectOptions.cmake - centralize build options and platform flags for nwt
#
# This file is intentionally very close to eva's ProjectOptions.cmake so that
# both projects share the same CMake switches for static builds and packaging.

include_guard(GLOBAL)

# ---- User-facing options ----

# Whether to create a redistributable package (enables post-build bundling).
option(BODY_PACK "Pack the nwt application (Qt runtime, AppImage, etc.)" OFF)

# Enable gcov/llvm-cov instrumentation for coverage reports.
option(NWT_ENABLE_COVERAGE "Enable gcov/llvm-cov instrumentation for coverage reports" OFF)

if(NWT_ENABLE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "NWT: coverage instrumentation is enabled (compiler: ${CMAKE_CXX_COMPILER_ID}).")
        foreach(_lang C CXX)
            set(CMAKE_${_lang}_FLAGS "${CMAKE_${_lang}_FLAGS} --coverage -O0 -g")
            set(CMAKE_${_lang}_FLAGS_RELEASE "${CMAKE_${_lang}_FLAGS_RELEASE} --coverage -O0 -g")
            set(CMAKE_${_lang}_FLAGS_RELWITHDEBINFO "${CMAKE_${_lang}_FLAGS_RELWITHDEBINFO} --coverage -O0 -g")
            set(CMAKE_${_lang}_FLAGS_DEBUG "${CMAKE_${_lang}_FLAGS_DEBUG} --coverage -O0 -g")
        endforeach()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")
        add_compile_definitions(NWT_COVERAGE_BUILD)
    else()
        message(WARNING "NWT_ENABLE_COVERAGE currently supports GCC/Clang toolchains only. Option ignored.")
        set(NWT_ENABLE_COVERAGE OFF CACHE BOOL "Enable gcov/llvm-cov instrumentation for coverage reports" FORCE)
    endif()
endif()

# ---- Dependency hint helpers ----

if(DEFINED ENV{OPENSSL_PREFIX} AND NOT "$ENV{OPENSSL_PREFIX}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{OPENSSL_PREFIX}" _NWT_OPENSSL_ROOT)
    if(EXISTS "${_NWT_OPENSSL_ROOT}")
        if(NOT DEFINED OPENSSL_ROOT_DIR OR OPENSSL_ROOT_DIR STREQUAL "")
            set(OPENSSL_ROOT_DIR "${_NWT_OPENSSL_ROOT}" CACHE PATH "OpenSSL root derived from OPENSSL_PREFIX")
        endif()
        if(NOT OPENSSL_ROOT_DIR IN_LIST CMAKE_PREFIX_PATH)
            list(APPEND CMAKE_PREFIX_PATH "${OPENSSL_ROOT_DIR}")
        endif()
        if(NOT "${OPENSSL_ROOT_DIR}/include" IN_LIST CMAKE_INCLUDE_PATH AND EXISTS "${OPENSSL_ROOT_DIR}/include")
            list(APPEND CMAKE_INCLUDE_PATH "${OPENSSL_ROOT_DIR}/include")
        endif()
        if(EXISTS "${OPENSSL_ROOT_DIR}/lib64" AND NOT "${OPENSSL_ROOT_DIR}/lib64" IN_LIST CMAKE_LIBRARY_PATH)
            list(APPEND CMAKE_LIBRARY_PATH "${OPENSSL_ROOT_DIR}/lib64")
        endif()
        if(EXISTS "${OPENSSL_ROOT_DIR}/lib" AND NOT "${OPENSSL_ROOT_DIR}/lib" IN_LIST CMAKE_LIBRARY_PATH)
            list(APPEND CMAKE_LIBRARY_PATH "${OPENSSL_ROOT_DIR}/lib")
        endif()
        message(STATUS "NWT: Using OpenSSL from ${OPENSSL_ROOT_DIR} (OPENSSL_PREFIX)")
    else()
        message(WARNING "NWT: OPENSSL_PREFIX=${_NWT_OPENSSL_ROOT}, but the path does not exist. Ignoring.")
    endif()
endif()
unset(_NWT_OPENSSL_ROOT)

# ---- Static build options ----

# MinGW static runtime option.
# Default: Windows ON, others OFF. This aims to statically link libgcc/libstdc++/winpthread
# while keeping Qt dynamic unless you provide static Qt yourself.
if (WIN32)
    set(DEFAULT_NWT_STATIC ON)
else()
    set(DEFAULT_NWT_STATIC OFF)
endif()
option(NWT_STATIC  "MinGW: static libgcc/libstdc++/winpthread (Qt stays dynamic)" ${DEFAULT_NWT_STATIC})

# MinGW: whether to copy runtime DLLs after build. Default OFF when NWT_STATIC=ON.
if (WIN32 AND MINGW)
    if (NWT_STATIC)
        set(DEFAULT_BODY_COPY_MINGW_RUNTIME OFF)
    else()
        set(DEFAULT_BODY_COPY_MINGW_RUNTIME ON)
    endif()
    option(BODY_COPY_MINGW_RUNTIME "Copy MinGW runtime DLLs after build (libgcc/libstdc++/winpthread)" ${DEFAULT_BODY_COPY_MINGW_RUNTIME})
endif()

# Linux static Qt option.
if (UNIX AND NOT APPLE)
    option(NWT_LINUX_STATIC "Link against a static Qt build on Linux (skips AppImage packaging)" OFF)
    if (NWT_LINUX_STATIC)
        if(NOT DEFINED OPENSSL_USE_STATIC_LIBS)
            set(OPENSSL_USE_STATIC_LIBS ON CACHE BOOL "Prefer static OpenSSL libs when NWT_LINUX_STATIC=ON")
        endif()
        add_compile_definitions(NWT_LINUX_STATIC_BUILD)
    endif()
    set(NWT_FCITX_PLUGIN_PATH "" CACHE FILEPATH "Path to libfcitxplatforminputcontextplugin.a for Linux static builds")
    set(NWT_FCITX_PLUGIN_LIBS "" CACHE STRING "Extra libraries required by fcitx platform plugin when linked statically")
    option(NWT_LINUX_STATIC_SKIP_FLITE "Disable linking the Flite text-to-speech plugin when building statically on Linux" OFF)
    if (NWT_LINUX_STATIC_SKIP_FLITE)
        add_compile_definitions(NWT_SKIP_FLITE_PLUGIN)
    endif()
    set(NWT_TTS_FLITE_PLUGIN_PATH "" CACHE FILEPATH "Path to the static QTextToSpeech flite plugin library")
    set(NWT_TTS_FLITE_PLUGIN_LIBS "" CACHE STRING "Extra libraries required by the flite text-to-speech plugin")
    option(NWT_APPIMAGE_BUNDLE_COMMON_LIBS "Auto-detect and bundle common desktop runtime libraries into the AppImage" ON)
    set(NWT_APPIMAGE_COMMON_LIB_NAMES
        "X11;X11-xcb;Xau;Xdmcp;xcb;SM;ICE;EGL;GL;GLX;GLdispatch;wayland-client;wayland-egl;asound;pulse;pulse-mainloop-glib;uuid;z;bsd;gmp;gpg-error;Xtst;Xext;Xrender;Xi;Xcursor;Xfixes;xkbcommon-x11;xcb-image;xcb-shm;xcb-icccm;xcb-keysyms;xcb-randr;xcb-render;xcb-render-util;xcb-shape;xcb-sync;xcb-xfixes;xcb-xinerama;xcb-xinput;xcb-glx;gstapp-1.0;gstpbutils-1.0;gstaudio-1.0;gstvideo-1.0;gstbase-1.0;gstreamer-1.0;gobject-2.0;glib-2.0;gio-2.0;gmodule-2.0;gthread-2.0;mysqlclient;odbc;pq;tiff;webp;webpdemux;webpmux"
        CACHE STRING "Semicolon-separated library names that will be auto-detected and bundled into the AppImage")
    set(NWT_APPIMAGE_EXTRA_LIBS "" CACHE STRING "Semicolon-separated absolute library paths to bundle into the AppImage (Linux only)")
    set(NWT_APPIMAGE_EXTRA_BINS "" CACHE STRING "Semicolon-separated extra executables to place in the AppImage bin directory (Linux only)")
endif()

# Qt TextToSpeech toggle (kept for parity with eva; nwt may or may not use it).
set(_NWT_ENABLE_QT_TTS_DEFAULT ON)
if (UNIX AND NOT APPLE AND NWT_LINUX_STATIC)
    set(_NWT_ENABLE_QT_TTS_DEFAULT OFF)
endif()
option(NWT_ENABLE_QT_TTS "Enable Qt TextToSpeech support" ${_NWT_ENABLE_QT_TTS_DEFAULT})
if (UNIX AND NOT APPLE AND NWT_LINUX_STATIC AND NWT_ENABLE_QT_TTS)
    message(STATUS "NWT_LINUX_STATIC detected: forcing NWT_ENABLE_QT_TTS=OFF to drop Qt TextToSpeech dependency.")
    set(NWT_ENABLE_QT_TTS OFF CACHE BOOL "Enable Qt TextToSpeech support" FORCE)
endif()
if (NWT_ENABLE_QT_TTS)
    add_compile_definitions(NWT_ENABLE_QT_TTS)
else()
    add_compile_definitions(NWT_DISABLE_QT_TTS)
endif()

# ---- Global toggles that affect subprojects ----

option(MCP_SSL "Enable SSL support" ON)

add_compile_definitions(_WIN32_WINNT=0x0601)

# ---- Packaging specific flags ----

if (BODY_PACK)
    if (WIN32)
        set(BODY_PACK_EXE WIN32)
        add_compile_definitions(BODY_WIN_PACK)
    elseif(UNIX)
        add_compile_definitions(BODY_LINUX_PACK)
    endif()
endif()

# Control whether to run windeployqt during Windows packaging.
# Default: skip for MinGW (static Qt is common there), run for MSVC.
if (WIN32)
    if (MINGW)
        set(DEFAULT_BODY_SKIP_WINDEPLOYQT ON)
    else()
        set(DEFAULT_BODY_SKIP_WINDEPLOYQT OFF)
    endif()
    option(BODY_SKIP_WINDEPLOYQT "Skip running windeployqt when BODY_PACK=ON (useful for static Qt)" ${DEFAULT_BODY_SKIP_WINDEPLOYQT})
endif()

# ---- Compiler & platform flags ----

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /DNOMINMAX /DWIN32_LEAN_AND_MEAN")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /utf-8")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
    # Silence MSVC STL deprecation warning for checked array iterators
    add_compile_definitions(_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)
elseif (MINGW)
    add_compile_definitions(_XOPEN_SOURCE=600)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Wall -Wextra -ffunction-sections -fdata-sections -fexceptions -mthreads")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,--gc-sections -s")

    # Collect target-scoped options to apply after the executable is created.
    set(NWT_COMPILE_OPTIONS "")
    set(NWT_LINK_OPTIONS    "")

    if (NWT_STATIC)
        # Keep Qt dynamic; only make the MinGW runtime static to simplify deployment.
        list(APPEND NWT_COMPILE_OPTIONS
            -fno-keep-inline-dllexport
            -fopenmp
            -O2
            -Wall -Wextra
            -ffunction-sections -fdata-sections
            -fexceptions
            -mthreads)
        list(APPEND NWT_LINK_OPTIONS
            -static
            -static-libgcc
            -static-libstdc++
            -fopenmp
            -Wl,-s
            -Wl,--gc-sections
            -mthreads
            -lpthread)
    else()
        list(APPEND NWT_COMPILE_OPTIONS
            -fopenmp
            -O2
            -Wall -Wextra
            -ffunction-sections -fdata-sections
            -fexceptions
            -mthreads)
        list(APPEND NWT_LINK_OPTIONS
            -Wl,--gc-sections
            -mthreads
            -lpthread)
    endif()
elseif (UNIX)
    message(STATUS "Compiling on Unix/Linux")
    find_package(X11 REQUIRED)
    list(APPEND extra_LIBS X11::Xtst)
endif()

# ---- Language standard ----

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
