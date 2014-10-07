/**
 *  \file
 *  \brief  GTags result tree view UI
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
#include "TreeViewUI.h"
#include "DocLocation.h"


const TCHAR TreeViewUI::cClassName[] = _T("TreeViewUI");


using namespace GTags;


/**
 *  \brief
 */
void TreeViewUI::Show(CmdData& cmd)
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
void TreeViewUI::Update()
{
    AUTOLOCK(_lock);

    INpp::Get().UpdateDockingWin(_hWnd);
}


/**
 *  \brief
 */
TreeViewUI::CmdBranch::CmdBranch(CmdData& cmd) :
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
void TreeViewUI::CmdBranch::parseCmdOutput()
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
TreeViewUI::TreeViewUI()
{
    WNDCLASS wc         = {0};
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = wndProc;
    wc.hInstance        = HInst;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetSysColorBrush(cUIBackgroundColor);
    wc.lpszClassName    = cClassName;

    RegisterClass(&wc);

    INITCOMMONCONTROLSEX icex   = {0};
    icex.dwSize                 = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC                  = ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    createWindow();

    tTbData	data        = {0};
    data.hClient        = _hWnd;
    data.pszName        = const_cast<TCHAR*>(cPluginName);
    data.uMask          = 0;
    data.pszAddInfo     = NULL;
    data.uMask          = DWS_DF_CONT_BOTTOM;
    data.pszModuleName  = DllPath.GetFilename_C_str();
    data.dlgID          = 1;

    INpp& npp = INpp::Get();
    npp.RegisterWin(_hWnd);
    npp.RegisterDockingWin(data);
    npp.HideDockingWin(_hWnd);
}


/**
 *  \brief
 */
TreeViewUI::~TreeViewUI()
{
    removeAll();

    INpp::Get().UnregisterWin(_hWnd);

    if (_hFont)
        DeleteObject(_hFont);

    UnregisterClass(cClassName, NULL);
}


/**
 *  \brief
 */
