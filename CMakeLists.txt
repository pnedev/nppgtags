cmake_minimum_required (VERSION 2.8)

set (CMAKE_SYSTEM_NAME Windows)

set (CMAKE_SYSROOT "/home/paveln/bin/cross")

set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM  NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY  ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE  ONLY)

set (toolchain_prefix   i686-w64-mingw32)

set (CMAKE_C_COMPILER   ${toolchain_prefix}-gcc)
set (CMAKE_CXX_COMPILER ${toolchain_prefix}-g++)
SET (CMAKE_RC_COMPILER  ${toolchain_prefix}-windres)

set (win32_inc_dir /${toolchain_prefix}/include)
set (win32_lib_dir /${toolchain_prefix}/lib)

project (NppGTags)

set (defs
    -DDEVELOPMENT -DUNICODE -D_UNICODE -DMINGW_HAS_SECURE_API=1 -D_WIN32 -DWIN32 -DNDEBUG
)

set (CMAKE_CXX_FLAGS
    "-O3 -mwindows -mthreads -municode -mdll -Wall -Wno-unknown-pragmas"
)

set (CMAKE_MODULE_LINKER_FLAGS
    "-s"
)

set (project_rc_files
    src/nppgtags.rc
)

set (project_sources
    src/DocLocation.cpp
    src/GTagsCmd.cpp
    src/INpp.cpp
    src/Common.cpp
    src/PluginInterface.cpp
    src/GTags.cpp
    src/DBManager.cpp
    src/ReadPipe.cpp
    src/ActivityWindow.cpp
    src/IOWindow.cpp
    src/AutoCompleteUI.cpp
    src/ScintillaViewUI.cpp
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
    RENAME devNppGTags.dll
)
