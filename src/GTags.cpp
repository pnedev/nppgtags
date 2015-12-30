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
#include <memory>
#include "Common.h"
#include "INpp.h"
#include "DbConfig.h"
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
const TCHAR cAutoCompl[]        = _T("AutoComplete");
const TCHAR cAutoComplFile[]    = _T("AutoComplete File Name");
const TCHAR cFindFile[]         = _T("Find File");
const TCHAR cFindDefinition[]   = _T("Find Definition");
const TCHAR cFindReference[]    = _T("Find Reference");
const TCHAR cFindSymbol[]       = _T("Find Symbol");
const TCHAR cSearchSrc[]        = _T("Search in Source Files");
const TCHAR cSearchOther[]       = _T("Search in Other Files");
const TCHAR cVersion[]          = _T("About");


std::unique_ptr<CPath> ChangedFile;


/**
 *  \brief
 */
bool checkForGTagsBinaries(CPath& dllPath)
{
    dllPath.StripFilename();
    dllPath += cPluginName;
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
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(), MB_OK | MB_ICONEXCLAMATION);
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
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(), MB_OK | MB_ICONEXCLAMATION);
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
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(), MB_OK | MB_ICONEXCLAMATION);
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
void dbWriteCB(const CmdPtr_t& cmd)
{
    if (cmd->Status() != OK)
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
void halfAboutCB(const CmdPtr_t& cmd)
{
    if (cmd->Status() == OK)
    {
        CTextA txt("\nCurrent Ctags parser version:\n\n");
        cmd->AppendToResult(txt.Vector());
        cmd->Id(CTAGS_VERSION);
        CmdEngine::Run(cmd, aboutCB);
    }
    else
    {
        CText msg(_T("VERSION READ FAILED\n"));
        AboutWin::Show(msg.C_str());
    }
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

    DbHandle db = getDatabase();
    if (!db)
        return;

    if (db->GetConfig()._parserIdx == DbConfig::CTAGS_PARSER)
    {
        MessageBox(INpp::Get().GetHandle(), _T("Ctags parser doesn't support reference search"), cPluginName,
                MB_OK | MB_ICONINFORMATION);

        DbManager::Get().PutDb(db);

        return;
    }

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
void SearchSrc()
{
    SearchWin::Close();

    DbHandle db = getDatabase();
    if (!db)
        return;

    ParserPtr_t parser(new ResultWin::TabParser);
    CmdPtr_t cmd(new Cmd(GREP, cSearchSrc, db, parser));

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
void SearchOther()
{
    SearchWin::Close();

    DbHandle db = getDatabase();
    if (!db)
        return;

    ParserPtr_t parser(new ResultWin::TabParser);
    CmdPtr_t cmd(new Cmd(GREP_TEXT, cSearchOther, db, parser));

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

        if (!Tools::BrowseForFolder(npp.GetHandle(), currentFile))
            return;

        db = DbManager::Get().RegisterDb(currentFile);
    }

    CmdPtr_t cmd(new Cmd(CREATE_DATABASE, cCreateDatabase, db));
    CmdEngine::Run(cmd, dbWriteCB);
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
    SearchWin::Close();

    CPath currentFile;
    INpp::Get().GetFilePath(currentFile);

    bool success;
    DbHandle db = DbManager::Get().GetDb(currentFile, false, &success);
    if (db)
    {
        ConfigWin::Show(db->GetConfig(), db->GetPath().C_str());
        if (success)
            DbManager::Get().PutDb(db);
    }
    else
    {
        ConfigWin::Show(DefaultDbCfg);
    }
}


/**
 *  \brief
 */
void About()
{
    CmdPtr_t cmd(new Cmd(VERSION, cVersion));
    CmdEngine::Run(cmd, halfAboutCB);
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
    /* 5 */  FuncItem(cSearchSrc, SearchSrc),
    /* 6 */  FuncItem(cSearchOther, SearchOther),
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

HWND MainWndH = NULL;

DbConfig DefaultDbCfg;


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

    MainWndH = ResultWin::Register();
    if (MainWndH == NULL)
    {
        MessageBox(npp.GetHandle(), _T("Results Window init failed, plugin will not be operational"), cPluginName,
                MB_OK | MB_ICONERROR);
    }
    else
    {
        CPath cfgFolder;
        npp.GetPluginsConfDir(cfgFolder);

        if (!DefaultDbCfg.LoadFromFolder(cfgFolder))
            MessageBox(npp.GetHandle(), _T("Bad default config file, default settings will be used"), cPluginName,
                    MB_OK | MB_ICONEXCLAMATION);
    }
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
void OnFileBeforeChange(const CPath& file)
{
    ChangedFile.reset(new CPath(file));
}


/**
 *  \brief
 */
void OnFileChangeCancel()
{
    ChangedFile.reset();
}


/**
 *  \brief
 */
void OnFileChange(const CPath& file)
{
    CPath path(file);

    while (path.DirUp())
    {
        bool success;
        DbHandle db = DbManager::Get().GetDb(path, true, &success);
        if (!db)
            break;

        if (db->GetConfig()._autoUpdate)
        {
            if (success)
                db->Update(file);
            else
                db->ScheduleUpdate(file);
        }
        else if (success)
        {
            DbManager::Get().PutDb(db);
        }

        path = db->GetPath();
    }
}


/**
 *  \brief
 */
void OnFileRename(const CPath& file)
{
    OnFileChange(file);

    if (ChangedFile)
    {
        OnFileChange(*ChangedFile);
        ChangedFile.reset();
    }
}


/**
 *  \brief
 */
void OnFileDelete(const CPath& file)
{
    if (ChangedFile)
    {
        OnFileChange(*ChangedFile);
        ChangedFile.reset();
    }
    else
    {
        OnFileChange(file);
    }
}

} // namespace GTags
