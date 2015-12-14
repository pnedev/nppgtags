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


#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "CmdDefines.h"


struct FuncItem;
class CText;
class CPath;


namespace GTags
{

const TCHAR cPluginName[]   = VER_PLUGIN_NAME;
const TCHAR cBinsDir[]      = VER_PLUGIN_NAME;

enum PluginWinMessages_t
{
    WM_RUN_CMD_CALLBACK = WM_USER,
    WM_OPEN_ACTIVITY_WIN,
    WM_CLOSE_ACTIVITY_WIN
};

extern FuncItem     Menu[19];

extern HINSTANCE    HMod;
extern CPath        DllPath;

extern CText        UIFontName;
extern unsigned     UIFontSize;

extern HWND         MainWndH;

class CConfig;
extern CConfig      Config;


BOOL PluginLoad(HINSTANCE hMod);
void PluginInit();
void PluginDeInit();

bool OnFileBeforeChange(const CPath& file);
bool OnFileChange(const CPath& file);
bool OnFileChangeCancel(const CPath& file);

} // namespace GTags
