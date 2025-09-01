/**
 *  \file
 *  \brief  GTags CallTips window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>, Robert McDowell <github.com/RobertP-McDowell>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2024 Pavel Nedev
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
#include <winuser.h>
#include <commctrl.h>
#include "Common.h"
#include "INpp.h"
#include "GTags.h"
#include "CallTipWin.h"
#include "NppAPI/Notepad_plus_msgs.h"
#include <string>

namespace GTags
{

const TCHAR CallTipWin::cClassName[]   = _T("CallTipWin");
const int CallTipWin::cBackgroundColor = COLOR_INFOBK;
const int CallTipWin::cItemWidth           = 1024;
const int CallTipWin::cMinWidth            = 300;


std::unique_ptr<CallTipWin> CallTipWin::CTW {nullptr};

/**
 *  \brief
 */
int CallTipParser::FindListIndexFromLine(TCHAR* findLine) { // Returns -1 if couldn't find line in list.
    for (int i = 0; i < GetList().size(); i++) {
        TCHAR* word = GetList().at(i);
        if (_tcscmp(findLine, word) == 0) {
            return i;
        }
    }
    return 0;
}


/**
 *  \brief
 */
intptr_t CallTipParser::Parse(const CmdPtr_t& cmd) {
    intptr_t result = 0;
    _lines.clear();
    _paths.clear();
    _buf = cmd->Result();
    TCHAR* pTmp = NULL;
    for (TCHAR* pToken = _tcstok_s(_buf.C_str(), _T("\n\r"), &pTmp); pToken; 
            pToken = _tcstok_s(NULL, _T("\n\r"), &pTmp)) {
        int colon_count = 0;
        int i = 0;
        int function_start_idx = 0;
        while (i < 1024) { // Find end of path.
            if (pToken[i] == '\0')
                break;
            if (pToken[i] == ':') { // Absoulute paths have 3 colons, "c:/path/file:line_number:"
                colon_count += 1;
                if (colon_count == 3) {
                    TCHAR* path_token = pToken;
                    path_token[i] = '\0';
                    _paths.push_back(path_token);
                    i++;
                    function_start_idx = i;
                    while (isspace(pToken[function_start_idx])) { // Remove whitespace.
                        function_start_idx++;
                    }
                }
            }
            if (colon_count >= 3) { // Remove possible defintion.
                if (pToken[i] == '{' || pToken[i] == ';' || (colon_count >= 6 && pToken[i] == ':')) {
                    pToken[i] = '\0';
                    break;
                }
            }
            i++;
        }
        TCHAR* function_token = &pToken[function_start_idx];
        _lines.push_back(function_token);
        result++;
    }
    return result;
}

/**
 *  \brief
 */
void CallTipWin::GetCallTipFunction(CTextA& func_name, intptr_t& overload, intptr_t& func_start_pos, intptr_t caret_pos)
{
    INpp& npp = INpp::Get();
    npp.ReadSciHandle();

    intptr_t currpos = caret_pos;
    if (caret_pos == -1)
        currpos = npp.GetPos();
    intptr_t line = npp.GetLineFromPosition(currpos);
    intptr_t startpos = npp.PositionFromLine(line);
    intptr_t endpos = npp.LineEndPosition(line);
    intptr_t len = endpos - startpos + 3; // Also take CRLF in account, even if not there.

    intptr_t offset = currpos - startpos;

    if (offset < 2) { // 'a(' is the shortest possible function.
        return;
    }
    CTextA line_buf;
    npp.GetLineText(line_buf, len, line);

    intptr_t nests = 0;
    offset -= 1;
    for (intptr_t i = offset; i >= 0; i--) { // Find all of the '(' and ','.
        char symbol = line_buf.C_str()[i];
        if (symbol == '(') {
            nests -= 1;
            if (nests == -1) {
                intptr_t name_end = i - 1;;
                intptr_t n = i - 1;
                while (n >= 0) {
                    symbol = line_buf.C_str()[n];
                    if (isspace(symbol) && n == name_end) { // Whitespace between name and params, remove and continue.
                        name_end--;
                        n--;
                        continue;
                    }
                    if (!isalpha(symbol) && !isdigit(symbol) && symbol != '_') {
                        n += 1;
                        break;
                    }
                    n--;
                }
                func_start_pos = startpos + n;
                for (n; n <= name_end; n++) { // Reverse the name back, so it's normal.
                    func_name += line_buf.C_str()[n];
                }
                return;
            }
        }
        else if (symbol == ')') {
            nests += 1;
        }
        else if (symbol == ',' && nests == 0) {
            overload += 1;
        }
        else if (symbol == ';' || symbol == '{') {
            return;
        }
    }
    return;
}

