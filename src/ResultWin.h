/**
 *  \file
 *  \brief  GTags result Scintilla view window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2022 Pavel Nedev
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
#include <unordered_set>
#include <unordered_map>
#include <string>
#include "NppAPI/Scintilla.h"
#include "Common.h"
#include "Cmd.h"


namespace GTags
{

/**
 *  \class  ResultWin
 *  \brief
 */
class ResultWin
{
public:
    /**
     *  \class  TabParser
     *  \brief
     */
    class TabParser : public ResultParser
    {
    public:
        TabParser() : _filesCount(0), _hits(0), _headerStatusLen(0) {}
        virtual ~TabParser() {}

        virtual intptr_t Parse(const CmdPtr_t&);

        inline intptr_t getFilesCount() const { return _filesCount; }
        inline intptr_t getHitsCount() const { return _hits ? _hits : _filesCount; }
        inline int getHeaderStatusLen() const { return _headerStatusLen; }

        inline void addResultFile(const char* pFile, size_t len, intptr_t line)
        {
            _fileResults.emplace(std::string(pFile, len), line);
        }

        inline bool isFileInResults(const std::string& file) const
        {
            return (_fileResults.find(file) != _fileResults.end());
        }

        inline const std::string& getFirstFileResult() const { return _fileResults.begin()->first; }
        inline const std::unordered_map<std::string, intptr_t>& getFileResults() const { return _fileResults; }

    private:
        static bool filterEntry(const DbConfig& cfg, const char* pEntry, size_t len);

        intptr_t parseCmd(const CmdPtr_t&);
        intptr_t parseFindFile(const CmdPtr_t&);

        intptr_t    _filesCount;
        intptr_t    _hits;
        int         _headerStatusLen;

        std::unordered_map<std::string, intptr_t> _fileResults;
    };


    static HWND Register();
    static void Unregister();

    static void Show()
    {
        if (RW)
            RW->show();
    }

    static void Show(const CmdPtr_t& cmd)
    {
        if (RW)
            RW->show(cmd);
    }

    static void Close(const CmdPtr_t& cmd)
    {
        if (RW)
            RW->close(cmd);
    }

    static void ApplyStyle()
    {
        if (RW)
            RW->applyStyle();
    }

    static void NotifyDBUpdate(const CmdPtr_t& cmd)
    {
        if (RW)
            RW->notifyDBUpdate(cmd);
    }

    static HWND GetSciHandleIfFocused()
    {
        if (RW && GetFocus() == RW->_hSci)
            return RW->_hSci;

        return NULL;
    }

    static CPath GetDbPath()
    {
        if (RW && RW->_activeTab)
            return CPath(RW->_activeTab->_projectPath.C_str());

        return CPath();
    }

private:
    /**
     *  \struct  Tab
     *  \brief
     */
    struct Tab
    {
        Tab(const CmdPtr_t& cmd);
        ~Tab() {}
        Tab& operator=(const Tab&) = delete;

        inline bool operator==(const Tab& tab) const
        {
            return (_cmdId == tab._cmdId && _projectPath == tab._projectPath && _search == tab._search &&
                    _ignoreCase == tab._ignoreCase && _regExp == tab._regExp);
        }

        const CmdId_t   _cmdId;
        const bool      _regExp;
        const bool      _ignoreCase;
        CTextA          _projectPath;
        CTextA          _search;
        intptr_t        _currentLine;
        intptr_t        _firstVisibleLine;
        ParserPtr_t     _parser;

        bool            _dirty;

        inline void SetFolded(intptr_t lineNum);
        inline void SetAllFolded();
        inline void ClearFolded(intptr_t lineNum);
        inline bool IsFolded(intptr_t lineNum);

        void RestoreView(const Tab& oldTab);

    private:
        std::unordered_set<intptr_t> _expandedLines;
    };


    static const COLORREF   cBlack = RGB(0,0,0);
    static const COLORREF   cWhite = RGB(255,255,255);

    static const TCHAR      cClassName[];
    static const TCHAR      cSearchClassName[];

    static const int        cSearchBkgndColor;
    static const unsigned   cSearchFontSize;
    static const int        cSearchWidth;

    static LRESULT CALLBACK keyHookProc(int code, WPARAM wParam, LPARAM lParam);
    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT APIENTRY searchWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    ResultWin() : _hWnd(NULL), _hSci(NULL), _hKeyHook(NULL), _sciFunc(NULL), _sciPtr(0), _activeTab(NULL),
            _hSearch(NULL), _hSearchFont(NULL), _hBtnFont(NULL),
            _lastRE(false), _lastIC(false), _lastWW(true) {}
    ResultWin(const ResultWin&);
    ~ResultWin();

    void show();
    void show(const CmdPtr_t& cmd);
    void close(const CmdPtr_t& cmd);
    void reRunCmd();
    void applyStyle();
    void notifyDBUpdate(const CmdPtr_t& cmd);

    inline LRESULT sendSci(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        return _sciFunc(_sciPtr, static_cast<unsigned int>(Msg),
                static_cast<uptr_t>(wParam), static_cast<sptr_t>(lParam));
    }

    void setStyle(int style, COLORREF fore = cBlack, COLORREF back = cWhite, bool bold = false, bool italic = false,
            int size = 0, const char *font = NULL);

    void configScintilla();
    HWND composeWindow();
    void createSearchWindow();
    void onSearchWindowCreate(HWND hWnd);
    void showWindow(HWND hFocus = NULL);
    void hideWindow();
    void releaseKeys();

    void hookKeyboard()
    {
        _hKeyHook = SetWindowsHookEx(WH_KEYBOARD, keyHookProc, NULL, GetCurrentThreadId());
    }

    Tab* getTab(int i = -1);
    void loadTab(Tab* tab, bool firstTimeLoad = false);
    bool visitSingleResult(Tab* tab);
    bool openItem(intptr_t lineNum, unsigned matchNum = 1);

    bool findString(const char* str, intptr_t* startPos, intptr_t* endPos,
            bool ignoreCase, bool wholeWord, bool regExp);
    void toggleFolding(intptr_t lineNum);
    void foldAll(int foldAction);
    void onStyleNeeded(SCNotification* notify);
    void onNewPosition();
    void onHotspotClick(SCNotification* notify);
    void onDoubleClick(intptr_t pos);
    void onMarginClick(SCNotification* notify);
    bool onKeyPress(WORD keyCode, bool alt);
    void onTabChange();
    void onCloseTab();
    void closeAllTabs();
    void onResize(int width, int height);
    void onMove();
    void onSearch(bool reverseDir = false, bool keepFocus = false);

    static ResultWin* RW;

    HWND        _hWnd;
    HWND        _hSci;
    HWND        _hTab;
    HHOOK       _hKeyHook;
    SciFnDirect _sciFunc;
    sptr_t      _sciPtr;
    Tab*        _activeTab;

    HWND        _hSearch;
    HWND        _hSearchTxt;
    HWND        _hRE;
    HWND        _hIC;
    HWND        _hWW;
    HWND        _hUp;
    HWND        _hDown;
    HFONT       _hSearchFont;
    HFONT       _hBtnFont;

    int         _searchTxtHeight;

    bool        _lastRE;
    bool        _lastIC;
    bool        _lastWW;
    CText       _lastSearchTxt;
};

} // namespace GTags
