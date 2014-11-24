cmake_minimum_required (VERSION 2.8)

set (CMAKE_SYSTEM_NAME Windows)

# set (CMAKE_SYSROOT "")

# set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM  NEVER)
# set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY  ONLY)
# set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE  ONLY)

set (toolchain_prefix   i686-w64-mingw32)

set (CMAKE_C_COMPILER   ${toolchain_prefix}-gcc)
set (CMAKE_CXX_COMPILER ${toolchain_prefix}-g++)
SET (CMAKE_RC_COMPILER  ${toolchain_prefix}-windres)

set (win32_inc_dir /usr/${toolchain_prefix}/include)
set (win32_lib_dir /usr/${toolchain_prefix}/lib)

project (NppGTags)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_BUILD_TYPE       Release)

set (defs
    -DUNICODE -D_UNICODE
)

set (CMAKE_C_FLAGS
    "-march=native -m32 -mwindows -mwin32 -mthreads -mabi=ms -municode -mdll -Wall -v"
)

set (CMAKE_CXX_FLAGS    ${CMAKE_C_FLAGS})

set (CMAKE_MODULE_LINKER_FLAGS_RELEASE
    "-s -v"
)

set (project_rc_files
    src/nppgtags.rc
)

set (project_inc_dir
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
    src/TreeViewUI.cpp
)

find_library (win32_libs
    NAMES libcomctl32.a
    PATHS ${win32_lib_dir}
)

add_definitions (${defs})

include_directories (${win32_inc_dir} ${project_inc_dir})

add_library (NppGTags MODULE ${project_rc_files} ${project_sources})

target_link_libraries (NppGTags ${win32_libs})
