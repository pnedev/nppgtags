/**
 *  \file
 *  \brief  IO window for text input / output
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
#include "IOWindow.h"
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <stdlib.h>


const TCHAR IOWindow::cClassName[]      = _T("IOWindow");
const int IOWindow::cBackgroundColor    = COLOR_INFOBK;


HINSTANCE IOWindow::HMod = NULL;


/**
 *  \brief
 */
void IOWindow::Register(HINSTANCE hMod)
{
    HMod = hMod;
    if (hMod == NULL)
        GetModuleHandleEx(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_PIN, cClassName, &HMod);

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


/**
 *  \brief
 */
void IOWindow::Unregister()
{
    UnregisterClass(cClassName, HMod);
    FreeLibrary(GetModuleHandle(_T("Riched20.dll")));
}


/**
 *  \brief
 */
bool IOWindow::create(HWND hOwner,
        const TCHAR* font, unsigned fontSize, bool readOnly,
        int minWidth, int minHeight,
        const TCHAR* header, TCHAR* text, int txtLimit)
{
    if (!text)
        return false;

    if (!readOnly)
    {
        HDC hdc = GetWindowDC(hOwner);
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        minHeight = MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72) +
                tm.tmInternalLeading;
        ReleaseDC(hOwner, hdc);
    }

    IOWindow iow(hOwner, readOnly, minWidth, minHeight, text);
    if (iow.composeWindow(font, fontSize, readOnly,
            minWidth, minHeight, header, text, txtLimit) == NULL)
        return false;

    BOOL r;
    MSG msg;
    while ((r = GetMessage(&msg, NULL, 0, 0)) != 0 && r != -1)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return iow._success;
}


/**
 *  \brief
 */
RECT IOWindow::adjustSizeAndPos(DWORD styleEx, DWORD style,
        int width, int height)
{
    if (width <= 0) width = 100;
    if (height <= 0) height = 100;

    RECT maxWin;
    GetWindowRect(GetDesktopWindow(), &maxWin);

    POINT center;
    if (_hOwner)
    {
        RECT biasWin;
        GetWindowRect(_hOwner, &biasWin);
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

    AdjustWindowRectEx(&win, style, FALSE, styleEx);

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
IOWindow::~IOWindow()
{
    if (_hFont)
        DeleteObject(_hFont);
}


/**
 *  \brief
 */
HWND IOWindow::composeWindow(const TCHAR* font, unsigned fontSize,
        bool readOnly, int minWidth, int minHeight,
        const TCHAR* header, const TCHAR* text, int txtLimit)
{
    DWORD styleEx = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;
    DWORD style = WS_POPUP | WS_CAPTION;
    RECT win = adjustSizeAndPos(styleEx, style, minWidth, minHeight);
    _hWnd = CreateWindowEx(styleEx, cClassName, header,
            style, win.left, win.top,
            win.right - win.left, win.bottom - win.top,
            _hOwner, NULL, HMod, (LPVOID) this);
    if (_hWnd == NULL)
        return NULL;

    GetClientRect(_hWnd, &win);

    style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
    if (readOnly)
        style |= ES_MULTILINE | ES_READONLY;
    HWND hEdit = CreateWindowEx(0, RICHEDIT_CLASS, NULL, style, 0, 0,
            win.right - win.left, win.bottom - win.top,
            _hWnd, NULL, HMod, NULL);

    SendMessage(hEdit, EM_SETBKGNDCOLOR, 0,
            (LPARAM)GetSysColor(cBackgroundColor));

    CHARFORMAT fmt  = {0};
    fmt.cbSize      = sizeof(fmt);
    fmt.dwMask      = CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_SIZE;
    fmt.dwEffects   = CFE_AUTOCOLOR;
    fmt.yHeight     = fontSize * 20;
    if (font)
        _tcscpy_s(fmt.szFaceName, _countof(fmt.szFaceName), font);
    SendMessage(hEdit, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&fmt);

    HDC hdc = GetWindowDC(hEdit);
    _hFont = CreateFont(
            -MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, font);
    ReleaseDC(hEdit, hdc);
    if (_hFont)
        SendMessage(hEdit, WM_SETFONT, (WPARAM)_hFont, (LPARAM)TRUE);

    DWORD events = ENM_KEYEVENTS;

    if (readOnly)
    {
        events |= (ENM_MOUSEEVENTS | ENM_REQUESTRESIZE | ENM_LINK);
        SendMessage(hEdit, EM_AUTOURLDETECT, (WPARAM)TRUE, 0);
    }
    else
    {
        SendMessage(hEdit, EM_EXLIMITTEXT, 0, (LPARAM)txtLimit);
    }

    SendMessage(hEdit, EM_SETEVENTMASK, 0, (LPARAM)events);

    if (_tcslen(text))
    {
        Edit_SetText(hEdit, text);
        if (!readOnly)
            Edit_SetSel(hEdit, 0, -1);
    }

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);

    return _hWnd;
}


/**
 *  \brief
 */
int IOWindow::onKeyDown(DWORD key)
{
    switch (key)
    {
        case VK_ESCAPE:
        case VK_TAB:
            SendMessage(_hWnd, WM_CLOSE, 0, 0);
            return 1;

        case VK_RETURN:
        {
            HWND hEdit = GetTopWindow(_hWnd);
            if (!(GetWindowLongPtr(hEdit, GWL_STYLE) & ES_READONLY))
            {
                int len = Edit_GetTextLength(hEdit) + 1;
                if (len > 1)
                {
                    Edit_GetText(hEdit, _text, len);
                    _success = true;
                }
                SendMessage(_hWnd, WM_CLOSE, 0, 0);
                return 1;
            }
        }
    }

    return 0;
}


/**
 *  \brief
 */
int IOWindow::onAutoSize(REQRESIZE* pReqResize)
{
    HWND hEdit = pReqResize->nmhdr.hwndFrom;
    int width = pReqResize->rc.right - pReqResize->rc.left + 30;
    int height = pReqResize->rc.bottom - pReqResize->rc.top;

    if (width < _minWidth) width = _minWidth;
    if (height < _minHeight) height = _minHeight;

    RECT win = IOWindow::adjustSizeAndPos(
            GetWindowLongPtr(_hWnd, GWL_EXSTYLE),
            GetWindowLongPtr(_hWnd, GWL_STYLE), width, height);
    MoveWindow(_hWnd, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);
    GetClientRect(_hWnd, &win);
    MoveWindow(hEdit, 0, 0,
            win.right - win.left, win.bottom - win.top, TRUE);

    DWORD events = SendMessage(hEdit, EM_GETEVENTMASK, 0, 0);
    events &= ~ENM_REQUESTRESIZE;
    SendMessage(hEdit, EM_SETEVENTMASK, 0, (LPARAM) events);

    return 1;
}


/**
 *  \brief
 */
LRESULT APIENTRY IOWindow::wndProc(HWND hwnd, UINT umsg,
        WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
        case WM_CREATE:
        {
            IOWindow* iow =
                    (IOWindow*)((LPCREATESTRUCT)lparam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToUlong(iow));
            return 0;
        }

        case WM_SETFOCUS:
            SetFocus(GetTopWindow(hwnd));
            return 0;

        case WM_COMMAND:
            if (HIWORD(wparam) == EN_SETFOCUS)
            {
                HWND hEdit = GetTopWindow(hwnd);
                if (GetWindowLongPtr(hEdit, GWL_STYLE) & ES_READONLY)
                    DestroyCaret();
                return 0;
            }
            if (HIWORD(wparam) == EN_KILLFOCUS)
            {
                DestroyCaret();
                SendMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }
        break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lparam)->code)
            {
                case EN_MSGFILTER:
                {
                    HWND hEdit = GetTopWindow(hwnd);
                    if (GetWindowLongPtr(hEdit, GWL_STYLE) & ES_READONLY)
                        DestroyCaret();
                    IOWindow* iow =
                            reinterpret_cast<IOWindow*>(static_cast<LONG_PTR>
                                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
                    MSGFILTER* pMsgFilter = (MSGFILTER*)lparam;
                    if (pMsgFilter->msg == WM_KEYDOWN)
                        return iow->onKeyDown(pMsgFilter->wParam);
                    return 0;
                }

                case EN_REQUESTRESIZE:
                {
                    IOWindow* iow = reinterpret_cast<IOWindow*>
                            (static_cast<LONG_PTR>
                            (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
                    return iow->onAutoSize((REQRESIZE*)lparam);
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
                    return 0;
                }
            }
        break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}
