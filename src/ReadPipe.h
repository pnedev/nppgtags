/**
 *  \file
 *  \brief  Class that reads piped process output
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
#include <vector>


/**
 *  \class  ReadPipe
 *  \brief
 */
class ReadPipe
{
public:
    ReadPipe();
    ~ReadPipe();

    HANDLE GetInputHandle() { return _hIn; }
    bool Open();
    DWORD Wait(DWORD time_ms);
    std::vector<char>& GetOutput();

private:
    static const unsigned cChunkSize;

    static unsigned __stdcall threadFunc(void* data);

    ReadPipe(const ReadPipe&);
    const ReadPipe& operator=(const ReadPipe&);

    unsigned thread();

    BOOL                _ready;
    HANDLE              _hIn;
    HANDLE              _hOut;
    HANDLE              _hThread;
    std::vector<char>   _output;
};
