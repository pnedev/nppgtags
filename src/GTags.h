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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include "resource.h"


class CPath;
struct FuncItem;


namespace GTags
{

const TCHAR cPluginName[]           = VER_PLUGIN_NAME;
const TCHAR cBinsDir[]              = VER_PLUGIN_NAME;
const unsigned cMaxTagLen           = 128;

const TCHAR cCreateDatabase[]       = _T("Create Database");
const TCHAR cUpdateSingle[]         = _T("Database Single File Update");
const TCHAR cAutoCompl[]            = _T("AutoComplete");
const TCHAR cAutoComplFile[]        = _T("AutoComplete File");
const TCHAR cFindFile[]             = _T("Find File");
const TCHAR cFindDefinition[]       = _T("Find Definition");
const TCHAR cFindReference[]        = _T("Find Reference");
const TCHAR cFindSymbol[]           = _T("Find Symbol");
const TCHAR cGrep[]                 = _T("Grep");
const TCHAR cVersion[]              = _T("About");

const TCHAR cDefaultParser[]        = _T("default");
const TCHAR cCtagsParser[]          = _T("ctags");
const TCHAR cPygmentsParser[]       = _T("pygments");

enum
{
    DEFAULT_PARSER = 0,
    CTAGS_PARSER,
    PYGMENTS_PARSER
};


/**
 *  \struct
 *  \brief
 */
struct Settings
{
    Settings(int parserIdx = DEFAULT_PARSER, bool autoUpdate = true,
            const TCHAR* libraryDBsPath = NULL);

    int     _parserIdx;
    bool    _autoUpdate;
    TCHAR   _libraryDBsPath[1024];
};


extern FuncItem Menu[16];

extern HINSTANCE HMod;
extern CPath DllPath;

extern TCHAR UIFontName[32];
extern unsigned UIFontSize;

extern const TCHAR* cParsers[3];
extern Settings Config;


BOOL PluginInit(HINSTANCE hMod);
void PluginDeInit();
void EnablePluginMenuItem(int itemIdx, bool enable = true);

void AutoComplete();
void AutoCompleteFile();
void FindFile();
void FindDefinition();
void FindReference();
void Grep();
void GoBack();
void GoForward();
void CreateDatabase();
bool UpdateSingleFile(const TCHAR* file = NULL);
void DeleteDatabase();
void SettingsCfg();
void About();

} // namespace GTags
