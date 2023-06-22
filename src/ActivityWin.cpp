/**
 *  \file
 *  \brief  Process Activity window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2016 Pavel Nedev
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


#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "Common.h"
#include "INpp.h"
#include "GTags.h"
#include "ActivityWin.h"


namespace GTags
{

const TCHAR ActivityWin::cClassName[]   = _T("ActivityWin");
const int ActivityWin::cBackgroundColor = COLOR_WINDOW;
const unsigned ActivityWin::cFontSize   = 8;
const int ActivityWin::cWidth           = 600;


std::list<ActivityWin*> ActivityWin::WindowList;
HFONT                   ActivityWin::HFont = NULL;
unsigned                ActivityWin::TxtHeight = 0;


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
void ActivityWin::Show(const TCHAR* text, HANDLE hCancel)
{
    if (!hCancel)
        return;

    ActivityWin* aw = new ActivityWin(hCancel);
    if (aw->composeWindow(text) == NULL)
        delete aw;
}


/**
 *  \brief
 */
void ActivityWin::UpdatePositions()
{
    int i = 1;
    for (auto iWin = WindowList.begin(); iWin != WindowList.end(); ++iWin, ++i)
        (*iWin)->onResize(i);
}


/**
 *  \brief
 */
HWND ActivityWin::GetHwnd(HANDLE hCancel)
{
    for (auto iWin = WindowList.begin(); iWin != WindowList.end(); ++iWin)
    {
        if ((*iWin)->_hCancel == hCancel)
            return (*iWin)->_hWnd;
    }

    return NULL;
}


/**
 *  \brief
 */
ActivityWin::~ActivityWin()
{
    for (auto iWin = WindowList.begin(); iWin != WindowList.end(); ++iWin)
    {
        if (*iWin == this)
        {
            WindowList.erase(iWin);
            break;
        }
    }

    if (WindowList.empty())
    {
        if (HFont)
            DeleteObject(HFont);
    }
    else
    {
        int i = 1;
        for (auto iWin = WindowList.begin(); iWin != WindowList.end(); ++iWin, ++i)
            (*iWin)->onResize(i);
    }
}


/**
 *  \brief
 */
void ActivityWin::adjustSizeAndPos(int width, int height, int winNum)
{
    HWND hBias = INpp::Get().GetSciHandle(0);

    RECT maxWin;
    GetWindowRect(hBias, &maxWin);

    RECT win = {0};
    win.right = width;
    win.bottom = height;

    AdjustWindowRect(&win, (DWORD)GetWindowLongPtr(_hWnd, GWL_STYLE), FALSE);

    width = win.right - win.left;
    height = win.bottom - win.top;
    win.left = maxWin.left;
    win.right = win.left + width;

    if (win.right > maxWin.right)
        win.right = maxWin.right;

    win.top = maxWin.bottom - (winNum * height);
    win.bottom = win.top + height;

    MoveWindow(_hWnd, win.left, win.top, win.right - win.left, win.bottom - win.top, TRUE);
}


/**
 *  \brief
 */
HWND ActivityWin::composeWindow(const TCHAR* text)
{
    _hWnd = CreateWindow(cClassName, NULL, WS_POPUP | WS_BORDER,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            INpp::Get().GetHandle(), NULL, HMod, this);
    if (_hWnd == NULL)
        return NULL;

    INpp::Get().RegisterWinForDarkMode(_hWnd);

    if (WindowList.empty())
    {
        HWND hParent = INpp::Get().GetHandle();
        HDC hdc = GetWindowDC(hParent);
        HFont = Tools::CreateFromSystemMessageFont(hdc, cFontSize);
        TxtHeight = Tools::GetFontHeight(hdc, HFont);
        ReleaseDC(hParent, hdc);
    }

    WindowList.push_back(this);
    int winNum = (int)WindowList.size();

    HWND hWndTxt = CreateWindowEx(0, _T("STATIC"), text,
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_PATHELLIPSIS,
            0, 0, 0, 0, _hWnd, NULL, HMod, NULL);

    adjustSizeAndPos(cWidth, TxtHeight + 25, winNum);

    RECT win;
    GetClientRect(_hWnd, &win);
    int width = win.right - win.left;
    int height = win.bottom - win.top;

    MoveWindow(hWndTxt, 5, 5, width - 95, TxtHeight, TRUE);

    HWND hPBar = CreateWindowEx(0, PROGRESS_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
            5, TxtHeight + 10, width - 95, 10,
            _hWnd, NULL, HMod, NULL);
    SendMessage(hPBar, PBM_SETMARQUEE, TRUE, 100);

    _hBtn = CreateWindowEx(0, _T("BUTTON"), _T("Cancel"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            width - 85, (height - 25) / 2, 80, 25, _hWnd,
            NULL, HMod, NULL);

    if (HFont)
    {
        SendMessage(hWndTxt, WM_SETFONT, (WPARAM)HFont, TRUE);
        SendMessage(_hBtn, WM_SETFONT, (WPARAM)HFont, TRUE);
    }

    ShowWindow(_hWnd, SW_SHOWNOACTIVATE);
    UpdateWindow(_hWnd);

    return _hWnd;
}


/**
 *  \brief
 */
void ActivityWin::onResize(int winNum)
{
    RECT win;
    GetClientRect(_hWnd, &win);
    adjustSizeAndPos(win.right - win.left, win.bottom - win.top, winNum);
}


/**
 *  \brief
 */
LRESULT APIENTRY ActivityWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            ActivityWin* aw = (ActivityWin*)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(aw));
        }
        return 0;

        case WM_SETFOCUS:
        {
            ActivityWin* aw = reinterpret_cast<ActivityWin*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            SetFocus(aw->_hBtn);
        }
        return 0;

        case WM_CTLCOLORSTATIC:
            SetBkColor((HDC) wParam, GetSysColor(cBackgroundColor));
        return (INT_PTR) GetSysColorBrush(cBackgroundColor);

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                ActivityWin* aw = reinterpret_cast<ActivityWin*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
                EnableWindow(aw->_hBtn, FALSE);
                SetEvent(aw->_hCancel);
                return 0;
            }
        break;

        case WM_DESTROY:
        {
            ActivityWin* aw = reinterpret_cast<ActivityWin*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            delete aw;
        }
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
