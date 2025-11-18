# PostBuild.cmake - post-build deployment and packaging hooks (Windows: bundle Qt, Linux: AppImage)
#
# This file is adapted from eva's PostBuild.cmake so that nwt shares the same
# static-linking and packaging behaviour. It assumes:
#   - NWT_TARGET is the main executable target
#   - Qt5_BIN_DIR is set (see QtSetup.cmake)
#   - eva_OUTPUT_NAME is set (see EnvInfo.cmake)

include_guard(GLOBAL)

if (UNIX)
    # Resolve linker scripts/symlinks to real shared objects so we can grab the SONAME later.
    function(_eva_resolve_library_real_path _out_var _requested_path)
        if (NOT EXISTS "${_requested_path}")
            set(${_out_var} "" PARENT_SCOPE)
            return()
        endif()

        set(_resolved "")
        # GNU ld scripts start with INPUT/GROUP, try to parse the actual targets.
        file(STRINGS "${_requested_path}" _ld_lines LIMIT_COUNT 8)
        foreach(_ld_line IN LISTS _ld_lines)
            string(STRIP "${_ld_line}" _ld_line_stripped)
            if (_ld_line_stripped MATCHES "^(INPUT|GROUP)\\s*\\(([^\\)]+)\\)")
                set(_payload "${CMAKE_MATCH_2}")
                string(REGEX REPLACE "[ \\t]+" ";" _entries "${_payload}")
                foreach(_entry IN LISTS _entries)
                    string(STRIP "${_entry}" _candidate)
                    string(REGEX REPLACE "^[\\\"']" "" _candidate "${_candidate}")
                    string(REGEX REPLACE "[\\\"']$" "" _candidate "${_candidate}")
                    if (IS_ABSOLUTE "${_candidate}" AND EXISTS "${_candidate}")
                        set(_resolved "${_candidate}")
                        break()
                    endif()
                endforeach()
                if (_resolved)
                    break()
                endif()
            endif()
        endforeach()

        if (NOT _resolved)
            get_filename_component(_realpath "${_requested_path}" REALPATH)
            if (EXISTS "${_realpath}")
                set(_resolved "${_realpath}")
            endif()
        endif()

        set(${_out_var} "${_resolved}" PARENT_SCOPE)
    endfunction()

    # Extract SONAME via objdump so we can recreate versioned links inside AppDir/usr/lib.
    function(_eva_extract_soname _out_var _lib_path)
        set(_soname "")
        if (CMAKE_OBJDUMP AND EXISTS "${CMAKE_OBJDUMP}" AND EXISTS "${_lib_path}")
            execute_process(
                COMMAND "${CMAKE_OBJDUMP}" -p "${_lib_path}"
                OUTPUT_VARIABLE _objdump_text
                RESULT_VARIABLE _objdump_rc
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)
            if (_objdump_rc EQUAL 0)
                string(REGEX MATCH "SONAME[ \\t]+([^ \\t\\r\\n]+)" _match "${_objdump_text}")
                if (_match)
                    set(_soname "${CMAKE_MATCH_1}")
                endif()
            endif()
        endif()
        set(${_out_var} "${_soname}" PARENT_SCOPE)
    endfunction()
endif()

