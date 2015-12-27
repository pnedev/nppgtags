/**
 *  \file
 *  \brief  GTags config window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2015 Pavel Nedev
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
#include "CmdDefines.h"
#include "GTags.h"


class CPath;


namespace GTags
{

/**
 *  \class  ConfigWin
 *  \brief
 */
class ConfigWin
{
public:
    static void Show(const DbConfig& cfg, const TCHAR* cfgPath = NULL);

private:
    static const TCHAR  cClassName[];
    static const TCHAR  cHeader[];
    static const int    cBackgroundColor;
    static const int    cFontSize;

    static void dbWriteReady(const CmdPtr_t& cmd);
    static void createDbCB(const CmdPtr_t& cmd);
    static void updateDbCB(const CmdPtr_t& cmd);

    static LRESULT CALLBACK keyHookProc(int code, WPARAM wParam, LPARAM lParam);
    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static RECT adjustSizeAndPos(HWND hOwner, DWORD styleEx, DWORD style, int width, int height);

    ConfigWin(const DbConfig& cfg, const TCHAR* cfgPath);
    ConfigWin(const ConfigWin&);
    ~ConfigWin();

    HWND composeWindow(HWND hOwner);

    void onUpdateDb();
    void onSave();
    void readData();
    void saveConfig(CPath& cfgFolder);
    void fillLibDb(const CPath& lib);

    bool createLibDatabase(CPath& dbPath, CompletionCB complCB);

    static ConfigWin* CW;

    DbConfig    _cfg;
    CPath       _cfgPath;

    HWND        _hWnd;
    HWND        _hParser;
    HWND        _hAutoUpdate;
    HWND        _hEnLibDb;
    HWND        _hCreateDb;
    HWND        _hUpdateDb;
    HWND        _hLibDb;
    HWND        _hSave;
    HWND        _hCancel;
    HHOOK       _hKeyHook;
    HFONT       _hFont;
    unsigned    _hUpdateCount;
};

} // namespace GTags
