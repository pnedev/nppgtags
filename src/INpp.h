/**
 *  \file
 *  \brief  Npp API
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
#include <vector>
#include "Common.h"
#include "NppAPI/Notepad_plus_msgs.h"
#include "NppAPI/Docking.h"
#include "NppAPI/PluginInterface.h"


/**
 *  \class  INpp
 *  \brief
 */
class INpp
{
public:
    static inline INpp& Get()
    {
        static INpp Instance;
        return Instance;
    }

    inline HWND ReadSciHandle()
    {
        int currentEdit;

        SendMessage(_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);

        _hSC = currentEdit ? _nppData._scintillaSecondHandle : _nppData._scintillaMainHandle;

        return _hSC;
    }

    inline void SetData(NppData nppData)
    {
        _nppData = nppData;
        ReadSciHandle();
    }

    inline HWND GetHandle() const { return _nppData._nppHandle; }

    inline void SetSciHandle(HWND hSciWnd)
    {
        if (hSciWnd == _nppData._scintillaMainHandle || hSciWnd == _nppData._scintillaSecondHandle ||
                isSciWndRegistered(hSciWnd))
            _hSC = hSciWnd;
    }

    inline HWND GetSciHandle() const { return _hSC; }

    inline HWND GetSciHandle(int sciNum) const
    {
        return (sciNum ? _nppData._scintillaSecondHandle : _nppData._scintillaMainHandle);
    }

    inline void SetPluginMenuFlag(int cmdId, bool enable) const
    {
        SendMessage(_nppData._nppHandle, NPPM_SETMENUITEMCHECK, cmdId, enable);
    }

    inline int GetVersion()
    {
        return (int)::SendMessage(_nppData._nppHandle, NPPM_GETNPPVERSION, 1, 0);
    }

    inline HMENU GetPluginMenu() const
    {
        return (HMENU)SendMessage(_nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);
    }

    inline void RegisterWinForDarkMode(HWND hWnd) const
    {
        SendMessage(_nppData._nppHandle, NPPM_DARKMODESUBCLASSANDTHEME, (WPARAM)NppDarkMode::dmfInit, (LPARAM)hWnd);
    }

    inline void RegisterWin(HWND hWnd) const
    {
        SendMessage(_nppData._nppHandle, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)hWnd);
    }

    inline void UnregisterWin(HWND hWnd) const
    {
        SendMessage(_nppData._nppHandle, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (LPARAM)hWnd);
    }

    inline void RegisterDockingWin(tTbData& data) const
    {
        SendMessage(_nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);
    }

    inline void ShowDockingWin(HWND hWnd) const
    {
        SendMessage(_nppData._nppHandle, NPPM_DMMSHOW, 0, (LPARAM)hWnd);
    }

    inline void HideDockingWin(HWND hWnd) const
    {
        SendMessage(_nppData._nppHandle, NPPM_DMMHIDE, 0, (LPARAM)hWnd);
    }

    inline void UpdateDockingWin(HWND hWnd) const
    {
        SendMessage(_nppData._nppHandle, NPPM_DMMUPDATEDISPINFO, 0, (LPARAM)hWnd);
    }

    inline HWND CreateSciHandle(const HWND hParentWnd)
    {
        HWND hSciWnd = (HWND)SendMessage(_nppData._nppHandle, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)hParentWnd);

        if (hSciWnd)
            _hSciWndList.push_back(hSciWnd);

        return hSciWnd;
    }

    inline void DestroySciHandle(const HWND hSciWnd)
    {
        if (isSciWndRegistered(hSciWnd, true))
            SendMessage(_nppData._nppHandle, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciWnd);
    }

    inline void GetMainDir(CPath& path) const
    {
        path.Resize(MAX_PATH);
        SendMessage(_nppData._nppHandle, NPPM_GETNPPDIRECTORY, (WPARAM)path.Size(), (LPARAM)path.C_str());
        path += _T("\\");
    }

    inline void GetPluginsConfDir(CPath& path) const
    {
        path.Resize(MAX_PATH);
        SendMessage(_nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)path.Size(), (LPARAM)path.C_str());
        path += _T("\\");
    }

    inline void GetFilePath(CPath& filePath) const
    {
        filePath.Resize(MAX_PATH);
        SendMessage(_nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)filePath.C_str());
        filePath.AutoFit();
    }

    inline void GetFilePathFromBufID(LRESULT bufId, CPath& filePath) const
    {
        filePath.Resize(MAX_PATH);
        SendMessage(_nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, bufId, (LPARAM)filePath.C_str());
        filePath.AutoFit();
    }

    inline void GetFileNamePart(CPath& fileName) const
    {
        fileName.Resize(MAX_PATH);
        SendMessage(_nppData._nppHandle, NPPM_GETNAMEPART, 0, (LPARAM)fileName.C_str());
        fileName.AutoFit();
    }

    inline int OpenFile(const TCHAR* filePath)
    {
        if (!SendMessage(_nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)filePath))
            return -1;

        ReadSciHandle();
        return 0;
    }

    inline void SwitchToFile(const TCHAR* filePath) const
    {
        SendMessage(_nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)filePath);
    }

    inline int GetCaretLineBack() const
    {
        return (int)SendMessage(_hSC, SCI_GETCARETLINEBACK, 0, 0);
    }

    inline void GetFontName(int style, char* fontName) const
    {
        SendMessage(_hSC, SCI_STYLEGETFONT, style, (LPARAM)fontName);
    }

    inline int GetFontSize(int style) const
    {
        return (int)SendMessage(_hSC, SCI_STYLEGETSIZE, style, 0);
    }

    inline int GetForegroundColor(int style) const
    {
        return (int)SendMessage(_hSC, SCI_STYLEGETFORE, style, 0);
    }

    inline int GetBackgroundColor(int style) const
    {
        return (int)SendMessage(_hSC, SCI_STYLEGETBACK, style, 0);
    }

    inline int GetTextHeight() const
    {
        intptr_t currPos = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
        currPos = SendMessage(_hSC, SCI_LINEFROMPOSITION, currPos, 0);
        return (int)SendMessage(_hSC, SCI_TEXTHEIGHT, currPos, 0);
    }

    inline void GetPointPos(int* x, int* y) const
    {
        intptr_t currPos = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
        *x = (int)SendMessage(_hSC, SCI_POINTXFROMPOSITION, 0, currPos) + 2;
        *y = (int)SendMessage(_hSC, SCI_POINTYFROMPOSITION, 0, currPos) + 2;
    }

    inline void GoToPos(intptr_t pos) const
    {
        SendMessage(_hSC, SCI_GOTOPOS, pos, 0);
    }

    inline void GoToLine(intptr_t line) const
    {
        SendMessage(_hSC, SCI_GOTOLINE, line, 0);
    }

    inline intptr_t PositionFromLine(intptr_t line) const
    {
        return SendMessage(_hSC, SCI_POSITIONFROMLINE, line, 0);
    }

    inline intptr_t LineEndPosition(intptr_t line) const
    {
        return SendMessage(_hSC, SCI_GETLINEENDPOSITION, line, 0);
    }

    inline bool IsSelectionVertical() const
    {
        return (bool)SendMessage(_hSC, SCI_SELECTIONISRECTANGLE, 0, 0);
    }

    inline intptr_t GetSelectionSize() const
    {
        return SendMessage(_hSC, SCI_GETSELTEXT, 0, 0) + 1;
    }

    inline void GetSelection(CTextA& sel) const
    {
        intptr_t selLen = GetSelectionSize();

        sel.Resize(selLen);
        SendMessage(_hSC, SCI_GETSELTEXT, 0, (LPARAM)sel.C_str());
        sel.AutoFit();
    }

    inline void SetSelection(intptr_t startPos, intptr_t endPos) const
    {
        SendMessage(_hSC, SCI_SETSEL, startPos, endPos);
    }

    inline void SelectWord(bool partial = false) const
    {
        intptr_t currPos    = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
        intptr_t wordStart  = SendMessage(_hSC, SCI_WORDSTARTPOSITION, currPos, true);
        intptr_t wordEnd    = partial ? currPos : SendMessage(_hSC, SCI_WORDENDPOSITION, currPos, true);

        SendMessage(_hSC, SCI_SETSEL, wordStart, wordEnd);
    }

    inline void ClearSelection() const
    {
        intptr_t currPos = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
        SendMessage(_hSC, SCI_SETSEL, currPos, currPos);
    }

    inline intptr_t GetPos() const
    {
        return SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
    }

    inline intptr_t GetWordStartPos(intptr_t currPos, bool onlyWord = true) const
    {
        return SendMessage(_hSC, SCI_WORDSTARTPOSITION, currPos, onlyWord);
    }

    inline intptr_t GetWordEndPos(intptr_t currPos, bool onlyWord = true) const
    {
        return SendMessage(_hSC, SCI_WORDENDPOSITION, currPos, onlyWord);
    }

    void EnsureCurrentLineVisible() const;
    void SetView(intptr_t startPos, intptr_t endPos = 0) const;

    intptr_t GetWordSize(bool partial = false) const;
    void GetWord(CTextA& word, bool partial = false, bool select = false) const;
    void ReplaceWord(const char* replText, bool partial = false) const;
    bool SearchText(const char* text, bool ignoreCase, bool wholeWord, bool regExp,
            intptr_t* startPos = NULL, intptr_t* endPos = NULL) const;

    inline void Backspace() const
    {
        SendMessage(_hSC, SCI_DELETEBACK, 0, 0);
    }

    inline char GetChar(intptr_t pos) const
    {
        return (char)SendMessage(_hSC, SCI_GETCHARAT, pos, 0);
    }

    inline void AddText(char* txt, size_t len) const
    {
        SendMessage(_hSC, SCI_ADDTEXT, len, (LPARAM)txt);
    }

    inline void BeginUndoAction() const
    {
        SendMessage(_hSC, SCI_BEGINUNDOACTION, 0, 0);
    }

    inline void EndUndoAction() const
    {
        SendMessage(_hSC, SCI_ENDUNDOACTION, 0, 0);
    }

private:
    INpp() : _hSC(NULL) {}
    INpp(const INpp&);
    ~INpp() {}

    bool isSciWndRegistered(HWND hSciWnd, bool remove = false);

    NppData             _nppData;
    HWND                _hSC;
    std::vector<HWND>   _hSciWndList;
};
