/**
 *  \file
 *  \brief  GTags config window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2015-2016 Pavel Nedev
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
            CmdPtr_t cmd(new Cmd(CREATE_DATABASE, _T("Create Database"), _db));
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

    int i = TabCtrl_InsertItem(SW->_hTab, TabCtrl_GetItemCount(SW->_hTab), &tci);
    if (i == -1)
    {
        delete tab;
        SendMessage(SW->_hWnd, WM_CLOSE, 0, 0);

        return;
    }

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
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

    NONCLIENTMETRICS ncmInfo;
    ncmInfo.cbSize = sizeof(ncmInfo);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncmInfo.cbSize, &ncmInfo, 0);

    int txtHeight;
    int txtInfoHeight;
    {
        const int fontInfoSize = 8;

        TEXTMETRIC tm;
        HDC hdc = GetWindowDC(hOwner);

        ncm.lfMessageFont.lfHeight = -MulDiv(cFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ncmInfo.lfMessageFont.lfHeight = -MulDiv(fontInfoSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ncmInfo.lfMessageFont.lfWeight = FW_EXTRABOLD;

        GetTextMetrics(hdc, &tm);

        ReleaseDC(hOwner, hdc);

        txtHeight = tm.tmInternalLeading - ncm.lfMessageFont.lfHeight;
        txtInfoHeight = tm.tmInternalLeading - ncmInfo.lfMessageFont.lfHeight;
    }

    DWORD styleEx   = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;
    DWORD style     = WS_POPUP | WS_CAPTION | WS_SYSMENU;

    RECT win = Tools::GetWinRect(hOwner, styleEx, style, 500, 9 * txtHeight + txtInfoHeight + 210);
    int width = win.right - win.left;
    int height = win.bottom - win.top;

    _hWnd = CreateWindowEx(styleEx, cClassName, PLUGIN_NAME _T(" Settings"), style,
            win.left, win.top, width, height,
            hOwner, NULL, HMod, NULL);
    if (_hWnd == NULL)
        return NULL;

    GetClientRect(_hWnd, &win);
    width   = win.right - win.left - 20;
    height  = win.bottom - win.top - 20;

    int xPos = win.left + 10;
    int yPos = win.top + 10;

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
            win.left, win.top, win.right - win.left, win.bottom - win.top,
            _hWnd, NULL, HMod, NULL);

    yPos += (win.bottom - win.top + 15);
    _hTab = CreateWindowEx(0, WC_TABCONTROL, NULL,
            WS_CHILD | WS_VISIBLE | TCS_BUTTONS | TCS_FOCUSNEVER,
            xPos, yPos, width, height - yPos - 40,
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

    GetClientRect(_hWnd, &win);
    win.top     = yPos;
    win.bottom  = win.top + height - yPos - 40;
    win.left    = xPos;
    win.right   = win.left + width;

    TabCtrl_AdjustRect(_hTab, FALSE, &win);
    width = win.right - win.left;
    xPos = win.left;
    yPos = win.top + 10;

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
            win.left, win.top, win.right - win.left, win.bottom - win.top,
            _hWnd, NULL, HMod, NULL);

    yPos += (win.bottom - win.top + 10);
    width = width / 5;
    _hSave = CreateWindowEx(0, _T("BUTTON"), _T("Save"),
            WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_PUSHBUTTON,
            xPos + width, yPos, width, 25,
            _hWnd, NULL, HMod, NULL);

    _hCancel = CreateWindowEx(0, _T("BUTTON"), _T("Cancel"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xPos + 3 * width, yPos, width, 25,
            _hWnd, NULL, HMod, NULL);

    {
        CHARFORMAT fmt  = {0};
        fmt.cbSize      = sizeof(fmt);
        fmt.dwMask      = CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_SIZE;
        fmt.dwEffects   = CFE_AUTOCOLOR;
        fmt.yHeight     = cFontSize * 20;
        _tcscpy_s(fmt.szFaceName, _countof(fmt.szFaceName), ncm.lfMessageFont.lfFaceName);

        SendMessage(_hDefDb, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&fmt);
        SendMessage(_hLibDbs, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&fmt);
    }

    _hFont = CreateFontIndirect(&ncm.lfMessageFont);

    if (_hFont)
    {
        SendMessage(_hEnDefDb, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hDefDb, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hInfo, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hParser, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hAutoUpdDb, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hEnLibDb, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hLibDbs, WM_SETFONT, (WPARAM)_hFont, TRUE);
    }

    _hFontInfo = CreateFontIndirect(&ncmInfo.lfMessageFont);

    if (_hFontInfo)
        SendMessage(_hParserInfo, WM_SETFONT, (WPARAM)_hFontInfo, TRUE);

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
bool SettingsWin::isDbConfigured(const CPath& dbPath)
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

    defDb.StripTrailingSpaces();
    if (defDb.Exists())
        createDatabase(defDb, updateDbCB);
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
        db.StripTrailingSpaces();
        if (db.Exists())
            dbs.push_back(db);
    }

    unsigned updateCount = dbs.size();

    if (updateCount)
        for (unsigned i = 0; i < updateCount; ++i)
            createDatabase(dbs[i], updateDbCB);
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
        ret = ret && saveConfig(tab);
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

    Button_SetCheck(_hAutoUpdDb, _activeTab->_cfg._autoUpdate ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(_hEnLibDb, _activeTab->_cfg._useLibDb ? BST_CHECKED : BST_UNCHECKED);

    SendMessage(_hParser, CB_SETCURSEL, _activeTab->_cfg._parserIdx, 0);
}


/**
 *  \brief
 */
void SettingsWin::readTabData()
{
    _activeTab->_cfg._libDbPaths.clear();

    const int len = Edit_GetTextLength(_hLibDbs);
    if (len)
    {
        CText libDbPaths(len);
        Edit_GetText(_hLibDbs, libDbPaths.C_str(), libDbPaths.Size());
        _activeTab->_cfg.DbPathsFromBuf(libDbPaths.C_str(), _T("\n\r"));
    }

    _activeTab->_cfg._autoUpdate    = (Button_GetCheck(_hAutoUpdDb) == BST_CHECKED) ? true : false;
    _activeTab->_cfg._useLibDb      = (Button_GetCheck(_hEnLibDb) == BST_CHECKED) ? true : false;

    _activeTab->_cfg._parserIdx = SendMessage(_hParser, CB_GETCURSEL, 0, 0);
}


/**
 *  \brief
 */
bool SettingsWin::saveConfig(SettingsWin::Tab* tab)
{
    CPath cfgFolder;
    bool saved = false;

    if (tab->_db)
    {
        cfgFolder = tab->_db->GetPath();
        saved = tab->_cfg.SaveToFolder(cfgFolder);
    }
    else
    {
        GTagsSettings._useDefDb = (Button_GetCheck(_hEnDefDb) == BST_CHECKED) ? true : false;

        GTagsSettings._defDbPath.Clear();

        const int len = Edit_GetTextLength(_hDefDb);
        if (len)
        {
            GTagsSettings._defDbPath.Resize(len);
            Edit_GetText(_hDefDb, GTagsSettings._defDbPath.C_str(), GTagsSettings._defDbPath.Size());

            GTagsSettings._defDbPath.StripTrailingSpaces();
            if (!GTagsSettings._defDbPath.Exists())
                GTagsSettings._defDbPath.Clear();
        }

        INpp::Get().GetPluginsConfDir(cfgFolder);
        saved = GTagsSettings.Save();
    }

    if (!saved)
    {
        cfgFolder += cPluginCfgFileName;

        CText msg(_T("Failed saving config to\n\""));
        msg += cfgFolder.C_str();
        msg += _T("\"\nIs the path read only?");

        MessageBox(_hWnd, msg.C_str(), cPluginName, MB_OK | MB_ICONEXCLAMATION);

        return false;
    }

    if (tab->_db)
    {
        if (tab->_db->GetConfig()._parserIdx != tab->_cfg._parserIdx)
            tab->_updateDb = true;

        tab->_db->SetConfig(tab->_cfg);
    }
    else
    {
        GTagsSettings._genericDbCfg = tab->_cfg;
    }

    return true;
}


/**
 *  \brief
 */
void SettingsWin::fillDefDb(const CPath& defDb)
{
    Edit_SetText(SW->_hDefDb, defDb.C_str());

    SetFocus(SW->_hDefDb);
    Edit_SetSel(SW->_hDefDb, defDb.Len(), defDb.Len());
    Edit_ScrollCaret(SW->_hDefDb);
}


/**
 *  \brief
 */
void SettingsWin::fillLibDb(const CPath& lib)
{
    int len = Edit_GetTextLength(SW->_hLibDbs);
    CText buf(len);
    bool found = false;

    if (len)
    {
        Edit_GetText(SW->_hLibDbs, buf.C_str(), buf.Size());

        int libLen = lib.Len();

        for (TCHAR* ptr = _tcsstr(buf.C_str(), lib.C_str()); ptr; ptr = _tcsstr(ptr, lib.C_str()))
        {
            if (ptr[libLen] == 0 || ptr[libLen] == _T('\n') || ptr[libLen] == _T('\r'))
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
        buf += lib;
        Edit_SetText(SW->_hLibDbs, buf.C_str());
    }

    SetFocus(SW->_hLibDbs);
    Edit_SetSel(SW->_hLibDbs, buf.Len(), buf.Len());
    Edit_ScrollCaret(SW->_hLibDbs);
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

    if (isDbConfigured(dbPath))
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
        db = DbManager::Get().GetDb(dbPath, true, &success);

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

    CmdPtr_t cmd(new Cmd(CREATE_DATABASE, _T("Creating Database"), db));
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
void SettingsWin::updateDbCB(const CmdPtr_t& cmd)
{
    dbWriteReady(cmd);

    if (SW == NULL)
        return;
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

                if ((HWND)lParam == SW->_hAutoUpdDb)
                    EnableWindow(SW->_hSave, TRUE);
            }
            else if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE)
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