/**
 *  \brief
 */
void CallTipWin::Register()
{
    WNDCLASS wc         = {0};
    wc.style            = CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
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

    if (CTW->composeWindow() == NULL)
        CTW = nullptr;
}


/**
 *  \brief
 */
CallTipWin::CallTipWin(const CmdPtr_t& cmd) :
    _hWnd(NULL), _hLVWnd(NULL), _hFont(NULL), _cmdId(cmd->Id()), _tag(cmd->Tag()),
    _parser(std::static_pointer_cast<CallTipParser>(cmd->Parser())), _selItem(-1), _queued_for_deletion(false)
{}


/**
 *  \brief
 */
CallTipWin::~CallTipWin()
{
    if (_hFont)
        DeleteObject(_hFont);
}

HWND CallTipWin::composeWindow()
{
    HWND hOwner = (INpp::Get().GetSciHandle());
    RECT win;

    GetWindowRect(hOwner, &win);

    _hWnd = CreateWindow(cClassName, NULL,
            WS_CHILD | WS_BORDER, // Make a child window, as a popup window will hide the editors caret.
            win.left, win.top, win.right - win.left, win.bottom - win.top,
            hOwner, NULL, HMod, NULL);
    if (_hWnd == NULL)
        return NULL;

    INpp::Get().RegisterWinForDarkMode(_hWnd);

    GetClientRect(_hWnd, &win);

    _hLVWnd = CreateWindow(WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE |
            LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER | LVS_SORTASCENDING,
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

    ListView_SetExtendedListViewStyle(_hLVWnd, LVS_EX_HEADERINALLVIEWS | LVS_EX_COLUMNOVERFLOW |
        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_AUTOSIZECOLUMNS);

    TCHAR buf[32];
    _stprintf(buf, TEXT( "%d (CallTip)" ), int(_parser->overload) + 1);

    LVCOLUMN lvCol      = {0};
    lvCol.mask          = LVCF_TEXT | LVCF_WIDTH;
    lvCol.pszText       = buf;
    lvCol.cchTextMax    = _countof(buf);
    lvCol.cx            = cItemWidth;
    ListView_InsertColumn(_hLVWnd, 0, &lvCol);

    DWORD backgroundColor = GetSysColor(cBackgroundColor);
    EnableWindow(_hWnd, true);
    ListView_SetBkColor(_hLVWnd, backgroundColor);
    ListView_SetTextBkColor(_hLVWnd, backgroundColor);

    if (!filterLV())
    {
        SendMessage(_hWnd, WM_CLOSE, 0, 0);
        return NULL;
    }

    resizeLV();

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);
    EnableWindow(_hLVWnd, false);

    SetFocus(_hWnd);
    
    return _hWnd;
}

/**
 *  \brief
 */
int CallTipWin::getDefParamCount(TCHAR* word) {
    int parameter_count = 0;
    bool parameter_start = false;
    for (TCHAR ch = *word; ch; ch=*++word) {
        if (parameter_count == 0) {
            if (ch == _T('(')) {
                parameter_start = true;
            }
            else if (parameter_start == true) {
                if (ch == _T(')'))
                    return 0;
                else
                    parameter_count += 1;
            }
        }
        if (ch == _T(','))
            parameter_count++;
    }
    return parameter_count;
}

/**
 *  \brief
 */
