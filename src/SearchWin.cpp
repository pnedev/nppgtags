/**
 *  \file
 *  \brief  Search input window
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


#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>
#include "INpp.h"
#include "CmdEngine.h"
#include "SearchWin.h"


namespace GTags
{

const TCHAR SearchWin::cClassName[] = _T("SearchWin");
const TCHAR SearchWin::cBtnFont[]   = _T("Tahoma");
const int SearchWin::cWidth         = 420;
const int SearchWin::cComplAfter    = 2;


SearchWin* SearchWin::SW = NULL;


/**
 *  \brief
 */
void SearchWin::Register()
{
    WNDCLASS wc         = {0};
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = wndProc;
    wc.hInstance        = HMod;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName    = cClassName;

    RegisterClass(&wc);

    INITCOMMONCONTROLSEX icex   = {0};
    icex.dwSize                 = sizeof(icex);
    icex.dwICC                  = ICC_STANDARD_CLASSES;

    InitCommonControlsEx(&icex);
}


/**
 *  \brief
 */
void SearchWin::Unregister()
{
    UnregisterClass(cClassName, HMod);
}


/**
 *  \brief
 */
void SearchWin::Show(const std::shared_ptr<Cmd>& cmd, CompletionCB complCB,
        bool enRE, bool enMC)
{
    if (SW)
        SendMessage(SW->_hWnd, WM_CLOSE, 0, 0);

    HWND hOwner = INpp::Get().GetHandle();

    SW = new SearchWin(cmd, complCB);
    if (SW->composeWindow(hOwner, enRE, enMC) == NULL)
    {
        delete SW;
        SW = NULL;
    }
}


/**
 *  \brief
 */
void SearchWin::Close()
{
    if (SW)
        SendMessage(SW->_hWnd, WM_CLOSE, 0, 0);
}


/**
 *  \brief
 */
