/**
 *  \file
 *  \brief  GTags command execution engine
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
#include <process.h>
#include "Common.h"
#include "INpp.h"
#include "Config.h"
#include "GTags.h"
#include "ReadPipe.h"
#include "CmdEngine.h"
#include "Cmd.h"


namespace GTags
{

const TCHAR* CmdEngine::CmdLine[] = {
    _T("\"%s\\gtags.exe\" -c --skip-unreadable"),                           // CREATE_DATABASE
    _T("\"%s\\gtags.exe\" -c --skip-unreadable --single-update \"%s\""),    // UPDATE_SINGLE
    _T("\"%s\\global.exe\" -cT \"%s\""),                                    // AUTOCOMPLETE
    _T("\"%s\\global.exe\" -cs \"%s\""),                                    // AUTOCOMPLETE_SYMBOL
    _T("\"%s\\global.exe\" -cP --match-part=all \"%s\""),                   // AUTOCOMPLETE_FILE
    _T("\"%s\\global.exe\" -P \"%s\""),                                     // FIND_FILE
    _T("\"%s\\global.exe\" -dT --result=grep \"%s\""),                      // FIND_DEFINITION
    _T("\"%s\\global.exe\" -r --result=grep \"%s\""),                       // FIND_REFERENCE
    _T("\"%s\\global.exe\" -s --result=grep \"%s\""),                       // FIND_SYMBOL
    _T("\"%s\\global.exe\" -g --result=grep \"%s\""),                       // GREP
    _T("\"%s\\global.exe\" -gO --result=grep \"%s\""),                      // GREP_TEXT
    _T("\"%s\\global.exe\" --version"),                                     // VERSION
    _T("\"%s\\ctags.exe\" --version")                                       // CTAGS_VERSION
};


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
CmdEngine::CmdEngine(const CmdPtr_t& cmd, CompletionCB complCB) :
    _cmd(cmd), _complCB(complCB), _hThread(NULL)
{
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

            if (_cmd->_id != VERSION && _cmd->_id != CTAGS_VERSION)
            {
                header += _T(" - \"");
                if (_cmd->_id == CREATE_DATABASE)
                    header += _cmd->Db()->GetPath();
                else
                    header += _cmd->Tag();
                header += _T('\"');
            }

            SendMessage(MainWndH, WM_OPEN_ACTIVITY_WIN,
                    reinterpret_cast<WPARAM>(header.C_str()), reinterpret_cast<LPARAM>(hCancel));

            HANDLE waitHandles[] = {pi.hProcess, hCancel};
            DWORD handleId = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE) - WAIT_OBJECT_0;
            if (handleId > 0 && handleId < 2 && waitHandles[handleId] == hCancel)
                _cmd->_status = CANCELLED;

            SendMessage(MainWndH, WM_CLOSE_ACTIVITY_WIN, 0, reinterpret_cast<LPARAM>(hCancel));

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
        _cmd->AppendToResult(dataPipe.GetOutput());
    }
    else if (!errorPipe.GetOutput().empty())
    {
        _cmd->SetResult(errorPipe.GetOutput());

        if (_cmd->_id != CREATE_DATABASE && _cmd->_id != UPDATE_SINGLE)
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
            const int parsedEntries = _cmd->_parser->Parse(_cmd);

            if (parsedEntries < 0)
            {
                _cmd->_status = PARSE_ERROR;
                return 1;
            }
            else if (parsedEntries == 0)
            {
                _cmd->_status = PARSE_EMPTY; // No results to display actually (due to some filtering)
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
        _cmd->Db()->SaveCfg();

    return 0;
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

    if (_cmd->_id == CREATE_DATABASE || _cmd->_id == VERSION || _cmd->_id == CTAGS_VERSION)
        _sntprintf_s(buf.C_str(), buf.Size(), _TRUNCATE, CmdLine[_cmd->_id], path.C_str());
    else
        _sntprintf_s(buf.C_str(), buf.Size(), _TRUNCATE, CmdLine[_cmd->_id], path.C_str(),
                _cmd->Tag().C_str());

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
    else if (_cmd->_id != VERSION && _cmd->_id != CTAGS_VERSION)
    {
        if (_cmd->_ignoreCase)
            buf += _T(" -i");
        else
            buf += _T(" -M");

        if (!_cmd->_regExp)
            buf += _T(" --literal");
    }
}


/**
 *  \brief
 */
void CmdEngine::setEnvironmentVars() const
{
    CText buf;

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

    if (_cmd->Db())
        SetEnvironmentVariable(_T("GTAGSDBPATH"), _cmd->Db()->GetPath().C_str());

    SetEnvironmentVariable(_T("GTAGSLIBPATH"), buf.C_str());
}


/**
 *  \brief
 */
bool CmdEngine::runProcess(PROCESS_INFORMATION& pi, ReadPipe& dataPipe, ReadPipe& errorPipe)
{
    const DWORD createFlags = NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT;
    const TCHAR* currentDir = (_cmd->_id == VERSION || _cmd->_id == CTAGS_VERSION) ?
            NULL : _cmd->Db()->GetPath().C_str();

    CText cmdBuf;
    composeCmd(cmdBuf);

    setEnvironmentVars();

    STARTUPINFO si  = {0};
    si.cb           = sizeof(si);
    si.dwFlags      = STARTF_USESTDHANDLES;
    si.hStdError    = errorPipe.GetInputHandle();
    si.hStdOutput   = dataPipe.GetInputHandle();

    if (!CreateProcess(NULL, cmdBuf.C_str(), NULL, NULL, TRUE, createFlags, NULL, currentDir, &si, &pi))
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
