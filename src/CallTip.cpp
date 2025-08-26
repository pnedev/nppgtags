

#pragma comment (lib, "comctl32")


#include <windows.h>
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

intptr_t CallTipParser::Parse(const CmdPtr_t& cmd)
{

    intptr_t result = 0;

    _lines.clear();
    _paths.clear();
    _buf = cmd->Result();

    // if (_buf.Len() > 0) {
        // MessageBox(NULL, _buf.C_str(), CText(_T("Parser")).C_str(), MB_OK);
    // }
    TCHAR* pTmp = NULL;
    for (TCHAR* pToken = _tcstok_s(_buf.C_str(), _T("\n\r"), &pTmp); pToken; 
            pToken = _tcstok_s(NULL, _T("\n\r"), &pTmp)) {
        TCHAR* inner_context = NULL;
        TCHAR* inner_token = _tcstok_s(pToken, _T(":"), &inner_context);
        inner_token = _tcstok_s(NULL, _T(":"), &inner_context);
        //_paths.push_back(&inner_token);
        inner_token = _tcstok_s(NULL, _T("\n\r"), &inner_context);
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
    _parser(std::static_pointer_cast<CallTipParser>(cmd->Parser()))
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
    EnableWindow(_hWnd, true);
    ListView_SetBkColor(_hLVWnd, backgroundColor);
    ListView_SetTextBkColor(_hLVWnd, backgroundColor);

    filterLV();

    resizeLV();

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);
    SetFocus(_hWnd);
    
    return _hWnd;
}


/**
 *  \brief
 */
int CallTipWin::filterLV()
{
    LVITEM lvItem   = {0};
    lvItem.mask     = LVIF_TEXT | LVIF_STATE;

    ListView_DeleteAllItems(_hLVWnd);

    for (int i = 0; i < _parser->GetList().size(); i++)
    {
        TCHAR* word = _parser->GetList().at(i);
        intptr_t parameter_count = 0;
        for (TCHAR ch = *word; ch; ch=*++word) {
            if (ch == _T(',')) {
                parameter_count++;
            }
        }
        if (_parser->overload <= parameter_count) {
            lvItem.pszText = _parser->GetList().at(i);
            ListView_InsertItem(_hLVWnd, &lvItem);
            ++lvItem.iItem;
        }
    }

    if (lvItem.iItem > 0)
        ListView_SetItemState(_hLVWnd, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

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
    npp.GetPointPos(&xOffset, &yOffset);

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


/**
 *  \brief
 */
LRESULT APIENTRY CallTipWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND npp_handle = INpp::Get().ReadSciHandle();
    
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_SETFOCUS:
            SetFocus(CTW->_hLVWnd);
        return 0;
        

        case WM_NOTIFY: {
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_KILLFOCUS: {
                    if (GetParent(CTW->_hWnd) == GetFocus()) {
                        SetFocus(CTW->_hLVWnd);
                    }
                    else { // Yeild focus to non parent windows
                        DestroyCurrentWin();
                    }
                    return 0;
                }
                case LVN_KEYDOWN: {
                    int keyCode = ((LPNMLVKEYDOWN)lParam)->wVKey;
                    if (keyCode == VK_ESCAPE) {
                        DestroyCurrentWin();
                        return 0;
                    }
                    BYTE keysState[256];
                    WORD character;
                    if (!GetKeyboardState(keysState))
                        return false;
                    if (ToAscii(keyCode, MapVirtualKey(keyCode, MAPVK_VK_TO_VSC), keysState, &character, 1) != 1)
                        return false;
                    SendMessage(npp_handle, WM_CHAR, (WPARAM)character, (LPARAM)1);
                    return 1;
                }
            }
        }
        break;
        
        case WM_DESTROY:
            CTW = nullptr;
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags