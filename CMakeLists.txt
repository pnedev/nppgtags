cmake_minimum_required (VERSION 3.15)
cmake_policy (SET CMP0091 NEW)

set (CMAKE_SYSTEM_NAME Windows)

if (UNIX OR MINGW)
    set (CMAKE_SYSROOT "/usr")

    set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM  NEVER)
    set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY  ONLY)
    set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE  ONLY)

    if (WIN64)
        set (toolchain_prefix x86_64-w64-mingw32)
    else ()
        set (toolchain_prefix i686-w64-mingw32)
    endif ()

    set (CMAKE_C_COMPILER   ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-gcc)
    set (CMAKE_CXX_COMPILER ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-g++)
    set (CMAKE_RC_COMPILER  ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-windres)

    set (win32_inc_dir ${CMAKE_SYSROOT}/${toolchain_prefix}/include)
    set (win32_lib_dir ${CMAKE_SYSROOT}/${toolchain_prefix}/lib)
endif ()

project (NppGTags)

set(CMAKE_CXX_STANDARD 14)

if (UNIX OR MINGW)
    set (defs
        -DUNICODE -D_UNICODE -DMINGW_HAS_SECURE_API=1 -D_WIN32 -DWIN32
        -D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN -DNOCOMM -DNDEBUG
    )

    if (WIN64)
        set (defs ${defs} -D_WIN64)
    endif ()

    if (DEVEL)
        set (defs ${defs} -DDEVEL)
    endif ()

    set (CMAKE_CXX_FLAGS
        "-O3 -static-libgcc -static-libstdc++ -Wall -Wno-unknown-pragmas"
    )

    set (CMAKE_MODULE_LINKER_FLAGS "-s")

    set (CMAKE_SHARED_MODULE_PREFIX "")
else ()
    set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    if (CMAKE_CL_64 OR CMAKE_GENERATOR MATCHES Win64)
        set (defs
            -DUNICODE -D_UNICODE -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES -D_WIN32 -DWIN32
            -D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN -DNOCOMM -D_WIN64
        )
    else ()
        set (defs
            -DUNICODE -D_UNICODE -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES -D_WIN32 -DWIN32
            -D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN -DNOCOMM
        )
    endif ()

    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /LD /MP /W4")
endif ()

set (project_rc_files
    src/nppgtags.rc
)

set (project_sources
    src/Common.cpp
    src/INpp.cpp
    src/PluginInterface.cpp
    src/ReadPipe.cpp
    src/GTags.cpp
    src/LineParser.cpp
    src/Cmd.cpp
    src/CmdEngine.cpp
    src/DbManager.cpp
    src/Config.cpp
    src/DocLocation.cpp
    src/ActivityWin.cpp
    src/SearchWin.cpp
    src/SettingsWin.cpp
    src/AboutWin.cpp
    src/AutoCompleteWin.cpp
    src/ResultWin.cpp
)

add_definitions (${defs})

add_library (NppGTags MODULE ${project_rc_files} ${project_sources})

if (UNIX OR MINGW)
    include_directories (${win32_inc_dir})

    find_library (comctl32
        NAMES libcomctl32.a
        PATHS ${win32_lib_dir}
    )

    target_link_libraries (NppGTags ${comctl32})

    set (INSTALL_PATH
        "$ENV{HOME}/.wine/drive_c/Program Files/Notepad++/plugins/NppGTags"
    )

    message ("Install destination: ${INSTALL_PATH}")

    install (FILES ${CMAKE_BINARY_DIR}/NppGTags.dll
        DESTINATION "${INSTALL_PATH}"
    )

    install (DIRECTORY bin/bin
        DESTINATION "${INSTALL_PATH}"
    )

    install (DIRECTORY bin/share
        DESTINATION "${INSTALL_PATH}"
    )
else ()
    target_link_libraries (NppGTags comctl32)
endif ()
