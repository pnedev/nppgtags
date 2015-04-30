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

    static int Show(HWND hOwner, HANDLE procHndl, int width,
            const TCHAR *text, int showDelay_ms);

private:
    static const TCHAR cClassName[];
    static const unsigned cUpdate_ms;
    static const TCHAR cFont[];
    static const unsigned cFontSize;
    static const int cBackgroundColor;

    static volatile LONG RefCount;

    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg,
            WPARAM wparam, LPARAM lparam);

    ActivityWin(HWND hOwner, HANDLE procHndl);
    ActivityWin(const ActivityWin&);
    ~ActivityWin();

    void adjustSizeAndPos(HWND hwnd, int width, int height);
    HWND composeWindow(int width, const TCHAR* text);
    void onTimerRefresh(HWND hwnd);

    HWND _hOwner;
    HANDLE _hProc;
    HFONT _hFont;
    HWND _hBtn;
    UINT_PTR _timerId;
    int _initRefCount;
    int _isCancelled;
};

} // namespace GTags
