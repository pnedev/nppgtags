/**
 *  \file
 *  \brief  IO window for text input / output
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014 Pavel Nedev
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
#include <richedit.h>


/**
 *  \class  IOWindow
 *  \brief
 */
class IOWindow
{
public:
    static void Register(HINSTANCE hInst = NULL);
    static void Unregister();

    static bool In(HWND hOwner,
            const TCHAR* font, unsigned fontSize, int width,
            const TCHAR* header, TCHAR* text, int txtLimit)
    {
        return Create(hOwner, font, fontSize, false,
                width, 0, header, text, txtLimit);
    }

    static void Out(HWND hOwner,
            const TCHAR* font, unsigned fontSize,
            const TCHAR* header, TCHAR* text,
            int minWidth = 0, int minHeight = 0)
    {
        Create(hOwner, font, fontSize, true, minWidth, minHeight,
                header, text, 0);
    }

private:
    static const TCHAR cClassName[];
    static const int cBackgroundColor;

    static volatile LONG RefCount;
    static HINSTANCE HInst;

    static bool Create(HWND hOwner,
            const TCHAR *font, unsigned fontSize, bool readOnly,
            int minWidth, int minHeight,
            const TCHAR *header, TCHAR *text, int txtLimit);
    static RECT adjustSizeAndPos(DWORD style, int width, int height);
    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg,
            WPARAM wparam, LPARAM lparam);

    IOWindow(bool readOnly, int minWidth, int minHeight, TCHAR* text);
    IOWindow(const IOWindow &);
    ~IOWindow();

    HWND composeWindow(HWND hOwner,
            const TCHAR* font, unsigned fontSize, bool readOnly,
            int minWidth, int minHeight,
            const TCHAR* header, const TCHAR* text, int txtLimit);
    int onKeyDown(DWORD key);
    int onAutoSize(REQRESIZE* pReqResize);

    HWND _hWnd;
    HFONT _hFont;
    bool _readOnly;
    int _minWidth;
    int _minHeight;
    TCHAR* _text;
    bool _success;
};
