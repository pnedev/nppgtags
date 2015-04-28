/**
 *  \file
 *  \brief  GTags command execution and result classes
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


#include "Cmd.h"
#include "GTags.h"
#include "INpp.h"
#include "ActivityWin.h"
#include "ReadPipe.h"
#include <process.h>


namespace GTags
{

const TCHAR Cmd::cCreateDatabaseCmd[] =
        _T("\"%s\\gtags.exe\" -c");
const TCHAR Cmd::cUpdateSingleCmd[] =
        _T("\"%s\\gtags.exe\" -c --single-update \"%s\"");
const TCHAR Cmd::cAutoComplCmd[]       =
        _T("\"%s\\global.exe\" -cT \"%s\"");
const TCHAR Cmd::cAutoComplSymCmd[] =
        _T("\"%s\\global.exe\" -cs \"%s\"");
const TCHAR Cmd::cAutoComplFileCmd[] =
        _T("\"%s\\global.exe\" -cP --match-part=all \"%s\"");
const TCHAR Cmd::cFindFileCmd[] =
        _T("\"%s\\global.exe\" -P \"%s\"");
const TCHAR Cmd::cFindDefinitionCmd[] =
        _T("\"%s\\global.exe\" -dT --result=grep \"%s\"");
const TCHAR Cmd::cFindReferenceCmd[] =
        _T("\"%s\\global.exe\" -r --result=grep \"%s\"");
const TCHAR Cmd::cFindSymbolCmd[] =
        _T("\"%s\\global.exe\" -s --result=grep \"%s\"");
const TCHAR Cmd::cGrepCmd[] =
        _T("\"%s\\global.exe\" -g --result=grep \"%s\"");
const TCHAR Cmd::cVersionCmd[] =
        _T("\"%s\\global.exe\" --version");


/**
 *  \brief
 */
CmdData::CmdData(CmdID_t id, const TCHAR* name, DBhandle db, const TCHAR* tag,
        bool regExp, bool matchCase) :
    _id(id), _error(false), _regExp(regExp), _matchCase(matchCase)
{
    if (db)
        _dbPath = *db;

    _name[0] = 0;
    if (name)
        _tcscpy_s(_name, _countof(_name), name);

    if (tag)
    {
        _tag(_tcslen(tag) + 1);
        _tag = tag;
    }
}


/**
 *  \brief
 */
void CmdData::SetResult(const char* result)
{
    if (result == NULL)
        return;

    _result(strlen(result) + 1);
    _result = result;
}


/**
 *  \brief
 */
void CmdData::AppendResult(const char* result)
{
    if (result == NULL)
        return;

    if (!NoResult())
    {
        CCharArray oldResult(_result.Size());
        oldResult = &_result;
        _result(oldResult.Size() + strlen(result));
        _result = &oldResult;
        _result += result;
    }
    else
    {
        SetResult(result);
    }
}


/**
 *  \brief
 */
bool Cmd::Run(std::shared_ptr<CmdData>& cmdData, DBhandle db,
        CompletionCB complCB)
{
    Cmd* cmd = new Cmd(cmdData, complCB, db);

    cmd->_hThread = (HANDLE)_beginthreadex(NULL, 0, threadFunc,
            (void*)cmd, 0, NULL);
    if (!cmd->_hThread)
    {
        if (db)
            DBManager::Get().PutDB(db);
        delete cmd;
        return false;
    }

    if (complCB)
        return true;

    // If no callback is given then block until command is ready and return
    // the exit code. On false, command has failed or has been terminated
    WaitForSingleObject(cmd->_hThread, INFINITE);

    DWORD exitCode;
    GetExitCodeThread(cmd->_hThread, &exitCode);

    delete cmd;

    return !exitCode;
}


/**
 *  \brief
 */
Cmd::~Cmd()
{
    if (_hThread)
        CloseHandle(_hThread);
}


/**
 *  \brief
 */
unsigned __stdcall Cmd::threadFunc(void* data)
{
    Cmd* cmd = static_cast<Cmd*>(data);
    unsigned r = cmd->thread();

    if (cmd->_complCB)
    {
        if (!r)
            cmd->_complCB(cmd->_cmd);
        delete cmd;
    }

    return r;
}


/**
 *  \brief
 */
unsigned Cmd::thread()
{
    if (!runProcess())
    {
        if (_db)
        {
            if (_cmd->_id == CREATE_DATABASE)
                DBManager::Get().UnregisterDB(_db);
            else
                DBManager::Get().PutDB(_db);
        }
        return 1;
    }

    if (_db)
        DBManager::Get().PutDB(_db);

    return 0;
}


/**
 *  \brief
 */
const TCHAR* Cmd::getCmdLine()
{
    switch (_cmd->_id)
    {
        case CREATE_DATABASE:
            return cCreateDatabaseCmd;
        case UPDATE_SINGLE:
            return cUpdateSingleCmd;
        case AUTOCOMPLETE:
            return cAutoComplCmd;
        case AUTOCOMPLETE_SYMBOL:
            return cAutoComplSymCmd;
        case AUTOCOMPLETE_FILE:
            return cAutoComplFileCmd;
        case FIND_FILE:
            return cFindFileCmd;
        case FIND_DEFINITION:
            return cFindDefinitionCmd;
        case FIND_REFERENCE:
            return cFindReferenceCmd;
        case FIND_SYMBOL:
            return cFindSymbolCmd;
        case GREP:
            return cGrepCmd;
        case VERSION:
            return cVersionCmd;
    }

    return NULL;
}


/**
 *  \brief
 */
void Cmd::composeCmd(TCHAR* cmd, unsigned len)
{
    CPath path(DllPath);
    path.StripFilename();
    path += cBinsDir;

    if (_cmd->_id == CREATE_DATABASE || _cmd->_id == VERSION)
        _sntprintf_s(cmd, len, _TRUNCATE, getCmdLine(), path.C_str());
    else
        _sntprintf_s(cmd, len, _TRUNCATE, getCmdLine(), path.C_str(),
                _cmd->GetTag());

    if (_cmd->_id == CREATE_DATABASE || _cmd->_id == UPDATE_SINGLE)
    {
        path += _T("\\gtags.conf");
        if (path.FileExists())
        {
            _tcscat_s(cmd, len, _T(" --gtagsconf \""));
            _tcscat_s(cmd, len, path.C_str());
            _tcscat_s(cmd, len, _T("\""));
            _tcscat_s(cmd, len, _T(" --gtagslabel="));
            _tcscat_s(cmd, len, cParsers[Config._parserIdx]);
        }
    }
    else if (_cmd->_id != VERSION)
    {
        if (_cmd->_matchCase)
            _tcscat_s(cmd, len, _T(" -M"));
        else
            _tcscat_s(cmd, len, _T(" -i"));

        if (!_cmd->_regExp)
            _tcscat_s(cmd, len, _T(" --literal"));
    }
}


/**
 *  \brief
 */
bool Cmd::runProcess()
{
    TCHAR cmd[2048];
    composeCmd(cmd, _countof(cmd));

    TCHAR header[512];
    if (_cmd->_id == CREATE_DATABASE)
        _sntprintf_s(header, _countof(header), _TRUNCATE,
                _T("%s - \"%s\""), _cmd->GetName(), _cmd->GetDBPath());
    else if (_cmd->_id == VERSION)
        _sntprintf_s(header, _countof(header), _TRUNCATE,
                _T("%s"), _cmd->GetName());
    else
        _sntprintf_s(header, _countof(header), _TRUNCATE,
                _T("%s - \"%s\""), _cmd->GetName(), _cmd->GetTag());

    DWORD createFlags = NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW;
    const TCHAR* environment = NULL;
    const TCHAR* currentDir = _cmd->GetDBPath();

    CText env(_T("GTAGSLIBPATH="));
    if (Config._useLibraryDB)
        env += Config._libraryDBpath;

    if (_cmd->_id == VERSION)
        currentDir = NULL;
    else if (_cmd->_id == AUTOCOMPLETE || _cmd->_id == FIND_DEFINITION)
    {
        environment = env.C_str();
        createFlags |= CREATE_UNICODE_ENVIRONMENT;
    }

    ReadPipe errorPipe;
    ReadPipe dataPipe;

    STARTUPINFO si  = {0};
    si.cb           = sizeof(si);
    si.dwFlags      = STARTF_USESTDHANDLES;
    si.hStdError    = errorPipe.GetInputHandle();
    si.hStdOutput   = dataPipe.GetInputHandle();

    PROCESS_INFORMATION pi;
    if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, createFlags,
        (LPVOID)environment, currentDir, &si, &pi))
        return false;

    SetThreadPriority(pi.hThread, THREAD_PRIORITY_NORMAL);

    bool ret = errorPipe.Open() && dataPipe.Open();
    if (ret)
    {
        ret = !ActivityWin::Show(INpp::Get().GetSciHandle(), pi.hProcess,
                600, header,
                (_cmd->_id == CREATE_DATABASE || _cmd->_id == UPDATE_SINGLE) ?
                0 : 300);
    }
    if (ret)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (dataPipe.GetOutput())
        {
            _cmd->AppendResult(dataPipe.GetOutput());
        }
        else if (errorPipe.GetOutput())
        {
            _cmd->_error = true;
            _cmd->SetResult(errorPipe.GetOutput());
        }
    }
    else
    {
        terminate(pi);
    }

    return ret;
}


/**
 *  \brief
 */
bool Cmd::isActive(PROCESS_INFORMATION& pi)
{
    bool active = false;

    if (pi.hProcess)
    {
        DWORD dwRet;
        GetExitCodeProcess(pi.hProcess, &dwRet);
        if (dwRet == STILL_ACTIVE)
        {
            active = true;
        }
        else
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    return active;
}


/**
 *  \brief
 */
void Cmd::terminate(PROCESS_INFORMATION& pi)
{
    if (isActive(pi))
    {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

} // namespace GTags