void TreeViewUI::createWindow()
{
    HWND hOwnerWnd = INpp::Get().GetMainHandle();
    RECT win;
    GetWindowRect(hOwnerWnd, &win);
    DWORD style = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_SIZEBOX;
    _hWnd = CreateWindow(cClassName, cPluginName,
            style, win.left, win.top,
            win.right - win.left, win.bottom - win.top,
            hOwnerWnd, NULL, HInst, (LPVOID) this);

    GetClientRect(_hWnd, &win);
    _hTVWnd = CreateWindowEx(0, WC_TREEVIEW, _T("Tree View"),
            WS_CHILD | WS_VISIBLE |
            TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS |
            TVS_DISABLEDRAGDROP | TVS_NOTOOLTIPS |
            TVS_SHOWSELALWAYS | TVS_SINGLEEXPAND,
            0, 0, win.right - win.left, win.bottom - win.top,
            _hWnd, NULL, HInst, NULL);

    HDC hdc = GetWindowDC(hOwnerWnd);
    _hFont = CreateFont(-MulDiv(cUIFontSize,
            GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, cUIFontName);
    ReleaseDC(hOwnerWnd, hdc);
    if (_hFont)
        SendMessage(_hTVWnd, WM_SETFONT, (WPARAM) _hFont, (LPARAM) TRUE);

    AdjustWindowRect(&win, style, FALSE);
    MoveWindow(_hWnd, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);
    GetClientRect(_hWnd, &win);
    MoveWindow(_hTVWnd, 0, 0,
            win.right - win.left, win.bottom - win.top, TRUE);

    TreeView_SetBkColor(_hTVWnd, GetSysColor(cUIBackgroundColor));
}


/**
 *  \brief
 */
void TreeViewUI::add(CmdData& cmd)
{
    CmdBranch* branch = new CmdBranch(cmd);

    remove(branch);

    TVITEM tvItem       = {0};
    tvItem.mask         = TVIF_PARAM | TVIF_TEXT | TVIF_STATE;
    tvItem.lParam       = (LPARAM) branch;
    tvItem.pszText      = branch->_cmdName.C_str();
    tvItem.cchTextMax   = _tcslen(tvItem.pszText);
    tvItem.stateMask    = TVIS_BOLD;
    tvItem.state        = TVIS_BOLD;

    TVINSERTSTRUCT tvIns    = {0};
    tvIns.item              = tvItem;
    tvIns.hInsertAfter      = TVI_FIRST;
    tvIns.hParent           = TVI_ROOT;

    HTREEITEM hLvl0 = TreeView_InsertItem(_hTVWnd, &tvIns);
    HTREEITEM hLvl1 = TVI_FIRST;

    tvItem.state = 0;

    unsigned leaves_cnt = branch->_leaves.size();
    for (unsigned i = 0; i < leaves_cnt; i++)
    {
        CmdBranch::Leaf& leaf = branch->_leaves[i];

        tvItem.lParam       = (LPARAM)&leaf;
        tvItem.pszText      = leaf.file;
        tvItem.cchTextMax   = _tcslen(tvItem.pszText);

        tvIns.item          = tvItem;
        tvIns.hInsertAfter  = hLvl1;
        tvIns.hParent       = hLvl0;

        hLvl1 = TreeView_InsertItem(_hTVWnd, &tvIns);

        if (branch->_cmdID != FIND_FILE)
        {
            HTREEITEM hLvl2 = TVI_FIRST;
            for (; i < leaves_cnt; i++)
            {
                CmdBranch::Leaf& next = branch->_leaves[i];
                if (_tcscmp(next.file, leaf.file))
                {
                    i--;
                    break;
                }

                tvItem.lParam       = (LPARAM) &next;
                tvItem.pszText      = next.preview;
                tvItem.cchTextMax   = _tcslen(tvItem.pszText);

                tvIns.item          = tvItem;
                tvIns.hInsertAfter  = hLvl2;
                tvIns.hParent       = hLvl1;

                hLvl2 = TreeView_InsertItem(_hTVWnd, &tvIns);
            }
        }
    }

    TreeView_Expand(_hTVWnd, hLvl0, TVE_EXPAND);
}


/**
 *  \brief
 */
void TreeViewUI::remove(const CmdBranch* branch)
{
    if (TreeView_GetCount(_hTVWnd) != 0)
    {
        TVITEM tvItem   = {0};
        tvItem.mask     = TVIF_HANDLE | TVIF_PARAM;

		HTREEITEM hItem = TreeView_GetRoot(_hTVWnd);
        while (hItem != NULL)
        {
            tvItem.hItem = hItem;
            TreeView_GetItem(_hTVWnd, &tvItem);

            CmdBranch* old_branch = (CmdBranch*) tvItem.lParam;

            if (*branch == *old_branch)
            {
                delete old_branch;
                TreeView_DeleteItem(_hTVWnd, hItem);
                break;
            }

            hItem = TreeView_GetNextSibling(_hTVWnd, hItem);
        }
    }
}


/**
 *  \brief
 */
void TreeViewUI::remove()
{
    AUTOLOCK(_lock);

    HTREEITEM hItem = TreeView_GetSelection(_hTVWnd);

	HTREEITEM hTmp;
    while ((hTmp = TreeView_GetParent(_hTVWnd, hItem)) != NULL)
        hItem = hTmp;

    TVITEM tvItem   = {0};
    tvItem.mask     = TVIF_HANDLE | TVIF_PARAM;
    tvItem.hItem    = hItem;
    TreeView_GetItem(_hTVWnd, &tvItem);

    CmdBranch* branch = (CmdBranch*)tvItem.lParam;
    delete branch;

    TreeView_DeleteItem(_hTVWnd, hItem);

    INpp& npp = INpp::Get();
    npp.UpdateDockingWin(_hWnd);
    if (TreeView_GetCount(_hTVWnd) == 0)
    {
        npp.HideDockingWin(_hWnd);
        SetFocus(npp.ReadHandle());
    }
}


/**
 *  \brief
 */
void TreeViewUI::removeAll()
{
    AUTOLOCK(_lock);

    if (TreeView_GetCount(_hTVWnd) != 0)
    {
        TVITEM tvItem   = {0};
        tvItem.mask     = TVIF_HANDLE | TVIF_PARAM;

		HTREEITEM hItem;
        while ((hItem = TreeView_GetRoot(_hTVWnd)) != NULL)
        {
            tvItem.hItem = hItem;
            TreeView_GetItem(_hTVWnd, &tvItem);

            CmdBranch* branch = (CmdBranch*)tvItem.lParam;
            delete branch;

            TreeView_DeleteItem(_hTVWnd, hItem);
        }

        INpp& npp = INpp::Get();
        npp.UpdateDockingWin(_hWnd);
        npp.HideDockingWin(_hWnd);
        SetFocus(npp.ReadHandle());
    }
}


/**
 *  \brief
 */
bool TreeViewUI::openItem()
{
    AUTOLOCK(_lock);

    HTREEITEM hItem = TreeView_GetSelection(_hTVWnd);

    if (TreeView_GetChild(_hTVWnd, hItem))
        return false;

    TVITEM tvItem   = {0};
    tvItem.mask     = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT;
    tvItem.hItem    = hItem;
    TreeView_GetItem(_hTVWnd, &tvItem);

	CmdBranch::Leaf* leaf = (CmdBranch::Leaf*)tvItem.lParam;

    while ((hItem = TreeView_GetParent(_hTVWnd, tvItem.hItem)) != NULL)
        tvItem.hItem = hItem;
    TreeView_GetItem(_hTVWnd, &tvItem);

    CmdBranch* branch = (CmdBranch*)tvItem.lParam;

    CPath file(branch->_basePath);
	file += leaf->file;

    INpp& npp = INpp::Get();
    if (!file.FileExists())
    {
        MessageBox(npp.GetMainHandle(),
                _T("File not found, update database and search again"),
                cPluginName, MB_OK | MB_ICONEXCLAMATION);
        return true;
    }

    DocLocation::Get().Push();
    npp.OpenFile(file);
    SetFocus(npp.ReadHandle());

    if (branch->_cmdID == FIND_FILE)
    {
        npp.ClearSelection();
        return true;
    }

    bool wholeWord =
            (branch->_cmdID != GREP && branch->_cmdID != FIND_LITERAL);

    char str[cMaxTagLen];
    size_t cnt;
    wcstombs_s(&cnt, str, cMaxTagLen, leaf->name, _TRUNCATE);
    long line = _ttoi(leaf->line);

    if (!npp.SearchText(str, true, wholeWord,
            npp.PositionFromLine(line), npp.PositionFromLine(line + 1)))
    {
        MessageBox(npp.GetMainHandle(),
                _T("Look-up mismatch, update database and search again"),
                cPluginName, MB_OK | MB_ICONINFORMATION);
    }

    return true;
}


/**
 *  \brief
 */
bool TreeViewUI::onKeyDown(int keyCode)
{
    switch (keyCode)
    {
        case VK_SPACE:
        case VK_RETURN:
            return openItem();

        case VK_DELETE:
            remove();
            break;

        default:
            return false;
    }

    return true;
}


/**
 *  \brief
 */
void TreeViewUI::onResize(int width, int height)
{
    MoveWindow(_hTVWnd, 0, 0, width, height, TRUE);
    TreeView_EnsureVisible(_hTVWnd, TreeView_GetSelection(_hTVWnd));
}


/**
 *  \brief
 */
LRESULT APIENTRY TreeViewUI::wndProc(HWND hwnd, UINT umsg,
        WPARAM wparam, LPARAM lparam)
{
    TreeViewUI* ui;

    switch (umsg)
    {
        case WM_CREATE:
            ui = (TreeViewUI*)((LPCREATESTRUCT)lparam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToUlong(ui));
            return 0;

        case WM_SETFOCUS:
            ui = reinterpret_cast<TreeViewUI*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            SetFocus(ui->_hTVWnd);
            return 0;

        case WM_NOTIFY:
            ui = reinterpret_cast<TreeViewUI*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));

            switch (((LPNMHDR)lparam)->code)
            {
                case NM_DBLCLK:
                    ui->openItem();
                    break;

                case NM_RCLICK:
                    ui->remove();
                    return 1;

                case TVN_KEYDOWN:
                    if (ui->onKeyDown(((LPNMTVKEYDOWN)lparam)->wVKey))
                        return 1;
                    break;
            }
            break;

        case WM_SIZE:
            ui = reinterpret_cast<TreeViewUI*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            ui->onResize(LOWORD(lparam), HIWORD(lparam));
            return 0;

        case WM_DESTROY:
            return 0;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}
