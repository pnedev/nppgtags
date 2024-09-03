/**
 *  \file
 *  \brief  About window
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
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <richedit.h>
#include "Common.h"
#include "INpp.h"
#include "GTags.h"
#include "AboutWin.h"


namespace GTags
{

const TCHAR AboutWin::cClassName[]      = _T("AboutWin");
const int AboutWin::cBackgroundColor    = COLOR_INFOBK;
const unsigned AboutWin::cFontSize      = 10;

AboutWin* AboutWin::AW = NULL;


/**
 *  \brief
 */
void AboutWin::Show(const TCHAR* info)
{
    if (AW)
    {
        SetFocus(AW->_hWnd);
        return;
    }

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

    HWND hOwner = INpp::Get().GetHandle();

    AW = new AboutWin;
    if (AW->composeWindow(hOwner, info) == NULL)
    {
        delete AW;
        AW = NULL;
    }
}


/**
 *  \brief
 */
AboutWin::~AboutWin()
{
    if (_hFont)
        DeleteObject(_hFont);

    UnregisterClass(cClassName, HMod);
}


/**
 *  \brief
 */
HWND AboutWin::composeWindow(HWND hOwner, const TCHAR* info)
{
    DWORD styleEx   = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;
    DWORD style     = WS_POPUP | WS_CAPTION | WS_SYSMENU;

    RECT win = Tools::GetWinRect(hOwner, styleEx, style, 300, 200);

    _hWnd = CreateWindowEx(styleEx, cClassName, _T("About ") PLUGIN_NAME, style,
            win.left, win.top, win.right - win.left, win.bottom - win.top,
            hOwner, NULL, HMod, NULL);
    if (_hWnd == NULL)
        return NULL;

    INpp::Get().RegisterWinForDarkMode(_hWnd);

    GetClientRect(_hWnd, &win);

    style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_NOOLEDRAGDROP;

    HWND hEdit = CreateWindowEx(0, RICHEDIT_CLASS, NULL, style,
            0, 0, win.right - win.left, win.bottom - win.top,
            _hWnd, NULL, HMod, NULL);

    SendMessage(hEdit, EM_SETBKGNDCOLOR, 0, GetSysColor(cBackgroundColor));

    HDC hdc = GetWindowDC(hEdit);
    _hFont = Tools::CreateFromSystemMessageFont(hdc, cFontSize);
    ReleaseDC(hEdit, hdc);

    if (_hFont)
        SendMessage(hEdit, WM_SETFONT, (WPARAM)_hFont, TRUE);

    DWORD events = ENM_KEYEVENTS | ENM_MOUSEEVENTS | ENM_REQUESTRESIZE | ENM_LINK;
    SendMessage(hEdit, EM_SETEVENTMASK, 0, events);
    SendMessage(hEdit, EM_AUTOURLDETECT, TRUE, 0);

    CText str(_T("\n"));
    str += VER_DESCRIPTION;
    str += _T("\n\nVersion: ");
    str += VER_VERSION_STR;
    str += _T("\nBuild date: ");
    str += _T(__DATE__);
    str += _T(',');
    str += _T(' ');
    str += _T(__TIME__);
    str += _T('\n');
    str += VER_COPYRIGHT;
    str += _T(" <pg.nedev@gmail.com>");
    str += _T("\nURL: ");
    str += VER_URL;
    str += _T("\n\nUser Guide: ");
    str += VER_USER_GUIDE;
    str += _T("\n\nLicensed under GNU GPLv2 as published by the Free Software Foundation.\n\n")
            _T("This plugin is frontend to GNU Global source code tagging system (GTags):\n")
            _T("https://www.gnu.org/software/global\n")
            _T("Windows ported GTags binaries can be found at:\n")
            _T("http://adoxa.altervista.org/global\n\n")
            _T("Thanks to:\n\tShigio YAMAGUCHI (the GNU Global developer) and all GTags contributors;\n")
            _T("\tJason Hood for porting GTags to Windows.\n")
            _T("Thanks also to all Universal Ctags parser developers.\n\n")
            _T("Current GTags version:\n\n");
    str += info;

    Edit_SetText(hEdit, str.C_str());

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);

    return _hWnd;
}


/**
 *  \brief
 */
LRESULT APIENTRY AboutWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_COMMAND:
            if (HIWORD(wParam) == EN_SETFOCUS)
            {
                DestroyCaret();
                SetFocus(hWnd);
                return 0;
            }
        break;

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                SendMessage(hWnd, WM_CLOSE, 0, 0);
                return 0;
            }
        break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case EN_MSGFILTER:
                {
                    DestroyCaret();
                    MSGFILTER* pMsgFilter = (MSGFILTER*)lParam;
                    if ((pMsgFilter->msg != WM_LBUTTONDOWN) && (pMsgFilter->msg != WM_LBUTTONUP))
                        return 1;
                }
                break;

                case EN_REQUESTRESIZE:
                {
                    REQRESIZE* pReqResize = (REQRESIZE*)lParam;
                    HWND hEdit  = pReqResize->nmhdr.hwndFrom;
                    int width   = pReqResize->rc.right - pReqResize->rc.left + 30;
                    int height  = pReqResize->rc.bottom - pReqResize->rc.top;

                    RECT win = Tools::GetWinRect(GetParent(hWnd), (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE),
                            (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE), width, height);
                    MoveWindow(hWnd, win.left, win.top, win.right - win.left, win.bottom - win.top, TRUE);
                    GetClientRect(hWnd, &win);
                    MoveWindow(hEdit, 0, 0, win.right - win.left, win.bottom - win.top, TRUE);
                }
                return 1;

                case EN_LINK:
                {
                    ENLINK* pEnLink = (ENLINK*)lParam;
                    if (pEnLink->msg == WM_LBUTTONUP)
                    {
                        CText link(pEnLink->chrg.cpMax - pEnLink->chrg.cpMin);
                        TEXTRANGE range;
                        range.chrg = pEnLink->chrg;
                        range.lpstrText = link.C_str();
                        SendMessage(pEnLink->nmhdr.hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM) &range);
                        ShellExecute(NULL, _T("open"), link.C_str(), NULL, NULL, SW_SHOWNORMAL);
                        return 1;
                    }
                }
            }
        break;

        case WM_DESTROY:
            DestroyCaret();
            delete AW;
            AW = NULL;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
