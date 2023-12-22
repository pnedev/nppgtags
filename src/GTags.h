/**
 *  \file
 *  \brief  GTags plugin API
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2022 Pavel Nedev
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
#include <memory>
#include "resource.h"
#include "CmdDefines.h"
#include "DbManager.h"

// These are also used as ComboBox array indices in SettingsWin.cpp
// If the MIN value is changed, it must be accounted for as arrays 
// are 0-based indices and this currently starts with 1
#define SCIAUTOCNCHAR_MIN 1
#define SCIAUTOCNCHAR_MAX 9

struct FuncItem;
class CText;
class CPath;


namespace GTags
{

const TCHAR cPluginName[]           = PLUGIN_NAME;
const TCHAR cPluginCfgFileName[]    = PLUGIN_NAME _T(".cfg");
const TCHAR cBinariesFolder[]       = _T("bin");

enum PluginWinMessages_t
{
    WM_RUN_CMD_CALLBACK = WM_USER,
    WM_OPEN_ACTIVITY_WIN,
    WM_CLOSE_ACTIVITY_WIN
};

extern FuncItem     Menu[21];

extern HINSTANCE    HMod;
extern CPath        DllPath;

extern CText        UIFontName;
extern unsigned     UIFontSize;

extern HWND         MainWndH;

class Settings;

extern Settings GTagsSettings;


DbHandle getDatabaseAt(const CPath& dbPath);
void showResultCB(const CmdPtr_t& cmd);

BOOL PluginLoad(HINSTANCE hMod);
void PluginInit();
void PluginDeInit();

void OnNppReady();
void OnFileBeforeChange(const CPath& file);
void OnFileChangeCancel();
void OnFileChange(const CPath& file);
void OnFileRename(const CPath& file);
void OnFileDelete(const CPath& file);
void SciAutoComplete(int ch);

} // namespace GTags
