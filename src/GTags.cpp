/**
 *  \file
 *  \brief  GTags plugin main routines
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


#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include "Common.h"
#include "INpp.h"
#include "Config.h"
#include "DbManager.h"
#include "Cmd.h"
#include "CmdEngine.h"
#include "DocLocation.h"
#include "SearchWin.h"
#include "ActivityWin.h"
#include "AutoCompleteWin.h"
#include "ResultWin.h"
#include "ConfigWin.h"
#include "AboutWin.h"
#include "GTags.h"
#include "LineParser.h"


namespace
{

using namespace GTags;


const TCHAR cCreateDatabase[]   = _T("Create Database");
const TCHAR cUpdateSingle[]     = _T("Database Single File Update");
const TCHAR cAutoCompl[]        = _T("AutoComplete");
const TCHAR cAutoComplFile[]    = _T("AutoComplete File Name");
const TCHAR cFindFile[]         = _T("Find File");
const TCHAR cFindDefinition[]   = _T("Find Definition");
const TCHAR cFindReference[]    = _T("Find Reference");
const TCHAR cFindSymbol[]       = _T("Find Symbol");
const TCHAR cSearch[]           = _T("Search");
const TCHAR cSearchText[]       = _T("Search in Text Files");
const TCHAR cVersion[]          = _T("About");


/**
 *  \brief
 */
bool checkForGTagsBinaries(CPath& dllPath)
{
    dllPath.StripFilename();
    dllPath += cBinsDir;
    dllPath += _T("\\global.exe");

    bool gtagsBinsFound = dllPath.FileExists();
    if (gtagsBinsFound)
    {
        dllPath.StripFilename();
        dllPath += _T("gtags.exe");
        gtagsBinsFound = dllPath.FileExists();
    }

    if (!gtagsBinsFound)
    {
        dllPath.StripFilename();

        CText msg(_T("GTags binaries not found in\n\""));
        msg += dllPath.C_str();
        msg += _T("\"\n");
        msg += cPluginName;
        msg += _T(" plugin will not be loaded!");

        MessageBox(NULL, msg.C_str(), cPluginName, MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}


/**
 *  \brief
 */
int CALLBACK browseFolderCB(HWND hWnd, UINT uMsg, LPARAM, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED)
        SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
    return 0;
}


/**
 *  \brief
 */
bool browseForDbFolder(HWND hOwnerWin, CPath& dbPath)
{
    TCHAR path[MAX_PATH];

    BROWSEINFO bi       = {0};
    bi.hwndOwner        = hOwnerWin;
    bi.pszDisplayName   = path;
    bi.lpszTitle        = _T("Select the database root (indexed recursively)");
    bi.ulFlags          = BIF_RETURNONLYFSDIRS;
    bi.lpfn             = browseFolderCB;

    if (!dbPath.IsEmpty() && dbPath.Exists())
        bi.lParam = (DWORD)dbPath.C_str();

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (!pidl)
        return false;

    SHGetPathFromIDList(pidl, path);

    IMalloc* imalloc = NULL;
    if (SUCCEEDED(SHGetMalloc(&imalloc)))
    {
        imalloc->Free(pidl);
        imalloc->Release();
    }

    dbPath = path;
    dbPath += _T("\\");

    return true;
}


enum WordSelection_t
{
    DONT_SELECT,
    FULL_SELECT,
    PARTIAL_SELECT
};


/**
 *  \brief
 */
CText getSelection(bool skipPreSelect = false, WordSelection_t selectWord = FULL_SELECT, bool highlightWord = true)
{
    INpp& npp = INpp::Get();

    npp.ReadSciHandle();
    if (npp.IsSelectionVertical())
        return CText();

    CTextA tagA;

    if (!skipPreSelect)
        npp.GetSelection(tagA);

    if (skipPreSelect || (tagA.IsEmpty() && selectWord != DONT_SELECT))
        npp.GetWord(tagA, selectWord == PARTIAL_SELECT, highlightWord);

    if (tagA.IsEmpty())
        return CText();

    return CText(tagA.C_str());
}


/**
 *  \brief
 */
DbHandle getDatabase(bool writeEn = false)
{
    INpp& npp = INpp::Get();
    bool success;
    CPath currentFile;
    npp.GetFilePath(currentFile);

    DbHandle db = DbManager::Get().GetDb(currentFile, writeEn, &success);
    if (!db)
    {
        MessageBox(npp.GetHandle(), _T("GTags database not found"), cPluginName, MB_OK | MB_ICONINFORMATION);
    }
    else if (!success)
    {
        MessageBox(npp.GetHandle(), _T("GTags database is currently in use"), cPluginName, MB_OK | MB_ICONINFORMATION);
        db = NULL;
    }

    return db;
}


/**
 *  \brief
 */
void autoComplCB(const CmdPtr_t& cmd)
{
    DbManager::Get().PutDb(cmd->Db());

    if (cmd->Status() == OK && cmd->Result())
    {
        AutoCompleteWin::Show(cmd);
        return;
    }

    INpp::Get().ClearSelection();

    if (cmd->Status() == FAILED)
    {
        CText msg(cmd->Result());
        msg += _T("\nTry re-creating database.");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(), MB_OK | MB_ICONERROR);
    }
    else if (cmd->Status() == RUN_ERROR)
    {
        MessageBox(INpp::Get().GetHandle(), _T("Running GTags failed"), cmd->Name(), MB_OK | MB_ICONERROR);
    }
}


