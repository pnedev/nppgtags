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
#include "DBManager.h"
#include "Cmd.h"
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
    Tools::ReleaseKey(VK_SHIFT);
    Tools::ReleaseKey(VK_CONTROL);
    Tools::ReleaseKey(VK_MENU);
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
            MessageBox(npp.GetHandle(), _T("Tag string too long"),
                    cPluginName, MB_OK | MB_ICONEXCLAMATION);
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
DBhandle getDatabase(bool writeEn = false)
{
    INpp& npp = INpp::Get();
    bool success;
    TCHAR file[MAX_PATH];
    npp.GetFilePath(file);
    CPath currentFile(file);

    DBhandle db = DBManager::Get().GetDB(currentFile, writeEn, &success);
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
unsigned enterTag(GTags::SearchData* searchData, const TCHAR* uiName,
        bool enMatchCase = true, bool enRegExp = false)
{
    SearchWin::Show(uiName, searchData, enMatchCase, enRegExp);

    return _tcslen(searchData->_str);
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
void checkError(std::shared_ptr<CmdData>& cmd)
{
    runSheduledUpdate(cmd->GetDBPath());

    if (cmd->Error())
    {
        CText msg(cmd->GetResult());
        MessageBox(INpp::Get().GetHandle(), msg.C_str(),
                cmd->GetName(), MB_OK | MB_ICONERROR);
    }
}


/**
 *  \brief
 */
void autoComplReady(std::shared_ptr<CmdData>& cmd)
{
    runSheduledUpdate(cmd->GetDBPath());

    if (cmd->Error())
    {
        CText msg(cmd->GetResult());
        msg += _T("\nTry re-creating database.");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->GetName(),
                MB_OK | MB_ICONERROR);
        return;
    }

    if (cmd->NoResult())
        INpp::Get().ClearSelection();
    else
        AutoCompleteWin::Show(cmd);
}


/**
 *  \brief
 */
void autoComplSymbol(std::shared_ptr<CmdData>& cmd)
{
    if (cmd->Error())
    {
        CText msg(cmd->GetResult());
        msg += _T("\nTry re-creating database.");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->GetName(),
                MB_OK | MB_ICONERROR);
        return;
    }

    DBhandle db = getDatabase();
    if (db)
    {
        cmd->SetID(AUTOCOMPLETE_SYMBOL);
        cmd->SetDB(db);
        if (Cmd::Run(cmd, db))
            autoComplReady(cmd);
    }
}


/**
 *  \brief
 */
