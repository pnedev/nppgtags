/**
 *  \file
 *  \brief  GTags config window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2015-2022 Pavel Nedev
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
#include <commctrl.h>
#include <richedit.h>
#include <vector>
#include "Common.h"
#include "INpp.h"
#include "GTags.h"
#include "SettingsWin.h"
#include "Cmd.h"
#include "CmdEngine.h"
#include "ResultWin.h"


namespace GTags
{

const TCHAR SettingsWin::cClassName[]   = _T("SettingsWin");
const int SettingsWin::cBackgroundColor = COLOR_BTNFACE;
const int SettingsWin::cFontSize        = 10;


SettingsWin* SettingsWin::SW = NULL;


/**
 *  \brief
 */
SettingsWin::Tab::Tab(const DbHandle db) : _db(db), _updateDb(false)
{
    if (db)
        _cfg = db->GetConfig();
    else
        _cfg = GTagsSettings._genericDbCfg;
}


/**
 *  \brief
 */
SettingsWin::Tab::~Tab()
{
    if (_db)
    {
        if (_updateDb)
        {
            CmdPtr_t cmd(new Cmd(CREATE_DATABASE, _db));
            CmdEngine::Run(cmd, SettingsWin::dbWriteReady);
        }
        else
        {
            DbManager::Get().PutDb(_db);
        }
    }
}


/**
 *  \brief
 */
void SettingsWin::Show()
{
    if (!createWin())
        return;

    SW->fillTabData();

    ShowWindow(SW->_hWnd, SW_SHOWNORMAL);
    UpdateWindow(SW->_hWnd);
}


/**
 *  \brief
 */
void SettingsWin::Show(const DbHandle& db)
{
    if (!createWin())
    {
        DbManager::Get().PutDb(db);
        return;
    }

    Tab* tab = new Tab(db);

    TCHAR buf[64]   = _T("Current database config");
    TCITEM tci      = {0};
    tci.mask        = TCIF_TEXT | TCIF_PARAM;
    tci.pszText     = buf;
    tci.lParam      = (LPARAM)tab;

    int tabsCount = TabCtrl_GetItemCount(SW->_hTab);

    int i = TabCtrl_InsertItem(SW->_hTab, tabsCount, &tci);
    if (i == -1)
    {
        delete tab;
        SendMessage(SW->_hWnd, WM_CLOSE, 0, 0);
        DbManager::Get().PutDb(db);
        return;
    }

    ++tabsCount;

    RECT tabRC;
    TabCtrl_GetItemRect(SW->_hTab, 0, &tabRC);

    RECT tabWin;
    GetWindowRect(SW->_hTab, &tabWin);

    TabCtrl_SetItemSize(SW->_hTab, (tabWin.right - tabWin.left - 3 - (5 * tabsCount)) / tabsCount,
            tabRC.bottom - tabRC.top);

    TabCtrl_SetCurSel(SW->_hTab, i);
    SW->_activeTab = tab;
    SW->fillTabData();

    ShowWindow(SW->_hWnd, SW_SHOWNORMAL);
    UpdateWindow(SW->_hWnd);
}


/**
 *  \brief
 */
bool SettingsWin::createWin()
{
    if (SW)
    {
        SetFocus(SW->_hWnd);
        return false;
    }

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
    icex.dwICC                  = ICC_STANDARD_CLASSES;

    InitCommonControlsEx(&icex);

    HWND hOwner = INpp::Get().GetHandle();

    SW = new SettingsWin();
    if (SW->composeWindow(hOwner) == NULL)
    {
        delete SW;
        SW = NULL;
        return false;
    }

    return true;
}


/**
 *  \brief
 */
SettingsWin::SettingsWin() : _hKeyHook(NULL), _hFont(NULL), _hFontInfo(NULL)
{
}


/**
 *  \brief
 */
SettingsWin::~SettingsWin()
{
    for (int i = TabCtrl_GetItemCount(_hTab); i; --i)
    {
        Tab* tab = getTab(i - 1);
        if (tab)
            delete tab;
        TabCtrl_DeleteItem(_hTab, i - 1);
    }

    if (_hKeyHook)
        UnhookWindowsHookEx(_hKeyHook);

    if (_hFont)
        DeleteObject(_hFont);

    if (_hFontInfo)
        DeleteObject(_hFontInfo);

    UnregisterClass(cClassName, HMod);
}


