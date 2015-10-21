/**
 *  \file
 *  \brief  Process Activity window
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
#include <list>


namespace GTags
{

/**
 *  \class  ActivityWin
 *  \brief
 */
class ActivityWin
{
public:
    static void Register();
    static void Unregister();

    static void Show(const TCHAR* text, HANDLE hCancel);
    static HWND GetHwnd(HANDLE hCancel);

    static void UpdatePositions();

private:
    static const TCHAR      cClassName[];
    static const int        cBackgroundColor;
    static const unsigned   cFontSize;
    static const int        cWidth;

    static std::list<ActivityWin*>  WindowList;
    static HFONT                    HFont;

    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    ActivityWin(HANDLE hCancel) : _hCancel(hCancel), _hWnd(NULL) {};
    ActivityWin(const ActivityWin&);
    ~ActivityWin();

    void adjustSizeAndPos(int width, int height, int winNum);
    HWND composeWindow(const TCHAR* text);
    void onResize(int winNum);

    HANDLE  _hCancel;
    HWND    _hWnd;
    HWND    _hBtn;
    int     _initRefCount;
};

} // namespace GTags
