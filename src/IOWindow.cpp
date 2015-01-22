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


#pragma comment (lib, "comctl32")


#define WIN32_LEAN_AND_MEAN
#include "IOWindow.h"
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <stdlib.h>


const TCHAR IOWindow::cClassName[]      = _T("IOWindow");
const int IOWindow::cBackgroundColor    = COLOR_INFOBK;


volatile LONG IOWindow::RefCount = 0;
HINSTANCE IOWindow::HInst = NULL;


/**
 *  \brief
 */
void IOWindow::Register(HINSTANCE hInst)
{
    HInst = hInst;
    if (hInst == NULL)
        GetModuleHandleEx(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_PIN, cClassName, &HInst);

    WNDCLASS wc         = {0};
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = wndProc;
    wc.hInstance        = HInst;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetSysColorBrush(cBackgroundColor);
    wc.lpszClassName    = cClassName;

    RegisterClass(&wc);

    INITCOMMONCONTROLSEX icex   = {0};
    icex.dwSize                 = sizeof(icex);
    icex.dwICC                  = ICC_STANDARD_CLASSES;

    InitCommonControlsEx(&icex);
    LoadLibrary(_T("Msftedit.dll"));
}


/**
 *  \brief
 */
void IOWindow::Unregister()
{
    UnregisterClass(cClassName, HInst);
    FreeLibrary(GetModuleHandle(_T("Msftedit.dll")));
}


/**
 *  \brief
 */
