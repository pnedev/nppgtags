/**
 *  \file
 *  \brief  Search input window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
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
#include <tchar.h>
#include <commctrl.h>
#include "Common.h"
#include "INpp.h"
#include "GTags.h"
#include "CmdEngine.h"
#include "SearchWin.h"
#include "Cmd.h"
#include "LineParser.h"
#include "Config.h"
#include "ResultWin.h"


namespace GTags
{

const TCHAR SearchWin::cClassName[] = _T("SearchWin");
const int SearchWin::cWidth         = 450;
const int SearchWin::cComplAfter    = 3;


std::unique_ptr<SearchWin> SearchWin::SW {nullptr};


/**
 *  \brief
 */
void SearchWin::Register()
{
    WNDCLASS wc         = {0};
    wc.style            = CS_PARENTDC | CS_HREDRAW | CS_VREDRAW;
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
void SearchWin::Show(CmdId_t cmdId, CompletionCB complCB, const TCHAR* hint, bool enRE, bool enIC)
{
    if (!SW)
    {
        HWND hOwner = INpp::Get().GetHandle();

        SW = std::make_unique<SearchWin>(cmdId, complCB);

        if (SW->composeWindow(hOwner, hint, enRE, enIC) == NULL)
            SW = nullptr;
    }
    else
    {
        SW->reinit(cmdId, complCB, hint, enRE, enIC);
    }

    if (hint)
        SW->startCompletion();
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
SearchWin::~SearchWin()
{
    if (_hKeyHook)
        UnhookWindowsHookEx(_hKeyHook);

    if (_hBtnFont)
        DeleteObject(_hBtnFont);
    if (_hTxtFont)
        DeleteObject(_hTxtFont);

    if (_cancelled && _cmd)
    {
        _cmd->Status(CANCELLED);
        _complCB(_cmd);
    }
}


/**
 *  \brief
 */
HWND SearchWin::composeWindow(HWND hOwner, const TCHAR* hint, bool enRE, bool enIC)
{
    HDC hdc = GetWindowDC(hOwner);

    _hTxtFont = CreateFont(
            -MulDiv(UIFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, UIFontName.C_str());
    _hBtnFont = Tools::CreateFromSystemMenuFont(hdc, UIFontSize - 1);

    int txtHeight = Tools::GetFontHeight(hdc, _hTxtFont) + 1;
    int btnHeight = Tools::GetFontHeight(hdc, _hBtnFont) + 2;

    ReleaseDC(hOwner, hdc);

    DWORD styleEx   = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;
    DWORD style     = WS_POPUP | WS_CAPTION | WS_SYSMENU;

    RECT win    = Tools::GetWinRect(hOwner, styleEx, style, cWidth, txtHeight + btnHeight + 17);
    int width   = win.right - win.left;

    _hWnd = CreateWindowEx(styleEx, cClassName, Cmd::CmdName[_cmdId], style,
            win.left, win.top, width, win.bottom - win.top,
            hOwner, NULL, HMod, NULL);
    if (_hWnd == NULL)
        return NULL;

    INpp::Get().RegisterWinForDarkMode(_hWnd);

    GetClientRect(_hWnd, &win);
    width = (win.right - win.left - 20) / 3;

    _hRE = CreateWindowEx(0, _T("BUTTON"), _T("RegExp"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            5, 5, width, btnHeight,
            _hWnd, NULL, HMod, NULL);

    _hIC = CreateWindowEx(0, _T("BUTTON"), _T("IgnoreCase"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            width + 10, 5, width, btnHeight,
            _hWnd, NULL, HMod, NULL);

    _hOK = CreateWindowEx(0, _T("BUTTON"), _T("OK"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            2 * width + 15, 5, width, btnHeight,
            _hWnd, NULL, HMod, NULL);

    _hSearch = CreateWindowEx(0, WC_COMBOBOX, NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            CBS_DROPDOWN | CBS_HASSTRINGS | CBS_AUTOHSCROLL | CBS_SORT,
            2, btnHeight + 10, win.right - win.left - 4, txtHeight,
            _hWnd, NULL, HMod, NULL);

    if (_hTxtFont)
        SendMessage(_hSearch, WM_SETFONT, (WPARAM)_hTxtFont, TRUE);

    ComboBox_SetMinVisible(_hSearch, 7);

    if (hint)
    {
        _initialCompl = true;
        ComboBox_SetText(_hSearch, hint);
    }

    if (_hBtnFont)
    {
        SendMessage(_hRE, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
        SendMessage(_hIC, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
        SendMessage(_hOK, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
    }

    if (enRE)
    {
        Button_SetCheck(_hRE, GTagsSettings._re ? BST_CHECKED : BST_UNCHECKED);
    }
    else
    {
        Button_SetCheck(_hRE, BST_UNCHECKED);
        EnableWindow(_hRE, FALSE);
    }

    if (enIC)
    {
        Button_SetCheck(_hIC, GTagsSettings._ic ? BST_CHECKED : BST_UNCHECKED);
    }
    else
    {
        Button_SetCheck(_hIC, BST_UNCHECKED);
        EnableWindow(_hIC, FALSE);
    }

    _hKeyHook = SetWindowsHookEx(WH_KEYBOARD, keyHookProc, NULL, GetCurrentThreadId());

    ShowWindow(_hWnd, SW_SHOWNORMAL);
    UpdateWindow(_hWnd);

    return _hWnd;
}


/**
 *  \brief
 */
void SearchWin::reinit(CmdId_t cmdId, CompletionCB complCB, const TCHAR* hint, bool enRE, bool enIC)
{
    _cmdId = cmdId;
    _complCB = complCB;

    _cancelled = true;
    _cmd = nullptr;

    SetWindowText(_hWnd, Cmd::CmdName[_cmdId]);

    if (hint)
    {
        _initialCompl = true;
        ComboBox_SetText(_hSearch, hint);
        PostMessage(_hSearch, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
    }
    else
    {
        ComboBox_SetText(_hSearch, _T(""));
    }

    EnableWindow(_hRE, enRE);
    EnableWindow(_hIC, enIC);

    bool re = (Button_GetCheck(_hRE) == BST_CHECKED);
    bool ic = (Button_GetCheck(_hIC) == BST_CHECKED);

    if (enRE != re)
    {
        if (enRE)
            Button_SetCheck(_hRE, GTagsSettings._re ? BST_CHECKED : BST_UNCHECKED);
        else
            Button_SetCheck(_hRE, BST_UNCHECKED);
    }

    if (enIC != ic)
    {
        if (enIC)
            Button_SetCheck(_hIC, GTagsSettings._ic ? BST_CHECKED : BST_UNCHECKED);
        else
            Button_SetCheck(_hIC, BST_UNCHECKED);
    }

    SetFocus(_hSearch);
}


/**
 *  \brief
 */
void SearchWin::startCompletion()
{
    if (Button_GetCheck(_hRE) == BST_CHECKED)
        return;

    CmdId_t cmplId;
    TCHAR tag[cComplAfter + 2];
    CompletionCB complCB;
    ParserPtr_t parser;

    if (_cmdId == FIND_FILE)
    {
        cmplId = AUTOCOMPLETE_FILE;

        tag[0] = _T('/');
        ComboBox_GetText(_hSearch, tag + 1, _countof(tag) - 1);
        tag[cComplAfter + 1] = 0;

        complCB = endCompletion;
        parser = std::make_shared<LineParser>();
    }
    else
    {
        cmplId = AUTOCOMPLETE;

        ComboBox_GetText(_hSearch, tag, _countof(tag));
        tag[cComplAfter] = 0;

        for (int i = 0; tag[i] != 0; ++i)
            if (tag[i] == _T(' ') || tag[i] == _T('\t'))
                return;

        complCB = halfComplete;
    }

    DbHandle db = getDatabase(false, true);

    if (!db)
    {
        _initialCompl = false;
        return;
    }

    CmdPtr_t cmpl = std::make_shared<Cmd>(cmplId, db, parser, tag, (Button_GetCheck(_hIC) == BST_CHECKED), false);

    if (_cmdId != FIND_DEFINITION)
        cmpl->SkipLibs(true);

    _completionStarted = true;

    CmdEngine::Run(cmpl, complCB);
}


/**
 *  \brief
 */
void SearchWin::halfComplete(const CmdPtr_t& cmpl)
{
    if (!SW)
        return;

    if (ComboBox_GetTextLength(SW->_hSearch) < cComplAfter)
    {
        SW->_completionStarted = false;
        return;
    }

    if (cmpl->Status() == OK)
    {
        cmpl->Id(AUTOCOMPLETE_SYMBOL);

        ParserPtr_t parser = std::make_shared<LineParser>();
        cmpl->Parser(parser);

        CmdEngine::Run(cmpl, endCompletion);
    }
    else
    {
        DbManager::Get().PutDb(cmpl->Db());

        SW->_completionStarted = false;
        SW->_completionDone = true;
    }
}


/**
 *  \brief
 */
void SearchWin::endCompletion(const CmdPtr_t& cmpl)
{
    if (!SW)
        return;

    SW->_completionStarted = false;

    if (ComboBox_GetTextLength(SW->_hSearch) < cComplAfter)
        return;

    DbManager::Get().PutDb(cmpl->Db());

    if (cmpl->Status() == OK && cmpl->Result())
    {
        SW->_completion = cmpl->Parser();
        SW->filterComplList();
    }

    SW->_completionDone = true;
}


/**
 *  \brief
 */
void SearchWin::clearCompletion()
{
    if (_completionStarted)
        return;

    const int pos = LOWORD(SendMessage(_hSearch, CB_GETEDITSEL, 0, 0));

    CText txt(pos);

    ComboBox_GetText(_hSearch, txt.C_str(), (int)txt.Size());

    ComboBox_ResetContent(_hSearch);
    ComboBox_ShowDropdown(_hSearch, FALSE);
    ComboBox_SetText(_hSearch, txt.C_str());
    PostMessage(_hSearch, CB_SETEDITSEL, 0, MAKELPARAM(pos, pos));

    _completion.reset();
    _completionDone = false;
}


/**
 *  \brief
 */
void SearchWin::filterComplList()
{
    if (!_completion)
        return;

    CText filter(ComboBox_GetTextLength(_hSearch));

    ComboBox_GetText(_hSearch, filter.C_str(), (int)filter.Size());

    const int pos = HIWORD(SendMessage(_hSearch, CB_GETEDITSEL, 0, 0));

    int (*pCompare)(const TCHAR*, const TCHAR*, size_t);

    if (Button_GetCheck(_hIC) == BST_CHECKED)
        pCompare = &_tcsnicmp;
    else
        pCompare = &_tcsncmp;

    ComboBox_ResetContent(_hSearch);
    ComboBox_ShowDropdown(_hSearch, FALSE);
    ComboBox_SetText(_hSearch, filter.C_str());
    PostMessage(_hSearch, CB_SETEDITSEL, 0, MAKELPARAM(pos, pos));

    SendMessage(_hSearch, WM_SETREDRAW, FALSE, 0);

    if (filter.Len() == cComplAfter)
    {
        for (const auto& complEntry : _completion->GetList())
            ComboBox_AddString(_hSearch, complEntry);
    }
    else
    {
        for (const auto& complEntry : _completion->GetList())
            if (!pCompare(complEntry, filter.C_str(), filter.Len()))
                ComboBox_AddString(_hSearch, complEntry);
    }

    if (ComboBox_GetCount(_hSearch))
    {
        ComboBox_ShowDropdown(_hSearch, TRUE);
        ComboBox_SetText(_hSearch, filter.C_str());

        if (_initialCompl)
        {
            _initialCompl = false;
            PostMessage(_hSearch, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
        }
        else
        {
            PostMessage(_hSearch, CB_SETEDITSEL, 0, MAKELPARAM(pos, -1));
        }
    }

    SendMessage(_hSearch, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(_hSearch, NULL, NULL, RDW_UPDATENOW);
}


/**
 *  \brief
 */
void SearchWin::saveSearchOptions()
{
    bool re = (Button_GetCheck(_hRE) == BST_CHECKED);

    if (IsWindowEnabled(_hRE) && re != GTagsSettings._re)
    {
        GTagsSettings._re = re;
        GTagsSettings.Save();
    }
}


/**
 *  \brief
 */
void SearchWin::onEditChange()
{
    if (_completionStarted)
        return;

    int len = ComboBox_GetTextLength(_hSearch);

    if (_completionDone)
    {
        const int pos = HIWORD(SendMessage(_hSearch, CB_GETEDITSEL, 0, 0));

        if (pos < cComplAfter)
            clearCompletion();
        else if (pos == cComplAfter && _keyPressed != VK_BACK && _keyPressed != VK_DELETE)
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
        DbHandle db = getDatabase();

        if (!db)
            return;

        CText tag(ComboBox_GetTextLength(_hSearch));

        ComboBox_GetText(_hSearch, tag.C_str(), (int)tag.Size());

        const int pos = HIWORD(SendMessage(_hSearch, CB_GETEDITSEL, 0, 0));

        PostMessage(_hSearch, CB_SETEDITSEL, 0, MAKELPARAM(pos, pos));

        bool re = (Button_GetCheck(_hRE) == BST_CHECKED);
        bool ic = (Button_GetCheck(_hIC) == BST_CHECKED);

        _cancelled = false;

        ParserPtr_t parser = std::make_shared<ResultWin::TabParser>();
        _cmd = std::make_shared<Cmd>(_cmdId, db, parser, tag.C_str(), ic, re);

        CmdEngine::Run(_cmd, _complCB);
    }

    if (!GTagsSettings._keepSearchWinOpen)
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
                if (wParam == VK_CONTROL || wParam == VK_MENU)
                {
                    SW->clearCompletion();
                    return 1;
                }

                SW->_keyPressed = (int)wParam;
            }
        }
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}


/**
 *  \brief
 */
LRESULT APIENTRY SearchWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            SW->saveSearchOptions();

            SW = nullptr;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
