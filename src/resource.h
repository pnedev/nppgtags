#include <tchar.h>

#define VER_VERSION         4,3,0,0
#define VER_VERSION_STR     _T("4.3.0\0")

#define IS_PRERELEASE       1

#if (IS_PRERELEASE)
#define VER_PRERELEASE      VS_FF_PRERELEASE
#else
#define VER_PRERELEASE      0
#endif

#define VER_AUTHOR          _T("Pavel Nedev\0")
#define VER_COPYRIGHT       _T("Copyright (C) 2014-2016 Pavel Nedev\0")
#define VER_URL             _T("https://github.com/pnedev/nppgtags\0")

#define PLUGIN_NAME         _T("NppGTags")
#define VER_PLUGIN_NAME     PLUGIN_NAME _T("\0")
#define VER_DLLFILENAME     PLUGIN_NAME _T(".dll\0")

#ifndef DEVELOPMENT

#define VER_DESCRIPTION     _T("GTags plugin for Notepad++\0")

#else

#define VER_DESCRIPTION     _T("GTags plugin for Notepad++ (dev. version)\0")

#endif
