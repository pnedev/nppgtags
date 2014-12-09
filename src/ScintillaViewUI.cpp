/**
 *  \file
 *  \brief  GTags result Scintilla view UI
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014 Pavel Nedev
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
#include <windows.h>
#include <commctrl.h>
#include "INpp.h"
#include "DBManager.h"
#include "GTags.h"
#include "ScintillaViewUI.h"
#include "DocLocation.h"


const TCHAR ScintillaViewUI::cClassName[] = _T("ScintillaViewUI");


using namespace GTags;


/**
 *  \brief
 */
void ScintillaViewUI::Show(CmdData& cmd)
{
    AUTOLOCK(_lock);

    add(cmd);

    INpp& npp = INpp::Get();
    npp.UpdateDockingWin(_hWnd);
    npp.ShowDockingWin(_hWnd);
}


/**
 *  \brief
 */
void ScintillaViewUI::Update()
{
    AUTOLOCK(_lock);

    INpp::Get().UpdateDockingWin(_hWnd);
}


/**
 *  \brief
 */
ScintillaViewUI::CmdBranch::CmdBranch(CmdData& cmd) :
    _cmdID(cmd.GetID()), _basePath(cmd.GetDBPath()),
    _cmdName(cmd.GetName()), _cmdOutput(cmd.GetResult())
{
    _cmdName.ToUpper();
    _cmdName += _T(" \"");
    _cmdName += cmd.GetTag();
    _cmdName += _T("\"");
    parseCmdOutput();
}


/**
 *  \brief
 */
void ScintillaViewUI::CmdBranch::parseCmdOutput()
{
    TCHAR* pTmp;

    for (TCHAR* pToken = _tcstok_s(_cmdOutput.C_str(), _T("\n\r"), &pTmp);
        pToken; pToken = _tcstok_s(NULL, _T("\n\r"), &pTmp))
    {
        Leaf leaf = {0};
        if (_cmdID == FIND_FILE)
            leaf.file = pToken;
        else
            leaf.name = pToken;
        _leaves.push_back(leaf);
    }

    if (_cmdID == FIND_FILE)
        return;

    for (unsigned i = 0; i < _leaves.size(); i++)
    {
        Leaf& leaf = _leaves[i];
        unsigned currentResultField = CTAGS_F_TAG;
        for (TCHAR* pToken = _tcstok_s(leaf.name, _T(" \t"), &pTmp);
            pToken; pToken = _tcstok_s(NULL, _T(" \t"), &pTmp))
        {
            switch (currentResultField)
            {
                case CTAGS_F_TAG:
                    leaf.name = pToken;
                    break;

                case CTAGS_F_LINE:
                    leaf.line = pToken;
                    break;

                case CTAGS_F_FILE:
                    leaf.file = pToken;
            }

            if (++currentResultField == CTAGS_F_PREVIEW)
            {
                while (*pTmp == _T(' ') || *pTmp == _T('\t'))
                    pTmp++;
                leaf.preview = pTmp;
                for (int j = _tcslen(leaf.preview) - 1; j > 0; j--)
                    if (leaf.preview[j] == _T('\t'))
                        leaf.preview[j] = _T(' ');
                break;
            }
        }
    }
}


/**
 *  \brief
 */
ScintillaViewUI::ScintillaViewUI()
{
    WNDCLASS wc         = {0};
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = wndProc;
    wc.hInstance        = HInst;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetSysColorBrush(cUIBackgroundColor);
    wc.lpszClassName    = cClassName;

    RegisterClass(&wc);

    createWindow();

    tTbData	data        = {0};
    data.hClient        = _hWnd;
    data.pszName        = const_cast<TCHAR*>(cPluginName);
    data.uMask          = 0;
    data.pszAddInfo     = NULL;
    data.uMask          = DWS_DF_CONT_BOTTOM;
    data.pszModuleName  = DllPath.GetFilename_C_str();
    data.dlgID          = 0;

    INpp& npp = INpp::Get();
    npp.RegisterWin(_hWnd);
    npp.RegisterDockingWin(data);
    npp.HideDockingWin(_hWnd);
}


/**
 *  \brief
 */
ScintillaViewUI::~ScintillaViewUI()
{
    removeAll();

    INpp& npp = INpp::Get();
    npp.DestroySciHandle(_hSci);
    npp.UnregisterWin(_hWnd);

    UnregisterClass(cClassName, NULL);
}


/**
 *  \brief
 */
