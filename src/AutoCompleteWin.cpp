/**
 *  \file
 *  \brief  GTags AutoComplete window
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
#include <commctrl.h>
#include "Common.h"
#include "INpp.h"
#include "GTags.h"
#include "AutoCompleteWin.h"
#include "Cmd.h"
#include "LineParser.h"


namespace GTags
{

const TCHAR AutoCompleteWin::cClassName[]   = _T("AutoCompleteWin");
const int AutoCompleteWin::cBackgroundColor = COLOR_INFOBK;
const int AutoCompleteWin::cWidth           = 400;


AutoCompleteWin* AutoCompleteWin::ACW = NULL;


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
void AutoCompleteWin::Show(const CmdPtr_t& cmd)
{
    if (ACW)
        return;

    ACW = new AutoCompleteWin(cmd);
    if (ACW->composeWindow(cmd->Name()) == NULL)
    {
        delete ACW;
        ACW = NULL;
    }
}


/**
 *  \brief
 */
AutoCompleteWin::AutoCompleteWin(const CmdPtr_t& cmd) :
    _hWnd(NULL), _hLVWnd(NULL), _hFont(NULL), _cmdId(cmd->Id()), _ic(cmd->IgnoreCase()),
    _cmdTagLen((int)(_cmdId == AUTOCOMPLETE_FILE ? cmd->Tag().Len() - 1 : cmd->Tag().Len())), _completion(cmd->Parser())
{}


/**
 *  \brief
 */
AutoCompleteWin::~AutoCompleteWin()
{
    INpp::Get().ClearSelection();
    INpp::Get().EndUndoAction();

    if (_hFont)
        DeleteObject(_hFont);
}


/**
 *  \brief
 */
HWND AutoCompleteWin::composeWindow(const TCHAR* header)
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
    INpp::Get().GetWord(wordA, true, true);
    CText word(wordA.C_str());

    if (!filterLV(word))
    {
        SendMessage(_hWnd, WM_CLOSE, 0, 0);
        return NULL;
    }

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);

    INpp::Get().BeginUndoAction();

    return _hWnd;
}


/**
 *  \brief
 */
int AutoCompleteWin::filterLV(const CText& filter)
{
    LVITEM lvItem   = {0};
    lvItem.mask     = LVIF_TEXT | LVIF_STATE;

    size_t len = filter.Len();

    ListView_DeleteAllItems(_hLVWnd);

    int (*pCompare)(const TCHAR*, const TCHAR*, size_t);

    if (_ic)
        pCompare = &_tcsnicmp;
    else
        pCompare = &_tcsncmp;

    for (const auto& complEntry : _completion->GetList())
    {
        if (!len || !pCompare(complEntry, filter.C_str(), len))
        {
            lvItem.pszText = complEntry;
            ListView_InsertItem(_hLVWnd, &lvItem);
            ++lvItem.iItem;
        }
    }

    if (lvItem.iItem > 0)
    {
        ListView_SetItemState(_hLVWnd, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
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
    int rowsCount = ListView_GetItemCount(_hLVWnd);
    if (rowsCount > 7)
    {
        rowsCount = 7;
        scroll = true;
    }

    RECT win;
    ListView_GetItemRect(_hLVWnd, 0, &win, LVIR_BOUNDS);
    int lvWidth     = win.right - win.left;
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
void AutoCompleteWin::onDblClick()
{
    TCHAR itemTxt[MAX_PATH];
    LVITEM lvItem       = {0};
    lvItem.mask         = LVIF_TEXT;
    lvItem.iItem        = ListView_GetNextItem(_hLVWnd, -1, LVNI_SELECTED);
    lvItem.pszText      = itemTxt;
    lvItem.cchTextMax   = _countof(itemTxt);

    ListView_GetItem(_hLVWnd, &lvItem);
    lvItem.pszText[lvItem.cchTextMax - 1] = 0;

    CTextA completion(itemTxt);
    INpp::Get().ReplaceWord(completion.C_str(), true);

    SendMessage(_hWnd, WM_CLOSE, 0, 0);
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

        case VK_TAB:
        case VK_RETURN:
            onDblClick();
        return true;

        case VK_ESCAPE:
        case VK_CONTROL:
        case VK_MENU:
            SendMessage(_hWnd, WM_CLOSE, 0, 0);
        return true;

        case VK_DELETE:
            INpp::Get().ReplaceWord("", true);
            SendMessage(_hWnd, WM_CLOSE, 0, 0);
        return true;

        case VK_BACK:
        {
            INpp& npp = INpp::Get();
            npp.ClearSelection();
            npp.Backspace();
            if (npp.GetWordSize(true) < _cmdTagLen)
            {
                SendMessage(_hWnd, WM_CLOSE, 0, 0);
                return true;
            }
        }
        break;

        default:
        {
            BYTE keysState[256];
            WORD character;

            if (!GetKeyboardState(keysState))
                return false;
            if (ToAscii(keyCode, MapVirtualKey(keyCode, MAPVK_VK_TO_VSC), keysState, &character, 1) != 1)
                return false;

            INpp& npp = INpp::Get();
            npp.ClearSelection();
            npp.AddText((char*)&character, 1);

            if (keyCode == VK_SPACE)
            {
                SendMessage(_hWnd, WM_CLOSE, 0, 0);
                return true;
            }
        }
    }

    CTextA wordA;
    INpp::Get().GetWord(wordA, true, true);
    CText word(wordA.C_str());
    int lvItemsCnt = filterLV(word);

    if (lvItemsCnt == 0)
    {
        SendMessage(_hWnd, WM_CLOSE, 0, 0);
    }
    else if (lvItemsCnt == 1)
    {
        TCHAR itemTxt[MAX_PATH];
        LVITEM lvItem       = {0};
        lvItem.mask         = LVIF_TEXT;
        lvItem.iItem        = ListView_GetNextItem(_hLVWnd, -1, LVNI_SELECTED);
        lvItem.pszText      = itemTxt;
        lvItem.cchTextMax   = _countof(itemTxt);

        ListView_GetItem(_hLVWnd, &lvItem);
        lvItem.pszText[lvItem.cchTextMax - 1] = 0;

        if (!_tcscmp(word.C_str(), lvItem.pszText))
            SendMessage(_hWnd, WM_CLOSE, 0, 0);
    }

    return true;
}


/**
 *  \brief
 */
LRESULT APIENTRY AutoCompleteWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_SETFOCUS:
            SetFocus(ACW->_hLVWnd);
        return 0;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_KILLFOCUS:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                return 0;

                case LVN_KEYDOWN:
                    if (ACW->onKeyDown(((LPNMLVKEYDOWN)lParam)->wVKey))
                        return 1;
                break;

                case NM_DBLCLK:
                    ACW->onDblClick();
                return 0;
            }
        break;

        case WM_DESTROY:
            delete ACW;
            ACW = NULL;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