/**
 *  \brief
 */
HWND SettingsWin::composeWindow(HWND hOwner)
{
    int txtHeight;
    int txtInfoHeight;
    {
        HDC hdc = GetWindowDC(hOwner);

        _hFont      = Tools::CreateFromSystemMessageFont(hdc, cFontSize);
        _hFontInfo  = Tools::CreateFromSystemMenuFont(hdc, cFontSize - 2);

        txtHeight       = Tools::GetFontHeight(hdc, _hFont) + 1;
        txtInfoHeight   = Tools::GetFontHeight(hdc, _hFontInfo) + 1;

        ReleaseDC(hOwner, hdc);
    }

    DWORD styleEx   = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;
    DWORD style     = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;

    RECT win = Tools::GetWinRect(hOwner, styleEx, style, 500, 13 * txtHeight + txtInfoHeight + 270);
    int width = win.right - win.left;
    int height = win.bottom - win.top;

    _hWnd = CreateWindowEx(styleEx, cClassName, PLUGIN_NAME _T(" Settings"), style,
            win.left, win.top, width, height,
            hOwner, NULL, HMod, NULL);
    if (_hWnd == NULL)
        return NULL;

    GetClientRect(_hWnd, &win);
    const int totalWidth = win.right - win.left;

    width   = win.right - win.left - 30;
    height  = win.bottom - win.top;

    int xPos = win.left + 15;
    int yPos = win.top + 15;

    _hEnDefDb = CreateWindowEx(0, _T("BUTTON"), _T("Enable default database"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            xPos, yPos, (width / 2), txtHeight + 10,
            _hWnd, NULL, HMod, NULL);

    _hSetDefDb = CreateWindowEx(0, _T("BUTTON"), _T("Set DB"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xPos + (width / 2) + 5, yPos, (width / 4) - 5, 25,
            _hWnd, NULL, HMod, NULL);

    _hUpdDefDb = CreateWindowEx(0, _T("BUTTON"), _T("Update DB"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xPos + (width / 2) + (width / 4) + 5, yPos, (width / 4) - 5, 25,
            _hWnd, NULL, HMod, NULL);

    yPos += (((txtHeight + 10 > 25) ? txtHeight + 10 : 25) + 5);
    win.top     = yPos;
    win.bottom  = win.top + txtHeight;
    win.left    = xPos;
    win.right   = win.left + width;

    styleEx = WS_EX_CLIENTEDGE;
    style   = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;

    AdjustWindowRectEx(&win, style, FALSE, styleEx);
    _hDefDb = CreateWindowEx(styleEx, RICHEDIT_CLASS, NULL, style,
            win.left + (win.right - win.left - width) / 2, win.top, width, win.bottom - win.top,
            _hWnd, NULL, HMod, NULL);

    yPos += (win.bottom - win.top + 25);
    _hTab = CreateWindowEx(WS_EX_TRANSPARENT, WC_TABCONTROL, NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_BUTTONS | TCS_FIXEDWIDTH | TCS_FOCUSNEVER,
            xPos, yPos, width, height - yPos - 50,
            _hWnd, NULL, HMod, NULL);

    _activeTab = new Tab;
    {
        TCHAR buf[64]   = _T("Generic database config");
        TCITEM tci      = {0};
        tci.mask        = TCIF_TEXT | TCIF_PARAM;
        tci.pszText     = buf;
        tci.lParam      = (LPARAM)_activeTab;

        int i = TabCtrl_InsertItem(_hTab, TabCtrl_GetItemCount(_hTab), &tci);
        if (i == -1)
        {
            delete _activeTab;
            SendMessage(_hWnd, WM_CLOSE, 0, 0);

            return NULL;
        }

        TabCtrl_SetCurSel(_hTab, i);
    }

    RECT tabRC;
    TabCtrl_GetItemRect(_hTab, 0, &tabRC);

    GetClientRect(_hWnd, &win);
    win.top     = yPos;
    win.bottom  = win.top + height - yPos - 50;
    win.left    = xPos;
    win.right   = win.left + width;

    TabCtrl_SetItemSize(_hTab, width - 3, tabRC.bottom - tabRC.top);

    TabCtrl_AdjustRect(_hTab, FALSE, &win);
    width = win.right - win.left - 10;
    xPos = win.left + 5;
    yPos = win.top + 15;

    _hInfo = CreateWindowEx(0, _T("STATIC"), _T("Below settings apply to all new databases"),
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_SUNKEN,
            xPos, yPos, width, 2 * txtHeight + 10,
            _hWnd, NULL, HMod, NULL);

    yPos += (2 * txtHeight + 30);
    _hParserInfo = CreateWindowEx(0, _T("STATIC"), _T("Code Parser"),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            xPos, yPos, width, txtInfoHeight,
            _hWnd, NULL, HMod, NULL);

    yPos += (txtInfoHeight + 5);
    _hParser = CreateWindowEx(0, WC_COMBOBOX, NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
            xPos, yPos, (width / 2) - 10, txtHeight + 10,
            _hWnd, NULL, HMod, NULL);

    _hAutoUpdDb = CreateWindowEx(0, _T("BUTTON"), _T("Auto-update database"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            xPos + (width / 2) + 30, yPos, (width / 2) - 50, txtHeight + 10,
            _hWnd, NULL, HMod, NULL);

    yPos += (txtHeight + 30);
    _hEnLibDb = CreateWindowEx(0, _T("BUTTON"), _T("Enable library databases"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            xPos, yPos, (width / 2), txtHeight + 10,
            _hWnd, NULL, HMod, NULL);

    _hAddLibDb = CreateWindowEx(0, _T("BUTTON"), _T("Add DB"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xPos + (width / 2) + 5, yPos, (width / 4) - 5, 25,
            _hWnd, NULL, HMod, NULL);

    _hUpdLibDbs = CreateWindowEx(0, _T("BUTTON"), _T("Update DBs"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xPos + (width / 2) + (width / 4) + 5, yPos, (width / 4) - 5, 25,
            _hWnd, NULL, HMod, NULL);

    yPos += (((txtHeight + 10 > 25) ? txtHeight + 10 : 25) + 5);
    win.top     = yPos;
    win.bottom  = win.top + 3 * txtHeight;
    win.left    = xPos;
    win.right   = win.left + width;

    styleEx = WS_EX_CLIENTEDGE;
    style   = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
            ES_NOOLEDRAGDROP | ES_MULTILINE | ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL;

    AdjustWindowRectEx(&win, style, FALSE, styleEx);
    _hLibDbs = CreateWindowEx(styleEx, RICHEDIT_CLASS, NULL, style,
            win.left + (win.right - win.left - width) / 2, win.top, width, win.bottom - win.top,
            _hWnd, NULL, HMod, NULL);

    yPos += (win.bottom - win.top + 10);
    _hEnPathFilter = CreateWindowEx(0, _T("BUTTON"), _T("Ignore sub-folders"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            xPos, yPos, (width / 2), txtHeight + 10,
            _hWnd, NULL, HMod, NULL);

    _hAddPathFilter = CreateWindowEx(0, _T("BUTTON"), _T("Add sub-folder"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xPos + (width / 2) + (width / 4) + 5, yPos, (width / 4) - 5, 25,
            _hWnd, NULL, HMod, NULL);

    yPos += (((txtHeight + 10 > 25) ? txtHeight + 10 : 25) + 5);
    win.top     = yPos;
    win.bottom  = win.top + 3 * txtHeight;
    win.left    = xPos;
    win.right   = win.left + width;

    AdjustWindowRectEx(&win, style, FALSE, styleEx);
    _hPathFilters = CreateWindowEx(styleEx, RICHEDIT_CLASS, NULL, style,
            win.left + (win.right - win.left - width) / 2, win.top, width, win.bottom - win.top,
            _hWnd, NULL, HMod, NULL);

    yPos += (win.bottom - win.top + 20);
    width = totalWidth / 5;
    _hSave = CreateWindowEx(0, _T("BUTTON"), _T("Save"),
            WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_PUSHBUTTON,
            width, yPos, width, 25,
            _hWnd, NULL, HMod, NULL);

    _hCancel = CreateWindowEx(0, _T("BUTTON"), _T("Cancel"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            3 * width, yPos, width, 25,
            _hWnd, NULL, HMod, NULL);

    if (_hFont)
    {
        SendMessage(_hDefDb, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hInfo, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hParser, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hLibDbs, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hPathFilters, WM_SETFONT, (WPARAM)_hFont, TRUE);
    }

    if (_hFontInfo)
    {
        SendMessage(_hParserInfo, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hEnDefDb, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hSetDefDb, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hUpdDefDb, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hTab, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hAutoUpdDb, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hEnLibDb, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hAddLibDb, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hUpdLibDbs, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hEnPathFilter, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hAddPathFilter, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hSave, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
        SendMessage(_hCancel, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);
    }

    for (unsigned i = 0; DbConfig::Parser(i); ++i)
        SendMessage(_hParser, CB_ADDSTRING, 0, (LPARAM)DbConfig::Parser(i));

	SendMessage(_hDefDb, EM_SETEVENTMASK, 0, ENM_NONE);
    Edit_SetText(_hDefDb, GTagsSettings._defDbPath.C_str());
    SendMessage(_hDefDb, EM_SETEVENTMASK, 0, ENM_CHANGE);

    if (GTagsSettings._useDefDb)
    {
        EnableWindow(_hSetDefDb, TRUE);
        EnableWindow(_hUpdDefDb, TRUE);
        Edit_Enable(_hDefDb, TRUE);
        SendMessage(_hDefDb, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_WINDOW));
    }
    else
    {
        EnableWindow(_hSetDefDb, FALSE);
        EnableWindow(_hUpdDefDb, FALSE);
        Edit_Enable(_hDefDb, FALSE);
        SendMessage(_hDefDb, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    }

    Button_SetCheck(_hEnDefDb, GTagsSettings._useDefDb ? BST_CHECKED : BST_UNCHECKED);

    _hKeyHook = SetWindowsHookEx(WH_KEYBOARD, keyHookProc, NULL, GetCurrentThreadId());

    return _hWnd;
}


/**
 *  \brief
 */
SettingsWin::Tab* SettingsWin::getTab(int i)
{
    if (i == -1)
    {
        i = TabCtrl_GetCurSel(_hTab);
        if (i == -1)
            return NULL;
    }

    TCITEM tci  = {0};
    tci.mask    = TCIF_PARAM;

    if (!TabCtrl_GetItem(_hTab, i, &tci))
        return NULL;

    return (Tab*)tci.lParam;
}


/**
 *  \brief
 */
bool SettingsWin::isDbOpen(const CPath& dbPath)
{
    for (int i = TabCtrl_GetItemCount(_hTab); i; --i)
    {
        Tab* tab = getTab(i - 1);

        if (tab->_db && tab->_db->GetPath() == dbPath)
            return true;
    }

    return false;
}


/**
 *  \brief
 */
void SettingsWin::onUpdateDefDb()
{
    int len = Edit_GetTextLength(_hDefDb);
    if (!len)
        return;

    CPath defDb(len);

    Edit_GetText(_hDefDb, defDb.C_str(), defDb.Size());

    defDb.AsFolder();
    if (defDb.Exists())
        createDatabase(defDb, dbWriteReady);
}


/**
 *  \brief
 */
void SettingsWin::onUpdateLibDb()
{
    int len = Edit_GetTextLength(_hLibDbs);
    if (!len)
        return;

    CText buf(len);

    Edit_GetText(_hLibDbs, buf.C_str(), buf.Size());

    std::vector<CPath> dbs;

    TCHAR* pTmp = NULL;
    for (TCHAR* ptr = _tcstok_s(buf.C_str(), _T("\n\r"), &pTmp); ptr; ptr = _tcstok_s(NULL, _T("\n\r"), &pTmp))
    {
        CPath db(ptr);
        db.AsFolder();
        if (db.Exists())
            dbs.push_back(db);
    }

    unsigned updateCount = dbs.size();

    if (updateCount)
        for (unsigned i = 0; i < updateCount; ++i)
            createDatabase(dbs[i], dbWriteReady);
}


/**
 *  \brief
 */
void SettingsWin::onTabChange()
{
    readTabData();
    _activeTab = getTab();
    fillTabData();
    SetFocus(_hInfo);
}


/**
 *  \brief
 */
void SettingsWin::onSave()
{
    readTabData();

    bool ret = true;

    for (int i = TabCtrl_GetItemCount(_hTab); i; --i)
    {
        Tab* tab = getTab(i - 1);
        ret = ret && saveTab(tab);
    }

    if (ret)
        SendMessage(_hWnd, WM_CLOSE, 0, 0);
}


/**
 *  \brief
 */
void SettingsWin::fillTabData()
{
    if (_activeTab->_db)
    {
        CText txt(_T("Below settings apply to database at\n\""));
        txt += _activeTab->_db->GetPath();
        txt += _T("\"");
		SetWindowText(_hInfo, txt.C_str());
		SetWindowText(_hParserInfo, _T("Code Parser (database will be re-created on change!)"));
    }
    else
    {
		SetWindowText(_hInfo, _T("Below settings apply to all new databases"));
		SetWindowText(_hParserInfo, _T("Code Parser"));
    }

	SendMessage(_hLibDbs, EM_SETEVENTMASK, 0, ENM_NONE);

    if (_activeTab->_cfg._libDbPaths.empty())
    {
        Edit_SetText(_hLibDbs, _T(""));
    }
    else
    {
        CText libDbPaths;
        _activeTab->_cfg.DbPathsToBuf(libDbPaths, _T('\n'));
        Edit_SetText(_hLibDbs, libDbPaths.C_str());
    }

    SendMessage(_hLibDbs, EM_SETEVENTMASK, 0, ENM_CHANGE);

	SendMessage(_hPathFilters, EM_SETEVENTMASK, 0, ENM_NONE);

    if (_activeTab->_cfg._pathFilters.empty())
    {
        Edit_SetText(_hPathFilters, _T(""));
    }
    else
    {
        CText pathFilters;
        _activeTab->_cfg.FiltersToBuf(pathFilters, _T('\n'));
        Edit_SetText(_hPathFilters, pathFilters.C_str());
    }

    SendMessage(_hPathFilters, EM_SETEVENTMASK, 0, ENM_CHANGE);

    if (_activeTab->_cfg._useLibDb)
    {
        EnableWindow(_hAddLibDb, TRUE);
        EnableWindow(_hUpdLibDbs, TRUE);
        Edit_Enable(_hLibDbs, TRUE);
        SendMessage(_hLibDbs, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_WINDOW));
    }
    else
    {
        EnableWindow(_hAddLibDb, FALSE);
        EnableWindow(_hUpdLibDbs, FALSE);
        Edit_Enable(_hLibDbs, FALSE);
        SendMessage(_hLibDbs, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    }

    if (_activeTab->_cfg._usePathFilter)
    {
        EnableWindow(_hAddPathFilter, _activeTab->_db ? TRUE : FALSE);
        Edit_Enable(_hPathFilters, TRUE);
        SendMessage(_hPathFilters, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_WINDOW));
    }
    else
    {
        EnableWindow(_hAddPathFilter, FALSE);
        Edit_Enable(_hPathFilters, FALSE);
        SendMessage(_hPathFilters, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    }

    Button_SetCheck(_hAutoUpdDb, _activeTab->_cfg._autoUpdate ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(_hEnLibDb, _activeTab->_cfg._useLibDb ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(_hEnPathFilter, _activeTab->_cfg._usePathFilter ? BST_CHECKED : BST_UNCHECKED);

    SendMessage(_hParser, CB_SETCURSEL, _activeTab->_cfg._parserIdx, 0);
}


/**
 *  \brief
 */
void SettingsWin::readTabData()
{
    _activeTab->_cfg._libDbPaths.clear();

    int len = Edit_GetTextLength(_hLibDbs);
    if (len)
    {
        CText libDbPaths(len);
        Edit_GetText(_hLibDbs, libDbPaths.C_str(), libDbPaths.Size());
        _activeTab->_cfg.DbPathsFromBuf(libDbPaths.C_str(), _T("\n\r"));
    }

    _activeTab->_cfg._pathFilters.clear();

    len = Edit_GetTextLength(_hPathFilters);
    if (len)
    {
        CText pathFilters(len);
        Edit_GetText(_hPathFilters, pathFilters.C_str(), pathFilters.Size());
        _activeTab->_cfg.FiltersFromBuf(pathFilters.C_str(), _T("\n\r"));
    }

    _activeTab->_cfg._autoUpdate    = (Button_GetCheck(_hAutoUpdDb) == BST_CHECKED) ? true : false;
    _activeTab->_cfg._useLibDb      = (Button_GetCheck(_hEnLibDb) == BST_CHECKED) ? true : false;
    _activeTab->_cfg._usePathFilter = (Button_GetCheck(_hEnPathFilter) == BST_CHECKED) ? true : false;

    _activeTab->_cfg._parserIdx = SendMessage(_hParser, CB_GETCURSEL, 0, 0);
}


/**
 *  \brief
 */
bool SettingsWin::saveDbConfig(SettingsWin::Tab* tab)
{
    CPath cfgFolder(tab->_db->GetPath());

    if (!tab->_cfg.SaveToFolder(cfgFolder))
    {
        cfgFolder += cPluginCfgFileName;

        CText msg(_T("Failed saving config to\n\""));
        msg += cfgFolder.C_str();
        msg += _T("\"\nIs the path read only?");

        MessageBox(_hWnd, msg.C_str(), cPluginName, MB_OK | MB_ICONEXCLAMATION);

        return false;
    }

    if (tab->_db->GetConfig()._parserIdx != tab->_cfg._parserIdx)
        tab->_updateDb = true;

    tab->_db->SetConfig(tab->_cfg);

    return true;
}


/**
 *  \brief
 */
bool SettingsWin::saveSettings(const Settings& newSettings)
{
    if (!newSettings.Save())
    {
        CPath cfgFile;
        INpp::Get().GetPluginsConfDir(cfgFile);
        cfgFile += cPluginCfgFileName;

        CText msg(_T("Failed saving config to\n\""));
        msg += cfgFile.C_str();
        msg += _T("\"\nIs the path read only?");

        MessageBox(_hWnd, msg.C_str(), cPluginName, MB_OK | MB_ICONEXCLAMATION);

        return false;
    }

    GTagsSettings = newSettings;

    return true;
}


/**
 *  \brief
 */
bool SettingsWin::saveTab(SettingsWin::Tab* tab)
{
    if (tab->_db)
    {
        CPath cfgFile(tab->_db->GetPath());
        cfgFile += cPluginCfgFileName;

        if (cfgFile.FileExists() && tab->_db->GetConfig() == tab->_cfg)
            return true;

        return saveDbConfig(tab);
    }

    Settings newSettings;

    newSettings._genericDbCfg = tab->_cfg;
    newSettings._useDefDb = (Button_GetCheck(_hEnDefDb) == BST_CHECKED) ? true : false;

    const int len = Edit_GetTextLength(_hDefDb);
    if (len)
    {
        newSettings._defDbPath.Resize(len);
        Edit_GetText(_hDefDb, newSettings._defDbPath.C_str(), newSettings._defDbPath.Size());

        newSettings._defDbPath.AsFolder();
        if (!newSettings._defDbPath.Exists())
            newSettings._defDbPath.Clear();
    }

    newSettings._re = GTagsSettings._re;
    newSettings._ic = GTagsSettings._ic;

    CPath cfgFile;
    INpp::Get().GetPluginsConfDir(cfgFile);
    cfgFile += cPluginCfgFileName;

    if (cfgFile.FileExists() && GTagsSettings == newSettings)
        return true;

    return saveSettings(newSettings);
}


/**
 *  \brief
 */
void SettingsWin::fillDefDb(const CPath& defDb)
{
    Edit_SetText(_hDefDb, defDb.C_str());

    SetFocus(_hDefDb);
    Edit_SetSel(_hDefDb, defDb.Len(), defDb.Len());
    Edit_ScrollCaret(_hDefDb);
}


/**
 *  \brief
 */
void SettingsWin::fillMissing(HWND SettingsWin::*editCtrl, const CText& entry)
{
    int len = Edit_GetTextLength(this->*editCtrl);
    CText buf(len);
    bool found = false;

    if (len)
    {
        Edit_GetText(this->*editCtrl, buf.C_str(), buf.Size());

        int entryLen = entry.Len();

        for (TCHAR* ptr = _tcsstr(buf.C_str(), entry.C_str()); ptr; ptr = _tcsstr(ptr, entry.C_str()))
        {
            if (ptr[entryLen] == 0 || ptr[entryLen] == _T('\n') || ptr[entryLen] == _T('\r'))
            {
                found = true;
                break;
            }
        }

        if (!found)
            buf += _T('\n');
    }

    if (!found)
    {
        buf += entry;
        Edit_SetText(this->*editCtrl, buf.C_str());
    }

    SetFocus(this->*editCtrl);
    Edit_SetSel(this->*editCtrl, buf.Len(), buf.Len());
    Edit_ScrollCaret(this->*editCtrl);
}


/**
 *  \brief
 */
inline void SettingsWin::fillLibDb(const CPath& lib)
{
    fillMissing(&SettingsWin::_hLibDbs, lib);
}


/**
 *  \brief
 */
inline void SettingsWin::fillPathFilter(const CPath& filter)
{
    fillMissing(&SettingsWin::_hPathFilters, filter);
}


/**
 *  \brief
 */
bool SettingsWin::createDatabase(CPath& dbPath, CompletionCB complCB)
{
    if (dbPath.IsEmpty())
    {
        if (!Tools::BrowseForFolder(_hWnd, dbPath))
            return false;
    }

    if (isDbOpen(dbPath))
        return false;

    DbHandle db;

    if (DbManager::Get().DbExistsInFolder(dbPath))
    {
        CText msg(_T("Database at\n\""));
        msg += dbPath;
        msg += _T("\"\nexists.\nRe-create?");
        int choice = MessageBox(_hWnd, msg.C_str(), cPluginName, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
        if (choice != IDYES)
            return false;

        bool success;
        db = DbManager::Get().GetDbAt(dbPath, true, &success);

        if (!success)
        {
            MessageBox(_hWnd, _T("GTags database is currently in use.\nPlease try again later."),
                    cPluginName, MB_OK | MB_ICONINFORMATION);
            return false;
        }
    }
    else
    {
        db = DbManager::Get().RegisterDb(dbPath);
    }

    CmdPtr_t cmd(new Cmd(CREATE_DATABASE, db));
    CmdEngine::Run(cmd, complCB);

    return true;
}


/**
 *  \brief
 */
void SettingsWin::dbWriteReady(const CmdPtr_t& cmd)
{
    if (cmd->Status() != OK)
        DbManager::Get().UnregisterDb(cmd->Db());
    else
        DbManager::Get().PutDb(cmd->Db());

    if (cmd->Status() == RUN_ERROR)
    {
        HWND hWnd = (SW == NULL) ? INpp::Get().GetHandle() : SW->_hWnd;
        MessageBox(hWnd, _T("Running GTags failed"), cmd->Name(), MB_OK | MB_ICONERROR);
    }
    else if (cmd->Result())
    {
        CText msg(cmd->Result());
        HWND hWnd = (SW == NULL) ? INpp::Get().GetHandle() : SW->_hWnd;
        MessageBox(hWnd, msg.C_str(), cmd->Name(), MB_OK | MB_ICONEXCLAMATION);
    }

    if (cmd->Status() == OK)
        ResultWin::NotifyDBUpdate(cmd);
}


/**
 *  \brief
 */
void SettingsWin::createDefDbCB(const CmdPtr_t& cmd)
{
    dbWriteReady(cmd);

    if (SW == NULL)
        return;

    EnableWindow(SW->_hWnd, TRUE);

    if (cmd->Status() == OK)
        SW->fillDefDb(cmd->Db()->GetPath());
}


/**
 *  \brief
 */
void SettingsWin::createLibDbCB(const CmdPtr_t& cmd)
{
    dbWriteReady(cmd);

    if (SW == NULL)
        return;

    EnableWindow(SW->_hWnd, TRUE);

    if (cmd->Status() == OK)
        SW->fillLibDb(cmd->Db()->GetPath());
}


/**
 *  \brief
 */
LRESULT CALLBACK SettingsWin::keyHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0)
    {
        HWND hWnd = GetFocus();
        if ((SW->_hWnd == hWnd) || IsChild(SW->_hWnd, hWnd))
        {
            // Key is pressed
            if (!(lParam & (1 << 31)))
            {
                if (wParam == VK_ESCAPE)
                {
                    SendMessage(SW->_hWnd, WM_CLOSE, 0, 0);
                    return 1;
                }
            }
        }
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}


/**
 *  \brief
 */
LRESULT APIENTRY SettingsWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_CTLCOLORSTATIC:
            SetBkColor((HDC) wParam, GetSysColor(cBackgroundColor));
        return (INT_PTR) GetSysColorBrush(cBackgroundColor);

        case WM_COMMAND:
            if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                DestroyCaret();
                return 0;
            }
            if (HIWORD(wParam) == BN_CLICKED)
            {
                if ((HWND)lParam == SW->_hSave)
                {
                    SW->onSave();
                    return 0;
                }

                if ((HWND)lParam == SW->_hCancel)
                {
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    return 0;
                }

                if ((HWND)lParam == SW->_hEnDefDb)
                {
                    BOOL en;
                    int color;

                    if (Button_GetCheck(SW->_hEnDefDb) == BST_CHECKED)
                    {
                        en = TRUE;
                        color = COLOR_WINDOW;
                    }
                    else
                    {
                        en = FALSE;
                        color = COLOR_BTNFACE;
                    }

                    EnableWindow(SW->_hSetDefDb, en);
                    EnableWindow(SW->_hUpdDefDb, en);
                    Edit_Enable(SW->_hDefDb, en);
                    SendMessage(SW->_hDefDb, EM_SETBKGNDCOLOR, 0, GetSysColor(color));

                    EnableWindow(SW->_hSave, TRUE);

                    return 0;
                }

                if ((HWND)lParam == SW->_hEnLibDb)
                {
                    BOOL en;
                    int color;

                    if (Button_GetCheck(SW->_hEnLibDb) == BST_CHECKED)
                    {
                        en = TRUE;
                        color = COLOR_WINDOW;
                    }
                    else
                    {
                        en = FALSE;
                        color = COLOR_BTNFACE;
                    }

                    EnableWindow(SW->_hAddLibDb, en);
                    EnableWindow(SW->_hUpdLibDbs, en);
                    Edit_Enable(SW->_hLibDbs, en);
                    SendMessage(SW->_hLibDbs, EM_SETBKGNDCOLOR, 0, GetSysColor(color));

                    EnableWindow(SW->_hSave, TRUE);

                    return 0;
                }

                if ((HWND)lParam == SW->_hEnPathFilter)
                {
                    BOOL en;
                    int color;

                    if (Button_GetCheck(SW->_hEnPathFilter) == BST_CHECKED)
                    {
                        en = TRUE;
                        color = COLOR_WINDOW;
                    }
                    else
                    {
                        en = FALSE;
                        color = COLOR_BTNFACE;
                    }

                    EnableWindow(SW->_hAddPathFilter, (en && SW->_activeTab->_db) ? TRUE : FALSE);
                    Edit_Enable(SW->_hPathFilters, en);
                    SendMessage(SW->_hPathFilters, EM_SETBKGNDCOLOR, 0, GetSysColor(color));

                    EnableWindow(SW->_hSave, TRUE);

                    return 0;
                }

                if ((HWND)lParam == SW->_hSetDefDb)
                {
                    CPath defDbPath;

                    if (SW->createDatabase(defDbPath, createDefDbCB))
                    {
                        EnableWindow(hWnd, FALSE);
                        SetFocus(INpp::Get().GetSciHandle());
                    }
                    else if (!defDbPath.IsEmpty())
                    {
                        SW->fillDefDb(defDbPath);
                    }

                    return 0;
                }

                if ((HWND)lParam == SW->_hUpdDefDb)
                {
                    SW->onUpdateDefDb();
                    return 0;
                }

                if ((HWND)lParam == SW->_hAddLibDb)
                {
                    CPath libraryPath;

                    if (SW->createDatabase(libraryPath, createLibDbCB))
                    {
                        EnableWindow(hWnd, FALSE);
                        SetFocus(INpp::Get().GetSciHandle());
                    }
                    else if (!libraryPath.IsEmpty())
                    {
                        SW->fillLibDb(libraryPath);
                    }

                    return 0;
                }

                if ((HWND)lParam == SW->_hUpdLibDbs)
                {
                    SW->onUpdateLibDb();
                    return 0;
                }

                if ((HWND)lParam == SW->_hAddPathFilter)
                {
                    if (SW->_activeTab->_db)
                    {
                        CPath filterPath = SW->_activeTab->_db->GetPath();

                        if (Tools::BrowseForFolder(SW->_hWnd, filterPath,
                            _T("Select database sub-folder to ignore"), true))
                        {
                            if (filterPath.IsSubpathOf(SW->_activeTab->_db->GetPath()))
                            {
                                filterPath.Erase(0, SW->_activeTab->_db->GetPath().Len());
                                SW->fillPathFilter(filterPath);
                            }
                        }
                    }

                    return 0;
                }

                if ((HWND)lParam == SW->_hAutoUpdDb)
                    EnableWindow(SW->_hSave, TRUE);
            }
            else if (HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CBN_SELCHANGE)
            {
                EnableWindow(SW->_hSave, TRUE);
            }
        break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
            {
                SW->onTabChange();
                return 0;
            }
        break;

        case WM_DESTROY:
            DestroyCaret();
            delete SW;
            SW = NULL;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