/**
 *  \brief
 */
void halfComplCB(const CmdPtr_t& cmd)
{
    if (cmd->Status() == OK)
    {
        cmd->Id(AUTOCOMPLETE_SYMBOL);

        ParserPtr_t parser(new LineParser);
        cmd->Parser(parser);

        CmdEngine::Run(cmd, autoComplCB);
        return;
    }

    DbManager::Get().PutDb(cmd->Db());

    INpp::Get().ClearSelection();

    if (cmd->Status() == FAILED)
    {
        CText msg(cmd->Result());
        msg += _T("\nTry re-creating database.");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(), MB_OK | MB_ICONERROR);
    }
    else if (cmd->Status() == RUN_ERROR)
    {
        MessageBox(INpp::Get().GetHandle(), _T("Running GTags failed"), cmd->Name(), MB_OK | MB_ICONERROR);
    }
}


/**
 *  \brief
 */
void showResultCB(const CmdPtr_t& cmd)
{
    DbManager::Get().PutDb(cmd->Db());

    if (cmd->Status() == OK)
    {
        if (cmd->Result())
        {
            ResultWin::Show(cmd);
        }
        else
        {
            CText msg(_T("\""));
            msg += cmd->Tag();
            msg += _T("\" not found");
            MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(), MB_OK | MB_ICONINFORMATION);
        }
    }
    else if (cmd->Status() == FAILED)
    {
        CText msg(cmd->Result());
        msg += _T("\nTry re-creating database.");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(), MB_OK | MB_ICONERROR);
    }
    else if (cmd->Status() == RUN_ERROR)
    {
        MessageBox(INpp::Get().GetHandle(), _T("Running GTags failed"), cmd->Name(), MB_OK | MB_ICONERROR);
    }
    else if (cmd->Status() == PARSE_ERROR)
    {
        MessageBox(INpp::Get().GetHandle(), _T("Database seems outdated.\nPlease re-create it and redo the search"),
                cmd->Name(), MB_OK | MB_ICONEXCLAMATION);
    }
}


/**
 *  \brief
 */
void findCB(const CmdPtr_t& cmd)
{
    if (cmd->Status() == OK && cmd->Result() == NULL)
    {
        cmd->Id(FIND_SYMBOL);
        cmd->Name(cFindSymbol);

        CmdEngine::Run(cmd, showResultCB);
    }
    else
    {
        showResultCB(cmd);
    }
}


/**
 *  \brief
 */
void aboutCB(const CmdPtr_t& cmd)
{
    CText msg;

    if (cmd->Status() == OK)
        msg = cmd->Result();
    else
        msg = _T("VERSION READ FAILED\n");

    AboutWin::Show(msg.C_str());
}


/**
 *  \brief
 */
/*
void EnablePluginMenuItem(int itemIdx, bool enable)
{
    static HMENU HMenu = NULL;

    if (HMenu == NULL)
    {
        TCHAR buf[PLUGIN_ITEM_SIZE];
        HMENU hMenu = INpp::Get().GetPluginMenu();

        MENUITEMINFO mi = {0};
        mi.cbSize       = sizeof(mi);
        mi.fMask        = MIIM_STRING;

        int idx;
        int itemsCount = GetMenuItemCount(hMenu);
        for (idx = 0; idx < itemsCount; ++idx)
        {
            mi.dwTypeData = NULL;
            GetMenuItemInfo(hMenu, idx, TRUE, &mi);
            mi.dwTypeData = buf;
            ++mi.cch;
            GetMenuItemInfo(hMenu, idx, TRUE, &mi);
            mi.dwTypeData[mi.cch - 1] = 0;
            if (!_tcscmp(cPluginName, mi.dwTypeData))
                break;
        }
        if (idx == itemsCount)
            return;

        HMenu = GetSubMenu(hMenu, idx);
    }

    UINT flags = MF_BYPOSITION;
    flags |= enable ? MF_ENABLED : MF_GRAYED;

    EnableMenuItem(HMenu, itemIdx, flags);
}
*/


/**
 *  \brief
 */
void AutoComplete()
{
    CText tag = getSelection(true, PARTIAL_SELECT, false);
    if (tag.IsEmpty())
        return;

    DbHandle db = getDatabase();
    if (!db)
        return;

    CmdPtr_t cmd(new Cmd(AUTOCOMPLETE, cAutoCompl, db, NULL, tag.C_str()));

    CmdEngine::Run(cmd, halfComplCB);
}


/**
 *  \brief
 */
void AutoCompleteFile()
{
    CText tag = getSelection(true, PARTIAL_SELECT, false);
    if (tag.IsEmpty())
        return;

    tag.Insert(0, _T('/'));

    DbHandle db = getDatabase();
    if (!db)
        return;

    ParserPtr_t parser(new LineParser);
    CmdPtr_t cmd(new Cmd(AUTOCOMPLETE_FILE, cAutoComplFile, db, parser, tag.C_str()));

    CmdEngine::Run(cmd, autoComplCB);
}


/**
 *  \brief
 */
void FindFile()
{
    SearchWin::Close();

    DbHandle db = getDatabase();
    if (!db)
        return;

    ParserPtr_t parser(new ResultWin::TabParser);
    CmdPtr_t cmd(new Cmd(FIND_FILE, cFindFile, db, parser));

    CText tag = getSelection(false, DONT_SELECT);
    if (tag.IsEmpty())
    {
        CPath fileName;
        INpp::Get().GetFileNamePart(fileName);
        cmd->Tag(fileName);
        SearchWin::Show(cmd, showResultCB);
    }
    else
    {
        cmd->Tag(tag);
        CmdEngine::Run(cmd, showResultCB);
    }
}


/**
 *  \brief
 */
void FindDefinition()
{
    SearchWin::Close();

    DbHandle db = getDatabase();
    if (!db)
        return;

    ParserPtr_t parser(new ResultWin::TabParser);
    CmdPtr_t cmd(new Cmd(FIND_DEFINITION, cFindDefinition, db, parser));

    CText tag = getSelection();
    if (tag.IsEmpty())
    {
        SearchWin::Show(cmd, findCB, false);
    }
    else
    {
        cmd->Tag(tag);
        CmdEngine::Run(cmd, findCB);
    }
}


/**
 *  \brief
 */
void FindReference()
{
    SearchWin::Close();

    if (Config._parserIdx == CConfig::CTAGS_PARSER)
    {
        MessageBox(INpp::Get().GetHandle(), _T("Ctags parser doesn't support reference search"), cPluginName,
                MB_OK | MB_ICONINFORMATION);
        return;
    }

    DbHandle db = getDatabase();
    if (!db)
        return;

    ParserPtr_t parser(new ResultWin::TabParser);
    CmdPtr_t cmd(new Cmd(FIND_REFERENCE, cFindReference, db, parser));

    CText tag = getSelection();
    if (tag.IsEmpty())
    {
        SearchWin::Show(cmd, findCB, false);
    }
    else
    {
        cmd->Tag(tag);
        CmdEngine::Run(cmd, findCB);
    }
}


/**
 *  \brief
 */
void Search()
{
    SearchWin::Close();

    DbHandle db = getDatabase();
    if (!db)
        return;

    ParserPtr_t parser(new ResultWin::TabParser);
    CmdPtr_t cmd(new Cmd(GREP, cSearch, db, parser));

    CText tag = getSelection();
    if (tag.IsEmpty())
    {
        SearchWin::Show(cmd, showResultCB);
    }
    else
    {
        cmd->Tag(tag);
        CmdEngine::Run(cmd, showResultCB);
    }
}


/**
 *  \brief
 */
void SearchTextFiles()
{
    SearchWin::Close();

    DbHandle db = getDatabase();
    if (!db)
        return;

    ParserPtr_t parser(new ResultWin::TabParser);
    CmdPtr_t cmd(new Cmd(GREP_TEXT, cSearchText, db, parser));

    CText tag = getSelection();
    if (tag.IsEmpty())
    {
        SearchWin::Show(cmd, showResultCB);
    }
    else
    {
        cmd->Tag(tag);
        CmdEngine::Run(cmd, showResultCB);
    }
}


/**
 *  \brief
 */
void GoBack()
{
    DocLocation::Get().Back();
}


/**
 *  \brief
 */
void GoForward()
{
    DocLocation::Get().Forward();
}


/**
 *  \brief
 */
void CreateDatabase()
{
    SearchWin::Close();

    CPath currentFile;

    bool success;
    INpp& npp = INpp::Get();
    npp.GetFilePath(currentFile);

    DbHandle db = DbManager::Get().GetDb(currentFile, true, &success);
    if (db)
    {
        if (!success)
        {
            MessageBox(npp.GetHandle(), _T("GTags database is currently in use"), cPluginName,
                    MB_OK | MB_ICONINFORMATION);
            return;
        }

        CText msg(_T("Database at\n\""));
        msg += db->GetPath().C_str();
        msg += _T("\" exists.\nRe-create?");
        int choice = MessageBox(npp.GetHandle(), msg.C_str(), cPluginName, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
        if (choice != IDYES)
        {
            DbManager::Get().PutDb(db);
            return;
        }
    }
    else
    {
        currentFile.StripFilename();

        if (!browseForDbFolder(npp.GetHandle(), currentFile))
            return;

        db = DbManager::Get().RegisterDb(currentFile);
    }

    CmdPtr_t cmd(new Cmd(CREATE_DATABASE, cCreateDatabase, db));
    CmdEngine::Run(cmd, DbWriteCB);
}


/**
 *  \brief
 */
void DeleteDatabase()
{
    SearchWin::Close();

    DbHandle db = getDatabase(true);
    if (!db)
        return;

    INpp& npp = INpp::Get();
    TCHAR buf[512];
    _sntprintf_s(buf, _countof(buf), _TRUNCATE, _T("Delete database from\n\"%s\"?"), db->GetPath().C_str());
    int choice = MessageBox(npp.GetHandle(), buf, cPluginName, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
    if (choice != IDYES)
    {
        DbManager::Get().PutDb(db);
        return;
    }

    if (DbManager::Get().UnregisterDb(db))
        MessageBox(npp.GetHandle(), _T("GTags database deleted"), cPluginName, MB_OK | MB_ICONINFORMATION);
    else
        MessageBox(npp.GetHandle(), _T("Deleting database failed, is it read-only?"), cPluginName,
                MB_OK | MB_ICONERROR);
}


/**
 *  \brief
 */
void ToggleResultWinFocus()
{
    ResultWin::Show();
}


/**
 *  \brief
 */
void SettingsCfg()
{
    ConfigWin::Show(&Config);
}


/**
 *  \brief
 */
void About()
{
    CmdPtr_t cmd(new Cmd(VERSION, cVersion));
    CmdEngine::Run(cmd, aboutCB);
}

} // anonymous namespace


namespace GTags
{

FuncItem Menu[19] = {
    /* 0 */  FuncItem(cAutoCompl, AutoComplete),
    /* 1 */  FuncItem(cAutoComplFile, AutoCompleteFile),
    /* 2 */  FuncItem(cFindFile, FindFile),
    /* 3 */  FuncItem(cFindDefinition, FindDefinition),
    /* 4 */  FuncItem(cFindReference, FindReference),
    /* 5 */  FuncItem(cSearch, Search),
    /* 6 */  FuncItem(cSearchText, SearchTextFiles),
    /* 7 */  FuncItem(),
    /* 8 */  FuncItem(_T("Go Back"), GoBack),
    /* 9 */  FuncItem(_T("Go Forward"), GoForward),
    /* 10 */ FuncItem(),
    /* 11 */ FuncItem(cCreateDatabase, CreateDatabase),
    /* 12 */ FuncItem(_T("Delete Database"), DeleteDatabase),
    /* 13 */ FuncItem(),
    /* 14 */ FuncItem(_T("Toggle Results Window Focus"), ToggleResultWinFocus),
    /* 15 */ FuncItem(),
    /* 16 */ FuncItem(_T("Settings"), SettingsCfg),
    /* 17 */ FuncItem(),
    /* 18 */ FuncItem(cVersion, About)
};

HINSTANCE HMod = NULL;
CPath DllPath;

CText UIFontName;
unsigned UIFontSize;

HWND MainHwnd = NULL;

CConfig Config;


/**
 *  \brief
 */
BOOL PluginLoad(HINSTANCE hMod)
{
    CPath moduleFileName(MAX_PATH);
    GetModuleFileName((HMODULE)hMod, moduleFileName.C_str(), MAX_PATH);
    moduleFileName.AutoFit();

    DllPath = moduleFileName;

    if (!checkForGTagsBinaries(moduleFileName))
        return FALSE;

    HMod = hMod;

    return TRUE;
}


/**
 *  \brief
 */
void PluginInit()
{
    INpp& npp = INpp::Get();
    char font[32];

    npp.GetFontName(STYLE_DEFAULT, font);
    UIFontName = font;
    UIFontSize = (unsigned)npp.GetFontSize(STYLE_DEFAULT);

    ActivityWin::Register();
    SearchWin::Register();
    AutoCompleteWin::Register();

    MainHwnd = ResultWin::Register();
    if (MainHwnd == NULL)
        MessageBox(npp.GetHandle(), _T("Results Window init failed, plugin will not be operational"), cPluginName,
                MB_OK | MB_ICONERROR);
    else if (!Config.LoadFromFile())
        MessageBox(npp.GetHandle(), _T("Bad config file, default settings will be used"), cPluginName,
                MB_OK | MB_ICONEXCLAMATION);
}


/**
 *  \brief
 */
void PluginDeInit()
{
    ActivityWin::Unregister();
    SearchWin::Unregister();
    AutoCompleteWin::Unregister();
    ResultWin::Unregister();

    HMod = NULL;
}


/**
 *  \brief
 */
bool UpdateSingleFile(const CPath& file)
{
    bool success;
    DbHandle db = DbManager::Get().GetDb(file, true, &success);
    if (!db)
        return false;

    if (!success)
    {
        DbManager::Get().ScheduleUpdate(file);
        return true;
    }

    CmdPtr_t cmd(new Cmd(UPDATE_SINGLE, cUpdateSingle, db, NULL, file.C_str()));
    CmdEngine::Run(cmd, DbWriteCB);

    return true;
}


/**
 *  \brief
 */
bool CreateLibDatabase(HWND hOwnerWin, CPath& dbPath, CompletionCB complCB)
{
    if (!complCB)
        return false;

    if (dbPath.IsEmpty())
    {
        if (!browseForDbFolder(hOwnerWin, dbPath))
            return false;
    }

    DbHandle db;

    if (DbManager::Get().DbExistsInFolder(dbPath))
    {
        CText msg(_T("Database at\n\""));
        msg += dbPath;
        msg += _T("\" exists.\nRe-create?");
        int choice = MessageBox(hOwnerWin, msg.C_str(), cPluginName,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
        if (choice != IDYES)
            return false;

        bool success;
        db = DbManager::Get().GetDb(dbPath, true, &success);

        if (!success)
        {
            MessageBox(hOwnerWin, _T("GTags database is currently in use"), cPluginName,
                    MB_OK | MB_ICONINFORMATION);
            return false;
        }
    }
    else
    {
        db = DbManager::Get().RegisterDb(dbPath);
    }

    CmdPtr_t cmd(new Cmd(CREATE_DATABASE, cCreateDatabase, db));
    CmdEngine::Run(cmd, complCB);

    return true;
}


/**
 *  \brief
 */
void DbWriteCB(const CmdPtr_t& cmd)
{
    if (cmd->Status() != OK && cmd->Id() == CREATE_DATABASE)
        DbManager::Get().UnregisterDb(cmd->Db());
    else
        DbManager::Get().PutDb(cmd->Db());

    if (cmd->Status() == RUN_ERROR)
    {
        MessageBox(INpp::Get().GetHandle(), _T("Running GTags failed"), cmd->Name(), MB_OK | MB_ICONERROR);
    }
    else if (cmd->Status() == FAILED || cmd->Result())
    {
        CText msg(cmd->Result());
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(), MB_OK | MB_ICONEXCLAMATION);
    }
}

} // namespace GTags
