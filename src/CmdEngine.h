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
    static const TCHAR* CmdLine[];

    static unsigned __stdcall threadFunc(void* data);

    CmdEngine(const CmdPtr_t& cmd, CompletionCB complCB);
    ~CmdEngine();
    CmdEngine& operator=(const CmdEngine&) = delete;

    unsigned start();
    void composeCmd(CText& buf) const;
    void setEnvironmentVars() const;
    bool runProcess(PROCESS_INFORMATION& pi, ReadPipe& dataPipe, ReadPipe& errorPipe);
    void endProcess(PROCESS_INFORMATION& pi);

    CmdPtr_t            _cmd;
    CompletionCB const  _complCB;
    HANDLE              _hThread;
};

} // namespace GTags
