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


#pragma comment (lib, "comctl32")


#define WIN32_LEAN_AND_MEAN
#include "AboutWin.h"
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <richedit.h>
#include <stdlib.h>
#include "GTags.h"


namespace GTags
{

const TCHAR AboutWin::cClassName[]      = _T("AboutWin");
const int AboutWin::cBackgroundColor    = COLOR_INFOBK;
const TCHAR AboutWin::cFont[]           = _T("Tahoma");
const int AboutWin::cFontSize           = 10;

const TCHAR AboutWin::cAbout[] = {
    _T("\n%s\n\n")
    _T("Version: %s\n")
    _T("Build date: %s %s\n")
    _T("%s <pg.nedev@gmail.com>\n\n")
    _T("Licensed under GNU GPLv2 ")
    _T("as published by the Free Software Foundation.\n\n")
    _T("This plugin is frontend to ")
    _T("GNU Global source code tagging system (GTags):\n")
    _T("http://www.gnu.org/software/global/global.html\n")
    _T("Thanks to its developers and to ")
    _T("Jason Hood for porting it to Windows.\n\n")
    _T("Current GTags version:\n%s")
};


/**
 *  \brief
 */
void AboutWin::Show(HWND hOwner, const TCHAR* info)
{
    if (!info)
        return;
    else
    {
        WNDCLASS wc         = {0};
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = wndProc;
        wc.hInstance        = HMod;
        wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = GetSysColorBrush(cBackgroundColor);
        wc.lpszClassName    = cClassName;

        RegisterClass(&wc);

        INITCOMMONCONTROLSEX icex   = {0};
        icex.dwSize                 = sizeof(icex);
        icex.dwICC                  = ICC_STANDARD_CLASSES;

        InitCommonControlsEx(&icex);
        LoadLibrary(_T("Riched20.dll"));
    }

    AboutWin aw;
    if (aw.composeWindow(hOwner, info) == NULL)
        return;

    BOOL r;
    MSG msg;
    while ((r = GetMessage(&msg, aw._hWnd, 0, 0)) != 0 && r != -1)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


/**
 *  \brief
 */
RECT AboutWin::adjustSizeAndPos(HWND hWnd, int width, int height)
{
    RECT maxWin;
    GetWindowRect(GetDesktopWindow(), &maxWin);

    POINT center;
    HWND hOwner = GetParent(hWnd);
    if (hOwner)
    {
        RECT biasWin;
        GetWindowRect(hOwner, &biasWin);
        center.x = (biasWin.right + biasWin.left) / 2;
        center.y = (biasWin.bottom + biasWin.top) / 2;
    }
    else
    {
        center.x = (maxWin.right + maxWin.left) / 2;
        center.y = (maxWin.bottom + maxWin.top) / 2;
    }

    RECT win = {0};
    win.right = width;
    win.bottom = height;

    AdjustWindowRectEx(&win, GetWindowLongPtr(hWnd, GWL_STYLE),
            FALSE, GetWindowLongPtr(hWnd, GWL_EXSTYLE));

    width = win.right - win.left;
    height = win.bottom - win.top;

    if (width < maxWin.right - maxWin.left)
    {
        win.left = center.x - width / 2;
        if (win.left < maxWin.left) win.left = maxWin.left;
        win.right = win.left + width;
    }
    else
    {
        win.left = maxWin.left;
        win.right = maxWin.right;
    }

    if (height < maxWin.bottom - maxWin.top)
    {
        win.top = center.y - height / 2;
        if (win.top < maxWin.top) win.top = maxWin.top;
        win.bottom = win.top + height;
    }
    else
    {
        win.top = maxWin.top;
        win.bottom = maxWin.bottom;
    }

    return win;
}


/**
 *  \brief
 */
AboutWin::~AboutWin()
{
    if (_hWnd)
        UnregisterHotKey(_hWnd, 1);

    if (_hFont)
        DeleteObject(_hFont);

    UnregisterClass(cClassName, HMod);
    FreeLibrary(GetModuleHandle(_T("Riched20.dll")));
}


/**
 *  \brief
 */
HWND AboutWin::composeWindow(HWND hOwner, const TCHAR* info)
{
    DWORD styleEx = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;
    DWORD style = WS_POPUP | WS_CAPTION;

    _hWnd = CreateWindowEx(styleEx, cClassName, _T("About"),
            style, 0, 0, 100, 100, hOwner, NULL, HMod, NULL);
    if (_hWnd == NULL)
        return NULL;

    RECT win;
    GetClientRect(_hWnd, &win);

    style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
            ES_MULTILINE | ES_READONLY;

    HWND hEdit = CreateWindowEx(0, RICHEDIT_CLASS, NULL, style, 0, 0,
            win.right - win.left, win.bottom - win.top,
            _hWnd, NULL, HMod, NULL);

    SendMessage(hEdit, EM_SETBKGNDCOLOR, 0,
            (LPARAM)GetSysColor(cBackgroundColor));

    CHARFORMAT fmt  = {0};
    fmt.cbSize      = sizeof(fmt);
    fmt.dwMask      = CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_SIZE;
    fmt.dwEffects   = CFE_AUTOCOLOR;
    fmt.yHeight     = cFontSize * 20;
    _tcscpy_s(fmt.szFaceName, _countof(fmt.szFaceName), cFont);
    SendMessage(hEdit, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&fmt);

    HDC hdc = GetWindowDC(hEdit);
    _hFont = CreateFont(
            -MulDiv(cFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, cFont);
    ReleaseDC(hEdit, hdc);
    if (_hFont)
        SendMessage(hEdit, WM_SETFONT, (WPARAM)_hFont, (LPARAM)TRUE);

    DWORD events =
            ENM_KEYEVENTS | ENM_MOUSEEVENTS | ENM_REQUESTRESIZE | ENM_LINK;
    SendMessage(hEdit, EM_SETEVENTMASK, 0, (LPARAM)events);
    SendMessage(hEdit, EM_AUTOURLDETECT, (WPARAM)TRUE, 0);

    TCHAR text[2048];
    _sntprintf_s(text, _countof(text), _TRUNCATE, cAbout,
            VER_DESCRIPTION, VER_VERSION_STR,
            _T(__DATE__), _T(__TIME__), VER_COPYRIGHT, info);
    Edit_SetText(hEdit, text);

    RegisterHotKey(_hWnd, 1, 0, VK_ESCAPE);

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);

    return _hWnd;
}


/**
 *  \brief
 */
LRESULT APIENTRY AboutWin::wndProc(HWND hwnd, UINT umsg,
        WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
        case WM_CREATE:
            return 0;

        case WM_COMMAND:
            if (HIWORD(wparam) == EN_SETFOCUS)
            {
                DestroyCaret();
                return 0;
            }
        break;

        case WM_HOTKEY:
            if (hwnd != GetFocus())
            {
                Tools::MsgA("does not own the focus");
                break;
            }
            if (HIWORD(lparam) == VK_ESCAPE)
            {
                SendMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }
        break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lparam)->code)
            {
                case EN_MSGFILTER:
                {
                    DestroyCaret();
                    MSGFILTER* pMsgFilter = (MSGFILTER*)lparam;
                    if (pMsgFilter->msg != WM_MOUSEMOVE &&
                            pMsgFilter->msg != WM_LBUTTONUP)
                        return 1;
                }
                break;

                case EN_REQUESTRESIZE:
                {
                    REQRESIZE* pReqResize = (REQRESIZE*)lparam;
                    HWND hEdit = pReqResize->nmhdr.hwndFrom;
                    int width =
                        pReqResize->rc.right - pReqResize->rc.left + 30;
                    int height =
                        pReqResize->rc.bottom - pReqResize->rc.top;

                    RECT win = adjustSizeAndPos(hwnd, width, height);
                    MoveWindow(hwnd, win.left, win.top,
                            win.right - win.left, win.bottom - win.top, TRUE);
                    GetClientRect(hwnd, &win);
                    MoveWindow(hEdit, 0, 0,
                            win.right - win.left, win.bottom - win.top, TRUE);
                    return 1;
                }

                case EN_LINK:
                {
                    ENLINK* pEnLink = (ENLINK*)lparam;
                    if (pEnLink->msg == WM_LBUTTONUP)
                    {
                        TCHAR* link = new TCHAR[
                                pEnLink->chrg.cpMax - pEnLink->chrg.cpMin + 1];
                        TEXTRANGE range;
                        range.chrg = pEnLink->chrg;
                        range.lpstrText = link;
                        SendMessage(pEnLink->nmhdr.hwndFrom, EM_GETTEXTRANGE,
                                0, (LPARAM) &range);
                        ShellExecute(NULL, _T("open"), link, NULL, NULL,
                                SW_SHOWNORMAL);
                        delete [] link;
                        return 1;
                    }
                }
            }
        break;

        case WM_DESTROY:
            DestroyCaret();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}

} // namespace GTags
