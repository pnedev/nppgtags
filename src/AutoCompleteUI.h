/**
 *  \file
 *  \brief  GTags AutoComplete UI
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014 Pavel Nedev
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


/**
 *  \class  AutoCompleteUI
 *  \brief
 */
class AutoCompleteUI
{
public:
    static BOOL Show(const GTags::CmdData& cmd);

private:
    static const TCHAR cClassName[];

    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg,
            WPARAM wparam, LPARAM lparam);

    AutoCompleteUI(const GTags::CmdData& cmd);
    AutoCompleteUI(const AutoCompleteUI&);
    ~AutoCompleteUI();

    HWND composeWindow();
    int fillLV();
    int filterLV(const TCHAR* filter);
    void resizeLV();

    void onDblClick();
    bool onKeyDown(int keyCode);

    HWND _hwnd;
    HWND _hLVWnd;
    HFONT _hFont;
    const GTags::CmdData& _cmd;
    TCHAR* _result;
};