RECT SearchWin::adjustSizeAndPos(HWND hOwner, DWORD styleEx, DWORD style,
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
SearchWin::~SearchWin()
{
    if (_hKeyHook)
        UnhookWindowsHookEx(_hKeyHook);

    if (_hBtnFont)
        DeleteObject(_hBtnFont);
    if (_hTxtFont)
        DeleteObject(_hTxtFont);

    if (_cancelled)
    {
        _cmd->Status(CANCELLED);
        _complCB(_cmd);
    }
}


/**
 *  \brief
 */
HWND SearchWin::composeWindow(HWND hOwner, bool enRE, bool enMC)
{
    TEXTMETRIC tm;
    HDC hdc = GetWindowDC(hOwner);
    GetTextMetrics(hdc, &tm);
    int txtHeight =
            MulDiv(UIFontSize + 1, GetDeviceCaps(hdc, LOGPIXELSY), 72) +
                tm.tmInternalLeading + 1;
    int btnHeight = MulDiv(UIFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72) +
            tm.tmInternalLeading;
    _hTxtFont = CreateFont(
            -MulDiv(UIFontSize + 1, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, UIFontName);
    _hBtnFont = CreateFont(
            -MulDiv(UIFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, cBtnFont);
    ReleaseDC(hOwner, hdc);

    DWORD styleEx = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;
    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;

    RECT win = adjustSizeAndPos(hOwner, styleEx, style,
            cWidth, txtHeight + btnHeight + 17);
    int width = win.right - win.left;

    _hWnd = CreateWindowEx(styleEx, cClassName, _cmd->Name(),
            style, win.left, win.top, width, win.bottom - win.top,
            hOwner, NULL, HMod, (LPVOID) this);
    if (_hWnd == NULL)
        return NULL;

    GetClientRect(_hWnd, &win);
    width = (win.right - win.left - 20) / 3;

    _hRE = CreateWindowEx(0, _T("BUTTON"), _T("RegExp"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            5, 5, width, btnHeight,
            _hWnd, NULL, HMod, NULL);

    _hMC = CreateWindowEx(0, _T("BUTTON"), _T("MatchCase"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            width + 10, 5, width, btnHeight,
            _hWnd, NULL, HMod, NULL);

    _hOK = CreateWindowEx(0, _T("BUTTON"), _T("OK"),
            WS_CHILD | WS_VISIBLE | BS_TEXT | BS_DEFPUSHBUTTON,
            2 * width + 15, 5, width, btnHeight,
            _hWnd, NULL, HMod, NULL);

    _hSearch = CreateWindowEx(0, WC_COMBOBOX, NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            CBS_DROPDOWN | CBS_HASSTRINGS | CBS_AUTOHSCROLL | CBS_SORT,
            2, btnHeight + 10, win.right - win.left - 4, txtHeight,
            _hWnd, NULL, HMod, NULL);

    if (_hTxtFont)
        SendMessage(_hSearch, WM_SETFONT, (WPARAM)_hTxtFont, TRUE);

    ComboBox_SetMinVisible(_hSearch, 8);

    if (_cmd->Tag())
        ComboBox_SetText(_hSearch, _cmd->Tag());

    if (_hBtnFont)
    {
        SendMessage(_hRE, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
        SendMessage(_hMC, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
    }

    Button_SetCheck(_hRE, _cmd->RegExp() ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(_hMC, _cmd->MatchCase() ? BST_CHECKED : BST_UNCHECKED);

    if (!enMC)
        EnableWindow(_hMC, FALSE);
    if (!enRE)
        EnableWindow(_hRE, FALSE);

    _hKeyHook = SetWindowsHookEx(WH_KEYBOARD, keyHookProc, NULL,
            GetCurrentThreadId());

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);

    return _hWnd;
}


/**
 *  \brief
 */
void SearchWin::startCompletion()
{
    if (Button_GetCheck(_hRE) == BST_CHECKED)
        return;

    TCHAR tag[cMaxTagLen];

    if (_cmd->Id() == FIND_FILE)
    {
        tag[0] = _T('/');
        ComboBox_GetText(_hSearch, tag + 1, _countof(tag) - 1);
        tag[cComplAfter + 1] = 0;
    }
    else
    {
        ComboBox_GetText(_hSearch, tag, _countof(tag));
        tag[cComplAfter] = 0;
    }

    std::shared_ptr<Cmd>
        cmpl(new Cmd(AUTOCOMPLETE, _T("AutoComplete"), _cmd->Db(), tag,
                false, (Button_GetCheck(_hMC) == BST_CHECKED)));

    if (_cmd->Id() == FIND_FILE)
    {
        cmpl->Id(AUTOCOMPLETE_FILE);
    }
    else
    {
        CmdEngine::Run(cmpl);
        cmpl->Id(AUTOCOMPLETE_SYMBOL);
    }

    CmdEngine::Run(cmpl);
    endCompletion(cmpl);
}


/**
 *  \brief
 */
void SearchWin::endCompletion(const std::shared_ptr<Cmd>& cmpl)
{
    if (cmpl->Status() == OK && cmpl->Result())
    {
        _complData(cmpl->ResultLen() + 1);
        Tools::AtoW(&_complData, _complData.Size(), cmpl->Result());
        parseCompletion();
        filterComplList();
    }

    _completionDone = true;
}


/**
 *  \brief
 */
void SearchWin::parseCompletion()
{
    TCHAR* pTmp = NULL;
    for (TCHAR* pToken = _tcstok_s(&_complData, _T("\n\r"), &pTmp);
        pToken; pToken = _tcstok_s(NULL, _T("\n\r"), &pTmp));
}


/**
 *  \brief
 */
void SearchWin::clearCompletion()
{
    SendMessage(_hSearch, WM_SETREDRAW, FALSE, 0);

    for (int count = ComboBox_GetCount(_hSearch); count >= 0; count--)
        ComboBox_DeleteString(_hSearch, count);

    ComboBox_ShowDropdown(_hSearch, FALSE);

    SendMessage(_hSearch, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(_hSearch, NULL, NULL, RDW_UPDATENOW);

    _completionDone = false;
}


/**
 *  \brief
 */
void SearchWin::filterComplList()
{
    if (&_complData == NULL)
        return;

    TCHAR filter[cMaxTagLen];
    ComboBox_GetText(_hSearch, filter, _countof(filter));

    int (*pCompare)(const TCHAR*, const TCHAR*, size_t);

    if (Button_GetCheck(_hMC) == BST_CHECKED)
        pCompare = &_tcsncmp;
    else
        pCompare = &_tcsnicmp;

    SendMessage(_hSearch, WM_SETREDRAW, FALSE, 0);

    for (int count = ComboBox_GetCount(_hSearch); count >= 0; count--)
        ComboBox_DeleteString(_hSearch, count);

    ComboBox_ShowDropdown(_hSearch, FALSE);

    TCHAR* pRes = &_complData;
    TCHAR* pEnd = pRes + _complData.Size() - 1;

    if (_cmd->Id() == FIND_FILE)
        pRes++;

    int len = _tcslen(filter);

    while (pRes < pEnd)
    {
        if (!pCompare(pRes, filter, len))
            ComboBox_AddString(_hSearch, pRes);

        pRes += (_tcslen(pRes) + 1);

        while (pRes < pEnd &&
                (*pRes == _T('\n') || *pRes == _T('\r') || *pRes == 0))
            pRes++;

        if (_cmd->Id() == FIND_FILE)
            pRes++;
    }

    if (ComboBox_GetCount(_hSearch))
    {
        ComboBox_ShowDropdown(_hSearch, TRUE);

        if (_keyPressed == VK_BACK || _keyPressed == VK_DELETE)
            ComboBox_SetText(_hSearch, filter);

        PostMessage(_hSearch, CB_SETEDITSEL, 0, MAKELPARAM(len, -1));
    }

    SendMessage(_hSearch, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(_hSearch, NULL, NULL, RDW_UPDATENOW);
}


/**
 *  \brief
 */
void SearchWin::onEditChange()
{
    int len = ComboBox_GetTextLength(_hSearch);

    if (_completionDone)
    {
        int pos = HIWORD(SendMessage(_hSearch, CB_GETEDITSEL, 0, 0));

        if (pos <= cComplAfter)
            clearCompletion();
        else
            filterComplList();
    }

    if (!_completionDone && len >= cComplAfter)
        startCompletion();
}


/**
 *  \brief
 */
void SearchWin::onOK()
{
    if (ComboBox_GetTextLength(_hSearch))
    {
        TCHAR tag[cMaxTagLen];
        ComboBox_GetText(_hSearch, tag, _countof(tag));

        bool re = (Button_GetCheck(_hRE) == BST_CHECKED);
        bool mc = (Button_GetCheck(_hMC) == BST_CHECKED);

        _cmd->Tag(tag);
        _cmd->RegExp(re);
        _cmd->MatchCase(mc);

        _cancelled = false;
        CmdEngine::Run(_cmd, _complCB);
    }

    SendMessage(_hWnd, WM_CLOSE, 0, 0);
}


/**
 *  \brief
 */
LRESULT CALLBACK SearchWin::keyHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0)
    {
        HWND hWnd = GetFocus();
        if (SW->_hWnd == hWnd || IsChild(SW->_hWnd, hWnd))
        {
            // Key is pressed
            if (!(lParam & (1 << 31)))
            {
                if (wParam == VK_ESCAPE)
                {
                    SendMessage(SW->_hWnd, WM_CLOSE, 0, 0);
                    return 1;
                }
                if (wParam == VK_RETURN)
                {
                    SW->onOK();
                    return 1;
                }

                SW->_keyPressed = wParam;
            }
        }
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}


/**
 *  \brief
 */
LRESULT APIENTRY SearchWin::wndProc(HWND hWnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_SETFOCUS:
            SetFocus(SW->_hSearch);
        return 0;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                if ((HWND)lParam == SW->_hOK)
                {
                    SW->onOK();
                    return 0;
                }
                else if (SW->_completionDone)
                {
                    SW->clearCompletion();
                    return 0;
                }
            }
            else if (HIWORD(wParam) == CBN_EDITCHANGE)
            {
                SW->onEditChange();
                return 0;
            }
        break;

        case WM_DESTROY:
            delete SW;
            SW = NULL;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
