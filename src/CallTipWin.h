/**
 *  \file
 *  \brief  GTags CallTips window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>, Robert McDowell <github.com/RobertP-McDowell>
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
#include <cstdint>
#include <vector>
#include <memory>
#include "Common.h"
#include "CmdDefines.h"
#include "Cmd.h"


namespace GTags
{

class CallTipParser : public ResultParser
{
public:
    intptr_t overload = 0;
    intptr_t func_start_pos = 0;
    CallTipParser(intptr_t pOverload, intptr_t pFuncStart) :
        overload(pOverload), func_start_pos(pFuncStart) {}
    virtual ~CallTipParser() {}

    virtual intptr_t Parse(const CmdPtr_t&);
    virtual const std::vector<TCHAR*>& GetListPaths() const { return _paths; }
    int FindListIndexFromLine(TCHAR* findLine);

protected:
    std::vector<TCHAR*> _paths;
private:
    CText _buf;
};

class CallTipWin
{
public:
    static void Register();
    static void Unregister();

    static void Show(const CmdPtr_t& cmd);

    static bool IsShown()
    {
        return (CTW != nullptr);
    }
    
    static void DestroyCurrentWin()
    {
        if (CTW != nullptr)
        {
            CTW->_queued_for_deletion = true;
            SendMessage(CTW->_hWnd, WM_CLOSE, 0, 0);
        }
    }
    
    static void GetCallTipFunction(CTextA& func_name, intptr_t& overload, intptr_t& func_start_pos, intptr_t caret_pos = -1);
    
    CallTipWin(const CmdPtr_t& cmd);
    ~CallTipWin();
private:
    static const TCHAR  cClassName[];
    static const int    cBackgroundColor;
    static const int    cItemWidth;
    static const int    cMinWidth;

    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static int getDefParamCount(TCHAR* word);
    static TCHAR* getDefParamText(TCHAR* word, int wordSize);

    int getItemByName(TCHAR* itemText);

    CallTipWin& operator=(const CallTipWin&) = delete;

    HWND composeWindow();
    int filterLV();
    void resizeLV();

    void onClick(int item);
    void updateHeader(int overload, int high_overload = -1, TCHAR* header1 = _T("CallTip"), TCHAR* header2 = _T(""));
    void updateWindow(intptr_t position = -1);
    static std::unique_ptr<CallTipWin> CTW;
    int             _selItem;
    bool            _queued_for_deletion; // Used to know when not to auto refocus.
    HWND            _hWnd;
    HWND            _hLVWnd;
    HFONT           _hFont;
    const CmdId_t   _cmdId;
    const CText     _tag;
    std::shared_ptr<CallTipParser>      _parser;
};

} // namespace GTags
