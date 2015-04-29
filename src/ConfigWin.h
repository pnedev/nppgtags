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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>


namespace GTags
{

struct CConfig;


/**
 *  \class  ConfigWin
 *  \brief
 */
class ConfigWin
{
public:
    static void Show(CConfig* _cfg);

private:
    static const TCHAR cClassName[];
    static const TCHAR cHeader[];
    static const int cBackgroundColor;
    static const TCHAR cFont[];
    static const int cFontSize;

    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg,
            WPARAM wparam, LPARAM lparam);
    static RECT adjustSizeAndPos(HWND hOwner, DWORD styleEx, DWORD style,
            int width, int height);

    ConfigWin(CConfig* cfg) : _cfg(cfg) {}
    ConfigWin(const ConfigWin&);
    ~ConfigWin();

    HWND composeWindow(HWND hOwner);

    void onOK();

    static ConfigWin* CW;

    CConfig* _cfg;
    HWND _hWnd;
    HWND _hParser;
    HWND _hAutoUpdate;
    HWND _hEnLibDB;
    HWND _hCreateDB;
    HWND _hLibDB;
    HWND _hOK;
    HWND _hCancel;
    HFONT _hFont;
};

} // namespace GTags
