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
#include "ConfigWin.h"
#include "Cmd.h"
#include "CmdEngine.h"


namespace GTags
{

const TCHAR ConfigWin::cClassName[]   = _T("ConfigWin");
const int ConfigWin::cBackgroundColor = COLOR_BTNFACE;
const int ConfigWin::cFontSize        = 10;


ConfigWin* ConfigWin::CW = NULL;


/**
 *  \brief
 */
ConfigWin::Tab::Tab(const DbHandle db) : _db(db), _updateDb(false)
{
    if (db)
        _cfg = db->GetConfig();
    else
        _cfg = DefaultDbCfg;
}


/**
 *  \brief
 */
ConfigWin::Tab::~Tab()
{
    if (_db)
    {
        if (_updateDb)
        {
            CmdPtr_t cmd(new Cmd(CREATE_DATABASE, _T("Create Database"), _db));
            CmdEngine::Run(cmd, ConfigWin::dbWriteReady);
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
void ConfigWin::Show()
{
    if (!createWin())
        return;

    CW->fillData();

    ShowWindow(CW->_hWnd, SW_SHOWNORMAL);
    UpdateWindow(CW->_hWnd);
}


/**
 *  \brief
 */
void ConfigWin::Show(const DbHandle& db)
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

    int i = TabCtrl_InsertItem(CW->_hTab, TabCtrl_GetItemCount(CW->_hTab), &tci);
    if (i == -1)
    {
        delete tab;
        SendMessage(CW->_hWnd, WM_CLOSE, 0, 0);

        return;
    }

    TabCtrl_SetCurSel(CW->_hTab, i);
    CW->_activeTab = tab;
    CW->fillData();

    ShowWindow(CW->_hWnd, SW_SHOWNORMAL);
    UpdateWindow(CW->_hWnd);
}


/**
 *  \brief
 */
bool ConfigWin::createWin()
{
    if (CW)
    {
        SetFocus(CW->_hWnd);
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

    CW = new ConfigWin();
    if (CW->composeWindow(hOwner) == NULL)
    {
        delete CW;
        CW = NULL;
        return false;
    }

    return true;
}


/**
 *  \brief
 */
RECT ConfigWin::adjustSizeAndPos(HWND hOwner, DWORD styleEx, DWORD style, int width, int height)
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
ConfigWin::ConfigWin() : _hKeyHook(NULL), _hFont(NULL)
{
}


/**
 *  \brief
 */
ConfigWin::~ConfigWin()
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

    UnregisterClass(cClassName, HMod);
}


/**
 *  \brief
 */
HWND ConfigWin::composeWindow(HWND hOwner)
{
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

    int txtHeight;
    {
        TEXTMETRIC tm;

        HDC hdc = GetWindowDC(hOwner);
        ncm.lfMessageFont.lfHeight = -MulDiv(cFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        GetTextMetrics(hdc, &tm);
        ReleaseDC(hOwner, hdc);

        txtHeight = tm.tmInternalLeading - ncm.lfMessageFont.lfHeight;
    }

    DWORD styleEx   = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;
    DWORD style     = WS_POPUP | WS_CAPTION | WS_SYSMENU;

    RECT win = adjustSizeAndPos(hOwner, styleEx, style, 500, 11 * txtHeight + 180);
    int width = win.right - win.left;
    int height = win.bottom - win.top;

    _hWnd = CreateWindowEx(styleEx, cClassName, PLUGIN_NAME _T(" Settings"), style,
            win.left, win.top, width, height,
            hOwner, NULL, HMod, NULL);
    if (_hWnd == NULL)
        return NULL;

    GetClientRect(_hWnd, &win);
    width = win.right - win.left;
    height = win.bottom - win.top;

    _hTab = CreateWindowEx(0, WC_TABCONTROL, NULL,
            WS_CHILD | WS_VISIBLE | TCS_BUTTONS | TCS_FOCUSNEVER,
            3, 5, width - 6, height - 10,
            _hWnd, NULL, HMod, NULL);

    _activeTab = new Tab;
    {
        TCHAR buf[64]   = _T("Default database config");
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

    TabCtrl_AdjustRect(_hTab, FALSE, &win);
    width = win.right - win.left - 20;

    int yPos        = win.top + 20;
    const int xPos  = win.left + 10;

    _hInfo = CreateWindowEx(0, _T("STATIC"), _T("These settings apply to all new databases"),
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_SUNKEN,
            xPos, yPos, width, 2 * txtHeight + 10,
            _hWnd, NULL, HMod, NULL);

    yPos += (2 * txtHeight + 30);
    _hParserInfo = CreateWindowEx(0, _T("STATIC"), _T("Code Parser"),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            xPos, yPos, width, txtHeight,
            _hWnd, NULL, HMod, NULL);

    yPos += (txtHeight + 5);
    _hParser = CreateWindowEx(0, WC_COMBOBOX, NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
            xPos, yPos, (width / 2) - 10, txtHeight + 10,
            _hWnd, NULL, HMod, NULL);

    _hAutoUpdate = CreateWindowEx(0, _T("BUTTON"), _T("Auto update database"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            xPos + (width / 2) + 30, yPos, (width / 2) - 50, txtHeight + 10,
            _hWnd, NULL, HMod, NULL);

    yPos += (txtHeight + 30);
    _hEnLibDb = CreateWindowEx(0, _T("BUTTON"), _T("Enable library databases"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            xPos + 20, yPos, (width / 2) - 50, txtHeight + 10,
            _hWnd, NULL, HMod, NULL);

    _hCreateDb = CreateWindowEx(0, _T("BUTTON"), _T("Add Library DB"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xPos + (width / 2) + 10, yPos, (width / 2) - 10, 25,
            _hWnd, NULL, HMod, NULL);

    yPos += (txtHeight + 10);
    _hUpdateDb = CreateWindowEx(0, _T("BUTTON"), _T("Update Library DBs"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xPos + (width / 2) + 10, yPos, (width / 2) - 10, 25,
            _hWnd, NULL, HMod, NULL);

    yPos += (txtHeight + 5);
    CreateWindowEx(0, _T("STATIC"), _T("Paths to library databases"),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            xPos, yPos, width / 2, txtHeight,
            _hWnd, NULL, HMod, NULL);

    yPos += (txtHeight + 5);
    win.top     = yPos;
    win.bottom  = win.top + 4 * txtHeight;
    win.left    = xPos;
    win.right   = win.left + width;

    styleEx = WS_EX_CLIENTEDGE;
    style   = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
            ES_NOOLEDRAGDROP | ES_MULTILINE | ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL;

    AdjustWindowRectEx(&win, style, FALSE, styleEx);
    _hLibDb = CreateWindowEx(styleEx, RICHEDIT_CLASS, NULL, style,
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

        SendMessage(_hLibDb, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&fmt);
    }

    _hFont = CreateFontIndirect(&ncm.lfMessageFont);

    if (_hFont)
    {
        SendMessage(_hInfo, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hLibDb, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hAutoUpdate, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hParser, WM_SETFONT, (WPARAM)_hFont, TRUE);
        SendMessage(_hEnLibDb, WM_SETFONT, (WPARAM)_hFont, TRUE);
    }

    for (unsigned i = 0; DbConfig::Parser(i); ++i)
        SendMessage(_hParser, CB_ADDSTRING, 0, (LPARAM)DbConfig::Parser(i));

    _hKeyHook = SetWindowsHookEx(WH_KEYBOARD, keyHookProc, NULL, GetCurrentThreadId());

    return _hWnd;
}


/**
 *  \brief
 */
ConfigWin::Tab* ConfigWin::getTab(int i)
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
void ConfigWin::onUpdateDb()
{
    int len = Edit_GetTextLength(_hLibDb);
    if (!len)
        return;

    CText buf(len);

    Edit_GetText(_hLibDb, buf.C_str(), buf.Size());

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
            createLibDatabase(dbs[i], updateDbCB);
}


/**
 *  \brief
 */
void ConfigWin::onTabChange()
{
    readData();
    _activeTab = getTab();
    fillData();
    SetFocus(_hInfo);
}


/**
 *  \brief
 */
void ConfigWin::onSave()
{
    readData();

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
void ConfigWin::fillData()
{
    if (_activeTab->_db)
    {
        CText txt(_T("These settings apply to database at\n\""));
        txt += _activeTab->_db->GetPath();
        txt += _T("\"");
		SetWindowText(_hInfo, txt.C_str());
		SetWindowText(_hParserInfo, _T("Code Parser (database will be re-created on change!)"));
    }
    else
    {
		SetWindowText(_hInfo, _T("These settings apply to all new databases"));
		SetWindowText(_hParserInfo, _T("Code Parser"));
    }

	SendMessage(_hLibDb, EM_SETEVENTMASK, 0, ENM_NONE);

    if (_activeTab->_cfg._libDbPaths.empty())
    {
        Edit_SetText(_hLibDb, _T(""));
    }
    else
    {
        CText libDbPaths;
        _activeTab->_cfg.DbPathsToBuf(libDbPaths, _T('\n'));
        Edit_SetText(_hLibDb, libDbPaths.C_str());
    }

    SendMessage(_hLibDb, EM_SETEVENTMASK, 0, ENM_CHANGE);

    if (_activeTab->_cfg._useLibDb)
    {
        EnableWindow(_hCreateDb, TRUE);
        EnableWindow(_hUpdateDb, TRUE);
        Edit_Enable(_hLibDb, TRUE);
        SendMessage(_hLibDb, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_WINDOW));
    }
    else
    {
        EnableWindow(_hCreateDb, FALSE);
        EnableWindow(_hUpdateDb, FALSE);
        Edit_Enable(_hLibDb, FALSE);
        SendMessage(_hLibDb, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    }

    Button_SetCheck(_hAutoUpdate, _activeTab->_cfg._autoUpdate ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(_hEnLibDb, _activeTab->_cfg._useLibDb ? BST_CHECKED : BST_UNCHECKED);

    SendMessage(_hParser, CB_SETCURSEL, _activeTab->_cfg._parserIdx, 0);
}


/**
 *  \brief
 */
void ConfigWin::readData()
{
    _activeTab->_cfg._libDbPaths.clear();

    int len = Edit_GetTextLength(_hLibDb);
    if (len)
    {
        CText libDbPaths(len);
        Edit_GetText(_hLibDb, libDbPaths.C_str(), libDbPaths.Size());
        _activeTab->_cfg.DbPathsFromBuf(libDbPaths.C_str(), _T("\n\r"));
    }

    _activeTab->_cfg._autoUpdate    = (Button_GetCheck(_hAutoUpdate) == BST_CHECKED) ? true : false;
    _activeTab->_cfg._useLibDb      = (Button_GetCheck(_hEnLibDb) == BST_CHECKED) ? true : false;

    _activeTab->_cfg._parserIdx = SendMessage(_hParser, CB_GETCURSEL, 0, 0);
}


/**
 *  \brief
 */
bool ConfigWin::saveConfig(ConfigWin::Tab* tab)
{
    CPath cfgFolder;

    if (tab->_db)
        cfgFolder = tab->_db->GetPath();
    else
        INpp::Get().GetPluginsConfDir(cfgFolder);

    if (!tab->_cfg.SaveToFolder(cfgFolder))
    {
        cfgFolder += DbConfig::cCfgFileName;

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
        DefaultDbCfg = tab->_cfg;
    }

    return true;
}


/**
 *  \brief
 */
void ConfigWin::fillLibDb(const CPath& lib)
{
    int len = Edit_GetTextLength(CW->_hLibDb);
    CText buf(len);
    bool found = false;

    if (len)
    {
        Edit_GetText(CW->_hLibDb, buf.C_str(), buf.Size());

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
        Edit_SetText(CW->_hLibDb, buf.C_str());
    }

    SetFocus(CW->_hLibDb);
    Edit_SetSel(CW->_hLibDb, buf.Len(), buf.Len());
    Edit_ScrollCaret(CW->_hLibDb);
}


/**
 *  \brief
 */
bool ConfigWin::createLibDatabase(CPath& dbPath, CompletionCB complCB)
{
    if (dbPath.IsEmpty())
    {
        if (!Tools::BrowseForFolder(_hWnd, dbPath))
            return false;
    }

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

    CmdPtr_t cmd(new Cmd(CREATE_DATABASE, _T("Create Library Database"), db));
    CmdEngine::Run(cmd, complCB);

    return true;
}


/**
 *  \brief
 */
void ConfigWin::dbWriteReady(const CmdPtr_t& cmd)
{
    if (cmd->Status() != OK)
        DbManager::Get().UnregisterDb(cmd->Db());
    else
        DbManager::Get().PutDb(cmd->Db());

    if (cmd->Status() == RUN_ERROR)
    {
        HWND hWnd = (CW == NULL) ? INpp::Get().GetHandle() : CW->_hWnd;
        MessageBox(hWnd, _T("Running GTags failed"), cmd->Name(), MB_OK | MB_ICONERROR);
    }
    else if (cmd->Result())
    {
        CText msg(cmd->Result());
        HWND hWnd = (CW == NULL) ? INpp::Get().GetHandle() : CW->_hWnd;
        MessageBox(hWnd, msg.C_str(), cmd->Name(), MB_OK | MB_ICONEXCLAMATION);
    }
}


/**
 *  \brief
 */
void ConfigWin::createDbCB(const CmdPtr_t& cmd)
{
    dbWriteReady(cmd);

    if (CW == NULL)
        return;

    EnableWindow(CW->_hWnd, TRUE);

    if (cmd->Status() == OK)
        CW->fillLibDb(cmd->Db()->GetPath());
}


/**
 *  \brief
 */
void ConfigWin::updateDbCB(const CmdPtr_t& cmd)
{
    dbWriteReady(cmd);

    if (CW == NULL)
        return;
}


/**
 *  \brief
 */
LRESULT CALLBACK ConfigWin::keyHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0)
    {
        HWND hWnd = GetFocus();
        if ((CW->_hWnd == hWnd) || IsChild(CW->_hWnd, hWnd))
        {
            // Key is pressed
            if (!(lParam & (1 << 31)))
            {
                if (wParam == VK_ESCAPE)
                {
                    SendMessage(CW->_hWnd, WM_CLOSE, 0, 0);
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
LRESULT APIENTRY ConfigWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                if ((HWND)lParam == CW->_hSave)
                {
                    CW->onSave();
                    return 0;
                }

                if ((HWND)lParam == CW->_hCancel)
                {
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    return 0;
                }

                if ((HWND)lParam == CW->_hEnLibDb)
                {
                    BOOL en;
                    int color;

                    if (Button_GetCheck(CW->_hEnLibDb) == BST_CHECKED)
                    {
                        en = TRUE;
                        color = COLOR_WINDOW;
                    }
                    else
                    {
                        en = FALSE;
                        color = COLOR_BTNFACE;
                    }

                    EnableWindow(CW->_hCreateDb, en);
                    EnableWindow(CW->_hUpdateDb, en);
                    Edit_Enable(CW->_hLibDb, en);
                    SendMessage(CW->_hLibDb, EM_SETBKGNDCOLOR, 0, GetSysColor(color));

                    EnableWindow(CW->_hSave, TRUE);

                    return 0;
                }

                if ((HWND)lParam == CW->_hCreateDb)
                {
                    CPath libraryPath;

                    if (CW->createLibDatabase(libraryPath, createDbCB))
                    {
                        EnableWindow(hWnd, FALSE);
                        SetFocus(INpp::Get().GetSciHandle());
                    }
                    else if (!libraryPath.IsEmpty())
                    {
                        CW->fillLibDb(libraryPath);
                    }

                    return 0;
                }

                if ((HWND)lParam == CW->_hUpdateDb)
                {
                    CW->onUpdateDb();
                    return 0;
                }

                if ((HWND)lParam == CW->_hAutoUpdate)
                    EnableWindow(CW->_hSave, TRUE);
            }
            else if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE)
            {
                EnableWindow(CW->_hSave, TRUE);
            }
        break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
            {
                CW->onTabChange();
                return 0;
            }
        break;

        case WM_DESTROY:
            DestroyCaret();
            delete CW;
            CW = NULL;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
