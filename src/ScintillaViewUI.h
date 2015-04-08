/**
 *  \file
 *  \brief  GTags result Scintilla view UI
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
#include <vector>
#include "Scintilla.h"
#include "Common.h"
#include "AutoLock.h"
#include "GTags.h"
#include "Cmd.h"


namespace GTags
{

/**
 *  \class  ScintillaViewUI
 *  \brief
 */
class ScintillaViewUI
{
public:
    static ScintillaViewUI& Get()
    {
        static ScintillaViewUI Instance;
        return Instance;
    }

    int Register();
    void Unregister();
    void Show(const std::shared_ptr<CmdData>& cmd);
    void ApplyStyle();

private:
    /**
     *  \struct  Tab
     *  \brief
     */
    struct Tab
    {
        Tab(const std::shared_ptr<CmdData>& cmd);
        ~Tab() {}

        inline bool operator==(const Tab& tab) const
        {
            return (_cmdID == tab._cmdID &&
                    !strcmp(_projectPath, tab._projectPath) &&
                    !strcmp(_search, tab._search));
        }

        int _cmdID;
        bool _regexp;
        bool _matchCase;
        char _projectPath[MAX_PATH];
        char _search[cMaxTagLen];
        CTextA _uiBuf;
        int _currentLine;
        int _firstVisibleLine;

        void SetFolded(int lineNum);
        void ClearFolded(int lineNum);
        bool IsFolded(int lineNum);

    private:
        void parseCmd(CTextA& dst, const char* src);
        void parseFindFile(CTextA& dst, const char* src);

        std::vector<int> _expandedLines;
    };

    static const COLORREF cBlack = RGB(0,0,0);
    static const COLORREF cWhite = RGB(255,255,255);
    static const COLORREF cRed = RGB(255,0,0);
    static const COLORREF cBlue = RGB(0,0,255);

    static const TCHAR cClassName[];
    static const TCHAR cTabFont[];

    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg,
            WPARAM wparam, LPARAM lparam);

    ScintillaViewUI() :
        _hWnd{NULL}, _hSci{NULL}, _hFont{NULL}, _sciFunc{NULL},
        _sciPtr{NULL}, _activeTab{NULL} {}
    ScintillaViewUI(const ScintillaViewUI&);
    ~ScintillaViewUI()
    {
        Unregister();
    }

    inline LRESULT sendSci(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        return _sciFunc(_sciPtr, static_cast<unsigned int>(Msg),
                static_cast<uptr_t>(wParam), static_cast<sptr_t>(lParam));
    }

    void setStyle(int style, COLORREF fore = cBlack, COLORREF back = cWhite,
            bool bold = false, bool italic = false,
            int size = 0, const char *font = NULL);

    void configScintilla();
    HWND composeWindow();

    Tab* getTab(int i = -1);
    void loadTab(Tab* tab);
    bool openItem(int lineNum, unsigned matchNum = 1);

    bool findString(const char* str, int* startPos, int* endPos,
        bool matchCase, bool wholeWord, bool regExpr);
    void toggleFolding(int lineNum);
    void onStyleNeeded(SCNotification* notify);
    void onHotspotClick(SCNotification* notify);
    void onDoubleClick(SCNotification* notify);
    void onMarginClick(SCNotification* notify);
    void onCharAddTry(SCNotification* notify);
    void onTabChange();
    void onCloseTab();
    void closeAllTabs();
    void onResize(int width, int height);

    Mutex _lock;
    HWND _hWnd;
    HWND _hSci;
    HWND _hTab;
    HFONT _hFont;
	SciFnDirect _sciFunc;
	sptr_t _sciPtr;
    Tab* _activeTab;
};

} // namespace GTags
