/**
 *  \file
 *  \brief  GTags result Scintilla view window
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


#include <windows.h>
#include <tchar.h>
#include <vector>
#include "Scintilla.h"
#include "AutoLock.h"
#include "Common.h"
#include "GTags.h"
#include "CmdEngine.h"


namespace GTags
{

/**
 *  \class  ResultWin
 *  \brief
 */
class ResultWin
{
public:
    static HWND Register();
    static void Unregister();

    static void Show()
    {
        if (RW)
            RW->show();
    }

    static void Show(const std::shared_ptr<Cmd>& cmd)
    {
        if (RW)
            RW->show(cmd);
    }

    static void ApplyStyle()
    {
        if (RW)
            RW->applyStyle();
    }

private:
    /**
     *  \struct  Tab
     *  \brief
     */
    struct Tab
    {
        Tab(const std::shared_ptr<Cmd>& cmd);
        ~Tab() {}

        inline bool operator==(const Tab& tab) const
        {
            return (_cmdId == tab._cmdId && _projectPath == tab._projectPath && _search == tab._search);
        }

        const CmdId_t       _cmdId;
        const bool          _regExp;
        const bool          _matchCase;
        CTextA              _projectPath;
        CTextA              _search;
        bool                _outdated;
        CTextA              _uiBuf;
        int                 _currentLine;
        int                 _firstVisibleLine;

        void SetFolded(int lineNum);
        void ClearFolded(int lineNum);
        bool IsFolded(int lineNum);

    private:
        void parseCmd(CTextA& dst, const char* src);
        void parseFindFile(CTextA& dst, const char* src);

        std::vector<int> _expandedLines;
    };

    static const COLORREF   cBlack = RGB(0,0,0);
    static const COLORREF   cWhite = RGB(255,255,255);

    static const TCHAR      cClassName[];

    static LRESULT CALLBACK keyHookProc(int code, WPARAM wParam, LPARAM lParam);
    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    ResultWin() : _hWnd(NULL), _hSci(NULL), _hKeyHook(NULL), _sciFunc(NULL), _sciPtr(0), _activeTab(NULL) {}
    ResultWin(const ResultWin&);
    ~ResultWin();

    void show();
    void show(const std::shared_ptr<Cmd>& cmd);
    void applyStyle();

    inline LRESULT sendSci(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        return _sciFunc(_sciPtr, static_cast<unsigned int>(Msg),
                static_cast<uptr_t>(wParam), static_cast<sptr_t>(lParam));
    }

    void setStyle(int style, COLORREF fore = cBlack, COLORREF back = cWhite, bool bold = false, bool italic = false,
            int size = 0, const char *font = NULL);

    void configScintilla();
    HWND composeWindow();
    void showWindow();
    void hideWindow();

    void hookKeyboard()
    {
        _hKeyHook = SetWindowsHookEx(WH_KEYBOARD, keyHookProc, NULL, GetCurrentThreadId());
    }

    Tab* getTab(int i = -1);
    void loadTab(Tab* tab);
    bool openItem(int lineNum, unsigned matchNum = 1);

    bool findString(const char* str, int* startPos, int* endPos, bool matchCase, bool wholeWord, bool regExp);
    void toggleFolding(int lineNum);
    void onStyleNeeded(SCNotification* notify);
    void onHotspotClick(SCNotification* notify);
    void onDoubleClick(int pos);
    void onMarginClick(SCNotification* notify);
    bool onKeyPress(WORD keyCode);
    void onTabChange();
    void onCloseTab();
    void closeAllTabs();
    void onResize(int width, int height);

    static ResultWin* RW;

    Mutex       _lock;
    HWND        _hWnd;
    HWND        _hSci;
    HWND        _hTab;
    HHOOK       _hKeyHook;
    SciFnDirect _sciFunc;
    sptr_t      _sciPtr;
    Tab*        _activeTab;
};

} // namespace GTags
