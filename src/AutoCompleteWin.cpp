/**
 *  \file
 *  \brief  GTags AutoComplete window
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
#include "AutoCompleteWin.h"
#include <commctrl.h>
#include "Common.h"
#include "INpp.h"
#include "GTags.h"


namespace GTags
{

const TCHAR AutoCompleteWin::cClassName[]    = _T("AutoCompleteWin");
const int AutoCompleteWin::cBackgroundColor  = COLOR_INFOBK;


/**
 *  \brief
 */
void AutoCompleteWin::Register()
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
    icex.dwICC                  = ICC_LISTVIEW_CLASSES;

    InitCommonControlsEx(&icex);
}


/**
 *  \brief
 */
void AutoCompleteWin::Unregister()
{
    UnregisterClass(cClassName, HMod);
}


/**
 *  \brief
 */
BOOL AutoCompleteWin::Show(const std::shared_ptr<CmdData>& cmd)
{
    AutoCompleteWin ui(cmd);
    if (ui.composeWindow() == NULL)
        return -1;

    BOOL r;
    MSG msg;
    while ((r = GetMessage(&msg, NULL, 0, 0)) != 0 && r != -1)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return r;
}


/**
 *  \brief
 */
AutoCompleteWin::AutoCompleteWin(const std::shared_ptr<CmdData>& cmd) :
    _hwnd(NULL), _hLVWnd(NULL), _hFont(NULL), _cmd(cmd)
{
    unsigned len = cmd->GetResultLen();
    _result = new TCHAR[len + 1];
    Tools::AtoW(_result, len + 1, cmd->GetResult());
}


/**
 *  \brief
 */
AutoCompleteWin::~AutoCompleteWin()
{
    if (_result)
        delete [] _result;
    if (_hFont)
        DeleteObject(_hFont);
}


/**
 *  \brief
 */
