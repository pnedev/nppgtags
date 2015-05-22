/**
 *  \file
 *  \brief  Npp API
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
#include "Notepad_plus_msgs.h"
#include "Docking.h"
#include "PluginInterface.h"


/**
 *  \class  INpp
 *  \brief
 */
class INpp
{
public:
    static inline INpp& Get() { return Instance; }

    inline HWND ReadSciHandle()
    {
        int currentEdit;

        SendMessage(_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA,
                0, (LPARAM)&currentEdit);

        _hSC = currentEdit ? _nppData._scintillaSecondHandle :
                _nppData._scintillaMainHandle;

        return _hSC;
    }

    inline void SetData(NppData nppData)
    {
        _nppData = nppData;
        ReadSciHandle();
    }

    inline HWND GetHandle() { return _nppData._nppHandle; }
    inline HWND GetSciHandle() { return _hSC; }

    inline void SetPluginMenuFlag(int cmdId, bool enable) const
    {
        SendMessage(_nppData._nppHandle, NPPM_SETMENUITEMCHECK,
                (WPARAM)cmdId, (LPARAM)enable);
    }

    inline HMENU GetPluginMenu() const
    {
        return (HMENU)SendMessage(_nppData._nppHandle, NPPM_GETMENUHANDLE,
                (WPARAM)NPPPLUGINMENU, 0);
    }

    inline void RegisterWin(HWND hWnd) const
    {
        SendMessage(_nppData._nppHandle, NPPM_MODELESSDIALOG,
                MODELESSDIALOGADD, (LPARAM)hWnd);
    }

    inline void UnregisterWin(HWND hWnd) const
    {
        SendMessage(_nppData._nppHandle, NPPM_MODELESSDIALOG,
                MODELESSDIALOGREMOVE, (LPARAM)hWnd);
    }

    inline void RegisterDockingWin(tTbData& data) const
    {
        SendMessage(_nppData._nppHandle, NPPM_DMMREGASDCKDLG,
                0, (LPARAM)&data);
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
        SendMessage(_nppData._nppHandle, NPPM_DMMUPDATEDISPINFO,
                0, (LPARAM)hWnd);
    }

