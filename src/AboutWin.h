/**
 *  \file
 *  \brief  About window
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

namespace GTags
{

/**
 *  \class  AboutWin
 *  \brief
 */
class AboutWin
{
public:
    static void Show(const TCHAR *info);

private:
    static const TCHAR  cClassName[];
    static const int    cBackgroundColor;
    static const TCHAR  cFont[];
    static const int    cFontSize;
    static const TCHAR  cAbout[];

    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static RECT adjustSizeAndPos(HWND hWnd, int width, int height);

    AboutWin() {}
    AboutWin(const AboutWin&);
    ~AboutWin();

    HWND composeWindow(HWND hOwner, const TCHAR* info);

    static AboutWin* AW;

    HWND    _hWnd;
    HFONT   _hFont;
};

} // namespace GTags
