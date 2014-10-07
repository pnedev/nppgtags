/**
 *  \file
 *  \brief  Provides locking classes for abstraction and convenience
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


#pragma once


#define WIN32_LEAN_AND_MEAN
#include <windows.h>


#define AUTOLOCK(x)    AutoLock __lock_obj(x)


/**
 *  \class  Mutex
 *  \brief
 */
class Mutex
{
public:
    Mutex()
    {
        InitializeCriticalSectionAndSpinCount(&_lock, 1024);
    }

    ~Mutex()
    {
        DeleteCriticalSection(&_lock);
    }

    inline void Lock()
    {
        EnterCriticalSection(&_lock);
    }

    inline void Unlock()
    {
        LeaveCriticalSection(&_lock);
    }

private:
    CRITICAL_SECTION _lock;
};


/**
 *  \class  AutoLock
 *  \brief
 */
class AutoLock
{
public:
    AutoLock(Mutex& lock) : _lock(lock)
    {
        _lock.Lock();
    }

    ~AutoLock()
    {
        _lock.Unlock();
    }

private:
    Mutex& _lock;
};
