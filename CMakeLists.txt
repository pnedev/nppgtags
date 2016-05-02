cmake_minimum_required (VERSION 2.8)

set (CMAKE_SYSTEM_NAME Windows)

set (CMAKE_SYSROOT "$ENV{HOME}/bin/cross")

set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM  NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY  ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE  ONLY)

set (toolchain_prefix   i686-w64-mingw32)

set (CMAKE_C_COMPILER   ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-gcc)
set (CMAKE_CXX_COMPILER ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-g++)
set (CMAKE_RC_COMPILER  ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-windres)

set (win32_inc_dir ${CMAKE_SYSROOT}/${toolchain_prefix}/include)
set (win32_lib_dir ${CMAKE_SYSROOT}/${toolchain_prefix}/lib)

project (NppGTags)

set (defs
    -DDEVELOPMENT -DUNICODE -D_UNICODE -DMINGW_HAS_SECURE_API=1 -D_WIN32 -DWIN32
    -D_WIN32_WINNT=0x0501 -DWIN32_LEAN_AND_MEAN -DNOCOMM -DNDEBUG
)

set (CMAKE_CXX_FLAGS
    "-std=c++11 -O3 -mwindows -mthreads -municode -Wall -Wno-unknown-pragmas"
)

set (CMAKE_MODULE_LINKER_FLAGS
    "-s"
)

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
    src/GTagsConfig.cpp
    src/DocLocation.cpp
    src/ActivityWin.cpp
    src/SearchWin.cpp
    src/ConfigWin.cpp
    src/AboutWin.cpp
    src/AutoCompleteWin.cpp
    src/ResultWin.cpp
)

find_library (comctl32
    NAMES libcomctl32.a
    PATHS ${win32_lib_dir}
)

add_definitions (${defs})

include_directories (${win32_inc_dir})

add_library (NppGTags MODULE ${project_rc_files} ${project_sources})

target_link_libraries (NppGTags ${comctl32})

install (FILES ${CMAKE_BINARY_DIR}/libNppGTags.dll
    DESTINATION "$ENV{HOME}/.wine/drive_c/Program Files/Notepad++/plugins"
    RENAME NppGTags.dll
)
