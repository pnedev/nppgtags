/**
 *  \file
 *  \brief  GTags AutoComplete window
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
#include "Cmd.h"


namespace GTags
{

/**
 *  \class  AutoCompleteWin
 *  \brief
 */
class AutoCompleteWin
{
public:
    static void Register();
    static void Unregister();

    static void Show(const std::shared_ptr<CmdData>& cmd);

private:
    static const TCHAR cClassName[];
    static const int cBackgroundColor;

    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg,
            WPARAM wparam, LPARAM lparam);

    AutoCompleteWin(const std::shared_ptr<CmdData>& cmd);
    AutoCompleteWin(const AutoCompleteWin&);
    ~AutoCompleteWin();

    HWND composeWindow(const TCHAR* header);
    int fillLV();
    int filterLV(const TCHAR* filter);
    void resizeLV();

    void onDblClick();
    bool onKeyDown(int keyCode);

    static AutoCompleteWin* ACW;

    HWND _hWnd;
    HWND _hLVWnd;
    HFONT _hFont;
    const CmdID_t _cmdID;
    const int _cmdTagLen;
    CTcharArray _result;
};

} // namespace GTags
