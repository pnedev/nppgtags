/**
 *  \file
 *  \brief  GTags config window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2015 Pavel Nedev
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
#include "ConfigWin.h"
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#include <stdlib.h>
#include "GTags.h"


namespace GTags
{

const TCHAR ConfigWin::cClassName[]   = _T("ConfigWin");
const int ConfigWin::cBackgroundColor = COLOR_BTNFACE;
const TCHAR ConfigWin::cFont[]        = _T("Tahoma");
const int ConfigWin::cFontSize        = 10;


/**
 *  \brief
 */
void ConfigWin::Show(HWND hOwner, Settings* settings)
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

    ConfigWin cw(settings);
    if (cw.composeWindow(hOwner) == NULL)
        return;

    BOOL r;
    MSG msg;
    while ((r = GetMessage(&msg, NULL, 0, 0)) != 0 && r != -1)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


/**
 *  \brief
 */
RECT ConfigWin::adjustSizeAndPos(HWND hOwner, DWORD styleEx, DWORD style,
        int width, int height)
{
    RECT maxWin;
    GetWindowRect(GetDesktopWindow(), &maxWin);

    POINT center;
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
ConfigWin::~ConfigWin()
{
    if (_hWnd)
        UnregisterHotKey(_hWnd, 1);
        UnregisterHotKey(_hWnd, 2);

    if (_hFont)
        DeleteObject(_hFont);

    UnregisterClass(cClassName, HMod);
    FreeLibrary(GetModuleHandle(_T("Riched20.dll")));
}


/**
 *  \brief
 */
HWND ConfigWin::composeWindow(HWND hOwner)
{
    TEXTMETRIC tm;
    HDC hdc = GetWindowDC(hOwner);
    GetTextMetrics(hdc, &tm);
    int txtHeight = MulDiv(cFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72) +
            tm.tmInternalLeading;
    _hFont = CreateFont(
            -MulDiv(cFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, cFont);
    ReleaseDC(hOwner, hdc);

    DWORD styleEx = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;
    DWORD style = WS_POPUP | WS_CAPTION;

    RECT win = adjustSizeAndPos(hOwner, styleEx, style,
            500, 4 * txtHeight + 100);
    int width = win.right - win.left;
    int height = win.bottom - win.top;

    TCHAR header[32] = {VER_PLUGIN_NAME};
    _tcscat_s(header, _countof(header), _T(" Settings"));

    _hWnd = CreateWindowEx(styleEx, cClassName, header,
            style, win.left, win.top, width, height,
            hOwner, NULL, HMod, (LPVOID) this);
    if (_hWnd == NULL)
        return NULL;

    GetClientRect(_hWnd, &win);
    width = win.right - win.left;
    height = win.bottom - win.top;

    int yPos = 10;
    HWND hStatic = CreateWindowEx(0, _T("STATIC"), NULL,
            WS_CHILD | WS_VISIBLE | BS_TEXT | SS_LEFT,
            10, yPos, width - 20, txtHeight, _hWnd, NULL, HMod, NULL);
    SetWindowText(hStatic,
            _T("Parser (requires database re-creation on change)"));

    yPos += (txtHeight + 5);
    _hParser = CreateWindowEx(0, WC_COMBOBOX, NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
            10, yPos, (width / 2) - 20, txtHeight,
            _hWnd, NULL, HMod, NULL);

    _hAutoUpdate = CreateWindowEx(0, _T("BUTTON"),
            _T("Auto update database"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            (width / 2) + 10, yPos + 5, (width / 2) - 20, txtHeight,
            _hWnd, NULL, HMod, NULL);

    yPos += (txtHeight + 25);
    hStatic = CreateWindowEx(0, _T("STATIC"), NULL,
            WS_CHILD | WS_VISIBLE | BS_TEXT | SS_LEFT,
            10, yPos, width - 20, txtHeight, _hWnd, NULL, HMod, NULL);
    SetWindowText(hStatic, _T("Paths to library databases"));

    yPos += (txtHeight + 5);
    styleEx = WS_EX_CLIENTEDGE;
    style = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
    win.top = yPos;
    win.bottom = win.top + txtHeight;
    win.left = 10;
    win.right = width - 10;
    AdjustWindowRectEx(&win, style, FALSE, styleEx);
    _hLibraryDBs = CreateWindowEx(styleEx, RICHEDIT_CLASS, NULL, style,
            win.left, win.top, win.right - win.left, win.bottom - win.top,
            _hWnd, NULL, HMod, NULL);

    yPos += (win.bottom - win.top + 15);
    width = width / 5;
    _hOK = CreateWindowEx(0, _T("BUTTON"), _T("OK"),
            WS_CHILD | WS_VISIBLE | BS_TEXT | BS_DEFPUSHBUTTON,
            width, yPos, width, 25,
            _hWnd, NULL, HMod, NULL);

    _hCancel = CreateWindowEx(0, _T("BUTTON"), _T("Cancel"),
            WS_CHILD | WS_VISIBLE | BS_TEXT,
            3 * width, yPos, width, 25,
            _hWnd, NULL, HMod, NULL);

    SendMessage(_hLibraryDBs, EM_SETBKGNDCOLOR, 0,
            (LPARAM)GetSysColor(COLOR_WINDOW));

    CHARFORMAT fmt  = {0};
    fmt.cbSize      = sizeof(fmt);
    fmt.dwMask      = CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_SIZE;
    fmt.dwEffects   = CFE_AUTOCOLOR;
    fmt.yHeight     = cFontSize * 20;
    _tcscpy_s(fmt.szFaceName, _countof(fmt.szFaceName), cFont);
    SendMessage(_hLibraryDBs, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&fmt);

    if (_hFont)
        SendMessage(_hLibraryDBs, WM_SETFONT, (WPARAM)_hFont, (LPARAM)TRUE);

    SendMessage(_hLibraryDBs, EM_EXLIMITTEXT, 0,
            (LPARAM)(_countof(_settings->_libraryDBsPath) - 1));
    SendMessage(_hLibraryDBs, EM_SETEVENTMASK, 0, 0);

    if (_tcslen(_settings->_libraryDBsPath))
        Edit_SetText(_hLibraryDBs, _settings->_libraryDBsPath);

    if (_hFont)
    {
        SendMessage(_hAutoUpdate, WM_SETFONT, (WPARAM)_hFont, (LPARAM)TRUE);
        SendMessage(_hParser, WM_SETFONT, (WPARAM)_hFont, (LPARAM)TRUE);
    }

    Button_SetCheck(_hAutoUpdate, _settings->_autoUpdate ?
            BST_CHECKED : BST_UNCHECKED);

    for (unsigned i = 0; i < _countof(cParsers); i++)
        SendMessage(_hParser, CB_ADDSTRING, 0, (LPARAM)cParsers[i]);

    SendMessage(_hParser, CB_SETCURSEL, (WPARAM)_settings->_parserIdx, 0);

    RegisterHotKey(_hWnd, 1, 0, VK_ESCAPE);
    RegisterHotKey(_hWnd, 2, 0, VK_RETURN);

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);

    return _hWnd;
}


/**
 *  \brief
 */
void ConfigWin::onOK()
{
    int len = Edit_GetTextLength(_hLibraryDBs) + 1;
    if (len > 1)
        Edit_GetText(_hLibraryDBs, _settings->_libraryDBsPath, len);
    else
        _settings->_libraryDBsPath[0] = 0;

    _settings->_autoUpdate =
            (Button_GetCheck(_hAutoUpdate) == BST_CHECKED) ? true : false;

    _settings->_parserIdx = SendMessage(_hParser, CB_GETCURSEL, 0, 0);

    SendMessage(_hWnd, WM_CLOSE, 0, 0);
}


/**
 *  \brief
 */
LRESULT APIENTRY ConfigWin::wndProc(HWND hwnd, UINT umsg,
        WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
        case WM_CREATE:
        {
            ConfigWin* cw =
                    (ConfigWin*)((LPCREATESTRUCT)lparam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToUlong(cw));
            return 0;
        }

        case WM_HOTKEY:
            if (HIWORD(lparam) == VK_ESCAPE)
            {
                SendMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }
            if (HIWORD(lparam) == VK_RETURN)
            {
                ConfigWin* cw =
                        reinterpret_cast<ConfigWin*>(static_cast<LONG_PTR>
                                (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
                cw->onOK();
                return 0;
            }
        break;

        case WM_COMMAND:
            if (HIWORD(wparam) == EN_KILLFOCUS)
            {
                DestroyCaret();
                return 0;
            }
            if (HIWORD(wparam) == BN_CLICKED)
            {
                ConfigWin* cw =
                        reinterpret_cast<ConfigWin*>(static_cast<LONG_PTR>
                                (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
                if ((HWND)lparam == cw->_hOK)
                {
                    cw->onOK();
                    return 0;
                }
                if ((HWND)lparam == cw->_hCancel)
                {
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;
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
