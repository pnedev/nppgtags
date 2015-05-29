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


#pragma comment (lib, "comctl32")


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "INpp.h"
#include "GTags.h"
#include "ActivityWin.h"


namespace GTags
{

const TCHAR ActivityWin::cClassName[]   = _T("ActivityWin");
const TCHAR ActivityWin::cFont[]        = _T("Tahoma");
const unsigned ActivityWin::cFontSize   = 8;
const int ActivityWin::cBackgroundColor = COLOR_WINDOW;


volatile LONG ActivityWin::RefCount = 0;


/**
 *  \brief
 */
void ActivityWin::Register()
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
    icex.dwICC                  = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;

    InitCommonControlsEx(&icex);
}


/**
 *  \brief
 */
void ActivityWin::Unregister()
{
    UnregisterClass(cClassName, HMod);
}


/**
 *  \brief
 */
bool ActivityWin::Show(HANDLE hActivity, int width, const TCHAR* text,
        int showAfter_ms)
{
    if (!hActivity)
        return false;

    if (WaitForSingleObject(hActivity, showAfter_ms) == WAIT_OBJECT_0)
        return false;

    ActivityWin aw;
    HWND hWnd = aw.composeWindow(width, text);
    if (hWnd == NULL)
        return true;

    while (1)
    {
        // Wait for window event or activity signal
        DWORD r = MsgWaitForMultipleObjects(1, &hActivity, FALSE, INFINITE,
                QS_ALLINPUT);

        // Post close message if event is not related to the window
        if (r != WAIT_OBJECT_0 + 1)
            PostMessage(hWnd, WM_CLOSE, 0, 0);

        // Handle all window messages
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Window closed - exit
        if (msg.message == WM_QUIT)
            break;
    }

    return aw._isCancelled;
}


/**
 *  \brief
 */
ActivityWin::ActivityWin() : _hFont(NULL), _isCancelled(false)
{
    _initRefCount = InterlockedIncrement(&RefCount);
}


/**
 *  \brief
 */
ActivityWin::~ActivityWin()
{
    if (_hFont)
        DeleteObject(_hFont);

    InterlockedDecrement(&RefCount);
}


/**
 *  \brief
 */
void ActivityWin::adjustSizeAndPos(HWND hWnd, int width, int height)
{
    HWND hBias = INpp::Get().GetSciHandle();

    RECT maxWin;
    GetWindowRect(hBias, &maxWin);

    RECT win = {0};
    win.right = width;
    win.bottom = height;

    AdjustWindowRect(&win, GetWindowLongPtr(hWnd, GWL_STYLE), FALSE);

    width = win.right - win.left;
    height = win.bottom - win.top;
    win.left = maxWin.left;
    win.right = win.left + width;

    if (win.right > maxWin.right)
        win.right = maxWin.right;

    win.top = maxWin.bottom - (_initRefCount * height);
    win.bottom = win.top + height;

    MoveWindow(hWnd, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);
}


/**
 *  \brief
 */
HWND ActivityWin::composeWindow(int width, const TCHAR* text)
{
    HWND hWnd = CreateWindow(cClassName, NULL,
            WS_POPUP | WS_BORDER,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            INpp::Get().GetHandle(), NULL, HMod, (LPVOID)this);
    if (hWnd == NULL)
        return NULL;

    HWND hWndTxt = CreateWindowEx(0, _T("STATIC"), NULL,
            WS_CHILD | WS_VISIBLE | BS_TEXT | SS_LEFT | SS_PATHELLIPSIS,
            0, 0, 0, 0, hWnd, NULL, HMod, NULL);

    HDC hdc = GetWindowDC(hWndTxt);
    _hFont = CreateFont(
            -MulDiv(cFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, cFont);
    if (_hFont)
        SendMessage(hWndTxt, WM_SETFONT, (WPARAM)_hFont, TRUE);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    ReleaseDC(hWndTxt, hdc);

    int textHeight = tm.tmHeight;
    adjustSizeAndPos(hWnd, width, textHeight + 25);

    RECT win;
    GetClientRect(hWnd, &win);
    width = win.right - win.left;
    int height = win.bottom - win.top;

    MoveWindow(hWndTxt, 5, 5, width - 95, textHeight, TRUE);

    SetWindowText(hWndTxt, text);

    HWND hPBar = CreateWindowEx(0, PROGRESS_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
            5, textHeight + 10, width - 95, 10,
            hWnd, NULL, HMod, NULL);
    SendMessage(hPBar, PBM_SETMARQUEE, TRUE, 50);

    _hBtn = CreateWindowEx(0, _T("BUTTON"), _T("Cancel"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_TEXT,
            width - 85, (height - 25) / 2, 80, 25, hWnd,
            NULL, HMod, NULL);

    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);

    return hWnd;
}


/**
 *  \brief
 */
void ActivityWin::onResize(HWND hWnd)
{
    int refCount = (int)RefCount;
    if (_initRefCount > refCount)
    {
        _initRefCount = refCount;
        RECT win;
        GetClientRect(hWnd, &win);
        adjustSizeAndPos(hWnd, win.right - win.left, win.bottom - win.top);
    }
}


/**
 *  \brief
 */
LRESULT APIENTRY ActivityWin::wndProc(HWND hWnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
    ActivityWin* aw;

    switch (uMsg)
    {
        case WM_CREATE:
            aw = (ActivityWin*)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, PtrToUlong(aw));
        return 0;

        case WM_SETFOCUS:
            aw = reinterpret_cast<ActivityWin*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hWnd, GWLP_USERDATA)));
            SetFocus(aw->_hBtn);
        return 0;

        case WM_PAINT:
            aw = reinterpret_cast<ActivityWin*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hWnd, GWLP_USERDATA)));
            aw->onResize(hWnd);
        break;

        case WM_CTLCOLORSTATIC:
            SetBkColor((HDC) wParam, GetSysColor(cBackgroundColor));
        return (INT_PTR) GetSysColorBrush(cBackgroundColor);

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                aw = reinterpret_cast<ActivityWin*>(static_cast<LONG_PTR>
                        (GetWindowLongPtr(hWnd, GWLP_USERDATA)));
                EnableWindow(aw->_hBtn, FALSE);
                aw->_isCancelled = true;
                SendMessage(hWnd, WM_CLOSE, 0, 0);
                return 0;
            }
        break;

        case WM_DESTROY:
            PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
