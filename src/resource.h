#pragma once


#include <tchar.h>


#define PLUGIN_NAME         _T("NppGTags")

#define VER_VERSION         4,3,0,0
#define VER_VERSION_STR     _T("4.3.0\0")

#define VER_AUTHOR          _T("Pavel Nedev\0")
#define VER_COPYRIGHT       _T("Copyright (C) 2014-2016 Pavel Nedev\0")
#define VER_URL             _T("https://github.com/pnedev/nppgtags\0")

#define VER_DLLFILENAME     PLUGIN_NAME _T(".dll\0")

#ifndef _PRERELEASE
#define VER_PRERELEASE  0
#else
#define VER_PRERELEASE  VS_FF_PRERELEASE
#endif

#ifndef _DEBUG
#define VER_DEBUG   0
#else
#define VER_DEBUG   VS_FF_DEBUG
#endif

#ifndef _DEVELOPMENT
#define VER_DESCRIPTION     _T("GTags plugin for Notepad++\0")
#else
#define VER_DESCRIPTION     _T("GTags plugin for Notepad++ (dev. version)\0")
#endif

#ifndef _WIN64
#define VER_PRODUCT_NAME    PLUGIN_NAME _T(" (32-bit)\0")
#else
#define VER_PRODUCT_NAME    PLUGIN_NAME _T(" (64-bit)\0")
#endif