void ScintillaViewUI::createWindow()
{
    INpp& npp = INpp::Get();
    HWND hOwnerWnd = npp.GetHandle();
    RECT win;
    GetWindowRect(hOwnerWnd, &win);
    DWORD style = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_SIZEBOX;
    _hWnd = CreateWindow(cClassName, cPluginName,
            style, win.left, win.top,
            win.right - win.left, win.bottom - win.top,
            hOwnerWnd, NULL, HInst, (LPVOID) this);

    _hSci = npp.CreateSciHandle(_hWnd);

    AdjustWindowRect(&win, style, FALSE);
    MoveWindow(_hWnd, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);
    GetClientRect(_hWnd, &win);
    MoveWindow(_hSci, 0, 0,
            win.right - win.left, win.bottom - win.top, TRUE);

    SendMessage(_hSci, SCI_STYLERESETDEFAULT, 0, 0);
    SendMessage(_hSci, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
    SendMessage(_hSci, SCI_USEPOPUP, false, 0);
    SendMessage(_hSci, SCI_SETUNDOCOLLECTION, false, 0);
    SendMessage(_hSci, SCI_SETCARETLINEVISIBLE, true, 0);
    SendMessage(_hSci, SCI_SETCARETLINEBACK, 0xD5D8C3, 0);
    SendMessage(_hSci, SCI_SETCARETWIDTH, 0, 0);
    SendMessage(_hSci, SCI_SETREADONLY, true, 0);

#define SCLEX_SEARCHRESULT 150
    SendMessage(_hSci, SCI_SETLEXER, SCLEX_SEARCHRESULT, 0);

    ShowWindow(_hSci, SW_SHOW);
}


/**
 *  \brief
 */
void ScintillaViewUI::add(CmdData& cmd)
{
    CmdBranch branch(cmd);

    char str[512];
    size_t cnt;

    SendMessage(_hSci, SCI_SETSEL, 0, 0);
    SendMessage(_hSci, SCI_SETREADONLY, false, 0);

    SendMessage(_hSci, SCI_STYLESETBOLD, STYLE_DEFAULT, true);
    wcstombs_s(&cnt, str, 512, branch._cmdName.C_str(), _TRUNCATE);
    SendMessage(_hSci, SCI_ADDTEXT, (WPARAM)strlen(str), (LPARAM)str);
    SendMessage(_hSci, SCI_STYLESETBOLD, STYLE_DEFAULT, false);

    unsigned leaves_cnt = branch._leaves.size();
    for (unsigned i = 0; i < leaves_cnt; i++)
    {
        CmdBranch::Leaf& leaf = branch._leaves[i];

        SendMessage(_hSci, SCI_ADDTEXT, 2, (LPARAM)"\n\t");
        wcstombs_s(&cnt, str, 512, leaf.file, _TRUNCATE);
        SendMessage(_hSci, SCI_ADDTEXT, (WPARAM)strlen(str), (LPARAM)str);

        if (branch._cmdID != FIND_FILE)
        {
            for (; i < leaves_cnt; i++)
            {
                CmdBranch::Leaf& next = branch._leaves[i];
                if (_tcscmp(next.file, leaf.file))
                {
                    i--;
                    break;
                }

                SendMessage(_hSci, SCI_ADDTEXT, 3, (LPARAM)"\n\t\t");
                wcstombs_s(&cnt, str, 512, next.preview, _TRUNCATE);
                SendMessage(_hSci, SCI_ADDTEXT,
                        (WPARAM)strlen(str), (LPARAM)str);
            }
        }
    }

    SendMessage(_hSci, SCI_COLOURISE, 0, -1);
    SendMessage(_hSci, SCI_SETREADONLY, true, 0);
    SendMessage(_hSci, SCI_SETSEL, 0, 0);
}


/**
 *  \brief
 */
void ScintillaViewUI::remove()
{
    AUTOLOCK(_lock);

    INpp& npp = INpp::Get();
    npp.UpdateDockingWin(_hWnd);
    npp.HideDockingWin(_hWnd);
    SetFocus(npp.ReadSciHandle());
}


/**
 *  \brief
 */
void ScintillaViewUI::removeAll()
{
    AUTOLOCK(_lock);

    INpp& npp = INpp::Get();
    npp.UpdateDockingWin(_hWnd);
    npp.HideDockingWin(_hWnd);
    SetFocus(npp.ReadSciHandle());
}


/**
 *  \brief
 */
void ScintillaViewUI::onResize(int width, int height)
{
    MoveWindow(_hSci, 0, 0, width, height, TRUE);
}


/**
 *  \brief
 */
LRESULT APIENTRY ScintillaViewUI::wndProc(HWND hwnd, UINT umsg,
        WPARAM wparam, LPARAM lparam)
{
    ScintillaViewUI* ui;

    switch (umsg)
    {
        case WM_CREATE:
            ui = (ScintillaViewUI*)((LPCREATESTRUCT)lparam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToUlong(ui));
            return 0;

        case WM_SETFOCUS:
            ui = reinterpret_cast<ScintillaViewUI*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            SetFocus(ui->_hSci);
            return 0;

        case WM_NOTIFY:
            ui = reinterpret_cast<ScintillaViewUI*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));

            switch (((LPNMHDR)lparam)->code)
            {
                case SCN_DOUBLECLICK:
                    return 0;
            }
            break;

        case WM_SIZE:
            ui = reinterpret_cast<ScintillaViewUI*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            ui->onResize(LOWORD(lparam), HIWORD(lparam));
            return 0;

        case WM_DESTROY:
            return 0;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}
