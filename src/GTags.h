/**
 *  \file
 *  \brief  GTags plugin API
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2015 Pavel Nedev
 *
 *  \section LICENSE
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once


#include <tchar.h>
#include "resource.h"


struct FuncItem;
class CPath;


namespace GTags
{

const TCHAR cPluginName[]   = VER_PLUGIN_NAME;
const TCHAR cBinsDir[]      = VER_PLUGIN_NAME;

extern FuncItem     Menu[18];

extern HINSTANCE    HMod;
extern CPath        DllPath;

extern TCHAR        UIFontName[32];
extern unsigned     UIFontSize;

class CConfig;
extern CConfig      Config;


BOOL PluginLoad(HINSTANCE hMod);
void PluginInit();
void PluginDeInit();

bool UpdateSingleFile(const TCHAR* file = NULL);
const CPath CreateLibraryDatabase(HWND hWnd);

} // namespace GTags
