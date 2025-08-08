

#pragma comment (lib, "comctl32")


#include <windows.h>
#include <commctrl.h>
#include "Common.h"
#include "INpp.h"
#include "GTags.h"
#include "CallTip.h"
#include "Cmd.h"
#include "LineParser.h"


namespace GTags
{

const TCHAR CallTipWin::cClassName[]   = _T("CallTipWin");
const int CallTipWin::cBackgroundColor = COLOR_INFOBK;
const int CallTipWin::cWidth           = 400;


std::unique_ptr<CallTipWin> CallTipWin::CTW {nullptr};


/**
 *  \brief
 */
void CallTipWin::Register()
{
    WNDCLASS wc         = {0};
    wc.style            = CS_PARENTDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = wndProc;
    wc.hInstance        = HMod;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetSysColorBrush(cBackgroundColor);
    wc.lpszClassName    = cClassName;

    RegisterClass(&wc);

    INITCOMMONCONTROLSEX icex   = {0};
    icex.dwSize                 = sizeof(icex);
    icex.dwICC                  = ICC_LISTVIEW_CLASSES;

    InitCommonControlsEx(&icex);
}


/**
 *  \brief
 */
void CallTipWin::Unregister()
{
    UnregisterClass(cClassName, HMod);
}


/**
 *  \brief
 */
void CallTipWin::Show(const CmdPtr_t& cmd)
{
    if (CTW)
        return;

    CTW = std::make_unique<CallTipWin>(cmd);

    if (CTW->composeWindow(cmd->Name()) == NULL)
        CTW = nullptr;
}


/**
 *  \brief
 */
CallTipWin::CallTipWin(const CmdPtr_t& cmd) :
    _hWnd(NULL), _hLVWnd(NULL), _hFont(NULL), _cmdId(cmd->Id()), _ic(cmd->IgnoreCase()),
    _cmdTagLen((int)(cmd->Tag().Len())),
    _completion(cmd->Parser())
{}


/**
 *  \brief
 */
CallTipWin::~CallTipWin()
{
    INpp::Get().ClearSelectionMulti();
    INpp::Get().EndUndoAction();

    if (_hFont)
        DeleteObject(_hFont);
}


/**
 *  \brief
 */
HWND CallTipWin::composeWindow(const TCHAR* header)
{
    HWND hOwner = INpp::Get().GetSciHandle();
    RECT win;

    GetWindowRect(hOwner, &win);

    _hWnd = CreateWindow(cClassName, NULL,
            WS_POPUP | WS_BORDER,
            win.left, win.top, win.right - win.left, win.bottom - win.top,
            hOwner, NULL, HMod, NULL);
    if (_hWnd == NULL)
        return NULL;

    INpp::Get().RegisterWinForDarkMode(_hWnd);

    GetClientRect(_hWnd, &win);

    _hLVWnd = CreateWindow(WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE |
            LVS_REPORT | LVS_SINGLESEL | LVS_NOLABELWRAP | LVS_NOSORTHEADER | LVS_SORTASCENDING,
            0, 0, win.right - win.left, win.bottom - win.top,
            _hWnd, NULL, HMod, NULL);

    HDC hdc = GetWindowDC(_hLVWnd);

    _hFont = CreateFont(
            -MulDiv(UIFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, UIFontName.C_str());

    ReleaseDC(_hLVWnd, hdc);

    if (_hFont)
        SendMessage(_hLVWnd, WM_SETFONT, (WPARAM)_hFont, TRUE);

    ListView_SetExtendedListViewStyle(_hLVWnd, LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    TCHAR buf[32];
    _tcscpy_s(buf, _countof(buf), header);

    LVCOLUMN lvCol      = {0};
    lvCol.mask          = LVCF_TEXT | LVCF_WIDTH;
    lvCol.pszText       = buf;
    lvCol.cchTextMax    = _countof(buf);
    lvCol.cx            = cWidth;
    ListView_InsertColumn(_hLVWnd, 0, &lvCol);

    DWORD backgroundColor = GetSysColor(cBackgroundColor);
    ListView_SetBkColor(_hLVWnd, backgroundColor);
    ListView_SetTextBkColor(_hLVWnd, backgroundColor);

    CTextA wordA;
    INpp::Get().GetWord(wordA, true, true, true);
    CText word(wordA.C_str());

    // if (!filterLV(word))
    // {
        // SendMessage(_hWnd, WM_CLOSE, 0, 0);
        // return NULL;
    // }

    // resizeLV();

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);

    INpp::Get().BeginUndoAction();

    return _hWnd;
}


/**
 *  \brief
 */
void CallTipWin::resizeLV()
{
    bool scroll = false;
    int rowsCount = ListView_GetItemCount(_hLVWnd);
    rowsCount = 7;
    if (rowsCount > 7)
    {
        rowsCount = 7;
        scroll = true;
    }

    RECT win;
    ListView_GetItemRect(_hLVWnd, 0, &win, LVIR_BOUNDS);
    // int lvWidth     = win.right - win.left;
    int lvWidth     = 50;
    int lvHeight    = (win.bottom - win.top) * rowsCount;

    HWND hHeader = ListView_GetHeader(_hLVWnd);
    GetWindowRect(hHeader, &win);
    lvHeight += win.bottom - win.top;

    RECT maxWin;
    INpp& npp = INpp::Get();
    GetWindowRect(npp.GetSciHandle(), &maxWin);

    int maxWidth = (maxWin.right - maxWin.left) - 30;
    if (scroll)
        maxWidth -= GetSystemMetrics(SM_CXVSCROLL);
    if (lvWidth > maxWidth)
        lvWidth = maxWidth;

    ListView_SetColumnWidth(_hLVWnd, 0, lvWidth);

    if (scroll)
        lvWidth += GetSystemMetrics(SM_CXVSCROLL);

    win.left    = maxWin.left;
    win.top     = maxWin.top;
    win.right   = win.left + lvWidth;
    win.bottom  = win.top + lvHeight;

    AdjustWindowRect(&win, (DWORD)GetWindowLongPtr(_hWnd, GWL_STYLE), FALSE);
    lvWidth     = win.right - win.left;
    lvHeight    = win.bottom - win.top;

    int xOffset, yOffset;
    npp.GetPointPos(&xOffset, &yOffset);

    win.left    = maxWin.left + xOffset;
    win.top     = maxWin.top + yOffset + npp.GetTextHeight();
    win.right   = win.left + lvWidth;
    win.bottom  = win.top + lvHeight;

    xOffset = win.right - maxWin.right;
    if (xOffset > 0)
    {
        win.left    -= xOffset;
        win.right   -= xOffset;
    }

    if (win.bottom > maxWin.bottom)
    {
        win.bottom  = maxWin.top + yOffset;
        win.top     = win.bottom - lvHeight;
    }

    MoveWindow(_hWnd, win.left, win.top, win.right - win.left, win.bottom - win.top, TRUE);

    GetClientRect(_hWnd, &win);
    MoveWindow(_hLVWnd, 0, 0, win.right - win.left, win.bottom - win.top, TRUE);
}


/**
 *  \brief
 */
LRESULT APIENTRY CallTipWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_SETFOCUS:
            SetFocus(CTW->_hLVWnd);
        return 0;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_KILLFOCUS:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                return 0;
                
                // case NM_DBLCLK:
                    // CTW->onDblClick();
                // return  0;
            }
        break;

        case WM_DESTROY:
            CTW = nullptr;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags