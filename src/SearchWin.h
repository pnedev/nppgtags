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
#include "Common.h"
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
    static void Close();

private:
    static const TCHAR  cClassName[];
    static const TCHAR  cBtnFont[];
    static const int    cWidth;

    static LRESULT CALLBACK keyHookProc(int code,
            WPARAM wParam, LPARAM lParam);
    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg,
            WPARAM wParam, LPARAM lParam);
    static RECT adjustSizeAndPos(HWND hOwner, DWORD styleEx, DWORD style,
            int width, int height);

    static void endCompletion(const std::shared_ptr<Cmd>&);

    SearchWin(const std::shared_ptr<Cmd>& cmd, CompletionCB complCB) :
        _suggestionOn(0), _cmd(cmd), _complCB(complCB), _hKeyHook(NULL),
        _cancelled(true) {}
    SearchWin(const SearchWin&);
    ~SearchWin();

    HWND composeWindow(HWND hOwner, bool enRE, bool enMC);
    void startCompletion();

    void fillSuggestions();
    void clearSuggestions();

    void onOK();

    static SearchWin* SW;

    volatile unsigned int _suggestionOn;

    std::shared_ptr<Cmd>    _cmd;
    CompletionCB const      _complCB;

    HWND                    _hWnd;
    HWND                    _hSearch;
    HWND                    _hRE;
    HWND                    _hMC;
    HWND                    _hOK;
    HFONT                   _hTxtFont;
    HFONT                   _hBtnFont;
    HHOOK                   _hKeyHook;
    bool                    _cancelled;

    CTcharArray             _suggestions;
};

} // namespace GTags
