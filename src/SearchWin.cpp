/**
 *  \file
 *  \brief  Search input window
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


namespace GTags
{

const TCHAR SearchWin::cClassName[] = _T("SearchWin");
const int SearchWin::cWidth         = 450;
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
void SearchWin::Show(const CmdPtr_t& cmd, CompletionCB complCB, bool enRE, bool enMC)
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
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);

#if (WINVER >= 0x0600)
    if (Tools::GetWindowsVersion() <= 0x0502)
        ncm.cbSize -= sizeof(int);
#endif

    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

    HDC hdc = GetWindowDC(hOwner);

    ncm.lfMenuFont.lfHeight = -MulDiv(UIFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);

    _hTxtFont = CreateFont(
            -MulDiv(UIFontSize + 1, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, UIFontName.C_str());

    _hBtnFont = CreateFontIndirect(&ncm.lfMenuFont);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);

    int txtHeight = MulDiv(UIFontSize + 1, GetDeviceCaps(hdc, LOGPIXELSY), 72) + tm.tmInternalLeading + 1;
    int btnHeight = tm.tmInternalLeading - ncm.lfMenuFont.lfHeight + 2;

    ReleaseDC(hOwner, hdc);

    DWORD styleEx   = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;
    DWORD style     = WS_POPUP | WS_CAPTION | WS_SYSMENU;

    RECT win    = Tools::GetWinRect(hOwner, styleEx, style, cWidth, txtHeight + btnHeight + 17);
    int width   = win.right - win.left;

    _hWnd = CreateWindowEx(styleEx, cClassName, _cmd->Name(), style,
            win.left, win.top, width, win.bottom - win.top,
            hOwner, NULL, HMod, this);
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

    if (!_cmd->Tag().IsEmpty())
        ComboBox_SetText(_hSearch, _cmd->Tag().C_str());

    if (_hBtnFont)
    {
        SendMessage(_hRE, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
        SendMessage(_hMC, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
    }

    if (enRE)
    {
        Button_SetCheck(_hRE, GTagsSettings._re ? BST_CHECKED : BST_UNCHECKED);
    }
    else
    {
        Button_SetCheck(_hRE, _cmd->RegExp() ? BST_CHECKED : BST_UNCHECKED);
        EnableWindow(_hRE, FALSE);
    }

    if (enMC)
    {
        Button_SetCheck(_hMC, GTagsSettings._mc ? BST_CHECKED : BST_UNCHECKED);
    }
    else
    {
        Button_SetCheck(_hMC, _cmd->MatchCase() ? BST_CHECKED : BST_UNCHECKED);
        EnableWindow(_hMC, FALSE);
    }

    _hKeyHook = SetWindowsHookEx(WH_KEYBOARD, keyHookProc, NULL, GetCurrentThreadId());

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

    CmdId_t cmplId;
    TCHAR tag[cComplAfter + 2];
    CompletionCB complCB;
    ParserPtr_t parser;

    if (_cmd->Id() == FIND_FILE)
    {
        cmplId = AUTOCOMPLETE_FILE;

        tag[0] = _T('/');
        ComboBox_GetText(_hSearch, tag + 1, _countof(tag) - 1);
        tag[cComplAfter + 1] = 0;

        complCB = endCompletion;
        parser.reset(new LineParser);
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

    CmdPtr_t cmpl(new Cmd(cmplId, _T("AutoComplete"), _cmd->Db(), parser,
            tag, false, (Button_GetCheck(_hMC) == BST_CHECKED)));

    if (_cmd->Id() != FIND_DEFINITION)
        cmpl->SkipLibs(true);

    _completionStarted = true;

    CmdEngine::Run(cmpl, complCB);
}


/**
 *  \brief
 */
void SearchWin::halfComplete(const CmdPtr_t& cmpl)
{
    if (SW == NULL)
        return;

    if (ComboBox_GetTextLength(SW->_hSearch) < cComplAfter)
    {
        SW->_completionStarted = false;
        return;
    }

    if (cmpl->Status() == OK)
    {
        cmpl->Id(AUTOCOMPLETE_SYMBOL);

        ParserPtr_t parser(new LineParser);
        cmpl->Parser(parser);

        CmdEngine::Run(cmpl, endCompletion);
    }
    else
    {
        SW->_completionStarted = false;
        SW->_completionDone = true;
    }
}


/**
 *  \brief
 */
void SearchWin::endCompletion(const CmdPtr_t& cmpl)
{
    if (SW == NULL)
        return;

    SW->_completionStarted = false;

    if (ComboBox_GetTextLength(SW->_hSearch) < cComplAfter)
        return;

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

    CText txt(ComboBox_GetTextLength(_hSearch));

    ComboBox_GetText(_hSearch, txt.C_str(), txt.Size());
    int pos = HIWORD(SendMessage(_hSearch, CB_GETEDITSEL, 0, 0));

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

    ComboBox_GetText(_hSearch, filter.C_str(), filter.Size());

    int pos = HIWORD(SendMessage(_hSearch, CB_GETEDITSEL, 0, 0));

    int (*pCompare)(const TCHAR*, const TCHAR*, size_t);

    if (Button_GetCheck(_hMC) == BST_CHECKED)
        pCompare = &_tcsncmp;
    else
        pCompare = &_tcsnicmp;

    ComboBox_ResetContent(_hSearch);
    ComboBox_ShowDropdown(_hSearch, FALSE);
    ComboBox_SetText(_hSearch, filter.C_str());
    PostMessage(_hSearch, CB_SETEDITSEL, 0, MAKELPARAM(pos, pos));

    SendMessage(_hSearch, WM_SETREDRAW, FALSE, 0);

    const LineParser& completion = *(const LineParser*)_completion.get();
    if (filter.Len() == cComplAfter)
    {
        for (const auto& complEntry : completion())
            ComboBox_AddString(_hSearch, complEntry);
    }
    else
    {
        for (const auto& complEntry : completion())
            if (!pCompare(complEntry, filter.C_str(), filter.Len()))
                ComboBox_AddString(_hSearch, complEntry);
    }

    if (ComboBox_GetCount(_hSearch))
    {
        ComboBox_ShowDropdown(_hSearch, TRUE);

        if (_keyPressed == VK_BACK || _keyPressed == VK_DELETE)
            ComboBox_SetText(_hSearch, filter.C_str());

        PostMessage(_hSearch, CB_SETEDITSEL, 0, MAKELPARAM(pos, -1));
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
    bool mc = (Button_GetCheck(_hMC) == BST_CHECKED);

    bool saveCfg = false;

    if (IsWindowEnabled(_hRE) && re != GTagsSettings._re)
    {
        GTagsSettings._re = re;
        saveCfg = true;
    }

    if (IsWindowEnabled(_hMC) && mc != GTagsSettings._mc)
    {
        GTagsSettings._mc = mc;
        saveCfg = true;
    }

    if (saveCfg)
        GTagsSettings.Save();
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
        int pos = HIWORD(SendMessage(_hSearch, CB_GETEDITSEL, 0, 0));

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
        CText tag(ComboBox_GetTextLength(_hSearch));

        ComboBox_GetText(_hSearch, tag.C_str(), tag.Size());

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

            delete SW;
            SW = NULL;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
