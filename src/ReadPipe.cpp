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


#include "ReadPipe.h"
#include <process.h>


const unsigned ReadPipe::cChunkSize = 4096;


/**
 *  \brief
 */
ReadPipe::ReadPipe() : _hIn(NULL), _hOut(NULL), _hThread(NULL)
{
    SECURITY_ATTRIBUTES attr    = {0};
    attr.nLength                = sizeof(attr);
    attr.bInheritHandle         = TRUE;

    _ready = CreatePipe(&_hOut, &_hIn, &attr, 0);
    if (_ready)
        _ready = SetHandleInformation(_hOut, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
}


/**
 *  \brief
 */
ReadPipe::~ReadPipe()
{
    if (_hThread)
    {
        Wait(INFINITE);
    }
    else
    {
        if (_hIn)
            CloseHandle(_hIn);
        if (_hOut)
            CloseHandle(_hOut);
    }
}


/**
 *  \brief
 */
bool ReadPipe::Open()
{
    if (!_ready || !_hOut)
        return false;
    if (_hThread)
        return true;

    CloseHandle(_hIn);
    _hIn = NULL;
    _hThread = (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, NULL);
    if (_hThread)
        return true;

    // on error
    CloseHandle(_hOut);
    _hOut = NULL;
    return false;
}


/**
 *  \brief
 */
DWORD ReadPipe::Wait(DWORD time_ms)
{
    if (!_hThread)
        return WAIT_OBJECT_0;

    DWORD r = WaitForSingleObject(_hThread, time_ms);
    if (r != WAIT_TIMEOUT)
    {
        CloseHandle(_hOut);
        _hOut = NULL;
        CloseHandle(_hThread);
        _hThread = NULL;
    }

    return r;
}


/**
 *  \brief
 */
std::vector<char>& ReadPipe::GetOutput()
{
    if (_hThread)
        Wait(INFINITE);

    return _output;
}


/**
 *  \brief
 */
unsigned __stdcall ReadPipe::threadFunc(void* data)
{
    return static_cast<ReadPipe*>(data)->thread();
}


/**
 *  \brief
 */
unsigned ReadPipe::thread()
{
    DWORD bytesRead = 0;
    unsigned totalBytesRead = 0;
    unsigned chunkRemainingSize = 0;

    while (1)
    {
        if (!chunkRemainingSize)
        {
            _output.resize(totalBytesRead + cChunkSize);
            chunkRemainingSize = cChunkSize;
        }

        if (!ReadFile(_hOut, _output.data() + totalBytesRead, chunkRemainingSize, &bytesRead, NULL))
            break;

        chunkRemainingSize -= bytesRead;
        totalBytesRead += bytesRead;
    }

    _output.resize(totalBytesRead);
    if (totalBytesRead)
        _output.push_back(0);

    return 0;
}
