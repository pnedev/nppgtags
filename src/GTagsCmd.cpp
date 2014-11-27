/**
 *  \file
 *  \brief  GTags command execution and result classes
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


#include "GTagsCmd.h"
#include "GTags.h"
#include "INpp.h"
#include "ActivityWindow.h"
#include "ReadPipe.h"


namespace GTags
{

const TCHAR Cmd::cCreateDatabaseCmd[]  = _T("\"%s\\gtags.exe\" -c");
const TCHAR Cmd::cUpdateSingleCmd[]    = _T("\"%s\\gtags.exe\" -c")
                                        _T(" --single-update \"%s\"");
const TCHAR Cmd::cAutoComplCmd[]       = _T("\"%s\\global.exe\" -cM \"%s\"");
const TCHAR Cmd::cAutoComplSymCmd[]    = _T("\"%s\\global.exe\" -csM \"%s\"");
const TCHAR Cmd::cAutoComplFileCmd[]   = _T("\"%s\\global.exe\" -cPM \"%s\"");
const TCHAR Cmd::cFindFileCmd[]        = _T("\"%s\\global.exe\" -PM \"%s\"");
const TCHAR Cmd::cFindDefinitionCmd[]  = _T("\"%s\\global.exe\" -dxM \"%s\"");
const TCHAR Cmd::cFindReferenceCmd[]   = _T("\"%s\\global.exe\" -rxM \"%s\"");
const TCHAR Cmd::cFindSymbolCmd[]      = _T("\"%s\\global.exe\" -sxM \"%s\"");
const TCHAR Cmd::cGrepCmd[]            = _T("\"%s\\global.exe\" -gxME \"%s\"");
const TCHAR Cmd::cFindLiteralCmd[]     = _T("\"%s\\global.exe\" -gxM")
                                        _T(" --literal \"%s\"");
const TCHAR Cmd::cVersionCmd[]         = _T("\"%s\\global.exe\" --version");


/**
 *  \brief
 */
bool Cmd::Run(CmdID_t id, const TCHAR* name, DBhandle db, const TCHAR* tag,
        CompletionCB complCB, const TCHAR* result)
{
    Cmd* cmd = new Cmd(id, name, db, tag, complCB, result);

    cmd->_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadFunc,
            (LPVOID)cmd, 0, NULL);
    if (!cmd->_hThread)
    {
        if (db)
            DBManager::Get().PutDB(db);
        delete cmd;
        return false;
    }

    return true;
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
DWORD Cmd::threadFunc(LPVOID data)
{
    if (!data)
        return -1;

    Cmd* cmd = static_cast<Cmd*>(data);
    DWORD r = cmd->thread();

    delete cmd;

    return r;
}


/**
 *  \brief
 */
DWORD Cmd::thread()
{
    if (!runProcess())
    {
        if (_db)
        {
            if (_data._id == CREATE_DATABASE)
                DBManager::Get().UnregisterDB(_db);
            else
                DBManager::Get().PutDB(_db);
        }
        return -1;
    }

    if (_db)
        DBManager::Get().PutDB(_db);

    if (_complCB)
        _complCB(_data);

	return 0;
}


/**
 *  \brief
 */
const TCHAR* Cmd::getCmdLine()
{
    switch (_data._id)
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
        case FIND_LITERAL:
            return cFindLiteralCmd;
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

    if (_data._id == CREATE_DATABASE || _data._id == VERSION)
        _sntprintf_s(cmd, len, _TRUNCATE, getCmdLine(), path.C_str());
    else
        _sntprintf_s(cmd, len, _TRUNCATE, getCmdLine(), path.C_str(),
                _data.GetTag());

    if (_data._id == CREATE_DATABASE || _data._id == UPDATE_SINGLE)
    {
        path += _T("\\gtags.conf");
        if (path.FileExists())
        {
            _tcscat_s(cmd, len, _T(" --gtagsconf \""));
            _tcscat_s(cmd, len, path.C_str());
            _tcscat_s(cmd, len, _T("\""));
        }
    }
}


/**
 *  \brief
 */
bool Cmd::runProcess()
{
    TCHAR cmd[2048];
    composeCmd(cmd, 2048);

    TCHAR header[512];
    if (_data._id == CREATE_DATABASE)
        _sntprintf_s(header, 512, _TRUNCATE,
                _T("%s - \"%s\""), _data.GetName(), _data.GetDBPath());
    else if (_data._id == VERSION)
        _sntprintf_s(header, 512, _TRUNCATE, _T("%s"), _data.GetName());
    else
        _sntprintf_s(header, 512, _TRUNCATE,
                _T("%s - \"%s\""), _data.GetName(), _data.GetTag());

    int activityShowDelay = 300;
    if (_data._id == CREATE_DATABASE || _data._id == UPDATE_SINGLE)
        activityShowDelay = 0;

    ReadPipe errorPipe;
    ReadPipe dataPipe;

    STARTUPINFO si  = {0};
    si.cb           = sizeof(si);
    si.dwFlags      = STARTF_USESTDHANDLES;
    si.hStdError    = errorPipe.GetInputHandle();
    si.hStdOutput   = dataPipe.GetInputHandle();

    PROCESS_INFORMATION pi;
    if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE,
            NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL,
            _data._id == VERSION ? NULL : _data.GetDBPath(), &si, &pi))
        return false;

    SetThreadPriority(pi.hThread, THREAD_PRIORITY_NORMAL);

    bool ret = errorPipe.Open() && dataPipe.Open();
    if (ret)
        ret = !ActivityWindow::Show(HInst, INpp::Get().GetSciHandle(), 600,
                header, pi.hProcess, activityShowDelay);
    if (ret)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (errorPipe.GetOutput())
        {
            _data._error  = true;
            _data._result = errorPipe.GetOutput();
        }
        else if (dataPipe.GetOutput())
        {
            _data._result += dataPipe.GetOutput();
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
