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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>


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

    static HWND Create(HWND hOwner, HANDLE hTerminate, int width,
            const TCHAR *text, int showDelay_ms = 0);

private:
    static const TCHAR      cClassName[];
    static const TCHAR      cFont[];
    static const unsigned   cFontSize;
    static const int        cBackgroundColor;

    static volatile LONG RefCount;

    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg,
            WPARAM wParam, LPARAM lParam);

    ActivityWin(HANDLE hTerminate, int showDelay_ms);
    ActivityWin(const ActivityWin&);
    ~ActivityWin();

    void adjustSizeAndPos(int width, int height);
    HWND composeWindow(HWND hOwner, int width, const TCHAR* text);
    void showWindow();
    void onTimer();

    HANDLE      _hTerm;
    HFONT       _hFont;
    HWND        _hWnd;
    HWND        _hBtn;
    UINT_PTR    _timerId;
    int         _initRefCount;
    int         _showDelay_ms;
};

} // namespace GTags
