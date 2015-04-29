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
#include "GTags.h"


namespace GTags
{

/**
 *  \struct
 *  \brief
 */
struct SearchData
{
    SearchData(const TCHAR* str = NULL,
            bool regExp = false, bool matchCase = false) :
            _regExp(regExp), _matchCase(matchCase)
    {
        if (str)
            _tcscpy_s(_str, cMaxTagLen, str);
        else
            _str[0] = 0;
    }

    TCHAR   _str[cMaxTagLen];
    bool    _regExp;
    bool    _matchCase;
};


/**
 *  \class  SearchWin
 *  \brief
 */
class SearchWin
{
public:
    static void Register();
    static void Unregister();

    static void Show(const TCHAR *header, SearchData* searchData,
            bool enMatchCase, bool enRegExp);

private:
    static const TCHAR cClassName[];
    static const int cBackgroundColor;
    static const TCHAR cBtnFont[];
    static const int cWidth;

    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg,
            WPARAM wparam, LPARAM lparam);
    static RECT adjustSizeAndPos(HWND hOwner, DWORD styleEx, DWORD style,
            int width, int height);

    SearchWin(SearchData* searchData) : _searchData(searchData) {}
    SearchWin(const SearchWin&);
    ~SearchWin();

    HWND composeWindow(HWND hOwner, int width, const TCHAR* header,
            SearchData* searchData, bool enMatchCase, bool enRegExp);
    void onOK();

    HANDLE _hExit;
    HWND _hWnd;
    HWND _hEditWnd;
    HWND _hRegExp;
    HWND _hMatchCase;
    HWND _hOK;
    HFONT _hTxtFont;
    HFONT _hBtnFont;
    SearchData* _searchData;
};

} // namespace GTags