TCHAR* CallTipWin::getDefParamText(TCHAR* word, int wordSize) {
    TCHAR wrd_copy[256] = {0};
    memcpy_s(wrd_copy, 256, word, wordSize);

    TCHAR* pTmp = NULL;
    _tcstok_s(wrd_copy, _T("("), &pTmp);
    TCHAR* parameter_list = _tcstok_s(NULL, _T(")"), &pTmp);
    if (parameter_list == NULL)
        parameter_list = TEXT("");
    return parameter_list;
}

/**
 *  \brief
 */
int CallTipWin::getItemByName(TCHAR* word) {
    if (_hLVWnd == nullptr)
        return -1;
    for (int i = 0; i < ListView_GetItemCount(_hLVWnd); i++) {
        TCHAR buf[256] = {0};
        ListView_GetItemText(_hLVWnd, i, 0, buf, _countof(buf));
        if (_tcscmp(word, buf) == 0) {
            return i;
        }
    }
    return -1; // Item not listed.
}

/**
 *  \brief
 */
int CallTipWin::filterLV()
{
    LVITEM lvItem   = {0};
    lvItem.mask     = LVIF_TEXT | LVIF_STATE;

    ListView_DeleteAllItems(_hLVWnd);
    int lowest_parameter_count = 1000;
    int highest_parameter_count = 0;
    int overload_compare = int(_parser->overload);
    if (overload_compare > 0) { // 0 and 1 are interchangable with overload.
        overload_compare += 1;
    }
    for (int i = 0; i < _parser->GetList().size(); i++)
    {
        TCHAR* line = _parser->GetList().at(i);
        TCHAR* path = _parser->GetListPaths().at(i);
        // filter non header files, when searching by symbol.
        if (_cmdId == CALLTIP_SYMBOL && !_tcsstr(path, TEXT(".h:")) &&
            !_tcsstr(path, TEXT(".hpp:")) && !_tcsstr(path, TEXT(".hxx:")))
            continue;
        int parameter_count = getDefParamCount(line);
        if (parameter_count < lowest_parameter_count)
            lowest_parameter_count = parameter_count;
        if (parameter_count > highest_parameter_count)
            highest_parameter_count = parameter_count;
        
        if (overload_compare <= parameter_count) {
            lvItem.pszText = line;
            ListView_InsertItem(_hLVWnd, &lvItem);
            ++lvItem.iItem;
        }
    }
    if (lvItem.iItem == 0) {
        return 0;
    }
    if (lowest_parameter_count == highest_parameter_count)
        if (ListView_GetItemCount(_hLVWnd) == 1) {
            _selItem = 0;
            ListView_SetItemState(_hLVWnd, _selItem, LVIS_SELECTED, LVIS_SELECTED);
            onClick(_selItem);
        }
        else {
            if (_cmdId == CALLTIP_SYMBOL)
                updateHeader(lowest_parameter_count, -1, _T("CallTips"), _T("Symbol *.h"));
            else
                updateHeader(lowest_parameter_count, -1, _T("CallTips"));
        }
    else 
        updateHeader(lowest_parameter_count, highest_parameter_count);

    return lvItem.iItem;
}

/**
 *  \brief
 */
