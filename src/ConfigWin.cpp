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


#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#include <vector>
#include "tstring.h"
#include "Common.h"
#include "INpp.h"
#include "Config.h"
#include "GTags.h"
#include "ConfigWin.h"


namespace GTags
{

const TCHAR ConfigWin::cClassName[]   = _T("ConfigWin");
const int ConfigWin::cBackgroundColor = COLOR_BTNFACE;
const int ConfigWin::cFontSize        = 10;


ConfigWin* ConfigWin::CW = NULL;


/**
 *  \brief
 */
void ConfigWin::Show(CConfig* cfg)
{
    if (CW)
    {
        SetFocus(CW->_hWnd);
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

    CW = new ConfigWin(cfg);
    if (CW->composeWindow(hOwner) == NULL)
    {
        delete CW;
        CW = NULL;
    }
}


/**
 *  \brief
 */
RECT ConfigWin::adjustSizeAndPos(HWND hOwner, DWORD styleEx, DWORD style, int width, int height)
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
    if (_hKeyHook)
        UnhookWindowsHookEx(_hKeyHook);

    if (_hFont)
        DeleteObject(_hFont);

    UnregisterClass(cClassName, HMod);
}


/**
 *  \brief
 */
HWND ConfigWin::composeWindow(HWND hOwner)
{
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

    TEXTMETRIC tm;

    HDC hdc = GetWindowDC(hOwner);
    ncm.lfMessageFont.lfHeight = -MulDiv(cFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    GetTextMetrics(hdc, &tm);
    ReleaseDC(hOwner, hdc);

    int txtHeight = tm.tmInternalLeading - ncm.lfMessageFont.lfHeight;

    DWORD styleEx   = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;
    DWORD style     = WS_POPUP | WS_CAPTION | WS_SYSMENU;

    RECT win = adjustSizeAndPos(hOwner, styleEx, style, 500, 5 * txtHeight + 120);
    int width = win.right - win.left;
    int height = win.bottom - win.top;

    tstring header(VER_PLUGIN_NAME);
    header += _T(" Settings");

    _hWnd = CreateWindowEx(styleEx, cClassName, header.c_str(),
            style, win.left, win.top, width, height,
            hOwner, NULL, HMod, NULL);
    if (_hWnd == NULL)
        return NULL;

    GetClientRect(_hWnd, &win);
    width = win.right - win.left;
    height = win.bottom - win.top;

    int yPos = 10;
    HWND hStatic = CreateWindowEx(0, _T("STATIC"), NULL,
            WS_CHILD | WS_VISIBLE | BS_TEXT | SS_LEFT,
            10, yPos, width - 20, txtHeight, _hWnd, NULL, HMod, NULL);

    SetWindowText(hStatic, _T("Parser (requires database re-creation on change!)"));

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

    yPos += (txtHeight + 35);
    _hEnLibDb = CreateWindowEx(0, _T("BUTTON"),
            _T("Enable library databases"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            10, yPos, (width / 2) - 20, txtHeight,
            _hWnd, NULL, HMod, NULL);

    _hCreateDb = CreateWindowEx(0, _T("BUTTON"), _T("Add Library DB"),
            WS_CHILD | WS_VISIBLE | BS_TEXT,
            (width / 2) + 10, yPos, (width / 2) - 20, 25,
            _hWnd, NULL, HMod, NULL);

    yPos += (((txtHeight > 25) ? txtHeight : 25) + 10);
    hStatic = CreateWindowEx(0, _T("STATIC"), NULL,
            WS_CHILD | WS_VISIBLE | BS_TEXT | SS_LEFT,
            10, yPos, width - 20, txtHeight, _hWnd, NULL, HMod, NULL);

    SetWindowText(hStatic, _T("Paths to library databases (';' separated)"));

    yPos += (txtHeight + 5);
    styleEx     = WS_EX_CLIENTEDGE;
    style       = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NOOLEDRAGDROP;
    win.top     = yPos;
    win.bottom  = win.top + txtHeight;
    win.left    = 10;
    win.right   = width - 10;

    AdjustWindowRectEx(&win, style, FALSE, styleEx);
    _hLibDb = CreateWindowEx(styleEx, RICHEDIT_CLASS, NULL, style,
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

    CHARFORMAT fmt  = {0};
    fmt.cbSize      = sizeof(fmt);
    fmt.dwMask      = CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_SIZE;
    fmt.dwEffects   = CFE_AUTOCOLOR;
    fmt.yHeight     = cFontSize * 20;
    _tcscpy_s(fmt.szFaceName, _countof(fmt.szFaceName), ncm.lfMessageFont.lfFaceName);

    SendMessage(_hLibDb, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&fmt);

    _hFont = CreateFontIndirect(&ncm.lfMessageFont);

    if (_hFont)
        SendMessage(_hLibDb, WM_SETFONT, (WPARAM)_hFont, TRUE);

    SendMessage(_hLibDb, EM_SETEVENTMASK, 0, 0);

    if (_cfg->_libDbPath.Len())
        Edit_SetText(_hLibDb, _cfg->_libDbPath.C_str());
    if (!_cfg->_useLibDb)
    {
        EnableWindow(_hCreateDb, FALSE);
        Edit_Enable(_hLibDb, FALSE);
        SendMessage(_hLibDb, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    }
    else
    {
        SendMessage(_hLibDb, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_WINDOW));
    }

    if (_hFont)
    {
        SendMessage(_hAutoUpdate, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hParser, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hEnLibDb, WM_SETFONT, (WPARAM)_hFont, TRUE);
    }

    Button_SetCheck(_hAutoUpdate, _cfg->_autoUpdate ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(_hEnLibDb, _cfg->_useLibDb ? BST_CHECKED : BST_UNCHECKED);

    for (unsigned i = 0; CConfig::Parser(i); i++)
        SendMessage(_hParser, CB_ADDSTRING, 0, (LPARAM)CConfig::Parser(i));

    SendMessage(_hParser, CB_SETCURSEL, _cfg->_parserIdx, 0);

    _hKeyHook = SetWindowsHookEx(WH_KEYBOARD, keyHookProc, NULL, GetCurrentThreadId());

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);

    return _hWnd;
}


/**
 *  \brief
 */
void ConfigWin::onOK()
{
    int len = Edit_GetTextLength(_hLibDb);
    if (len)
    {
        std::vector<TCHAR> buf;
        buf.resize(len + 1);
        Edit_GetText(_hLibDb, buf.data(), buf.size());
        _cfg->_libDbPath = buf.data();
    }
    else
    {
        _cfg->_libDbPath = _T("");
    }

    _cfg->_autoUpdate   = (Button_GetCheck(_hAutoUpdate) == BST_CHECKED) ? true : false;
    _cfg->_useLibDb     = (Button_GetCheck(_hEnLibDb) == BST_CHECKED) ? true : false;

    _cfg->_parserIdx = SendMessage(_hParser, CB_GETCURSEL, 0, 0);

    if (!_cfg->SaveToFile())
    {
        CPath cfgFile;
        CConfig::GetDefaultCfgFile(cfgFile);

        CText msg(_T("Failed saving config to\n\""));
        msg += cfgFile.C_str();
        msg += _T("\"\nIs the path read only?");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cPluginName, MB_OK | MB_ICONEXCLAMATION);
    }
    else
    {
        SendMessage(_hWnd, WM_CLOSE, 0, 0);
    }
}


/**
 *  \brief
 */
LRESULT CALLBACK ConfigWin::keyHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0)
    {
        HWND hWnd = GetFocus();
        if ((CW->_hWnd == hWnd) || IsChild(CW->_hWnd, hWnd))
        {
            // Key is pressed
            if (!(lParam & (1 << 31)))
            {
                if (wParam == VK_ESCAPE)
                {
                    SendMessage(CW->_hWnd, WM_CLOSE, 0, 0);
                    return 1;
                }
            }
        }
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}


/**
 *  \brief
 */
LRESULT APIENTRY ConfigWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_COMMAND:
            if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                DestroyCaret();
                return 0;
            }
            if (HIWORD(wParam) == BN_CLICKED)
            {
                if ((HWND)lParam == CW->_hOK)
                {
                    CW->onOK();
                    return 0;
                }

                if ((HWND)lParam == CW->_hCancel)
                {
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    return 0;
                }

                if ((HWND)lParam == CW->_hEnLibDb)
                {
                    BOOL en;
                    int color;
                    if (Button_GetCheck(CW->_hEnLibDb) == BST_CHECKED)
                    {
                        en = TRUE;
                        color = COLOR_WINDOW;
                    }
                    else
                    {
                        en = FALSE;
                        color = COLOR_BTNFACE;
                    }
                    EnableWindow(CW->_hCreateDb, en);
                    Edit_Enable(CW->_hLibDb, en);
                    SendMessage(CW->_hLibDb, EM_SETBKGNDCOLOR, 0, GetSysColor(color));
                    return 0;
                }

                if ((HWND)lParam == CW->_hCreateDb)
                {
                    CPath libraryPath = CreateLibraryDatabase(hWnd);
                    int libLen = libraryPath.Len();

                    if (libLen)
                    {
                        std::vector<TCHAR> buf;
                        int len = Edit_GetTextLength(CW->_hLibDb);
                        bool found = false;

                        if (len)
                        {
                            buf.resize(len + 1, 0);
                            Edit_GetText(CW->_hLibDb, buf.data(), buf.size());

                            for (TCHAR* ptr = _tcsstr(buf.data(), libraryPath.C_str());
                                    ptr; ptr = _tcsstr(ptr, libraryPath.C_str()))
                            {
                                if (ptr[libLen] == 0 || ptr[libLen] == _T(';'))
                                {
                                    found = true;
                                    break;
                                }
                            }

                            buf.pop_back(); // Remove the terminating '\0'

                            if (!found)
                                buf.push_back(_T(';'));
                        }

                        if (!found)
                        {
                            buf.insert(buf.cend(), libraryPath.C_str(), libraryPath.C_str() + libLen + 1);
                            Edit_SetText(CW->_hLibDb, buf.data());
                        }

                        SetFocus(CW->_hLibDb);
                        Edit_SetSel(CW->_hLibDb, buf.size(), buf.size());
                        Edit_ScrollCaret(CW->_hLibDb);
                    }

                    return 0;
                }
            }
        break;

        case WM_DESTROY:
            DestroyCaret();
            delete CW;
            CW = NULL;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
