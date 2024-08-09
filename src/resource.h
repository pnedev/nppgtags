#pragma once


#include <tchar.h>


#define PLUGIN_NAME         _T("NppGTags")

#define VER_VERSION         5,1,2,0
#define VER_VERSION_STR     _T("5.1.2\0")

#define VER_DESCRIPTION     _T("GTags plugin for Notepad++\0")

#define VER_AUTHOR          _T("Pavel Nedev\0")
#define VER_COPYRIGHT       _T("Copyright (C) 2014-2024 Pavel Nedev\0")
#define VER_URL             _T("https://github.com/pnedev/nppgtags\0")
#define VER_USER_GUIDE      _T("https://github.com/pnedev/nppgtags/blob/master/README.md\0")

#define VER_DLLFILENAME     PLUGIN_NAME _T(".dll\0")

#ifndef _DEBUG
#define VER_DEBUG   0
#else
#define VER_DEBUG   VS_FF_DEBUG
#endif

#ifndef _WIN64
#define VER_PRODUCT_NAME    PLUGIN_NAME _T(" (32-bit)\0")
#else
#define VER_PRODUCT_NAME    PLUGIN_NAME _T(" (64-bit)\0")
#endif

#define IDI_PLUGINPANEL 2501
