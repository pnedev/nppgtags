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


#define WIN32_LEAN_AND_MEAN
#include "GTags.h"
#include <shlobj.h>
#include <list>
#include "AutoLock.h"
#include "INpp.h"
#include "DbManager.h"
#include "CmdEngine.h"
#include "DocLocation.h"
#include "SearchWin.h"
#include "ActivityWin.h"
#include "AutoCompleteWin.h"
#include "ResultWin.h"
#include "ConfigWin.h"
#include "AboutWin.h"


#define LINUX_WINE_WORKAROUNDS


namespace
{

const TCHAR cDefaultParser[]    = _T("default");
const TCHAR cCtagsParser[]      = _T("ctags");
const TCHAR cPygmentsParser[]   = _T("pygments");


using namespace GTags;

std::list<CPath> UpdateList;
Mutex UpdateLock;


/**
*  \brief
*/
inline void releaseKeys()
{
#ifdef LINUX_WINE_WORKAROUNDS
    Tools::ReleaseKey(VK_SHIFT, false);
    Tools::ReleaseKey(VK_CONTROL, false);
    Tools::ReleaseKey(VK_MENU, false);
#endif // LINUX_WINE_WORKAROUNDS
}


/**
 *  \brief
 */
bool checkForGTagsBinaries(const TCHAR* dllPath)
{
    CPath gtags(dllPath);
    gtags.StripFilename();
    gtags += cBinsDir;
    gtags += _T("\\global.exe");

    bool gtagsBinsFound = gtags.FileExists();
    if (gtagsBinsFound)
    {
        gtags.StripFilename();
        gtags += _T("gtags.exe");
        gtagsBinsFound = gtags.FileExists();
    }

    if (!gtagsBinsFound)
    {
        gtags.StripFilename();
        TCHAR msg[512];
        _sntprintf_s(msg, _countof(msg), _TRUNCATE,
                _T("GTags binaries not found in\n\"%s\"\n")
                _T("%s plugin will not be loaded!"),
                gtags.C_str(), cBinsDir);
        MessageBox(NULL, msg, cPluginName, MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}


/**
 *  \brief
 */
unsigned getSelection(TCHAR* sel, bool autoSelectWord = false,
        bool skipPreSelect = false)
{
    INpp& npp = INpp::Get();

    npp.ReadSciHandle();
    if (npp.IsSelectionVertical())
        return 0;

    char tagA[cMaxTagLen];
    unsigned len = npp.GetSelection(tagA, cMaxTagLen);
    if (skipPreSelect || (len == 0 && autoSelectWord))
        len = npp.GetWord(tagA, cMaxTagLen, true);
    if (len)
    {
        if (len >= cMaxTagLen)
        {
            MessageBox(npp.GetHandle(), _T("Search string too long"),
                    cPluginName, MB_OK | MB_ICONINFORMATION);
            return 0;
        }

        Tools::AtoW(sel, cMaxTagLen, tagA);
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
DbHandle getDatabase(bool writeEn = false)
{
    INpp& npp = INpp::Get();
    bool success;
    TCHAR file[MAX_PATH];
    npp.GetFilePath(file);
    CPath currentFile(file);

    DbHandle db = DbManager::Get().GetDb(currentFile, writeEn, &success);
    if (!db)
    {
        MessageBox(npp.GetHandle(), _T("GTags database not found"),
                cPluginName, MB_OK | MB_ICONINFORMATION);
    }
    else if (!success)
    {
        MessageBox(npp.GetHandle(), _T("GTags database is currently in use"),
                cPluginName, MB_OK | MB_ICONINFORMATION);
        db = NULL;
    }

    return db;
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
void sheduleForUpdate(const CPath& file)
{
    AUTOLOCK(UpdateLock);

    std::list<CPath>::reverse_iterator iFile;
    for (iFile = UpdateList.rbegin(); iFile != UpdateList.rend(); iFile++)
        if (*iFile == file)
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

        std::list<CPath>::iterator iFile;
        for (iFile = UpdateList.begin(); iFile != UpdateList.end(); iFile++)
            if (iFile->IsContainedIn(dbPath))
                break;

        if (iFile == UpdateList.end())
            return false;

        file = *iFile;
        UpdateList.erase(iFile);
    }

    if (!UpdateSingleFile(file.C_str()))
        return runSheduledUpdate(dbPath);

    return true;
}


/**
 *  \brief
 */
void dbWriteReady(const std::shared_ptr<Cmd>& cmd)
{
    if (cmd->Status() != OK && cmd->Id() == CREATE_DATABASE)
        DbManager::Get().UnregisterDb(cmd->Db());
    else
        DbManager::Get().PutDb(cmd->Db());

    runSheduledUpdate(cmd->DbPath());

    if (cmd->Status() == FAILED)
    {
        CText msg(cmd->Result());
        MessageBox(INpp::Get().GetHandle(), msg.C_str(),
                cmd->Name(), MB_OK | MB_ICONERROR);
    }
    else if (cmd->Status() == RUN_ERROR)
    {
        MessageBox(INpp::Get().GetHandle(), _T("Running GTags failed"),
                cmd->Name(), MB_OK | MB_ICONERROR);
    }
}


/**
 *  \brief
 */
void autoComplReady(const std::shared_ptr<Cmd>& cmd)
{
    DbManager::Get().PutDb(cmd->Db());

    runSheduledUpdate(cmd->DbPath());

    if (cmd->Status() == OK)
    {
        if (cmd->Result())
            AutoCompleteWin::Show(cmd);
        else
            INpp::Get().ClearSelection();
    }
    else if (cmd->Status() == FAILED)
    {
        INpp::Get().ClearSelection();

        CText msg(cmd->Result());
        msg += _T("\nTry re-creating database.");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(),
                MB_OK | MB_ICONERROR);
    }
    else if (cmd->Status() == RUN_ERROR)
    {
        INpp::Get().ClearSelection();

        MessageBox(INpp::Get().GetHandle(), _T("Running GTags failed"),
                cmd->Name(), MB_OK | MB_ICONERROR);
    }
    else
    {
        INpp::Get().ClearSelection();
    }
}


/**
 *  \brief
 */
void showResult(const std::shared_ptr<Cmd>& cmd)
{
    DbManager::Get().PutDb(cmd->Db());

    runSheduledUpdate(cmd->DbPath());

    if (cmd->Status() == OK)
    {
        if (cmd->Result())
        {
            ResultWin::Show(cmd);
        }
        else
        {
            TCHAR msg[cMaxTagLen + 32];
            _sntprintf_s(msg, _countof(msg), _TRUNCATE, _T("\"%s\" not found"),
                    cmd->Tag());
            MessageBox(INpp::Get().GetHandle(), msg, cmd->Name(),
                    MB_OK | MB_ICONINFORMATION);
        }
    }
    else if (cmd->Status() == FAILED)
    {
        CText msg(cmd->Result());
        msg += _T("\nTry re-creating database.");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->Name(),
                MB_OK | MB_ICONERROR);
    }
    else if (cmd->Status() == RUN_ERROR)
    {
        MessageBox(INpp::Get().GetHandle(), _T("Running GTags failed"),
                cmd->Name(), MB_OK | MB_ICONERROR);
    }
}


/**
 *  \brief
 */
void findReady(const std::shared_ptr<Cmd>& cmd)
{
    if (cmd->Status() == OK && cmd->Result() == NULL)
    {
        cmd->Id(FIND_SYMBOL);
        cmd->Name(cFindSymbol);

        CmdEngine::Run(cmd, showResult);
    }
    else
    {
        showResult(cmd);
    }
}

} // anonymous namespace


namespace GTags
{

FuncItem Menu[18] = {
    /* 0 */  FuncItem(cAutoCompl, AutoComplete),
    /* 1 */  FuncItem(cAutoComplFile, AutoCompleteFile),
    /* 2 */  FuncItem(cFindFile, FindFile),
    /* 3 */  FuncItem(cFindDefinition, FindDefinition),
    /* 4 */  FuncItem(cFindReference, FindReference),
    /* 5 */  FuncItem(cSearch, Search),
    /* 6 */  FuncItem(),
    /* 7 */  FuncItem(_T("Go Back"), GoBack),
    /* 8 */  FuncItem(_T("Go Forward"), GoForward),
    /* 9 */  FuncItem(),
    /* 10 */ FuncItem(cCreateDatabase, CreateDatabase),
    /* 11 */ FuncItem(_T("Delete Database"), DeleteDatabase),
    /* 12 */ FuncItem(),
    /* 13 */ FuncItem(_T("Toggle Results Window Focus"), ToggleResultWinFocus),
    /* 14 */ FuncItem(),
    /* 15 */ FuncItem(_T("Settings"), SettingsCfg),
    /* 16 */ FuncItem(),
    /* 17 */ FuncItem(cVersion, About)
};

HINSTANCE HMod = NULL;
CPath DllPath;

TCHAR UIFontName[32];
unsigned UIFontSize;

const TCHAR* cParsers[3];

CConfig Config;


/**
 *  \brief
 */
BOOL PluginInit(HINSTANCE hMod)
{
    TCHAR moduleFileName[MAX_PATH];
    GetModuleFileName((HMODULE)hMod, moduleFileName, MAX_PATH);
    DllPath = moduleFileName;

    if (!checkForGTagsBinaries(moduleFileName))
        return FALSE;

    HMod = hMod;

    cParsers[DEFAULT_PARSER]    = cDefaultParser;
    cParsers[CTAGS_PARSER]      = cCtagsParser;
    cParsers[PYGMENTS_PARSER]   = cPygmentsParser;

    ActivityWin::Register();
    SearchWin::Register();
    AutoCompleteWin::Register();

    return TRUE;
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
        for (idx = 0; idx < itemsCount; idx++)
        {
            mi.dwTypeData = NULL;
            GetMenuItemInfo(hMenu, idx, TRUE, &mi);
            mi.dwTypeData = buf;
            mi.cch++;
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


/**
 *  \brief
 */
void AutoComplete()
{
    TCHAR tag[cMaxTagLen];
    if (!getSelection(tag, true, true))
        return;

    DbHandle db = getDatabase();
    if (!db)
    {
        INpp::Get().ClearSelection();
        return;
    }

    std::shared_ptr<Cmd> cmd(new Cmd(AUTOCOMPLETE, cAutoCompl, db, tag));

    if (CmdEngine::Run(cmd))
    {
        cmd->Id(AUTOCOMPLETE_SYMBOL);
        CmdEngine::Run(cmd);
    }

    autoComplReady(cmd);
}


/**
 *  \brief
 */
void AutoCompleteFile()
{
    TCHAR tag[cMaxTagLen];
    tag[0] = _T('/');
    if (!getSelection(&tag[1], true, true))
        return;

    DbHandle db = getDatabase();
    if (!db)
    {
        INpp::Get().ClearSelection();
        return;
    }

    std::shared_ptr<Cmd>
            cmd(new Cmd(AUTOCOMPLETE_FILE, cAutoComplFile, db, tag));

    CmdEngine::Run(cmd);
    autoComplReady(cmd);
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

    std::shared_ptr<Cmd> cmd(new Cmd(FIND_FILE, cFindFile, db));
    TCHAR tag[cMaxTagLen];

    if (getSelection(tag))
    {
        cmd->Tag(tag);

        CmdEngine::Run(cmd, showResult);
    }
    else
    {
        TCHAR fileName[MAX_PATH];
        INpp::Get().GetFileNamePart(fileName);
        cmd->Tag(fileName);

        SearchWin::Show(cmd, showResult);
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

    std::shared_ptr<Cmd> cmd(new Cmd(FIND_DEFINITION, cFindDefinition, db));
    TCHAR tag[cMaxTagLen];

    if (getSelection(tag, true))
    {
        cmd->Tag(tag);

        CmdEngine::Run(cmd, findReady);
    }
    else
    {
        SearchWin::Show(cmd, findReady, false);
    }
}


/**
 *  \brief
 */
void FindReference()
{
    SearchWin::Close();

    if (Config._parserIdx == CTAGS_PARSER)
    {
        MessageBox(INpp::Get().GetHandle(),
                _T("Ctags parser doesn't support reference search"),
                cPluginName, MB_OK | MB_ICONINFORMATION);
        return;
    }

    DbHandle db = getDatabase();
    if (!db)
        return;

    std::shared_ptr<Cmd> cmd(new Cmd(FIND_REFERENCE, cFindReference, db));
    TCHAR tag[cMaxTagLen];

    if (getSelection(tag, true))
    {
        cmd->Tag(tag);

        CmdEngine::Run(cmd, findReady);
    }
    else
    {
        SearchWin::Show(cmd, findReady, false);
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

    std::shared_ptr<Cmd> cmd(new Cmd(GREP, cSearch, db));
    TCHAR tag[cMaxTagLen];

    if (getSelection(tag, true))
    {
        cmd->Tag(tag);

        CmdEngine::Run(cmd, showResult);
    }
    else
    {
        SearchWin::Show(cmd, showResult);
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

    INpp& npp = INpp::Get();
    bool success;
    TCHAR path[MAX_PATH];
    npp.GetFilePath(path);
    CPath currentFile(path);

    DbHandle db = DbManager::Get().GetDb(currentFile, true, &success);
    if (db)
    {
        if (!success)
        {
            MessageBox(npp.GetHandle(),
                    _T("GTags database exists and is currently in use"),
                    cPluginName, MB_OK | MB_ICONINFORMATION);
            return;
        }

        TCHAR buf[512];
        _sntprintf_s(buf, _countof(buf), _TRUNCATE,
                _T("Database at\n\"%s\" exists.\nRe-create?"), db->C_str());
        int choice = MessageBox(npp.GetHandle(), buf, cPluginName,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
        if (choice != IDYES)
        {
            DbManager::Get().PutDb(db);
            return;
        }
    }
    else
    {
        currentFile.StripFilename();

        BROWSEINFO bi       = {0};
        bi.hwndOwner        = npp.GetHandle();
        bi.pszDisplayName   = path;
        bi.lpszTitle        = _T("Select the project root");
        bi.ulFlags          = BIF_RETURNONLYFSDIRS;
        bi.lpfn             = browseFolderCB;
        bi.lParam           = (DWORD)currentFile.C_str();

        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (!pidl)
            return;

        SHGetPathFromIDList(pidl, path);

        IMalloc* imalloc = NULL;
        if (SUCCEEDED(SHGetMalloc(&imalloc)))
        {
            imalloc->Free(pidl);
            imalloc->Release();
        }

        currentFile = path;
        currentFile += _T("\\");
        db = DbManager::Get().RegisterDb(currentFile, true);
    }

    std::shared_ptr<Cmd> cmd(new Cmd(CREATE_DATABASE, cCreateDatabase, db));
    CmdEngine::Run(cmd, dbWriteReady);
}


/**
 *  \brief
 */
const CPath CreateLibraryDatabase(HWND hWnd)
{
    TCHAR path[MAX_PATH];
    CPath libraryPath;

    BROWSEINFO bi       = {0};
    bi.hwndOwner        = hWnd;
    bi.pszDisplayName   = path;
    bi.lpszTitle        = _T("Select the library root");
    bi.ulFlags          = BIF_RETURNONLYFSDIRS;
    bi.lpfn             = browseFolderCB;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (!pidl)
        return libraryPath;

    SHGetPathFromIDList(pidl, path);

    IMalloc* imalloc = NULL;
    if (SUCCEEDED(SHGetMalloc(&imalloc)))
    {
        imalloc->Free(pidl);
        imalloc->Release();
    }

    libraryPath = path;
    libraryPath += _T("\\");

    DbHandle db;

    if (DbManager::Get().DbExistsInFolder(libraryPath))
    {
        TCHAR buf[512];
        _sntprintf_s(buf, _countof(buf), _TRUNCATE,
                _T("Database at\n\"%s\" exists.\nRe-create?"),
                libraryPath.C_str());
        int choice = MessageBox(hWnd, buf, cPluginName,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
        if (choice != IDYES)
            return libraryPath;

        bool success;
        db = DbManager::Get().GetDb(libraryPath, true, &success);

        if (!success)
        {
            MessageBox(hWnd, _T("GTags database is currently in use"),
                    cPluginName, MB_OK | MB_ICONINFORMATION);
            libraryPath = _T("");

            return libraryPath;
        }
    }
    else
    {
        db = DbManager::Get().RegisterDb(libraryPath, true);
    }

    std::shared_ptr<Cmd> cmd(new Cmd(CREATE_DATABASE, cCreateDatabase, db));
    CmdEngine::Run(cmd);

    dbWriteReady(cmd);
    if (cmd->Status() != OK)
        libraryPath = _T("");

    return libraryPath;
}


/**
 *  \brief
 */
bool UpdateSingleFile(const TCHAR* file)
{
    CPath upFile(file);
    if (!file)
    {
        TCHAR filePath[MAX_PATH];
        INpp::Get().GetFilePath(filePath);
        upFile = filePath;
    }

    bool success;
    DbHandle db = DbManager::Get().GetDb(upFile, true, &success);
    if (!db)
        return false;

    if (!success)
    {
        sheduleForUpdate(upFile);
        return true;
    }

    releaseKeys();

    std::shared_ptr<Cmd>
            cmd(new Cmd(UPDATE_SINGLE, cUpdateSingle, db, upFile.C_str()));

    if (!CmdEngine::Run(cmd, dbWriteReady))
        return false;

    return true;
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
    _sntprintf_s(buf, _countof(buf), _TRUNCATE,
            _T("Delete database from\n\"%s\"?"), db->C_str());
    int choice = MessageBox(npp.GetHandle(), buf, cPluginName,
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
    if (choice != IDYES)
    {
        DbManager::Get().PutDb(db);
        return;
    }

    if (DbManager::Get().UnregisterDb(db))
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
void ToggleResultWinFocus()
{
    releaseKeys();
    GTags::ResultWin::Show();
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
    std::shared_ptr<Cmd> cmd(new Cmd(VERSION, cVersion));
    CmdEngine::Run(cmd);

    CText msg;
    if (cmd->Status() == OK)
        msg = cmd->Result();
    else
        msg = _T("VERSION READ FAILED\n");

    AboutWin::Show(msg.C_str());
}

} // namespace GTags
