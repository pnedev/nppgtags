/**
 *  \file
 *  \brief  GTags plugin main routines
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2022 Pavel Nedev
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
#include <objbase.h>
#include <memory>
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
#include "SettingsWin.h"
#include "AboutWin.h"
#include "GTags.h"
#include "LineParser.h"


namespace
{

using namespace GTags;


constexpr int MIN_NOTEPADPP_VERSION_MAJOR = 8;
constexpr int MIN_NOTEPADPP_VERSION_MINOR = 30;

constexpr int MIN_NOTEPADPP_VERSION = ((MIN_NOTEPADPP_VERSION_MAJOR << 16) | MIN_NOTEPADPP_VERSION_MINOR);


std::unique_ptr<CPath>  ChangedFile;
bool                    DeInitCOM = false;


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
        msg += dllPath;
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
CText getSelection(HWND hSci = NULL, bool skipPreSelect = false,
        WordSelection_t selectWord = FULL_SELECT, bool highlightWord = true)
{
    INpp& npp = INpp::Get();

    HWND nppHSci = npp.ReadSciHandle();

    if (hSci)
        npp.SetSciHandle(hSci);

    if (npp.IsSelectionVertical())
    {
        if (hSci)
            npp.SetSciHandle(nppHSci);
        return CText();
    }

    CTextA tagA;

    if (!skipPreSelect)
        npp.GetSelection(tagA);

    if (skipPreSelect || (tagA.IsEmpty() && selectWord != DONT_SELECT))
        npp.GetWord(tagA, selectWord == PARTIAL_SELECT, highlightWord);

    if (hSci)
        npp.SetSciHandle(nppHSci);

    if (tagA.IsEmpty())
        return CText();

    return CText(tagA.C_str());
}


void dbWriteCB(const CmdPtr_t& cmd);


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

    // Search default database for read operations (if no local DB found and default is configured to be used)
    if (!db && !writeEn && GTagsSettings._useDefDb && !GTagsSettings._defDbPath.IsEmpty())
        db = DbManager::Get().GetDbAt(GTagsSettings._defDbPath, writeEn, &success);

    if (!db)
    {
        if (writeEn)
        {
            MessageBox(npp.GetHandle(), _T("GTags database not found"), cPluginName, MB_OK | MB_ICONINFORMATION);
            return db;
        }

        int choice = MessageBox(npp.GetHandle(), _T("GTags database not found.\n\nDo you want to create it?"),
                cPluginName, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
        if (choice != IDYES)
            return db;

        currentFile.StripFilename();

        if (!Tools::BrowseForFolder(npp.GetHandle(), currentFile))
            return db;

        db = DbManager::Get().RegisterDb(currentFile);

        CmdPtr_t cmd(new Cmd(CREATE_DATABASE, db));
        CmdEngine::Run(cmd, dbWriteCB);

        return NULL;
    }
    else if (!success)
    {
        CText msg(_T("Database at\n\""));
        msg += db->GetPath();
        msg += _T("\"\nis currently in use.\nPlease try again later.");

        MessageBox(npp.GetHandle(), msg.C_str(), cPluginName, MB_OK | MB_ICONINFORMATION);
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
void findCB(const CmdPtr_t& cmd)
{
    if (cmd->Status() == OK && cmd->Result() == NULL)
    {
        cmd->Id(FIND_SYMBOL);

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
    else if (cmd->Result())
    {
        CText msg(cmd->Result());
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(), MB_OK | MB_ICONEXCLAMATION);
    }

    if (cmd->Status() == OK)
        ResultWin::NotifyDBUpdate(cmd);
}


/**
*  \brief
*/
void aboutCB(const CmdPtr_t& cmd)
{
    if (cmd->Status() != OK)
    {
        const CTextA txt("\nVERSION READ FAILED\n\n");
        cmd->AppendToResult(txt.Vector());
    }

	const CText msg = cmd->Result();

	AboutWin::Show(msg.C_str());
}


/**
 *  \brief
 */
void halfAboutCB(const CmdPtr_t& cmd)
{
    if (cmd->Status() != OK)
    {
        const CTextA txt("VERSION READ FAILED\n");
        cmd->SetResult(txt.Vector());
    }

    const CTextA txt("\nCurrent Ctags parser version:\n\n");
    cmd->AppendToResult(txt.Vector());

    cmd->Id(CTAGS_VERSION);
    CmdEngine::Run(cmd, aboutCB);
}


/**
 *  \brief
 */
void AutoComplete()
{
    CText tag = getSelection(NULL, true, PARTIAL_SELECT, false);
    if (tag.IsEmpty())
        return;

    DbHandle db = getDatabase();
    if (!db)
        return;

    CmdPtr_t cmd(new Cmd(AUTOCOMPLETE, db, NULL, tag.C_str(), GTagsSettings._ic));

    CmdEngine::Run(cmd, halfComplCB);
}


/**
 *  \brief
 */
void AutoCompleteFile()
{
    CText tag = getSelection(NULL, true, PARTIAL_SELECT, false);
    if (tag.IsEmpty())
        return;

    tag.Insert(0, _T('/'));

    DbHandle db = getDatabase();
    if (!db)
        return;

    ParserPtr_t parser(new LineParser);
    CmdPtr_t cmd(new Cmd(AUTOCOMPLETE_FILE, db, parser, tag.C_str(), GTagsSettings._ic));

    CmdEngine::Run(cmd, autoComplCB);
}


/**
 *  \brief
 */
void FindFile()
{
    SearchWin::Close();

    DbHandle db;
    HWND rwHSci = ResultWin::GetSciHandleIfFocused();

    if (rwHSci)
        db = getDatabaseAt(ResultWin::GetDbPath());
    else
        db = getDatabase();

    if (!db)
        return;

    ParserPtr_t parser(new ResultWin::TabParser);
    CmdPtr_t cmd(new Cmd(FIND_FILE, db, parser, NULL, GTagsSettings._ic));

    CText tag = getSelection(rwHSci, false, DONT_SELECT);
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

    DbHandle db;
    HWND rwHSci = ResultWin::GetSciHandleIfFocused();

    if (rwHSci)
        db = getDatabaseAt(ResultWin::GetDbPath());
    else
        db = getDatabase();

    if (!db)
        return;

    ParserPtr_t parser(new ResultWin::TabParser);
    CmdPtr_t cmd(new Cmd(FIND_DEFINITION, db, parser, NULL, GTagsSettings._ic));

    CText tag = getSelection(rwHSci);
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

    DbHandle db;
    HWND rwHSci = ResultWin::GetSciHandleIfFocused();

    if (rwHSci)
        db = getDatabaseAt(ResultWin::GetDbPath());
    else
        db = getDatabase();

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
    CmdPtr_t cmd(new Cmd(FIND_REFERENCE, db, parser, NULL, GTagsSettings._ic));

    CText tag = getSelection(rwHSci);
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

    DbHandle db;
    HWND rwHSci = ResultWin::GetSciHandleIfFocused();

    if (rwHSci)
        db = getDatabaseAt(ResultWin::GetDbPath());
    else
        db = getDatabase();

    if (!db)
        return;

    ParserPtr_t parser(new ResultWin::TabParser);
    CmdPtr_t cmd(new Cmd(GREP, db, parser, NULL, GTagsSettings._ic));

    CText tag = getSelection(rwHSci);
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

    DbHandle db;
    HWND rwHSci = ResultWin::GetSciHandleIfFocused();

    if (rwHSci)
        db = getDatabaseAt(ResultWin::GetDbPath());
    else
        db = getDatabase();

    if (!db)
        return;

    ParserPtr_t parser(new ResultWin::TabParser);
    CmdPtr_t cmd(new Cmd(GREP_TEXT, db, parser, NULL, GTagsSettings._ic));

    CText tag = getSelection(rwHSci);
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
void IgnoreCase()
{
    GTagsSettings._ic = !GTagsSettings._ic;

    INpp::Get().SetPluginMenuFlag(Menu[8]._cmdID, GTagsSettings._ic);

    GTagsSettings._dirty = true;
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
        CText msg(_T("Database at\n\""));
        msg += db->GetPath();

        if (!success)
        {
            msg += _T("\"\nis currently in use.\nPlease try again later.");
            MessageBox(npp.GetHandle(), msg.C_str(), cPluginName, MB_OK | MB_ICONINFORMATION);
            return;
        }

        msg += _T("\"\nexists.\nRe-create?");
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

    CmdPtr_t cmd(new Cmd(CREATE_DATABASE, db));
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
    DbHandle db = DbManager::Get().GetDb(currentFile, true, &success);
    if (db)
    {
        if (success)
        {
            SettingsWin::Show(db);
            return;
        }

        CText msg(_T("Database at\n\""));
        msg += db->GetPath();
        msg += _T("\"\nis currently in use.\nIts config cannot be modified at the moment.");

        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cPluginName, MB_OK | MB_ICONINFORMATION);
    }

    SettingsWin::Show();
}


/**
 *  \brief
 */
void About()
{
    CmdPtr_t cmd(new Cmd(VERSION));
    CmdEngine::Run(cmd, halfAboutCB);
}

} // anonymous namespace


namespace GTags
{

FuncItem Menu[21] = {
    /* 0 */  FuncItem(Cmd::CmdName[AUTOCOMPLETE], AutoComplete),
    /* 1 */  FuncItem(Cmd::CmdName[AUTOCOMPLETE_FILE], AutoCompleteFile),
    /* 2 */  FuncItem(Cmd::CmdName[FIND_FILE], FindFile),
    /* 3 */  FuncItem(Cmd::CmdName[FIND_DEFINITION], FindDefinition),
    /* 4 */  FuncItem(Cmd::CmdName[FIND_REFERENCE], FindReference),
    /* 5 */  FuncItem(Cmd::CmdName[GREP], SearchSrc),
    /* 6 */  FuncItem(Cmd::CmdName[GREP_TEXT], SearchOther),
    /* 7 */  FuncItem(),
    /* 8 */  FuncItem(_T("Ignore Case"), IgnoreCase), // Array number is important as it is used to toggle the flag!!!
    /* 9 */  FuncItem(),
    /* 10*/  FuncItem(_T("Go Back"), GoBack),
    /* 11*/  FuncItem(_T("Go Forward"), GoForward),
    /* 12 */ FuncItem(),
    /* 13 */ FuncItem(Cmd::CmdName[CREATE_DATABASE], CreateDatabase),
    /* 14 */ FuncItem(_T("Delete Database"), DeleteDatabase),
    /* 15 */ FuncItem(),
    /* 16 */ FuncItem(_T("Toggle Results Window Focus"), ToggleResultWinFocus),
    /* 17 */ FuncItem(),
    /* 18 */ FuncItem(_T("Settings..."), SettingsCfg),
    /* 19 */ FuncItem(),
    /* 20 */ FuncItem(_T("About..."), About)
};

HINSTANCE HMod = NULL;
CPath DllPath;

CText UIFontName;
unsigned UIFontSize;

HWND MainWndH = NULL;

Settings GTagsSettings;


/**
 *  \brief
 */
DbHandle getDatabaseAt(const CPath& dbPath)
{
    bool success;

    DbHandle db = DbManager::Get().GetDbAt(dbPath, false, &success);

    if (!db)
    {
        CText msg(_T("Database at\n\""));
        msg += dbPath;
        msg += _T("\"\ndoes not exist.");

        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cPluginName, MB_OK | MB_ICONINFORMATION);
    }
    else if (!success)
    {
        CText msg(_T("Database at\n\""));
        msg += db->GetPath();
        msg += _T("\"\nis currently in use.\nPlease try again later.");

        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cPluginName, MB_OK | MB_ICONINFORMATION);
        db = NULL;
    }

    return db;
}


/**
 *  \brief
 */
void showResultCB(const CmdPtr_t& cmd)
{
    DbManager::Get().PutDb(cmd->Db());

    if (cmd->Status() == OK || cmd->Status() == PARSE_EMPTY)
    {
        if (cmd->Result() && cmd->Status() == OK)
        {
            ResultWin::Show(cmd);
        }
        else
        {
            CText msg(_T("\""));
            msg += cmd->Tag();
            msg += _T("\" not found.");
            MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(), MB_OK | MB_ICONINFORMATION);

            ResultWin::Close(cmd);
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
        MessageBox(INpp::Get().GetHandle(), _T("Database seems outdated.\nPlease re-create it and redo the search."),
                cmd->Name(), MB_OK | MB_ICONEXCLAMATION);
    }
}


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

    const HRESULT coInitRes = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    DeInitCOM = (coInitRes == S_OK || coInitRes == S_FALSE) ? true : false;

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
        if (!GTagsSettings.Load())
            GTagsSettings.Save();
    }
}


/**
 *  \brief
 */
void PluginDeInit()
{
    if (GTagsSettings._dirty)
        GTagsSettings.Save();

    ActivityWin::Unregister();
    SearchWin::Unregister();
    AutoCompleteWin::Unregister();
    ResultWin::Unregister();

    if (DeInitCOM)
    {
        DeInitCOM = false;
        CoUninitialize();
    }

    HMod = NULL;
}


/**
 *  \brief
 */
void OnNppReady()
{
    INpp& npp = INpp::Get();

    npp.SetPluginMenuFlag(Menu[8]._cmdID, GTagsSettings._ic);

	// if (npp.GetVersion() < MIN_NOTEPADPP_VERSION)
	// {
		// TCHAR buf[256];

		// _sntprintf_s(buf, _countof(buf), _TRUNCATE,
				// _T("%s plugin version is for Notepad++ versions above v%d.%d (included).")
                // _T("\nIt might not function as expected and might cause instability or crash!"),
				// cPluginName, MIN_NOTEPADPP_VERSION_MAJOR, MIN_NOTEPADPP_VERSION_MINOR);

		// MessageBox(npp.GetHandle(), buf, cPluginName, MB_OK | MB_ICONERROR);
	// }
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
