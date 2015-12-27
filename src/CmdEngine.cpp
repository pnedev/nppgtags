/**
 *  \file
 *  \brief  GTags command execution engine
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
#include <process.h>
#include "Common.h"
#include "INpp.h"
#include "DbConfig.h"
#include "GTags.h"
#include "ReadPipe.h"
#include "CmdEngine.h"
#include "Cmd.h"


namespace GTags
{

const TCHAR CmdEngine::cCreateDatabaseCmd[] = _T("\"%s\\gtags.exe\" -c --skip-unreadable");
const TCHAR CmdEngine::cUpdateSingleCmd[]   = _T("\"%s\\gtags.exe\" -c --skip-unreadable --single-update \"%s\"");
const TCHAR CmdEngine::cAutoComplCmd[]      = _T("\"%s\\global.exe\" -cT \"%s\"");
const TCHAR CmdEngine::cAutoComplSymCmd[]   = _T("\"%s\\global.exe\" -cs \"%s\"");
const TCHAR CmdEngine::cAutoComplFileCmd[]  = _T("\"%s\\global.exe\" -cP --match-part=all \"%s\"");
const TCHAR CmdEngine::cFindFileCmd[]       = _T("\"%s\\global.exe\" -P \"%s\"");
const TCHAR CmdEngine::cFindDefinitionCmd[] = _T("\"%s\\global.exe\" -dT --result=grep \"%s\"");
const TCHAR CmdEngine::cFindReferenceCmd[]  = _T("\"%s\\global.exe\" -r --result=grep \"%s\"");
const TCHAR CmdEngine::cFindSymbolCmd[]     = _T("\"%s\\global.exe\" -s --result=grep \"%s\"");
const TCHAR CmdEngine::cGrepCmd[]           = _T("\"%s\\global.exe\" -g --result=grep \"%s\"");
const TCHAR CmdEngine::cGrepTxtCmd[]        = _T("\"%s\\global.exe\" -gO --result=grep \"%s\"");
const TCHAR CmdEngine::cVersionCmd[]        = _T("\"%s\\global.exe\" --version");


/**
 *  \brief
 */
bool CmdEngine::Run(const CmdPtr_t& cmd, CompletionCB complCB)
{
    if (!complCB)
        return false;

    CmdEngine* engine = new CmdEngine(cmd, complCB);
    cmd->Status(RUN_ERROR);

    engine->_hThread = (HANDLE)_beginthreadex(NULL, 0, threadFunc, engine, 0, NULL);
    if (engine->_hThread == NULL)
    {
        delete engine;
        return false;
    }

    return true;
}


/**
 *  \brief
 */
CmdEngine::~CmdEngine()
{
    SendMessage(MainWndH, WM_RUN_CMD_CALLBACK, (WPARAM)_complCB, (LPARAM)(&_cmd));

    if (_hThread)
        CloseHandle(_hThread);
}


/**
 *  \brief
 */
unsigned __stdcall CmdEngine::threadFunc(void* data)
{
    CmdEngine* engine = static_cast<CmdEngine*>(data);
    unsigned r = engine->start();

    delete engine;

    return r;
}


/**
 *  \brief
 */
unsigned CmdEngine::start()
{
    ReadPipe dataPipe;
    ReadPipe errorPipe;

    PROCESS_INFORMATION pi;

    if (!runProcess(pi, dataPipe, errorPipe))
        return 1;

    bool showActivityWin = true;
    if (_cmd->_id != CREATE_DATABASE && _cmd->_id != UPDATE_SINGLE)
    {
        // Wait 300 ms and if process has finished don't show Activity Window
        if (WaitForSingleObject(pi.hProcess, 300) == WAIT_OBJECT_0)
            showActivityWin = false;
    }

    if (showActivityWin)
    {
        HANDLE hCancel = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (hCancel)
        {
            CText header(_cmd->Name());
            header += _T(" - \"");
            if (_cmd->_id == CREATE_DATABASE)
                header += _cmd->Db()->GetPath();
            else if (_cmd->_id != VERSION)
                header += _cmd->Tag();
            header += _T('\"');

            SendMessage(MainWndH, WM_OPEN_ACTIVITY_WIN, (WPARAM)header.C_str(), (LPARAM)hCancel);

            HANDLE waitHandles[] = {pi.hProcess, hCancel};
            DWORD handleId = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE) - WAIT_OBJECT_0;
            if (handleId > 0 && handleId < 2 && waitHandles[handleId] == hCancel)
                _cmd->_status = CANCELLED;

            SendMessage(MainWndH, WM_CLOSE_ACTIVITY_WIN, 0, (LPARAM)hCancel);

            CloseHandle(hCancel);
        }
        else
        {
            WaitForSingleObject(pi.hProcess, INFINITE);
        }
    }

    endProcess(pi);

    if (_cmd->_status == CANCELLED)
        return 1;

    if (!dataPipe.GetOutput().empty())
    {
        _cmd->appendResult(dataPipe.GetOutput());
    }
    else if (!errorPipe.GetOutput().empty())
    {
        _cmd->setResult(errorPipe.GetOutput());

        if (_cmd->_id != CREATE_DATABASE)
        {
            _cmd->_status = FAILED;
            return 1;
        }
    }

    _cmd->_status = OK;

    if (_cmd->_parser)
    {
        if (_cmd->Result())
        {
            if (!_cmd->_parser->Parse(_cmd))
            {
                _cmd->_status = PARSE_ERROR;
                return 1;
            }
        }
        // Blink the auto-complete word to inform the user if nothing is found
        else if (_cmd->_id == AUTOCOMPLETE || _cmd->_id == AUTOCOMPLETE_FILE || _cmd->_id == AUTOCOMPLETE_SYMBOL)
        {
            CTextA wordA;
            INpp::Get().GetWord(wordA, true, true);

            if (wordA.Len())
            {
                CText word(wordA.C_str());

                TCHAR* tag = _cmd->_tag.C_str();
                int len = _cmd->_tag.Len();
                if (_cmd->_id == AUTOCOMPLETE_FILE)
                {
                    ++tag;
                    --len;
                }

                if (!_tcsncmp(word.C_str(), tag, len))
                    Sleep(50);
            }
        }
    }

    if (_cmd->_id == CREATE_DATABASE)
        _cmd->Db()->GetConfig().SaveToFolder(_cmd->Db()->GetPath());

    return 0;
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
        case GREP_TEXT:
            return cGrepTxtCmd;
        case VERSION:
            return cVersionCmd;
    }

    return NULL;
}