    inline HWND CreateSciHandle(const HWND hParentWnd) const
    {
        return (HWND)SendMessage(_nppData._nppHandle,
                NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)hParentWnd);
    }

    inline void DestroySciHandle(const HWND hSciWnd) const
    {
        SendMessage(_nppData._nppHandle, NPPM_DESTROYSCINTILLAHANDLE,
                0, (LPARAM)hSciWnd);
    }

    inline void GetMainDir(unsigned size, TCHAR* buf) const
    {
        SendMessage(_nppData._nppHandle, NPPM_GETNPPDIRECTORY,
                (WPARAM)size, (LPARAM)buf);
    }

    inline void GetPluginsConfDir(unsigned size, TCHAR* buf) const
    {
        SendMessage(_nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR,
                (WPARAM)size, (LPARAM)buf);
    }

    inline void GetFilePath(TCHAR* filePath) const
    {
        SendMessage(_nppData._nppHandle, NPPM_GETFULLCURRENTPATH,
                0, (LPARAM)filePath);
    }

    inline void GetFilePathFromBufID(int bufId, TCHAR* filePath) const
    {
        SendMessage(_nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID,
                bufId, (LPARAM)filePath);
    }

    inline void GetFileNamePart(TCHAR* fileName) const
    {
        SendMessage(_nppData._nppHandle, NPPM_GETNAMEPART,
                0, (LPARAM)fileName);
    }

    inline int OpenFile(const TCHAR* filePath) const
    {
        if (!SendMessage(_nppData._nppHandle, NPPM_DOOPEN,
                0, (LPARAM)filePath))
            return -1;
        return 0;
    }

    inline void SwitchToFile(const TCHAR* filePath) const
    {
        SendMessage(_nppData._nppHandle, NPPM_SWITCHTOFILE,
                0, (LPARAM)filePath);
    }

    inline int GetCaretLineBack() const
    {
        return SendMessage(_hSC, SCI_GETCARETLINEBACK, 0, 0);
    }

    inline void GetFontName(int style, char* fontName, int size) const
    {
        SendMessage(_hSC, SCI_STYLEGETFONT, (WPARAM)style,
                (LPARAM)fontName);
    }

    inline int GetFontSize(int style) const
    {
        return SendMessage(_hSC, SCI_STYLEGETSIZE, (WPARAM)style, 0);
    }

    inline int GetForegroundColor(int style) const
    {
        return SendMessage(_hSC, SCI_STYLEGETFORE, (WPARAM)style, 0);
    }

    inline int GetBackgroundColor(int style) const
    {
        return SendMessage(_hSC, SCI_STYLEGETBACK, (WPARAM)style, 0);
    }

    inline int GetTextHeight() const
    {
        long currPos = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
        currPos = SendMessage(_hSC, SCI_LINEFROMPOSITION, (WPARAM)currPos, 0);
        return SendMessage(_hSC, SCI_TEXTHEIGHT, (WPARAM)currPos, 0);
    }

    inline void GetPointPos(int* x, int* y) const
    {
        long currPos = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
        *x = SendMessage(_hSC, SCI_POINTXFROMPOSITION, 0, (LPARAM)currPos) + 2;
        *y = SendMessage(_hSC, SCI_POINTYFROMPOSITION, 0, (LPARAM)currPos) + 2;
    }

    inline void GoToPos(long pos) const
    {
        SendMessage(_hSC, SCI_GOTOPOS, (WPARAM)pos, 0);
    }

    inline void GoToLine(long line) const
    {
        SendMessage(_hSC, SCI_GOTOLINE, (WPARAM)line, 0);
    }

    inline long PositionFromLine(long line) const
    {
        return SendMessage(_hSC, SCI_POSITIONFROMLINE, (WPARAM)line, 0);
    }

    inline long LineEndPosition(long line) const
    {
        return SendMessage(_hSC, SCI_GETLINEENDPOSITION, (WPARAM)line, 0);
    }

    inline int IsSelectionVertical() const
    {
        return SendMessage(_hSC, SCI_SELECTIONISRECTANGLE, 0, 0);
    }

    inline long GetSelectionSize() const
    {
        return SendMessage(_hSC, SCI_GETSELTEXT, 0, 0) - 1;
    }

    inline long GetSelection(char* buf, int bufSize) const
    {
        long selLen = GetSelectionSize();
        if (selLen != 0 && bufSize > selLen)
            SendMessage(_hSC, SCI_GETSELTEXT, 0, (LPARAM)buf);
        return selLen;
    }

    inline void SetSelection(long startPos, long endPos) const
    {
        SendMessage(_hSC, SCI_SETSEL, (WPARAM)startPos, (LPARAM)endPos);
    }

    inline void ClearSelection() const
    {
        long currPos = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
        SendMessage(_hSC, SCI_SETSEL, (WPARAM)currPos, (LPARAM)currPos);
    }

    inline void GetView(long* firstVisibleLine, long* pos) const
    {
        *firstVisibleLine = SendMessage(_hSC, SCI_GETFIRSTVISIBLELINE, 0, 0);
        *pos = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
    }

    inline void SetView(long firstVisibleLine, long pos) const
    {
        SendMessage(_hSC, SCI_SETFIRSTVISIBLELINE, firstVisibleLine, 0);
        SendMessage(_hSC, SCI_SETSEL, (WPARAM)pos, (LPARAM)pos);
    }

    inline long GetWordSize() const
    {
        long currPos = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
        long wordStart = SendMessage(_hSC, SCI_WORDSTARTPOSITION,
                (WPARAM)currPos, (LPARAM)true);
        long wordEnd = SendMessage(_hSC, SCI_WORDENDPOSITION,
                (WPARAM)currPos, (LPARAM)true);

        return wordEnd - wordStart;
    }

    long GetWord(char* buf, int bufSize, bool select) const;
    void ReplaceWord(const char* replText) const;
    bool SearchText(const char* text,
            bool matchCase, bool wholeWord, bool regExp,
            long* startPos = NULL, long* endPos = NULL) const;

    inline void Backspace() const
    {
        SendMessage(_hSC, SCI_DELETEBACK, 0, 0);
    }

    inline char GetChar(long pos) const
    {
        return (char)SendMessage(_hSC, SCI_GETCHARAT, (WPARAM)pos, 0);
    }

    inline void AddText(char* txt, int len) const
    {
        SendMessage(_hSC, SCI_ADDTEXT, (WPARAM)len, (LPARAM)txt);
    }

private:
    static INpp Instance;

    INpp() : _hSC(NULL) {}
    INpp(const INpp&);
    ~INpp() {}

    NppData _nppData;
    HWND    _hSC;
};
