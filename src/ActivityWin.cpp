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
#include <windowsx.h>
#include <commctrl.h>
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
HWND ActivityWin::Create(HWND hOwner, HANDLE hTerminate,
        int width, const TCHAR* text, int showDelay_ms)
{
    if (!hTerminate)
        return NULL;

    ActivityWin* aw = new ActivityWin(hOwner, hTerminate, showDelay_ms);
    HWND hWnd = aw->composeWindow(width, text);
    if (hWnd == NULL)
    {
        delete aw;
        return NULL;
    }

    return hWnd;
}


/**
 *  \brief
 */
ActivityWin::ActivityWin(HWND hOwner, HANDLE hTerminate, int showDelay_ms) :
        _hTerm(hTerminate), _hFont(NULL), _hOwner(hOwner), _hWnd(NULL),
        _timerId(0), _showDelay_ms(showDelay_ms)
{
    _initRefCount = InterlockedIncrement(&RefCount);
}


/**
 *  \brief
 */
ActivityWin::~ActivityWin()
{
    if (_timerId)
        KillTimer(_hWnd, _timerId);

    if (_hFont)
        DeleteObject(_hFont);

    InterlockedDecrement(&RefCount);
}


/**
 *  \brief
 */
void ActivityWin::adjustSizeAndPos(int width, int height)
{
    HWND hBias = _hOwner ? _hOwner : GetDesktopWindow();

    RECT maxWin;
    GetWindowRect(hBias, &maxWin);

    RECT win = {0};
    win.right = width;
    win.bottom = height;

    AdjustWindowRect(&win, GetWindowLongPtr(_hWnd, GWL_STYLE), FALSE);

    width = win.right - win.left;
    height = win.bottom - win.top;
    win.left = maxWin.left;
    win.right = win.left + width;

    if (win.right > maxWin.right)
        win.right = maxWin.right;

    win.top = maxWin.bottom - (_initRefCount * height);
    win.bottom = win.top + height;

    MoveWindow(_hWnd, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);
}


/**
 *  \brief
 */
HWND ActivityWin::composeWindow(int width, const TCHAR* text)
{
    _hWnd = CreateWindow(cClassName, NULL,
            WS_POPUP | WS_BORDER,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            _hOwner, NULL, HMod, (LPVOID)this);
    if (_hWnd == NULL)
        return NULL;

    HWND hWndTxt = CreateWindowEx(0, _T("STATIC"), NULL,
            WS_CHILD | WS_VISIBLE | BS_TEXT | SS_LEFT | SS_PATHELLIPSIS,
            0, 0, 0, 0, _hWnd, NULL, HMod, NULL);

    HDC hdc = GetWindowDC(hWndTxt);
    _hFont = CreateFont(
            -MulDiv(cFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, cFont);
    if (_hFont)
        SendMessage(hWndTxt, WM_SETFONT, (WPARAM)_hFont, (LPARAM)TRUE);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    ReleaseDC(hWndTxt, hdc);

    int textHeight = tm.tmHeight;
    adjustSizeAndPos(width, textHeight + 25);

    RECT win;
    GetClientRect(_hWnd, &win);
    width = win.right - win.left;
    int height = win.bottom - win.top;

    MoveWindow(hWndTxt, 5, 5, width - 95, textHeight, TRUE);

    SetWindowText(hWndTxt, text);

    HWND hPBar = CreateWindowEx(0, PROGRESS_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
            5, textHeight + 10, width - 95, 10,
            _hWnd, NULL, HMod, NULL);
    SendMessage(hPBar, PBM_SETMARQUEE, (WPARAM)TRUE, (LPARAM)50);

    _hBtn = CreateWindowEx(0, _T("BUTTON"), _T("Cancel"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_TEXT,
            width - 85, (height - 25) / 2, 80, 25, _hWnd,
            NULL, HMod, NULL);

    if (_showDelay_ms)
        _timerId = SetTimer(_hWnd, 0, _showDelay_ms, NULL);
    else
        showWindow();

    return _hWnd;
}


/**
 *  \brief
 */
void ActivityWin::showWindow()
{
    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);
    if (_hOwner)
        SetFocus(_hOwner);
}


/**
 *  \brief
 */
void ActivityWin::onTimer()
{
    int refCount = (int)RefCount;
    if (_initRefCount > refCount)
    {
        _initRefCount = refCount;
        RECT win;
        GetClientRect(_hWnd, &win);
        adjustSizeAndPos(win.right - win.left, win.bottom - win.top);
    }

    KillTimer(_hWnd, _timerId);
    _timerId = 0;
    showWindow();
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

        case WM_CTLCOLORSTATIC:
            SetBkColor((HDC) wParam, GetSysColor(cBackgroundColor));
        return (INT_PTR) GetSysColorBrush(cBackgroundColor);

        case WM_TIMER:
            aw = reinterpret_cast<ActivityWin*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hWnd, GWLP_USERDATA)));
            aw->onTimer();
        return 0;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                aw = reinterpret_cast<ActivityWin*>(static_cast<LONG_PTR>
                        (GetWindowLongPtr(hWnd, GWLP_USERDATA)));
                EnableWindow(aw->_hBtn, FALSE);
                SetEvent(aw->_hTerm);
                return 0;
            }
        break;

        case WM_DESTROY:
            aw = reinterpret_cast<ActivityWin*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hWnd, GWLP_USERDATA)));
            delete aw;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
