/**
 *  \file
 *  \brief  Search input window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2019 Pavel Nedev
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
#include "Common.h"
#include "GTags.h"
#include "CmdDefines.h"


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

    static void Show(const CmdPtr_t& cmd, CompletionCB complCB, bool enRE = true, bool enIC = true);
    static void Close();

private:
    static const TCHAR  cClassName[];
    static const int    cWidth;
    static const int    cComplAfter;

    static LRESULT CALLBACK keyHookProc(int code, WPARAM wParam, LPARAM lParam);
    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static void halfComplete(const CmdPtr_t&);
    static void endCompletion(const CmdPtr_t&);

    SearchWin(const CmdPtr_t& cmd, CompletionCB complCB) :
        _cmd(cmd), _complCB(complCB), _hKeyHook(NULL), _cancelled(true), _keyPressed(0),
        _completionStarted(false), _completionDone(false) {}
    SearchWin(const SearchWin&);
    ~SearchWin();
    SearchWin& operator=(const SearchWin&) = delete;

    HWND composeWindow(HWND hOwner, bool enRE, bool enIC);
    void startCompletion();
    void clearCompletion();
    void filterComplList();

    void saveSearchOptions();
    void onEditChange();
    void onOK();

    static SearchWin* SW;

    CmdPtr_t            _cmd;
    CompletionCB const  _complCB;

    HWND        _hWnd;
    HWND        _hSearch;
    HWND        _hRE;
    HWND        _hIC;
    HWND        _hOK;
    HFONT       _hTxtFont;
    HFONT       _hBtnFont;
    HHOOK       _hKeyHook;
    bool        _cancelled;
    int         _keyPressed;
    bool        _completionStarted;
    bool        _completionDone;
    ParserPtr_t _completion;
};

} // namespace GTags