void showResult(std::shared_ptr<CmdData>& cmd)
{
    runSheduledUpdate(cmd->GetDBPath());

    if (cmd->Error())
    {
        CText msg(cmd->GetResult());
        msg += _T("\nTry re-creating database.");
        MessageBox(INpp::Get().GetHandle(), msg.C_str(), cmd->GetName(),
                MB_OK | MB_ICONERROR);
        return;
    }

    if (cmd->NoResult())
    {
        TCHAR msg[cMaxTagLen + 32];
        _sntprintf_s(msg, _countof(msg), _TRUNCATE, _T("\"%s\" not found"),
                cmd->GetTag());
        MessageBox(INpp::Get().GetHandle(), msg, cmd->GetName(),
                MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    ResultWin::Get().Show(cmd);
}


/**
 *  \brief
 */
void findReady(std::shared_ptr<CmdData>& cmd)
{
    if (cmd->NoResult())
    {
        DBhandle db = getDatabase();
        if (db)
        {
            cmd->SetID(FIND_SYMBOL);
            cmd->SetName(cFindSymbol);
            cmd->SetDB(db);
            Cmd::Run(cmd, db, showResult);
        }
        return;
    }

    showResult(cmd);
}

} // anonymous namespace


namespace GTags
{

FuncItem Menu[16] = {
    /* 0 */  FuncItem(cAutoCompl, AutoComplete),
    /* 1 */  FuncItem(cAutoComplFile, AutoCompleteFile),
    /* 2 */  FuncItem(cFindFile, FindFile),
    /* 3 */  FuncItem(cFindDefinition, FindDefinition),
    /* 4 */  FuncItem(cFindReference, FindReference),
    /* 5 */  FuncItem(cGrep, Grep),
    /* 6 */  FuncItem(),
    /* 7 */  FuncItem(_T("Go Back"), GoBack),
    /* 8 */  FuncItem(_T("Go Forward"), GoForward),
    /* 9 */  FuncItem(),
    /* 10 */ FuncItem(cCreateDatabase, CreateDatabase),
    /* 11 */ FuncItem(_T("Delete Database"), DeleteDatabase),
    /* 12 */ FuncItem(),
    /* 13 */ FuncItem(_T("Settings"), SettingsCfg),
    /* 14 */ FuncItem(),
    /* 15 */ FuncItem(cVersion, About)
};

HINSTANCE HMod = NULL;
CPath DllPath;

TCHAR UIFontName[32];
unsigned UIFontSize;

const TCHAR* cParsers[3];

Settings Config;



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

    ResultWin::Get().Unregister();

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

    DBhandle db = getDatabase();
    if (!db)
        return;

    std::shared_ptr<CmdData>
        cmd(new CmdData(AUTOCOMPLETE, cAutoCompl, db, tag));
    if (Cmd::Run(cmd, db))
        autoComplSymbol(cmd);
}


/**
 *  \brief
 */
void AutoCompleteFile()
{
    TCHAR tag[cMaxTagLen];
    if (!getSelection(&tag[1], true, true))
        return;

    DBhandle db = getDatabase();
    if (!db)
        return;

    tag[0] = '/';
    std::shared_ptr<CmdData>
            cmd(new CmdData(AUTOCOMPLETE_FILE, cAutoComplFile, db, tag));
    if (Cmd::Run(cmd, db))
        autoComplReady(cmd);
}


/**
 *  \brief
 */
void FindFile()
{
    SearchData searchData(NULL, false, true);
    if (!getSelection(searchData._str))
    {
        TCHAR fileName[MAX_PATH];
        INpp::Get().GetFileNamePart(fileName);
        if (_tcslen(fileName) >= cMaxTagLen)
            fileName[cMaxTagLen - 1] = 0;
        _tcscpy_s(searchData._str, cMaxTagLen, fileName);

        if (!enterTag(&searchData, cFindFile, true, true))
            return;
    }

    DBhandle db = getDatabase();
    if (!db)
        return;

    std::shared_ptr<CmdData>
            cmd(new CmdData(FIND_FILE, cFindFile, db,
            searchData._str, searchData._regExp, searchData._matchCase));
    Cmd::Run(cmd, db, showResult);
}


/**
 *  \brief
 */
void FindDefinition()
{
    SearchData searchData(NULL, false, true);
    if (!getSelection(searchData._str, true))
    {
        if (!enterTag(&searchData, cFindDefinition))
            return;
    }

    DBhandle db = getDatabase();
    if (!db)
        return;

    std::shared_ptr<CmdData>
            cmd(new CmdData(FIND_DEFINITION, cFindDefinition, db,
            searchData._str, searchData._regExp, searchData._matchCase));
    Cmd::Run(cmd, db, findReady);
}


/**
 *  \brief
 */
void FindReference()
{
    if (Config._parserIdx == CTAGS_PARSER)
    {
        MessageBox(INpp::Get().GetHandle(),
                _T("Ctags parser doesn't support reference search"),
                cPluginName, MB_OK | MB_ICONINFORMATION);
        return;
    }

    SearchData searchData(NULL, false, true);
    if (!getSelection(searchData._str, true))
    {
        if (!enterTag(&searchData, cFindReference))
            return;
    }

    DBhandle db = getDatabase();
    if (!db)
        return;

    std::shared_ptr<CmdData>
            cmd(new CmdData(FIND_REFERENCE, cFindReference, db,
            searchData._str, searchData._regExp, searchData._matchCase));
    Cmd::Run(cmd, db, findReady);
}


/**
 *  \brief
 */
void Grep()
{
    SearchData searchData(NULL, true, true);
    if (!getSelection(searchData._str, true))
    {
        if (!enterTag(&searchData, cGrep, true, true))
            return;
    }

    DBhandle db = getDatabase();
    if (!db)
        return;

    std::shared_ptr<CmdData>
            cmd(new CmdData(GREP, cGrep, db,
            searchData._str, searchData._regExp, searchData._matchCase));
    Cmd::Run(cmd, db, showResult);
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
    INpp& npp = INpp::Get();
    bool success;
    TCHAR path[MAX_PATH];
    npp.GetFilePath(path);
    CPath currentFile(path);

    DBhandle db = DBManager::Get().GetDB(currentFile, true, &success);
    if (db)
    {
        TCHAR buf[512];
        _sntprintf_s(buf, _countof(buf), _TRUNCATE,
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
        db = DBManager::Get().RegisterDB(currentFile, true);
    }

    std::shared_ptr<CmdData>
            cmd(new CmdData(CREATE_DATABASE, cCreateDatabase, db));
    Cmd::Run(cmd, db, checkError);
}


/**
 *  \brief
 */
const CPath CreateLibraryDatabase(HWND hwnd)
{
    TCHAR path[MAX_PATH];
    CPath libraryPath;

    BROWSEINFO bi       = {0};
    bi.hwndOwner        = hwnd;
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

    if (DBManager::Get().DB_ExistsInFolder(libraryPath))
    {
        TCHAR buf[512];
        _sntprintf_s(buf, _countof(buf), _TRUNCATE,
                _T("Database at\n\"%s\" exists.\nRe-create?"),
                libraryPath.C_str());
        int choice = MessageBox(hwnd, buf, cPluginName,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
        if (choice != IDYES)
            return libraryPath;
    }

    DBhandle db = DBManager::Get().RegisterDB(libraryPath, true);

    std::shared_ptr<CmdData>
            cmd(new CmdData(CREATE_DATABASE, cCreateDatabase, db));
    if (Cmd::Run(cmd, db))
    {
        checkError(cmd);
        if (cmd->Error())
            libraryPath = _T("");
    }
    else
    {
        libraryPath = _T("");
    }

    return libraryPath;
}


/**
 *  \brief
 */
bool UpdateSingleFile(const TCHAR* file)
{
    CPath currentFile(file);
    if (!file)
    {
        TCHAR filePath[MAX_PATH];
        INpp::Get().GetFilePath(filePath);
        currentFile = filePath;
    }

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

    std::shared_ptr<CmdData>
            cmd(new CmdData(UPDATE_SINGLE, cUpdateSingle, db,
                    currentFile.C_str()));
    if (!Cmd::Run(cmd, db, checkError))
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
    _sntprintf_s(buf, _countof(buf), _TRUNCATE,
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
void SettingsCfg()
{
    ConfigWin::Show(&Config);
}


/**
 *  \brief
 */
void About()
{
    std::shared_ptr<CmdData> cmd(new CmdData(VERSION, cVersion));
    bool success = Cmd::Run(cmd);

    CText msg;
    if (!success || cmd->Error() || cmd->NoResult())
        msg = _T("VERSION READ FAILED\n");
    else
        msg = cmd->GetResult();

    AboutWin::Show(msg.C_str());
}

} // namespace GTags
