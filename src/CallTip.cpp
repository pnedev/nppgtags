

#pragma comment (lib, "comctl32")


#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <commctrl.h>
#include "Common.h"
#include "INpp.h"
#include "GTags.h"
#include "CallTip.h"
#include "NppAPI/Notepad_plus_msgs.h"
#include <string>

namespace GTags
{

const TCHAR CallTipWin::cClassName[]   = _T("CallTipWin");
const int CallTipWin::cBackgroundColor = COLOR_INFOBK;
const int CallTipWin::cItemWidth           = 1024;
const int CallTipWin::cMinWidth            = 100;


std::unique_ptr<CallTipWin> CallTipWin::CTW {nullptr};

intptr_t CallTipParser::Parse(const CmdPtr_t& cmd) {

    intptr_t result = 0;

    _lines.clear();
    _paths.clear();
    _buf = cmd->Result();
    // MessageBox(NULL, _buf.C_str(), L"CALL", MB_OK);
    TCHAR* pTmp = NULL;
    for (TCHAR* pToken = _tcstok_s(_buf.C_str(), _T("\n\r"), &pTmp); pToken; 
            pToken = _tcstok_s(NULL, _T("\n\r"), &pTmp)) {
        int colon_count = 0;
        int i = 0;
        while (true) { // Find end of path.
            if (pToken[i] == '\0')
                break;
            if (pToken[i] == ':') { // Path has 3 colons, "c:/path/:line_number:"
                colon_count += 1;
                if (colon_count == 3) {
                    TCHAR* path_token = pToken;
                    path_token[i] = '\0';
                    _paths.push_back(path_token);
                    i++;
                    TCHAR* function_token = &pToken[i];
                    _lines.push_back(function_token);
                }
            }
            if (colon_count >= 3) { // Remove possible declaration after defintion.
                if (pToken[i] == '{' || pToken[i] == ';' || (colon_count >= 6 && pToken[i] == ':')) {
                    pToken[i-1] = '\0';
                    break;
                }
            }
            i++;
        }
        result++;
    }
    return result;
}

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
                while (true) {
                    symbol = line_buf.C_str()[n];
                    if (!isalpha(symbol) && !isdigit(symbol)) {
                        if (symbol == ' ' && n == name_end) { // Whitespace between name and params, remove and continue.
                            name_end--;
                            n--;
                            continue;
                        }
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

    filterLV();

    resizeLV();

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);
    EnableWindow(_hLVWnd, false);

    SetFocus(_hWnd);
    
    return _hWnd;
}

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
        TCHAR* word = _parser->GetList().at(i);
        int parameter_count = getDefParamCount(word);
        if (parameter_count < lowest_parameter_count)
            lowest_parameter_count = parameter_count;
        if (parameter_count > highest_parameter_count)
            highest_parameter_count = parameter_count;
        
        if (overload_compare <= parameter_count) {
            lvItem.pszText = _parser->GetList().at(i);
            ListView_InsertItem(_hLVWnd, &lvItem);
            ++lvItem.iItem;
        }
    }
    if (lowest_parameter_count == highest_parameter_count)
        if (ListView_GetItemCount(_hLVWnd) == 1) {
            // TCHAR buf[256];
            // ListView_GetItemText(CTW->_hLVWnd, 0, 0, buf, _countof(buf));
            // updateHeader(lowest_parameter_count, -1, getDefParamText(buf));
            ListView_SetItemState(_hLVWnd, _selItem, LVIS_SELECTED, LVIS_SELECTED);
            _selItem = 0;
            onClick(_selItem);
        }
        else {
            updateHeader(lowest_parameter_count);
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

void CallTipWin::updateHeader(int overload, int high_overload, TCHAR* header) {
    TCHAR buf[128] = { 0 };
    if (high_overload == -1) {
        _stprintf(buf, TEXT("%d/%d (%s)"), int(_parser->overload) + 1, overload, header);
    }
    else {
        _stprintf(buf, TEXT("%d/%d-%d (%s)"), int(_parser->overload) + 1, overload, high_overload, header);
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
        filterLV();
        resizeLV();
        _selItem = getItemByName(buf);
        if (_selItem >= 0) {
            ListView_SetItemState(_hLVWnd, _selItem, LVIS_SELECTED, LVIS_SELECTED);
            onClick(_selItem);
        }
    }
}

void CallTipWin::onClick(int item) {
    _selItem = item;
    TCHAR buf[256] = {0};
    ListView_GetItemText(CTW->_hLVWnd, item, 0, buf, _countof(buf));
    updateHeader(getDefParamCount(buf), -1, getDefParamText(buf, 256));
}

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
                    for (int i = 0; i < CTW->_parser->GetList().size(); i++) {
                        TCHAR* word = CTW->_parser->GetList().at(i);
                        if (_tcscmp(word, buf) == 0) {
                            TCHAR* path = CTW->_parser->GetListPaths().at(i);
                            TCHAR* line = _tcschr(path, _T(':'));
                            line = _tcschr(line+1, _T(':'));
                            line[0] = '\0';
                            line++;
                            CPath path_check(path);
                            if (!path_check.FileExists())
                            {
                                MessageBox(npp.GetHandle(),
                                        _T("File not found, database seems outdated.")
                                        _T("\nPlease re-create it and redo the search."),
                                        cPluginName, MB_OK | MB_ICONEXCLAMATION);
                                return 0;
                            }
                            npp.OpenFile(path);
                            npp.GoToLine(_tstoi(line));
                            DestroyCurrentWin();
                            return 0;
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