/**
 *  \brief
 */
void CmdEngine::composeCmd(CText& buf) const
{
    CPath path(DllPath);
    path.StripFilename();
    path += cPluginName;

    buf.Resize(2048);

    if (_cmd->_id == CREATE_DATABASE || _cmd->_id == VERSION)
        _sntprintf_s(buf.C_str(), buf.Size(), _TRUNCATE, getCmdLine(), path.C_str());
    else
        _sntprintf_s(buf.C_str(), buf.Size(), _TRUNCATE, getCmdLine(), path.C_str(), _cmd->Tag().C_str());

    if (_cmd->_id == CREATE_DATABASE || _cmd->_id == UPDATE_SINGLE)
    {
        path += _T("\\gtags.conf");
        if (path.FileExists())
        {
            buf += _T(" --gtagsconf \"");
            buf += path;
            buf += _T("\"");
            buf += _T(" --gtagslabel=");
            buf += _cmd->Db()->GetConfig().Parser();
        }
    }
    else if (_cmd->_id != VERSION)
    {
        if (_cmd->_matchCase)
            buf += _T(" -M");
        else
            buf += _T(" -i");

        if (!_cmd->_regExp)
            buf += _T(" --literal");
    }
}


/**
 *  \brief
 */
void CmdEngine::prepareEnvironmentVars(CText& buf) const
{
    buf = _T("GTAGSLIBPATH=");

    if (!_cmd->_skipLibs && (_cmd->_id == AUTOCOMPLETE || _cmd->_id == FIND_DEFINITION))
    {
        const DbConfig& cfg = _cmd->Db()->GetConfig();
        if (cfg._useLibDb && cfg._libDbPaths.size())
        {
            if (!cfg._libDbPaths[0].IsSubpathOf(_cmd->Db()->GetPath()))
                buf += cfg._libDbPaths[0];

            for (unsigned i = 1; i < cfg._libDbPaths.size(); ++i)
            {
                if (!cfg._libDbPaths[i].IsSubpathOf(_cmd->Db()->GetPath()))
                {
                    buf += _T(';');
                    buf += cfg._libDbPaths[i];
                }
            }
        }
    }

    buf += _T('\0');
}


/**
 *  \brief
 */
bool CmdEngine::runProcess(PROCESS_INFORMATION& pi, ReadPipe& dataPipe, ReadPipe& errorPipe)
{
    const DWORD createFlags = NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT;
    const TCHAR* currentDir = (_cmd->_id == VERSION) ? NULL : _cmd->Db()->GetPath().C_str();

    CText cmdBuf;
    composeCmd(cmdBuf);

    CText envBuf;
    prepareEnvironmentVars(envBuf);

    STARTUPINFO si  = {0};
    si.cb           = sizeof(si);
    si.dwFlags      = STARTF_USESTDHANDLES;
    si.hStdError    = errorPipe.GetInputHandle();
    si.hStdOutput   = dataPipe.GetInputHandle();

    if (!CreateProcess(NULL, cmdBuf.C_str(), NULL, NULL, TRUE, createFlags, envBuf.C_str(), currentDir, &si, &pi))
    {
        _cmd->_status = RUN_ERROR;
        return false;
    }

    SetThreadPriority(pi.hThread, THREAD_PRIORITY_NORMAL);

    if (!errorPipe.Open() || !dataPipe.Open())
    {
        endProcess(pi);
        _cmd->_status = RUN_ERROR;
        return false;
    }

    return true;
}


/**
 *  \brief
 */
void CmdEngine::endProcess(PROCESS_INFORMATION& pi)
{
    DWORD r;
    GetExitCodeProcess(pi.hProcess, &r);
    if (r == STILL_ACTIVE)
        TerminateProcess(pi.hProcess, 0);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

} // namespace GTags