void CallTipWin::resizeLV()
{
    int rowsCount = ListView_GetItemCount(_hLVWnd);

    int widest_width = 0;
    int widest_idx = 0;
    for (int i = 0; i <= ListView_GetItemCount(_hLVWnd); i++) {
        TCHAR buf[256] = {0};
        ListView_GetItemText(_hLVWnd, i, 0, buf, _countof(buf));
        int str_len = int(_tcsclen(buf));
        if (str_len > widest_width)
            widest_width = str_len;
            widest_idx = i;
    }
    TCHAR widest_buf[256] = {0};
    ListView_GetItemText(_hLVWnd, widest_idx, 0, widest_buf, _countof(widest_buf));
    SIZE fontSIZE;
    GetTextExtentPoint32(GetDC(_hLVWnd), widest_buf, widest_width, &fontSIZE);

    RECT win;
    ListView_GetItemRect(_hLVWnd, 0, &win, LVIR_BOUNDS);

    int lvWidth     = max(cMinWidth, fontSIZE.cx + 16);
    int lvHeight    = (win.bottom - win.top) * rowsCount;
    win.right = (win.left + lvWidth);

    HWND hHeader = ListView_GetHeader(_hLVWnd);
    GetWindowRect(hHeader, &win);
    lvHeight += win.bottom - win.top;

    RECT maxWin;
    INpp& npp = INpp::Get();
    GetWindowRect(npp.GetSciHandle(), &maxWin);
    maxWin.right -= GetSystemMetrics(SM_CXHSCROLL); // Take sci scrollbar into account.

    win.left    = 0;
    win.top     = 0;
    win.right   = win.left + lvWidth;
    win.bottom  = win.top + lvHeight;

    AdjustWindowRect(&win, (DWORD)GetWindowLongPtr(_hWnd, GWL_STYLE), FALSE);
    lvWidth     = win.right - win.left;
    lvHeight    = win.bottom - win.top;

    int xOffset, yOffset;
    npp.GetPointFromPos(_parser->func_start_pos, &xOffset, &yOffset);

    win.left    = 0 + xOffset;
    win.top     = 0 + yOffset + npp.GetTextHeight();
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
void CallTipWin::updateHeader(int overload, int high_overload, TCHAR* header1, TCHAR* header2) {
    TCHAR buf[128] = { 0 };
    if (high_overload == -1) {
        _stprintf(buf, TEXT("%d/%d (%s) %s"), int(_parser->overload) + 1, overload, header1, header2);
    }
    else {
        _stprintf(buf, TEXT("%d/%d-%d (%s) %s"), int(_parser->overload) + 1, overload, high_overload, header1, header2);
    }
    LVCOLUMN lvCol = { 0 };
    lvCol.mask = LVCF_TEXT | LVCF_WIDTH;
    lvCol.cchTextMax = _countof(buf);
    lvCol.pszText = buf;
    lvCol.cx = cItemWidth;
    ListView_SetColumn(_hLVWnd, 0, &lvCol);
}

void CallTipWin::updateWindow(intptr_t position) {
    CTextA tag;
    intptr_t overload = 0;
    intptr_t func_start_pos = 0;
    INpp& npp = INpp::Get();
    GetCallTipFunction(tag, overload, func_start_pos, position);

    if (tag.IsEmpty()) {
        CallTipWin::DestroyCurrentWin(); // Order is important here.
        SetFocus(npp.GetSciHandle());
        return;
    }
    if (CText(tag.C_str()) != _tag) {
        // TODO: Show new calltip.
        CallTipWin::DestroyCurrentWin(); // Again, order is important here.
        SetFocus(npp.GetSciHandle());
        return;
    }
    if (overload != _parser->overload || func_start_pos != _parser->func_start_pos) {
        _parser->overload = overload;
        TCHAR buf[256];
        ListView_GetItemText(_hLVWnd, _selItem, 0, buf, _countof(buf));
        if (!filterLV())
        {
            SendMessage(_hWnd, WM_CLOSE, 0, 0);
            return;
        }
        resizeLV();
        _selItem = getItemByName(buf);
        if (_selItem >= 0) {
            ListView_SetItemState(_hLVWnd, _selItem, LVIS_SELECTED, LVIS_SELECTED);
            onClick(_selItem);
        }
    }
}

/**
 *  \brief
 */
void CallTipWin::onClick(int item) {
    _selItem = item;
    TCHAR buf[256] = {0};
    ListView_GetItemText(CTW->_hLVWnd, item, 0, buf, _countof(buf));
    int listIndex = _parser->FindListIndexFromLine(buf);
    TCHAR pathBuf[256];
    if (listIndex != -1) {
        _stprintf(pathBuf, _tcsrchr(_parser->GetListPaths().at(listIndex), _T('/'))+1);
        _tcschr(pathBuf, _T(':'))[0] = '\0';
    }

    updateHeader(getDefParamCount(buf), -1, getDefParamText(buf, 256), pathBuf);
}

/**
 *  \brief
 */
LRESULT APIENTRY CallTipWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (CTW == nullptr) {
        return 0;
    }
    HWND npp_handle = INpp::Get().GetSciHandle();
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_DESTROY:
            CTW = nullptr;
        return 0;

        case WM_SETFOCUS: {
            if (CTW->_queued_for_deletion == true)
                return 0;
            SetFocus(CTW->_hWnd);
        }
        return 0;

        case WM_KILLFOCUS: {
            if (CTW->_queued_for_deletion == true)
                return 0;
            if (GetParent(CTW->_hWnd) == GetFocus()) {
                SetFocus(CTW->_hWnd);
        // The window has lost focus, so we know the user clicked on the npp window, but not yet where,
        // and we don't get the click notification, thus we need to find the new caret position ourselves.
                INpp& npp = INpp::Get();
                POINT caretPoint;
                GetCursorPos(&caretPoint);
                RECT maxWin;
                GetWindowRect(npp_handle, &maxWin);
                intptr_t cursor_pos = npp.GetPosFromPoint(caretPoint.x - maxWin.left, caretPoint.y - maxWin.top);
                CTW->updateWindow(cursor_pos);
            }
            else { // Yeild focus to non parent windows
                DestroyCurrentWin();
            }
        }
        return 0;

        case WM_CHAR:
            SendMessage(npp_handle, WM_CHAR, wParam, lParam);
            if (wParam == '(' || wParam == ')' || wParam == ',')
                CTW->updateWindow();
        return 0;

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE ) {
                DestroyCurrentWin();
                SetFocus(npp_handle);
                return 0;
            }
            SendMessage(npp_handle, WM_KEYDOWN, wParam, lParam);
            switch (wParam) // Update window *after* sending message to npp
            {
                case VK_UP:
                case VK_DOWN:
                case VK_LEFT:
                case VK_RIGHT:
                case VK_BACK: {
                    CTW->updateWindow();
                }
                return 1;
            }
        }
        return 0;

        case WM_PASTE: // Send ^xcv commands to npp.
        case WM_COPY:
        case WM_CUT:
        case WM_INPUTLANGCHANGE: // I don't know if NPP needs any of these, but I'll send them jsut in case.
        case WM_INPUTLANGCHANGEREQUEST:
            SendMessage(npp_handle, uMsg, wParam, lParam);
        return 0;

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
            SendMessage(CTW->_hLVWnd, uMsg, wParam, lParam);
        return 0;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_KILLFOCUS:
                    DestroyCurrentWin();
                return 0;

                case NM_CLICK: {
                    CTW->onClick(((LPNMITEMACTIVATE)lParam)->iItem);
                }
                return 0;

                case NM_DBLCLK: {
                    INpp& npp = INpp::Get();
                    TCHAR buf[256] = {0};
                    ListView_GetItemText(CTW->_hLVWnd, ((LPNMITEMACTIVATE)lParam)->iItem, 0, buf, _countof(buf));
                    int listIndex = CTW->_parser->FindListIndexFromLine(buf);
                    if (listIndex != -1) {
                        TCHAR* path = CTW->_parser->GetListPaths().at(listIndex);
                        TCHAR* line = _tcsrchr(path, _T(':'));
                        line[0] = '\0';
                        line++;
                        int intLine = max(0, _tstoi(line) - 1);

                        TCHAR path_buf[256] = { 0 };
                        _stprintf(path_buf, TEXT("%s"), path);
                        CPath path_check(path);
                        if (!path_check.FileExists())
                        {
                            MessageBox(npp.GetHandle(),
                                    _T("File not found, database seems outdated.")
                                    _T("\nPlease re-create it and redo the search."),
                                    cPluginName, MB_OK | MB_ICONEXCLAMATION);
                            return 0;
                        }

                        if (npp.OpenFile(path_buf) == 0) {
                            npp.GoToLine(intLine);
                            npp.SetFirstVisibleLine(intLine - (npp.LinesOnScreen()/2)); // Center view.
                        }
                    }
                    DestroyCurrentWin();
                }
                return 0;
            }
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags