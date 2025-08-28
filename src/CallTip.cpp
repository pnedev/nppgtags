

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
const int CallTipWin::cWidth           = 600;


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
        TCHAR* inner_context = NULL;
        TCHAR* inner_token = _tcstok_s(pToken, _T(":"), &inner_context);
        inner_token++;
        _tcscat(_tcscat(inner_token, _tcstok_s(NULL, _T(":"), &inner_context)), TEXT(":"));
        _tcscat(_tcscat(inner_token, _tcstok_s(NULL, _T(":"), &inner_context)), TEXT(":"));
        _paths.push_back(inner_token);
        inner_token = _tcstok_s(NULL, _T("{\n\r"), &inner_context); // cutoff curly brackets as well.
        _lines.push_back(inner_token);
        ++result;
    }

    return result;
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

    if (CTW->composeWindow(cmd->Name()) == NULL)
        CTW = nullptr;
}


/**
 *  \brief
 */
CallTipWin::CallTipWin(const CmdPtr_t& cmd) :
    _hWnd(NULL), _hLVWnd(NULL), _hFont(NULL), _cmdId(cmd->Id()), _tag(cmd->Tag()),
    _parser(std::static_pointer_cast<CallTipParser>(cmd->Parser())), _selItem(-1)
{}


/**
 *  \brief
 */
CallTipWin::~CallTipWin()
{
    if (_hFont)
        DeleteObject(_hFont);
}


/**
 *  \brief
 */
HWND CallTipWin::composeWindow(const TCHAR* header)
{
    HWND hOwner = (INpp::Get().ReadSciHandle());
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

    ListView_SetExtendedListViewStyle(_hLVWnd, LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    TCHAR buf[32];
    _stprintf(buf, TEXT( "%d (CallTip)" ), int(_parser->overload) + 1);

    LVCOLUMN lvCol      = {0};
    lvCol.mask          = LVCF_TEXT | LVCF_WIDTH;
    lvCol.pszText       = buf;
    lvCol.cchTextMax    = _countof(buf);
    lvCol.cx            = cWidth;
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

int CallTipWin::getItemByName(TCHAR* word) {
    for (int i = 0; i < ListView_GetItemCount(_hLVWnd); i++) {
        TCHAR buf[256];
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
    int overload_compare = _parser->overload;
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
        updateHeader(lowest_parameter_count);
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

    ListView_SetColumnWidth(_hLVWnd, 0, lvWidth);

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
    win.top     = 0 + yOffset - lvHeight;
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
        win.bottom  = maxWin.top - lvHeight;
        win.top     = win.bottom + yOffset;
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
    lvCol.cx = cWidth;
    ListView_SetColumn(_hLVWnd, 0, &lvCol);
}

void CallTipWin::updateWindow() {
    CTextA tag;
    intptr_t overload = 0;
    intptr_t func_start_pos = 0;
    INpp& npp = INpp::Get();
    npp.GetCursorFunction(tag, overload, func_start_pos);

    if (tag.IsEmpty() || CText(tag.C_str()) != _tag) {
        CallTipWin::DestroyCurrentWin();
        SetFocus(npp.ReadSciHandle());
        return;
    }
    if (overload != _parser->overload || func_start_pos != _parser->func_start_pos) {
        _parser->overload = overload;
        TCHAR buf[256];
        ListView_GetItemText(_hLVWnd, _selItem, 0, buf, _countof(buf));
        filterLV();
        resizeLV();
        _selItem = getItemByName(buf);
        if (_selItem >= 0)
            ListView_SetItemState(_hLVWnd, _selItem, LVIS_SELECTED, LVIS_SELECTED);
            onClick(_selItem);
    }

}

void CallTipWin::onClick(int item) {
    TCHAR buf[256];
    ListView_GetItemText(CTW->_hLVWnd, item, 0, buf, _countof(buf));

    TCHAR* pTmp = NULL;
    _tcstok_s(buf, _T("("), &pTmp);
    TCHAR* parameter_list = _tcstok_s(NULL, _T(")"), &pTmp);
    if (parameter_list == NULL)
        parameter_list = TEXT("");
    TCHAR buf2[256];
    ListView_GetItemText(CTW->_hLVWnd, item, 0, buf2, _countof(buf));
    CTW->updateHeader(getDefParamCount(buf2), -1, parameter_list);
}


LRESULT APIENTRY CallTipWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND npp_handle = INpp::Get().ReadSciHandle();
    
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_SETFOCUS:
            SetFocus(CTW->_hWnd);
        return 0;

        case WM_KILLFOCUS: {
            if (GetParent(CTW->_hWnd) == GetFocus()) {
                SetFocus(CTW->_hWnd);
            }
            else { // Yeild focus to non parent windows
                DestroyCurrentWin();
            }
        }
        return 0;
        
        case WM_CHAR:
            SendMessage(npp_handle, WM_CHAR, wParam, lParam);
            if (wParam == '(' || wParam == ')' || wParam == ',') {
                CTW->updateWindow();
            }
        return 0;
        
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
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
                break;
            }
        }
        return 0;
        
        case WM_LBUTTONDOWN: {
            // Report to LV, so we can get the translated message reported back as NM_CLICK or NM_DBLCLK.
            SendMessage(CTW->_hLVWnd, WM_LBUTTONDOWN, wParam, lParam);
        }
        return 0;
        case WM_LBUTTONDBLCLK: {
            // Report to LV, so we can get the translated message reported back as NM_CLICK or NM_DBLCLK.
            SendMessage(CTW->_hLVWnd, WM_LBUTTONDBLCLK, wParam, lParam);
        }
        return 0;
        
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_KILLFOCUS:
                    DestroyCurrentWin();
                return 0;
                
                case NM_CLICK: {
                    CTW->_selItem = ((LPNMITEMACTIVATE)lParam)->iItem;
                    CTW->onClick(CTW->_selItem);
                }
                return 0;
                
                case NM_DBLCLK: {
                    INpp& npp = INpp::Get();
                    TCHAR buf[256];
                    ListView_GetItemText(CTW->_hLVWnd, ((LPNMITEMACTIVATE)lParam)->iItem, 0, buf, _countof(buf));
                    for (int i = 0; i < CTW->_parser->GetList().size(); i++) {
                        TCHAR* word = CTW->_parser->GetList().at(i);
                        if (_tcscmp(word, buf) == 0) {
                            TCHAR* path_and_line = CTW->_parser->GetListPaths().at(i);
                            TCHAR* pTmp = NULL;
                            TCHAR* path = _tcstok_s(path_and_line, _T(":"), &pTmp);
                            TCHAR* line = _tcstok_s(NULL, _T(":"), &pTmp);
                            npp.OpenFile(path);
                            npp.GoToLine(_tstoi(line));
                            DestroyCurrentWin();
                            return 0;
                        }
                    }
                }
                return 0;
            }
        break;

        case WM_DESTROY:
            CTW = nullptr;
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags