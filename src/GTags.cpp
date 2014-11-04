/**
 *  \file
 *  \brief  GTags plugin main routines
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


#define WIN32_LEAN_AND_MEAN
#include "GTags.h"
#include <Shlobj.h>
#include "Common.h"
#include "AutoLock.h"
#include "INpp.h"
#include "DBManager.h"
#include "GTagsCmd.h"
#include "DocLocation.h"
#include "IOWindow.h"
#include "AutoCompleteUI.h"
#include "TreeViewUI.h"
#include <list>


#define LINUX_WINE_WORKAROUNDS


namespace
{

using namespace GTags;


const TCHAR cAbout[] = {
    _T("\n") VER_DESCRIPTION _T("\n\n")
    _T("Version: ") VER_VERSION_STR _T("\n")
    _T("Build date: %s %s\n")
    VER_COPYRIGHT _T(" <pg.nedev@gmail.com>\n\n")
    _T("Licensed under GNU GPLv2 ")
    _T("as published by the Free Software Foundation.\n\n")
    _T("This plugin is frontend to ")
    _T("GNU Global source code tagging system (GTags):\n")
    _T("http://www.gnu.org/software/global/global.html\n")
    _T("Thanks to its developers and to ")
    _T("Jason Hood for porting it to Windows.\n\n")
    _T("Current GTags version:\n%s")
};


std::list<CPath> UpdateList;
Mutex UpdateLock;


void releaseKeys();
int getSelection(TCHAR* sel, bool autoSelectWord = false,
        bool skipPreSelect = false);
DBhandle getDatabase(bool writeMode = false);
int CALLBACK browseFolderCB(HWND hwnd, UINT umsg, LPARAM, LPARAM lpData);
bool enterTag(TCHAR* tag, const TCHAR* uiName = NULL,
        const TCHAR* defaultTag = NULL);
void sheduleForUpdate(const CPath& file);
bool runSheduledUpdate(const TCHAR* dbPath);
void autoComplHalf(CmdData& cmd);
void autoComplReady(CmdData& cmd);
void findReady(CmdData& cmd);
void fillTreeView(CmdData& cmd);
void showInfo(CmdData& cmd);


/**
 *  \brief
 */
inline void releaseKeys()
{
#ifdef LINUX_WINE_WORKAROUNDS
    Tools::ReleaseKey(VK_SHIFT);
    Tools::ReleaseKey(VK_CONTROL);
    Tools::ReleaseKey(VK_MENU);
#endif // LINUX_WINE_WORKAROUNDS
}


/**
 *  \brief
 */
int getSelection(TCHAR* sel, bool autoSelectWord, bool skipPreSelect)
{
    INpp& npp = INpp::Get();

    npp.ReadSciHandle();
    if (npp.IsSelectionVertical())
        return 0;

    char tagA[cMaxTagLen];
    long len = npp.GetSelection(tagA, cMaxTagLen);
    if (skipPreSelect || (len == 0 && autoSelectWord))
        len = npp.GetWord(tagA, cMaxTagLen, true);
    if (len)
    {
        if (len >= cMaxTagLen)
        {
            MessageBox(npp.GetHandle(), _T("Tag string too long"),
                    cPluginName, MB_OK | MB_ICONEXCLAMATION);
            return 0;
        }

        size_t cnt;
        mbstowcs_s(&cnt, sel, cMaxTagLen, tagA, _TRUNCATE);
    }
    else
    {
        sel[0] = 0;
    }

    return len;
}


/**
 *  \brief
 */
DBhandle getDatabase(bool writeMode)
{
    INpp& npp = INpp::Get();
    bool success;
    CPath currentFile;

    npp.GetFilePath(currentFile);

    DBhandle db = DBManager::Get().GetDB(currentFile, writeMode, &success);
    if (!db)
    {
        MessageBox(npp.GetHandle(), _T("GTags database not found"),
                cPluginName, MB_OK | MB_ICONEXCLAMATION);
    }
    else if (!success)
    {
        MessageBox(npp.GetHandle(), _T("GTags database is in use"),
                cPluginName, MB_OK | MB_ICONEXCLAMATION);
        db = NULL;
    }

    return db;
}


/**
 *  \brief
 */
int CALLBACK browseFolderCB(HWND hwnd, UINT umsg, LPARAM, LPARAM lpData)
{
    if (umsg == BFFM_INITIALIZED)
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    return 0;
}


/**
 *  \brief
 */
bool enterTag(TCHAR* tag, const TCHAR* uiName, const TCHAR* defaultTag)
{
    if (defaultTag)
        _tcscpy_s(tag, cMaxTagLen, defaultTag);
    else
        tag[0] = 0;

    return IOWindow::In(HInst, INpp::Get().GetHandle(), UIFontName,
            UIFontSize + 2, 400, uiName, tag, cMaxTagLen - 1);
}


/**
 *  \brief
 */
void sheduleForUpdate(const CPath& file)
{
    AUTOLOCK(UpdateLock);

    std::list<CPath>::reverse_iterator updFile;
    for (updFile = UpdateList.rbegin(); updFile != UpdateList.rend();
        updFile++)
        if (*updFile == file)
            return;

    UpdateList.push_back(file);
}


/**
 *  \brief
 */
bool runSheduledUpdate(const TCHAR* dbPath)
{
    CPath file;
    {
        AUTOLOCK(UpdateLock);

        if (UpdateList.empty())
            return false;

        std::list<CPath>::iterator updFile;
        for (updFile = UpdateList.begin(); updFile != UpdateList.end();
            updFile++)
            if (updFile->IsContainedIn(dbPath))
                break;

        if (updFile == UpdateList.end())
            return false;

        file = *updFile;
        UpdateList.erase(updFile);
    }

    if (!UpdateSingleFile(file.C_str()))
        return runSheduledUpdate(dbPath);

    return true;
}


/**
 *  \brief
 */
void cmdReady(CmdData& cmd)
{
    runSheduledUpdate(cmd.GetDBPath());

    if (cmd.Error())
        MessageBox(INpp::Get().GetHandle(), cmd.GetResult(),
                cmd.GetName(), MB_OK | MB_ICONERROR);
}


/**
 *  \brief
 */
void autoComplHalf(CmdData& cmd)
{
    if (cmd.Error())
    {
        CText msg(cmd.GetResult());
        msg += _T("\nTry re-creating database.");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd.GetName(),
                MB_OK | MB_ICONERROR);
        return;
    }

    DBhandle db = getDatabase();
    if (db)
        Cmd::Run(AUTOCOMPLETE_SYMBOL, cmd.GetName(), db, cmd.GetTag(),
                autoComplReady, cmd.GetResult());
}


/**
 *  \brief
 */
void autoComplReady(CmdData& cmd)
{
    runSheduledUpdate(cmd.GetDBPath());

    INpp& npp = INpp::Get();

    if (cmd.Error())
    {
        CText msg(cmd.GetResult());
        msg += _T("\nTry re-creating database.");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd.GetName(),
                MB_OK | MB_ICONERROR);
        return;
    }

    if (cmd.NoResult())
        npp.ClearSelection();
    else
        AutoCompleteUI::Create(cmd);
}


/**
 *  \brief
 */
void findReady(CmdData& cmd)
{
    if (cmd.NoResult())
    {
        DBhandle db = getDatabase();
        if (db)
            Cmd::Run(FIND_SYMBOL, cFindSymbol, db, cmd.GetTag(), fillTreeView);
        return;
    }

    fillTreeView(cmd);
}


/**
 *  \brief
 */
void fillTreeView(CmdData& cmd)
{
    runSheduledUpdate(cmd.GetDBPath());

    INpp& npp = INpp::Get();

    if (cmd.Error())
    {
        CText msg(cmd.GetResult());
        msg += _T("\nTry re-creating database.");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd.GetName(),
                MB_OK | MB_ICONERROR);
        return;
    }

    if (cmd.NoResult())
    {
        TCHAR msg[cMaxTagLen + 32];
        _sntprintf_s(msg, cMaxTagLen + 32, _TRUNCATE, _T("\"%s\" not found"),
                cmd.GetTag());
        MessageBox(npp.GetHandle(), msg, cmd.GetName(),
                MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    TreeViewUI::Get().Show(cmd);
}


/**
 *  \brief
 */
void showInfo(CmdData& cmd)
{
    TCHAR text[2048];
    _sntprintf_s(text, 2048, _TRUNCATE, cAbout, _T(__DATE__), _T(__TIME__),
            cmd.Error() || cmd.NoResult() ?
            _T("GTAGS VERSION READ FAILED\n") : cmd.GetResult());

    IOWindow::Out(HInst, INpp::Get().GetHandle(), UIFontName, UIFontSize,
            cVersion, text);
}

} // anonymous namespace


namespace GTags
{

HINSTANCE HInst = NULL;
CPath DllPath;
TCHAR UIFontName[32];
unsigned UIFontSize;
bool AutoUpdate = true;


/**
 *  \brief
 */
void AutoComplete()
{
    TCHAR tag[cMaxTagLen];
    if (!getSelection(tag, true, true))
        return;

    DBhandle db = getDatabase();
    if (!db)
        return;

    releaseKeys();
    Cmd::Run(AUTOCOMPLETE, cAutoCompl, db, tag, autoComplHalf);
}


/**
 *  \brief
 */
void AutoCompleteFile()
{
    TCHAR tag[cMaxTagLen];
    if (!getSelection(tag, true, true))
        return;

    DBhandle db = getDatabase();
    if (!db)
        return;

    releaseKeys();
    Cmd::Run(AUTOCOMPLETE_FILE, cAutoComplFile, db, tag, autoComplReady);
}


/**
 *  \brief
 */
void FindFile()
{
    TCHAR tag[cMaxTagLen];
    if (!getSelection(tag))
    {
        CPath fileName;
        INpp::Get().GetFileNamePart(fileName);
        if (fileName.Len() >= cMaxTagLen)
            fileName.C_str()[cMaxTagLen - 1] = 0;

        if (!enterTag(tag, cFindFile, fileName.C_str()))
            return;
    }

    DBhandle db = getDatabase();
    if (!db)
        return;

    releaseKeys();
    Cmd::Run(FIND_FILE, cFindFile, db, tag, fillTreeView);
}


/**
 *  \brief
 */
void FindDefinition()
{
    TCHAR tag[cMaxTagLen];
    if (!getSelection(tag, true))
    {
        if (!enterTag(tag, cFindDefinition))
            return;
    }

    DBhandle db = getDatabase();
    if (!db)
        return;

    releaseKeys();
    Cmd::Run(FIND_DEFINITION, cFindDefinition, db, tag, findReady);
}


/**
 *  \brief
 */
void FindReference()
{
    TCHAR tag[cMaxTagLen];
    if (!getSelection(tag, true))
    {
        if (!enterTag(tag, cFindReference))
            return;
    }

    DBhandle db = getDatabase();
    if (!db)
        return;

    releaseKeys();
    Cmd::Run(FIND_REFERENCE, cFindReference, db, tag, findReady);
}


/**
 *  \brief
 */
void Grep()
{
    TCHAR tag[cMaxTagLen];
    if (!getSelection(tag, true))
    {
        if (!enterTag(tag, cGrep))
            return;
    }

    DBhandle db = getDatabase();
    if (!db)
        return;

    releaseKeys();
    Cmd::Run(GREP, cGrep, db, tag, fillTreeView);
}


/**
 *  \brief
 */
void FindLiteral()
{
    TCHAR tag[cMaxTagLen];
    if (!getSelection(tag, true))
    {
        if (!enterTag(tag, cFindLiteral))
            return;
    }

    DBhandle db = getDatabase();
    if (!db)
        return;

    releaseKeys();
    Cmd::Run(FIND_LITERAL, cFindLiteral, db, tag, fillTreeView);
}


/**
 *  \brief
 */
void GoBack()
{
    DocLocation::Get().Pop();
}


/**
 *  \brief
 */
void CreateDatabase()
{
    INpp& npp = INpp::Get();
    bool success;
    CPath currentFile;
    npp.GetFilePath(currentFile);

    DBhandle db = DBManager::Get().GetDB(currentFile, true, &success);
    if (db)
    {
        TCHAR buf[512];
        _sntprintf_s(buf, 512, _TRUNCATE,
                _T("Database at\n\"%s\" exists.\nRe-create?"), db->C_str());
        int choice = MessageBox(npp.GetHandle(), buf, cPluginName,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
        if (choice != IDYES)
        {
            DBManager::Get().PutDB(db);
            return;
        }
    }
    else
    {
        currentFile.StripFilename();

        BROWSEINFO bi       = {0};
        bi.hwndOwner        = npp.GetHandle();
        bi.pszDisplayName   = currentFile.C_str();
        bi.lpszTitle        = _T("Point to the root of your project");
        bi.ulFlags          = BIF_RETURNONLYFSDIRS;
        bi.lpfn             = browseFolderCB;
        bi.lParam           = (DWORD)currentFile.C_str();

        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (!pidl)
            return;

        SHGetPathFromIDList(pidl, currentFile.C_str());

        IMalloc* imalloc = NULL;
        if (SUCCEEDED(SHGetMalloc(&imalloc)))
        {
            imalloc->Free(pidl);
            imalloc->Release();
        }

		currentFile += _T("\\");
        db = DBManager::Get().RegisterDB(currentFile, true);
    }

    releaseKeys();
    Cmd::Run(CREATE_DATABASE, cCreateDatabase, db, NULL, cmdReady);
}


/**
 *  \brief
 */
bool UpdateSingleFile(const TCHAR* file)
{
    CPath currentFile(file);
    if (!file)
        INpp::Get().GetFilePath(currentFile);

    bool success;
    DBhandle db = DBManager::Get().GetDB(currentFile, true, &success);
    if (!db)
        return false;

    if (!success)
    {
        sheduleForUpdate(currentFile);
        return true;
    }

    releaseKeys();
    if (!Cmd::Run(UPDATE_SINGLE, cUpdateSingle, db, currentFile.C_str(),
            cmdReady))
        return false;

    return true;
}


/**
 *  \brief
 */
void DeleteDatabase()
{
    DBhandle db = getDatabase(true);
    if (!db)
        return;

    INpp& npp = INpp::Get();
    TCHAR buf[512];
    _sntprintf_s(buf, 512, _TRUNCATE,
            _T("Delete database from\n\"%s\"?"), db->C_str());
    int choice = MessageBox(npp.GetHandle(), buf, cPluginName,
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
    if (choice != IDYES)
    {
        DBManager::Get().PutDB(db);
        return;
    }

    if (DBManager::Get().UnregisterDB(db))
        MessageBox(npp.GetHandle(), _T("GTags database deleted"),
                cPluginName, MB_OK | MB_ICONINFORMATION);
    else
        MessageBox(npp.GetHandle(),
                _T("Deleting database failed, is it read-only?"),
                cPluginName, MB_OK | MB_ICONERROR);
}


/**
 *  \brief
 */
void About()
{
    releaseKeys();
    Cmd::Run(VERSION, cVersion, NULL, NULL, showInfo);
}

} // namespace GTags
