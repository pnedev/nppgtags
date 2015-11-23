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


#pragma once


#include <windows.h>
#include <tchar.h>
#include "Common.h"
#include "CmdDefines.h"


class ReadPipe;


namespace GTags
{

/**
 *  \class  CmdEngine
 *  \brief
 */
class CmdEngine
{
public:
    static bool Run(const CmdPtr_t& cmd, CompletionCB complCB);

private:
    static const TCHAR  cCreateDatabaseCmd[];
    static const TCHAR  cUpdateSingleCmd[];
    static const TCHAR  cAutoComplCmd[];
    static const TCHAR  cAutoComplSymCmd[];
    static const TCHAR  cAutoComplFileCmd[];
    static const TCHAR  cFindFileCmd[];
    static const TCHAR  cFindDefinitionCmd[];
    static const TCHAR  cFindReferenceCmd[];
    static const TCHAR  cFindSymbolCmd[];
    static const TCHAR  cGrepCmd[];
    static const TCHAR  cVersionCmd[];

    static unsigned __stdcall threadFunc(void* data);

    CmdEngine(const CmdPtr_t& cmd, CompletionCB complCB) : _cmd(cmd), _complCB(complCB), _hThread(NULL) {}
    ~CmdEngine();

    unsigned start();
    const TCHAR* getCmdLine() const;
    void composeCmd(CText& buf) const;
    int runProcess(PROCESS_INFORMATION& pi, ReadPipe& dataPipe, ReadPipe& errorPipe);
    void endProcess(PROCESS_INFORMATION& pi);

    CmdPtr_t            _cmd;
    CompletionCB const  _complCB;
    HANDLE              _hThread;
};

} // namespace GTags
