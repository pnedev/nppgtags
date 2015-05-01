/**
 *  \file
 *  \brief  Search input window
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
#include <memory>
#include "GTags.h"
#include "CmdEngine.h"


namespace GTags
{

/**
 *  \class  SearchWin
 *  \brief
 */
class SearchWin
{
public:
    static void Register();
    static void Unregister();

    static void Show(const std::shared_ptr<Cmd>& cmd, CompletionCB complCB,
            bool enRE = true, bool enMC = true);

private:
    static const TCHAR  cClassName[];
    static const int    cBackgroundColor;
    static const TCHAR  cBtnFont[];
    static const int    cWidth;

    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg,
            WPARAM wparam, LPARAM lparam);
    static RECT adjustSizeAndPos(HWND hOwner, DWORD styleEx, DWORD style,
            int width, int height);

    SearchWin(const std::shared_ptr<Cmd>& cmd, CompletionCB complCB) :
        _cmd(cmd), _complCB(complCB) {}
    SearchWin(const SearchWin&);
    ~SearchWin();

    HWND composeWindow(HWND hOwner, bool enRE, bool enMC);
    void onOK();
    void onCancel();

    static SearchWin* SW;

    std::shared_ptr<Cmd>    _cmd;
    CompletionCB const      _complCB;

    HWND                    _hWnd;
    HWND                    _hEdit;
    HWND                    _hRE;
    HWND                    _hMC;
    HWND                    _hOK;
    HFONT                   _hTxtFont;
    HFONT                   _hBtnFont;
};

} // namespace GTags
