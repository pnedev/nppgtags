/**
 *  \file
 *  \brief  Search input window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2024 Pavel Nedev
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

    static void Show(CmdId_t cmdId, CompletionCB complCB, const TCHAR* hint = nullptr,
            bool enRE = true, bool enIC = true);
    static void Close();

    static bool IsFocused(HWND hWnd = NULL)
    {
        if (hWnd == NULL)
            hWnd = GetFocus();

        return (SW && (SW->_hWnd == hWnd || IsChild(SW->_hWnd, hWnd)));
    };

    static bool Activate()
    {
        if (SW)
        {
            SetFocus(SW->_hSearch);
            return true;
        }

        return false;
    };

    SearchWin(CmdId_t cmdId, CompletionCB complCB) :
        _cmdId(cmdId), _complCB(complCB), _cmd(NULL), _hKeyHook(NULL), _cancelled(true), _keyPressed(0),
        _completionStarted(false), _completionDone(false) {}
    ~SearchWin();

private:
    static const TCHAR  cClassName[];
    static const int    cWidth;
    static const int    cComplAfter;

    static LRESULT CALLBACK keyHookProc(int code, WPARAM wParam, LPARAM lParam);
    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static void halfComplete(const CmdPtr_t&);
    static void endCompletion(const CmdPtr_t&);

    SearchWin(const SearchWin&);
    SearchWin& operator=(const SearchWin&) = delete;

    HWND composeWindow(HWND hOwner, const TCHAR* hint, bool enRE, bool enIC);
    void reinit(CmdId_t cmdId, CompletionCB complCB, const TCHAR* hint, bool enRE, bool enIC);

    void startCompletion();
    void clearCompletion();
    void filterComplList();

    void saveSearchOptions();
    void onEditChange();
    void onOK();

    static std::unique_ptr<SearchWin> SW;

    CmdId_t         _cmdId;
    CompletionCB    _complCB;

    CmdPtr_t    _cmd;

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