HWND AutoCompleteWin::composeWindow()
{
    HWND hOwner = INpp::Get().GetSciHandle();
    RECT win;
    GetWindowRect(hOwner, &win);

    _hwnd = CreateWindow(cClassName, NULL,
            WS_POPUP | WS_BORDER, win.left, win.top,
            win.right - win.left, win.bottom - win.top,
            hOwner, NULL, HMod, (LPVOID) this);
    if (_hwnd == NULL)
        return NULL;

    GetClientRect(_hwnd, &win);
    _hLVWnd = CreateWindow(WC_LISTVIEW, _T("List View"),
            WS_CHILD | WS_VISIBLE |
            LVS_REPORT | LVS_SINGLESEL | LVS_NOLABELWRAP |
            LVS_NOSORTHEADER | LVS_SORTASCENDING,
            0, 0, win.right - win.left, win.bottom - win.top,
            _hwnd, NULL, HMod, NULL);

    HDC hdc = GetWindowDC(_hLVWnd);
    _hFont = CreateFont(
            -MulDiv(UIFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, UIFontName);
    ReleaseDC(_hLVWnd, hdc);
    if (_hFont)
        SendMessage(_hLVWnd, WM_SETFONT, (WPARAM) _hFont, (LPARAM) TRUE);

    ListView_SetExtendedListViewStyle(_hLVWnd,
            LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    TCHAR buf[32];
    _tcscpy_s(buf, _countof(buf), _cmd->GetName());

    LVCOLUMN lvCol      = {0};
    lvCol.mask          = LVCF_TEXT | LVCF_WIDTH;
    lvCol.pszText       = buf;
    lvCol.cchTextMax    = _countof(buf);
    lvCol.cx            = 300;
    ListView_InsertColumn(_hLVWnd, 0, &lvCol);

    DWORD backgroundColor = GetSysColor(cBackgroundColor);
    ListView_SetBkColor(_hLVWnd, backgroundColor);
    ListView_SetTextBkColor(_hLVWnd, backgroundColor);

    if (!fillLV())
    {
        SendMessage(_hwnd, WM_CLOSE, 0, 0);
        return NULL;
    }

    ShowWindow(_hwnd, SW_SHOWNORMAL);
    UpdateWindow(_hwnd);

    return _hwnd;
}


/**
 *  \brief
 */
int AutoCompleteWin::fillLV()
{
    LVITEM lvItem   = {0};
    lvItem.mask     = LVIF_TEXT | LVIF_STATE;

    TCHAR* pTmp;
    for (TCHAR* pToken = _tcstok_s(_result, _T("\n\r"), &pTmp);
        pToken; pToken = _tcstok_s(NULL, _T("\n\r"), &pTmp))
    {
        lvItem.pszText =
                (_cmd->GetID() == AUTOCOMPLETE_FILE) ? pToken + 1 : pToken;
        ListView_InsertItem(_hLVWnd, &lvItem);
        lvItem.iItem++;
    }

    if (lvItem.iItem > 0)
    {
        ListView_SetItemState(_hLVWnd, 0, LVIS_FOCUSED | LVIS_SELECTED,
                LVIS_FOCUSED | LVIS_SELECTED);
        resizeLV();
    }

    return lvItem.iItem;
}


/**
 *  \brief
 */
int AutoCompleteWin::filterLV(const TCHAR* filter)
{
    LVITEM lvItem   = {0};
    lvItem.mask     = LVIF_TEXT | LVIF_STATE;

    int len = _tcslen(filter);

    TCHAR* pRes = _result;
    TCHAR* pEnd = pRes + _cmd->GetResultLen();

    ListView_DeleteAllItems(_hLVWnd);

    while (pRes < pEnd)
    {
        if (!_tcsncmp(pRes, filter, len))
        {
            lvItem.pszText = pRes;
            ListView_InsertItem(_hLVWnd, &lvItem);
            lvItem.iItem++;
        }

        pRes += (_tcslen(pRes) + 1);

        while (pRes < pEnd &&
                (*pRes == _T('\n') || *pRes == _T('\r') || *pRes == 0))
            pRes++;
    }

    if (lvItem.iItem > 0)
    {
        ListView_SetItemState(_hLVWnd, 0, LVIS_FOCUSED | LVIS_SELECTED,
                LVIS_FOCUSED | LVIS_SELECTED);
        resizeLV();
    }

    return lvItem.iItem;
}


/**
 *  \brief
 */
void AutoCompleteWin::resizeLV()
{
    bool scroll = false;
    int rowsCount = ListView_GetItemCount(_hLVWnd) - 1;
    if (rowsCount > 7)
    {
        rowsCount = 7;
        scroll = true;
    }

    DWORD rectSize  = ListView_ApproximateViewRect(_hLVWnd, -1, -1, rowsCount);
    int lvWidth     = LOWORD(rectSize);
    int lvHeight    = HIWORD(rectSize);

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

    RECT win;
    win.left    = maxWin.left;
    win.top     = maxWin.top;
    win.right   = win.left + lvWidth;
    win.bottom  = win.top + lvHeight;

    AdjustWindowRect(&win, GetWindowLongPtr(_hwnd, GWL_STYLE), FALSE);
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

    MoveWindow(_hwnd, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);

    GetClientRect(_hwnd, &win);
    MoveWindow(_hLVWnd, 0, 0,
            win.right - win.left, win.bottom - win.top, TRUE);
}


/**
 *  \brief
 */
void AutoCompleteWin::onDblClick()
{
    TCHAR itemTxt[MAX_PATH];
    LVITEM lvItem       = {0};
    lvItem.mask         = LVIF_TEXT;
    lvItem.iItem        = ListView_GetNextItem(_hLVWnd, -1, LVNI_SELECTED);
    lvItem.pszText      = itemTxt;
    lvItem.cchTextMax   = _countof(itemTxt);

    ListView_GetItem(_hLVWnd, &lvItem);

    char str[MAX_PATH];
    Tools::WtoA(str, _countof(str), lvItem.pszText);

    INpp::Get().ReplaceWord(str);
    SendMessage(_hwnd, WM_CLOSE, 0, 0);
}


/**
 *  \brief
 */
bool AutoCompleteWin::onKeyDown(int keyCode)
{
    switch (keyCode)
    {
        case VK_UP:
        case VK_DOWN:
            return false;

        case VK_SPACE:
        case VK_TAB:
        case VK_RETURN:
            onDblClick();
            return true;

        case VK_ESCAPE:
        case VK_CONTROL:
        case VK_MENU:
            SendMessage(_hwnd, WM_CLOSE, 0, 0);
            return true;

        case VK_DELETE:
            INpp::Get().ReplaceWord("");
            SendMessage(_hwnd, WM_CLOSE, 0, 0);
            return true;

        case VK_BACK:
        {
            INpp& npp = INpp::Get();
            npp.ClearSelection();
            npp.Backspace();
            if (npp.GetWordSize() < (int)_cmd->GetTagLen())
            {
                SendMessage(_hwnd, WM_CLOSE, 0, 0);
                return true;
            }
        }
        break;

        default:
        {
            BYTE keysState[256];
            WORD character;

            GetKeyboardState(keysState);
            if (ToAscii(keyCode, MapVirtualKey(keyCode, MAPVK_VK_TO_VSC),
                    keysState, &character, 1) != 1)
                return false;

            INpp& npp = INpp::Get();
            npp.ClearSelection();
            npp.AddText((char*)&character, 1);
        }
    }

    char wordA[MAX_PATH];
    INpp::Get().GetWord(wordA, _countof(wordA), true);

    TCHAR word[MAX_PATH];
    Tools::AtoW(word, _countof(word), wordA);
    int lvItemsCnt = filterLV(word);
    if (lvItemsCnt == 0)
        SendMessage(_hwnd, WM_CLOSE, 0, 0);
    else if (lvItemsCnt == 1)
    {
        TCHAR itemTxt[MAX_PATH];
        ListView_GetItemText(_hLVWnd,
                ListView_GetNextItem(_hLVWnd, -1, LVNI_SELECTED), 0,
                itemTxt, _countof(itemTxt));
        if (!_tcscmp(word, itemTxt))
            SendMessage(_hwnd, WM_CLOSE, 0, 0);
    }

    return true;
}


/**
 *  \brief
 */
LRESULT APIENTRY AutoCompleteWin::wndProc(HWND hwnd, UINT umsg,
        WPARAM wparam, LPARAM lparam)
{
    AutoCompleteWin* ui;

    switch (umsg)
    {
        case WM_CREATE:
            ui = (AutoCompleteWin*)((LPCREATESTRUCT)lparam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToUlong(ui));
            return 0;

        case WM_SETFOCUS:
            ui = reinterpret_cast<AutoCompleteWin*>(static_cast<LONG_PTR>
                            (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            SetFocus(ui->_hLVWnd);
            return 0;

        case WM_NOTIFY:
            ui = reinterpret_cast<AutoCompleteWin*>(static_cast<LONG_PTR>
                            (GetWindowLongPtr(hwnd, GWLP_USERDATA)));

            switch (((LPNMHDR)lparam)->code)
            {
                case NM_KILLFOCUS:
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;

                case LVN_KEYDOWN:
                    if (ui->onKeyDown(((LPNMLVKEYDOWN)lparam)->wVKey))
                        return 0;
                break;

                case NM_DBLCLK:
                    ui->onDblClick();
                    return 0;
            }
        break;

        case WM_DESTROY:
            INpp::Get().ClearSelection();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}

} // namespace GTags
