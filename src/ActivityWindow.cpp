/**
 *  \file
 *  \brief  Process Activity Window
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


#pragma comment (lib, "comctl32")


#define WIN32_LEAN_AND_MEAN
#include "ActivityWindow.h"
#include <windowsx.h>
#include <commctrl.h>


const TCHAR ActivityWindow::cClassName[]    = _T("ActivityWindow");
const unsigned ActivityWindow::cUpdate_ms   = 50;
const TCHAR ActivityWindow::cFontName[]     = _T("Tahoma");
const unsigned ActivityWindow::cFontSize    = 8;
const int ActivityWindow::cBackgroundColor  = COLOR_WINDOW;


volatile LONG ActivityWindow::RefCount = 0;


/**
 *  \brief
 */
int ActivityWindow::Show(HINSTANCE hInst, HWND hOwnerWnd, int width,
        const TCHAR* text, HANDLE procHndl, int showDelay_ms)
{
    if (!procHndl)
        return -1;

    DWORD dwRet;
    GetExitCodeProcess(procHndl, &dwRet);
    if (dwRet != STILL_ACTIVE)
        return 0;

    if (InterlockedIncrement(&RefCount) == 1)
    {
        WNDCLASS wc         = {0};
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = wndProc;
        wc.hInstance        = hInst;
        wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = GetSysColorBrush(cBackgroundColor);
        wc.lpszClassName    = cClassName;

        if (!RegisterClass(&wc))
        {
            InterlockedDecrement(&RefCount);
            return -1;
        }

        INITCOMMONCONTROLSEX icex   = {0};
        icex.dwSize                 = sizeof(icex);
        icex.dwICC                  = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;

        InitCommonControlsEx(&icex);
    }

    ActivityWindow aw(hOwnerWnd, procHndl, showDelay_ms, (int)RefCount);
    aw.composeWindow(hInst, width, text, showDelay_ms);

    BOOL r;
    MSG msg;
    while ((r = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (r == -1)
            break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return aw._exitCode;
}


/**
 *  \brief
 */
void ActivityWindow::adjustSizeAndPos(HWND hwnd, int width, int height)
{
    RECT win, maxWin;
    bool noAdjust = false;

    GetWindowRect(_hOwnerWnd, &maxWin);

    win = maxWin;
    if (width <= 0) width = 400;

    if ((int) width < maxWin.right - maxWin.left)
        win.right = win.left + width;
    else
        noAdjust = true;

    win.bottom = win.top + height;

    if (!noAdjust)
    {
        AdjustWindowRect(&win, GetWindowLong(hwnd, GWL_STYLE), FALSE);

        width = win.right - win.left;
        height = win.bottom - win.top;

        if (width < maxWin.right - maxWin.left)
        {
            win.left = maxWin.left;
            win.right = win.left + width;
        }
        else
        {
            win.left = maxWin.left;
            win.right = maxWin.right;
        }

        win.top = maxWin.bottom - (_initRefCount * height);
        win.bottom = win.top + height;
    }

    MoveWindow(hwnd, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);
}


/**
 *  \brief
 */
ActivityWindow::~ActivityWindow()
{
    if (_hFont)
        DeleteObject(_hFont);
    if (InterlockedDecrement(&RefCount) == 0)
        UnregisterClass(cClassName, NULL);
}


/**
 *  \brief
 */
HWND ActivityWindow::composeWindow(HINSTANCE hInst, int width,
        const TCHAR* text, int showDelay_ms)
{
    HWND hMainWin = CreateWindow(cClassName, NULL,
            WS_POPUP | WS_BORDER, 1, 1, 10, 10,
            _hOwnerWnd, NULL, hInst, (LPVOID)this);

    HWND hWndEdit = CreateWindowEx(0, _T("EDIT"), _T("Text"),
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_READONLY,
            0, 0, 5, 5,
            hMainWin, NULL, hInst, NULL);

    HDC hdc = GetWindowDC(_hOwnerWnd);
    _hFont = CreateFont(
            -MulDiv(cFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, cFontName);
    ReleaseDC(_hOwnerWnd, hdc);
    if (_hFont)
        SendMessage(hWndEdit, WM_SETFONT, (WPARAM)_hFont, (LPARAM)TRUE);

    hdc = GetWindowDC(hWndEdit);
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    ReleaseDC(hWndEdit, hdc);

    int textHeight = tm.tmHeight;
    adjustSizeAndPos(hMainWin, width, textHeight + 25);

    RECT win;
    GetClientRect(hMainWin, &win);
    width = win.right - win.left;
    int height = win.bottom - win.top;

    MoveWindow(hWndEdit, 5, 5, width - 95, textHeight, TRUE);

    Edit_SetText(hWndEdit, text);

    HWND hPBar = CreateWindowEx(0, PROGRESS_CLASS, _T("Progress Bar"),
            WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
            5, textHeight + 10, width - 95, 10,
            hMainWin, NULL, hInst, NULL);
    SendMessage(hPBar, PBM_SETMARQUEE, (WPARAM)TRUE, (LPARAM)50);

    _hBtn = CreateWindowEx(0, _T("BUTTON"), _T("Cancel"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_TEXT,
            width - 85, (height - 25) / 2, 80, 25, hMainWin,
            NULL, hInst, NULL);

    _timerID = SetTimer(hMainWin, 0, cUpdate_ms, NULL);

    if (showDelay_ms <= 0)
    {
        ShowWindow(hMainWin, SW_SHOWNORMAL);
        UpdateWindow(hMainWin);
        SetFocus(_hOwnerWnd);
    }

    return hMainWin;
}


/**
 *  \brief
 */
void ActivityWindow::onTimerRefresh(HWND hwnd)
{
    DWORD dwRet;
    GetExitCodeProcess(_hProc, &dwRet);
    if (dwRet != STILL_ACTIVE)
    {
        _exitCode = 0;
        KillTimer(hwnd, _timerID);
        SendMessage(hwnd, WM_CLOSE, 0, 0);
    }
    else if (_showDelay_ms > 0)
    {
        _showDelay_ms -= cUpdate_ms;
        if (_showDelay_ms <= 0)
        {
            ShowWindow(hwnd, SW_SHOWNORMAL);
            UpdateWindow(hwnd);
            SetFocus(_hOwnerWnd);
        }
    }
    else
    {
        int refCount = (int)RefCount;
        if (_initRefCount > refCount)
        {
            _initRefCount = refCount;
            RECT win;
            GetClientRect(hwnd, &win);
            adjustSizeAndPos(hwnd,
                    win.right - win.left, win.bottom - win.top);
        }
    }
}


/**
 *  \brief
 */
LRESULT APIENTRY ActivityWindow::wndProc(HWND hwnd, UINT umsg,
        WPARAM wparam, LPARAM lparam)
{
    ActivityWindow* aw;

    switch (umsg)
    {
        case WM_CREATE:
            aw = (ActivityWindow*)((LPCREATESTRUCT)lparam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToUlong(aw));
            return 0;

        case WM_SETFOCUS:
            aw = reinterpret_cast<ActivityWindow*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            SetFocus(aw->_hBtn);
            return 0;

        case WM_CTLCOLORSTATIC:
            SetBkColor((HDC) wparam, GetSysColor(cBackgroundColor));
            return (INT_PTR) GetSysColorBrush(cBackgroundColor);

        case WM_TIMER:
            aw = reinterpret_cast<ActivityWindow*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            aw->onTimerRefresh(hwnd);
            return 0;

        case WM_COMMAND:
            if (HIWORD(wparam) == BN_CLICKED) {
                aw = reinterpret_cast<ActivityWindow*>(static_cast<LONG_PTR>
                        (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
                KillTimer(hwnd, aw->_timerID);
                SendMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}
