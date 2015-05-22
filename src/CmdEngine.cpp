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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include "INpp.h"
#include "Config.h"
#include "GTags.h"
#include "ActivityWin.h"
#include "ReadPipe.h"
#include "CmdEngine.h"


namespace GTags
{

const TCHAR CmdEngine::cCreateDatabaseCmd[] =
        _T("\"%s\\gtags.exe\" -c");
const TCHAR CmdEngine::cUpdateSingleCmd[] =
        _T("\"%s\\gtags.exe\" -c --single-update \"%s\"");
const TCHAR CmdEngine::cAutoComplCmd[]       =
        _T("\"%s\\global.exe\" -cT \"%s\"");
const TCHAR CmdEngine::cAutoComplSymCmd[] =
        _T("\"%s\\global.exe\" -cs \"%s\"");
const TCHAR CmdEngine::cAutoComplFileCmd[] =
        _T("\"%s\\global.exe\" -cP --match-part=all \"%s\"");
const TCHAR CmdEngine::cFindFileCmd[] =
        _T("\"%s\\global.exe\" -P \"%s\"");
const TCHAR CmdEngine::cFindDefinitionCmd[] =
        _T("\"%s\\global.exe\" -dT --result=grep \"%s\"");
const TCHAR CmdEngine::cFindReferenceCmd[] =
        _T("\"%s\\global.exe\" -r --result=grep \"%s\"");
const TCHAR CmdEngine::cFindSymbolCmd[] =
        _T("\"%s\\global.exe\" -s --result=grep \"%s\"");
const TCHAR CmdEngine::cGrepCmd[] =
        _T("\"%s\\global.exe\" -g --result=grep \"%s\"");
const TCHAR CmdEngine::cVersionCmd[] =
        _T("\"%s\\global.exe\" --version");


/**
 *  \brief
 */
Cmd::Cmd(CmdId_t id, const TCHAR* name, DbHandle db, const TCHAR* tag,
        bool regExp, bool matchCase) : _id(id), _db(db),
        _regExp(regExp), _matchCase(matchCase), _status(CANCELLED)
{
    if (db)
        _dbPath = *db;

    _name[0] = 0;
    if (name)
        _tcscpy_s(_name, _countof(_name), name);

    if (tag)
        _tag(tag);
}


/**
 *  \brief
 */
void Cmd::appendResult(const char* result)
{
    if (result == NULL)
        return;

    if (&_result != NULL)
    {
        CCharArray oldResult;
        oldResult(&_result);
        _result(oldResult.Size() + strlen(result));
        _result = &oldResult;
        _result += result;
    }
    else
    {
        setResult(result);
    }
}


/**
 *  \brief
 */
bool CmdEngine::Run(const std::shared_ptr<Cmd>& cmd, CompletionCB complCB)
{
    CmdEngine* engine = new CmdEngine(cmd, complCB);
    cmd->Status(RUN_ERROR);

    engine->_hThread = (HANDLE)_beginthreadex(NULL, 0, threadFunc,
            (void*)engine, 0, NULL);
    if (engine->_hThread == NULL)
    {
        if (complCB)
            complCB(cmd);
        delete engine;
        return false;
    }

    if (complCB)
        return true;

    // If no callback is given then block until command is ready and return
    // the exit code. On false, command has failed or has been terminated
    WaitForSingleObject(engine->_hThread, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(engine->_hThread, &exitCode);

    delete engine;

    return !exitCode;
}


/**
 *  \brief
 */
CmdEngine::~CmdEngine()
{
    if (_hThread)
        CloseHandle(_hThread);
}


/**
 *  \brief
 */
unsigned __stdcall CmdEngine::threadFunc(void* data)
{
    CmdEngine* engine = static_cast<CmdEngine*>(data);
    unsigned r = engine->runProcess();

    if (engine->_complCB)
    {
        engine->_complCB(engine->_cmd);
        delete engine;
    }

    return r;
}


/**
 *  \brief
 */
const TCHAR* CmdEngine::getCmdLine() const
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
void CmdEngine::composeCmd(TCHAR* buf, unsigned len) const
{
    CPath path(DllPath);
    path.StripFilename();
    path += cBinsDir;

    if (_cmd->_id == CREATE_DATABASE || _cmd->_id == VERSION)
        _sntprintf_s(buf, len, _TRUNCATE, getCmdLine(), path.C_str());
    else
        _sntprintf_s(buf, len, _TRUNCATE, getCmdLine(), path.C_str(),
                _cmd->Tag());

    if (_cmd->_id == CREATE_DATABASE || _cmd->_id == UPDATE_SINGLE)
    {
        path += _T("\\gtags.conf");
        if (path.FileExists())
        {
            _tcscat_s(buf, len, _T(" --gtagsconf \""));
            _tcscat_s(buf, len, path.C_str());
            _tcscat_s(buf, len, _T("\""));
            _tcscat_s(buf, len, _T(" --gtagslabel="));
            _tcscat_s(buf, len, Config.Parser());
        }
    }
    else if (_cmd->_id != VERSION)
    {
        if (_cmd->_matchCase)
            _tcscat_s(buf, len, _T(" -M"));
        else
            _tcscat_s(buf, len, _T(" -i"));

        if (!_cmd->_regExp)
            _tcscat_s(buf, len, _T(" --literal"));
    }
}


/**
 *  \brief
 */
unsigned CmdEngine::runProcess()
{
    TCHAR buf[2048];
    composeCmd(buf, _countof(buf));

    TCHAR header[512];
    if (_cmd->_id == CREATE_DATABASE)
        _sntprintf_s(header, _countof(header), _TRUNCATE,
                _T("%s - \"%s\""), _cmd->Name(), _cmd->DbPath());
    else if (_cmd->_id == VERSION)
        _sntprintf_s(header, _countof(header), _TRUNCATE,
                _T("%s"), _cmd->Name());
    else
        _sntprintf_s(header, _countof(header), _TRUNCATE,
                _T("%s - \"%s\""), _cmd->Name(), _cmd->Tag());

    DWORD createFlags = NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW;
    const TCHAR* env = NULL;
    const TCHAR* currentDir = _cmd->DbPath();

    CText envVars(_T("GTAGSLIBPATH="));
    if (Config._useLibDb)
        envVars += Config._libDbPath;

    if (_cmd->_id == VERSION)
    {
        currentDir = NULL;
    }
    else if (_cmd->_id == AUTOCOMPLETE || _cmd->_id == FIND_DEFINITION)
    {
        env = envVars.C_str();
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
    if (!CreateProcess(NULL, buf, NULL, NULL, TRUE, createFlags,
            (LPVOID)env, currentDir, &si, &pi))
    {
        _cmd->_status = RUN_ERROR;
        return 1;
    }

    SetThreadPriority(pi.hThread, THREAD_PRIORITY_NORMAL);

    if (!errorPipe.Open() || !dataPipe.Open())
    {
        terminate(pi);
        _cmd->_status = RUN_ERROR;
        return 1;
    }

    if (ActivityWin::Show(INpp::Get().GetSciHandle(), pi.hProcess, 600, header,
            (_cmd->_id == CREATE_DATABASE || _cmd->_id == UPDATE_SINGLE) ?
            0 : 300))
    {
        terminate(pi);
        _cmd->_status = CANCELLED;
        return 1;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (dataPipe.GetOutput())
    {
        _cmd->appendResult(dataPipe.GetOutput());
    }
    else if (errorPipe.GetOutput())
    {
        _cmd->setResult(errorPipe.GetOutput());
        _cmd->_status = FAILED;
        return 1;
    }

    _cmd->_status = OK;

    return 0;
}


/**
 *  \brief
 */
bool CmdEngine::isActive(PROCESS_INFORMATION& pi) const
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
void CmdEngine::terminate(PROCESS_INFORMATION& pi) const
{
    if (isActive(pi))
    {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

} // namespace GTags