bool IOWindow::Create(HWND hOwner,
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

    IOWindow iow(readOnly, minWidth, minHeight, text);
    iow.composeWindow(hOwner, font, fontSize, readOnly,
            minWidth, minHeight, header, text, txtLimit);

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
RECT IOWindow::adjustSizeAndPos(DWORD style, int width, int height)
{
    RECT win, maxWin;
    int noAdjust = 0;
    int offset = (RefCount - 1) * 15;

    SystemParametersInfo(SPI_GETWORKAREA, 0, &maxWin, 0);

    win = maxWin;
    if (width <= 0) width = 100;
    if (height <= 0) height = 100;

    if ((int) width < maxWin.right - maxWin.left)
        win.right = win.left + width;
    else
        noAdjust++;

    if (height < maxWin.bottom - maxWin.top)
        win.bottom = win.top + height;
    else
        noAdjust++;

    if (noAdjust < 2)
    {
        AdjustWindowRect(&win, style, FALSE);

        width = win.right - win.left;
        height = win.bottom - win.top;

        if (width < maxWin.right - maxWin.left)
        {
            win.left = (maxWin.right - width) / 2 + offset;
            win.right = win.left + width;
        }
        else
        {
            win.left = maxWin.left;
            win.right = maxWin.right;
        }

        if (height < maxWin.bottom - maxWin.top)
        {
            win.top = (maxWin.bottom - height) / 2 + offset;
            win.bottom = win.top + height;
        }
        else
        {
            win.top = maxWin.top;
            win.bottom = maxWin.bottom;
        }
    }

    return win;
}


/**
 *  \brief
 */
IOWindow::IOWindow(bool readOnly, int minWidth, int minHeight, TCHAR* text) :
    _readOnly(readOnly), _minWidth(minWidth), _minHeight(minHeight),
    _text(text), _success(false)
{
    InterlockedIncrement(&RefCount);
}



/**
 *  \brief
 */
IOWindow::~IOWindow()
{
    if (_hFont)
        DeleteObject(_hFont);

    InterlockedDecrement(&RefCount);
}


/**
 *  \brief
 */
HWND IOWindow::composeWindow(HWND hOwner,
        const TCHAR* font, unsigned fontSize, bool readOnly,
        int minWidth, int minHeight,
        const TCHAR* header, const TCHAR* text, int txtLimit)
{
    DWORD style = WS_POPUPWINDOW | WS_CAPTION;
    RECT win = adjustSizeAndPos(style, minWidth, minHeight);
    _hWnd = CreateWindow(cClassName, header,
            style, win.left, win.top,
            win.right - win.left, win.bottom - win.top,
            hOwner, NULL, HInst, (LPVOID) this);

    GetClientRect(_hWnd, &win);

    style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
    if (readOnly)
        style |= ES_MULTILINE | ES_READONLY;
    HWND hWndEdit = CreateWindowEx(0, MSFTEDIT_CLASS, NULL, style, 0, 0,
            win.right - win.left, win.bottom - win.top,
            _hWnd, NULL, HInst, NULL);

    SendMessage(hWndEdit, EM_SETBKGNDCOLOR, 0,
            (LPARAM)GetSysColor(cBackgroundColor));

    CHARFORMAT fmt  = {0};
    fmt.cbSize      = sizeof(fmt);
    fmt.dwMask      = CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_SIZE;
    fmt.dwEffects   = CFE_AUTOCOLOR;
    fmt.yHeight     = fontSize * 20;
    if (font)
        _tcscpy_s(fmt.szFaceName, _countof(fmt.szFaceName), font);
    SendMessage(hWndEdit, EM_SETCHARFORMAT, (WPARAM) SCF_ALL, (LPARAM) &fmt);

    HDC hdc = GetWindowDC(hOwner);
    _hFont = CreateFont(
            -MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, font);
    ReleaseDC(hOwner, hdc);
    if (_hFont)
        SendMessage(hWndEdit, WM_SETFONT, (WPARAM) _hFont, (LPARAM) TRUE);

    SendMessage(hWndEdit, EM_AUTOURLDETECT, (WPARAM) TRUE, 0);

    DWORD events = ENM_KEYEVENTS | ENM_LINK;
    if (readOnly)
        events |= ENM_MOUSEEVENTS | ENM_REQUESTRESIZE;
    SendMessage(hWndEdit, EM_SETEVENTMASK, 0, (LPARAM) events);

    if (!readOnly)
        SendMessage(hWndEdit, EM_EXLIMITTEXT, 0, (LPARAM) txtLimit);

    if (_tcslen(text))
    {
        Edit_SetText(hWndEdit, text);
        if (!readOnly)
            Edit_SetSel(hWndEdit, 0, -1);
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
            if (!_readOnly)
            {
                HWND hwndEdit = GetTopWindow(_hWnd);
                int len = Edit_GetTextLength(hwndEdit) + 1;
                if (len > 1)
                {
                    Edit_GetText(hwndEdit, _text, len);
                    _success = true;
                }
                SendMessage(_hWnd, WM_CLOSE, 0, 0);
                return 1;
            }
    }

    return 0;
}



/* ========================================================================== */
/**
*  @fn  onAutoSize()  function_description
*/
/* ========================================================================== */
int IOWindow::onAutoSize(REQRESIZE* pReqResize)
{
    HWND hWndEdit = pReqResize->nmhdr.hwndFrom;
    int width = pReqResize->rc.right - pReqResize->rc.left + 30;
    int height = pReqResize->rc.bottom - pReqResize->rc.top;

    if (width < _minWidth) width = _minWidth;
    if (height < _minHeight) height = _minHeight;

    RECT win = IOWindow::adjustSizeAndPos(GetWindowLongPtr(_hWnd, GWL_STYLE),
            width, height);
    MoveWindow(_hWnd, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);
    GetClientRect(_hWnd, &win);
    MoveWindow(hWndEdit, 0, 0,
            win.right - win.left, win.bottom - win.top, TRUE);

    DWORD events = SendMessage(hWndEdit, EM_GETEVENTMASK, 0, 0);
    events &= ~ENM_REQUESTRESIZE;
    SendMessage(hWndEdit, EM_SETEVENTMASK, 0, (LPARAM) events);

    return 1;
}



/* ========================================================================== */
/**
*  @fn  wndProc()  function_description
*/
/* ========================================================================== */
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
                IOWindow* iow =
                        reinterpret_cast<IOWindow*>(static_cast<LONG_PTR>
                                (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
                if (iow->_readOnly)
                    HideCaret((HWND) lparam);
                return 0;
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lparam)->code == EN_MSGFILTER)
            {
                IOWindow* iow =
                        reinterpret_cast<IOWindow*>(static_cast<LONG_PTR>
                                (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
                if (iow->_readOnly)
                    HideCaret(GetTopWindow(hwnd));
                MSGFILTER* pMsgFilter = (MSGFILTER*)lparam;
                if (pMsgFilter->msg == WM_KEYDOWN)
                    return iow->onKeyDown(pMsgFilter->wParam);
                return 0;
            }
            if (((LPNMHDR)lparam)->code == EN_REQUESTRESIZE)
            {
                IOWindow* iow = reinterpret_cast<IOWindow*>
                        (static_cast<LONG_PTR>
                        (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
                return iow->onAutoSize((REQRESIZE*)lparam);
            }
            if (((LPNMHDR)lparam)->code == EN_LINK)
            {
                ENLINK* pEnLink = (ENLINK*)lparam;
                if (pEnLink->msg == WM_LBUTTONUP)
                {
                    TCHAR link[2048];
                    TCHAR* pLink = link;
                    unsigned len =
                            pEnLink->chrg.cpMax - pEnLink->chrg.cpMin + 1;
                    if (len > 2048)
                        pLink = new TCHAR[len];
                    TEXTRANGE range;
                    range.chrg = pEnLink->chrg;
                    range.lpstrText = pLink;
                    SendMessage(pEnLink->nmhdr.hwndFrom, EM_GETTEXTRANGE,
                            0, (LPARAM) &range);
                    ShellExecute(NULL, _T("open"), pLink, NULL, NULL,
                            SW_SHOWNORMAL);
                    if (pLink != link)
                        delete [] pLink;
                    return 1;
                }
                return 0;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}