if (WIN32)
    # Always ensure Qt SQLite plugin is available for dev runs (BODY_PACK off as well).
    get_filename_component(Qt5_PLUGINS_DIR "${Qt5_BIN_DIR}/../plugins" ABSOLUTE)
    if (EXISTS "${Qt5_PLUGINS_DIR}/sqldrivers/qsqlite.dll")
        add_custom_command(TARGET ${NWT_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${NWT_TARGET}>/sqldrivers"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${Qt5_PLUGINS_DIR}/sqldrivers/qsqlite.dll"
                    "$<TARGET_FILE_DIR:${NWT_TARGET}>/sqldrivers/qsqlite.dll"
            COMMENT "Copy Qt SQLite (qsqlite.dll) driver")
    else()
        message(WARNING "Qt SQLite plugin not found at ${Qt5_PLUGINS_DIR}/sqldrivers/qsqlite.dll")
    endif()

    if (BODY_PACK)
        # Deploy Qt runtime beside the executable using windeployqt unless explicitly skipped.
        if (NOT BODY_SKIP_WINDEPLOYQT)
            if (EXISTS "${Qt5_BIN_DIR}/windeployqt.exe")
                add_custom_command(TARGET ${NWT_TARGET} POST_BUILD
                    COMMAND "${Qt5_BIN_DIR}/windeployqt.exe"
                            --release
                            --no-translations
                            --compiler-runtime
                            --dir "$<TARGET_FILE_DIR:${NWT_TARGET}>"
                            "$<TARGET_FILE:${NWT_TARGET}>"
                    COMMENT "windeployqt: bundling Qt runtime into bin")
            else()
                message(WARNING "windeployqt.exe not found in ${Qt5_BIN_DIR}")
            endif()
        endif()

        # Optionally copy MinGW runtime DLLs if requested and we are using a shared MinGW runtime.
        if (MINGW AND BODY_COPY_MINGW_RUNTIME AND NOT NWT_STATIC)
            # Try to resolve compiler bin dir from CMAKE_CXX_COMPILER.
            get_filename_component(COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
            add_custom_command(TARGET ${NWT_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${COMPILER_BIN_DIR}/libgcc_s_seh-1.dll"
                        "$<TARGET_FILE_DIR:${NWT_TARGET}>/libgcc_s_seh-1.dll"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${COMPILER_BIN_DIR}/libstdc++-6.dll"
                        "$<TARGET_FILE_DIR:${NWT_TARGET}>/libstdc++-6.dll"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${COMPILER_BIN_DIR}/libwinpthread-1.dll"
                        "$<TARGET_FILE_DIR:${NWT_TARGET}>/libwinpthread-1.dll"
                COMMENT "Copy MinGW runtime DLLs")
        endif()
    endif()

elseif(UNIX)
    if (BODY_PACK)
        set(_NWT_APPDIR "${CMAKE_BINARY_DIR}/AppDir")
        set(_NWT_APPDIR_BIN "${_NWT_APPDIR}/usr/bin")
        set(_NWT_APPDIR_LIB "${_NWT_APPDIR}/usr/lib")
        set(_NWT_APPDIR_SHARE "${_NWT_APPDIR}/usr/share")
        set(_NWT_APPDIR_APPS "${_NWT_APPDIR_SHARE}/applications")
        set(_NWT_APPDIR_ICONS "${_NWT_APPDIR_SHARE}/icons/hicolor/64x64/apps")

        # Collect extra copy commands for optional executables.
        set(_NWT_APPIMAGE_BIN_COPY_COMMANDS "")
        set(_NWT_APPIMAGE_EXTRA_BIN_DESTINATIONS "")
        if (NWT_APPIMAGE_EXTRA_BINS)
            foreach(_extra_bin IN LISTS NWT_APPIMAGE_EXTRA_BINS)
                if (EXISTS "${_extra_bin}")
                    get_filename_component(_extra_bin_name "${_extra_bin}" NAME)
                    set(_extra_bin_dest "${_NWT_APPDIR_BIN}/${_extra_bin_name}")
                    list(APPEND _NWT_APPIMAGE_BIN_COPY_COMMANDS
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                "${_extra_bin}"
                                "${_extra_bin_dest}")
                    list(APPEND _NWT_APPIMAGE_EXTRA_BIN_DESTINATIONS "${_extra_bin_dest}")
                else()
                    message(WARNING "NWT_APPIMAGE_EXTRA_BINS entry '${_extra_bin}' does not exist and will be skipped.")
                endif()
            endforeach()
        endif()

        # Collect extra copy commands for shared libraries (auto + manual).
        set(_NWT_APPIMAGE_LIB_COPY_COMMANDS "")
        set(_NWT_APPIMAGE_EXTRA_LIB_DESTINATIONS "")
        set(_NWT_APPIMAGE_LIB_SOURCE_PATHS "")
        if (NWT_APPIMAGE_BUNDLE_COMMON_LIBS AND NWT_APPIMAGE_COMMON_LIB_NAMES)
            set(_NWT_APPIMAGE_AUTO_LIBS "")
            foreach(_auto_lib_name IN LISTS NWT_APPIMAGE_COMMON_LIB_NAMES)
                string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _auto_lib_var "${_auto_lib_name}")
                find_library(NWT_APPIMAGE_LIB_${_auto_lib_var} NAMES "${_auto_lib_name}")
                if (NWT_APPIMAGE_LIB_${_auto_lib_var})
                    list(APPEND _NWT_APPIMAGE_AUTO_LIBS "${NWT_APPIMAGE_LIB_${_auto_lib_var}}")
                    message(STATUS "AppImage: found ${_auto_lib_name} at ${NWT_APPIMAGE_LIB_${_auto_lib_var}}")
                else()
                    message(STATUS "AppImage: missing ${_auto_lib_name}, skip auto bundle")
                endif()
            endforeach()
            if (_NWT_APPIMAGE_AUTO_LIBS)
                list(APPEND _NWT_APPIMAGE_LIB_SOURCE_PATHS ${_NWT_APPIMAGE_AUTO_LIBS})
                message(STATUS "NWT AppImage auto bundling libs: ${_NWT_APPIMAGE_AUTO_LIBS}")
            else()
                message(STATUS "NWT AppImage auto bundling libs: none found for current toolchain.")
            endif()
        endif()
        if (NWT_APPIMAGE_EXTRA_LIBS)
            list(APPEND _NWT_APPIMAGE_LIB_SOURCE_PATHS ${NWT_APPIMAGE_EXTRA_LIBS})
        endif()
        if (_NWT_APPIMAGE_LIB_SOURCE_PATHS)
            list(REMOVE_DUPLICATES _NWT_APPIMAGE_LIB_SOURCE_PATHS)
            message(STATUS "NWT AppImage bundling libs (auto+extra): ${_NWT_APPIMAGE_LIB_SOURCE_PATHS}")
            foreach(_extra_lib IN LISTS _NWT_APPIMAGE_LIB_SOURCE_PATHS)
                if (EXISTS "${_extra_lib}")
                    get_filename_component(_requested_lib_name "${_extra_lib}" NAME)
                    set(_resolved_lib "${_extra_lib}")
                    if (UNIX)
                        _eva_resolve_library_real_path(_resolved_lib "${_extra_lib}")
                    endif()
                    if (NOT _resolved_lib)
                        message(WARNING "AppImage library source '${_extra_lib}' cannot be resolved and will be skipped.")
                        continue()
                    endif()
                    get_filename_component(_resolved_lib_name "${_resolved_lib}" NAME)
                    set(_resolved_lib_dest "${_NWT_APPDIR_LIB}/${_resolved_lib_name}")
                    list(APPEND _NWT_APPIMAGE_LIB_COPY_COMMANDS
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                "${_resolved_lib}"
                                "${_resolved_lib_dest}")
                    list(APPEND _NWT_APPIMAGE_EXTRA_LIB_DESTINATIONS "${_resolved_lib_dest}")
                    if (UNIX)
                        _eva_extract_soname(_resolved_soname "${_resolved_lib}")
                        if (_resolved_soname)
                            set(_soname_dest "${_NWT_APPDIR_LIB}/${_resolved_soname}")
                            if (NOT _soname_dest STREQUAL _resolved_lib_dest)
                                list(APPEND _NWT_APPIMAGE_LIB_COPY_COMMANDS
                                    COMMAND ${CMAKE_COMMAND} -E remove -f "${_soname_dest}"
                                    COMMAND ${CMAKE_COMMAND} -E create_symlink
                                            "${_resolved_lib_name}"
                                            "${_soname_dest}")
                            endif()
                        endif()
                        if (_requested_lib_name
                            AND NOT _requested_lib_name STREQUAL _resolved_lib_name
                            AND (NOT _resolved_soname OR NOT _requested_lib_name STREQUAL _resolved_soname))
                            set(_requested_dest "${_NWT_APPDIR_LIB}/${_requested_lib_name}")
                            list(APPEND _NWT_APPIMAGE_LIB_COPY_COMMANDS
                                COMMAND ${CMAKE_COMMAND} -E remove -f "${_requested_dest}"
                                COMMAND ${CMAKE_COMMAND} -E create_symlink
                                        "${_resolved_lib_name}"
                                        "${_requested_dest}")
                        endif()
                    endif()
                else()
                    message(WARNING "AppImage library source '${_extra_lib}' does not exist and will be skipped.")
                endif()
            endforeach()
        endif()

        # Compose linuxdeploy arguments (allow bundling of extra shared libs/executables).
        set(_NWT_LINUXDEPLOY_ARGS "--appdir" "${_NWT_APPDIR}" "--executable" "${_NWT_APPDIR_BIN}/nwt")
        if (_NWT_APPIMAGE_EXTRA_LIB_DESTINATIONS)
            foreach(_extra_lib_dest IN LISTS _NWT_APPIMAGE_EXTRA_LIB_DESTINATIONS)
                list(APPEND _NWT_LINUXDEPLOY_ARGS "--library" "${_extra_lib_dest}")
            endforeach()
        endif()
        if (_NWT_APPIMAGE_EXTRA_BIN_DESTINATIONS)
            foreach(_extra_bin_dest IN LISTS _NWT_APPIMAGE_EXTRA_BIN_DESTINATIONS)
                list(APPEND _NWT_LINUXDEPLOY_ARGS "--executable" "${_extra_bin_dest}")
            endforeach()
        endif()

        set(_NWT_LINUXDEPLOY_COMMANDS
            COMMAND "${Qt5_BIN_DIR}/linuxdeploy" ${_NWT_LINUXDEPLOY_ARGS})
        if (NWT_LINUX_STATIC)
            list(APPEND _NWT_LINUXDEPLOY_COMMANDS
                COMMAND "${Qt5_BIN_DIR}/appimagetool" "${_NWT_APPDIR}" "--runtime-file" "${Qt5_BIN_DIR}/runtime-appimage" "${eva_OUTPUT_NAME}.appimage")
        else()
            list(APPEND _NWT_LINUXDEPLOY_COMMANDS
                COMMAND env QMAKE="${Qt5_BIN_DIR}/qmake" "${Qt5_BIN_DIR}/linuxdeploy-plugin-qt" "--appdir" "${_NWT_APPDIR}"
                COMMAND "${Qt5_BIN_DIR}/appimagetool" "${_NWT_APPDIR}" "--runtime-file" "${Qt5_BIN_DIR}/runtime-appimage" "${eva_OUTPUT_NAME}.appimage")
        endif()

        add_custom_target(appimage ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_NWT_APPDIR_BIN}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_NWT_APPDIR_LIB}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_NWT_APPDIR_APPS}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_NWT_APPDIR_ICONS}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:${NWT_TARGET}>"
                    "${_NWT_APPDIR_BIN}/nwt"
            ${_NWT_APPIMAGE_BIN_COPY_COMMANDS}
            ${_NWT_APPIMAGE_LIB_COPY_COMMANDS}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/resource/nwt.desktop"
                    "${_NWT_APPDIR_APPS}/nwt.desktop"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/resource/aicun2.png"
                    "${_NWT_APPDIR_ICONS}/nwt.png"
            ${_NWT_LINUXDEPLOY_COMMANDS}
            DEPENDS ${NWT_TARGET}
            COMMENT "Building AppImage for ${NWT_TARGET}")
    endif()
endif()
