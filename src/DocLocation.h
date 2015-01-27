/**
 *  \file
 *  \brief  Stores documents locations in a stack to be visited at later time
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
#include <tchar.h>
#include <vector>
#include "AutoLock.h"


/**
 *  \class  DocLocation
 *  \brief
 */
class DocLocation
{
public:
    static DocLocation& Get() { return Instance; }

    unsigned GetDepth() { return _maxDepth; }
    void SetDepth(unsigned depth);
    void Push();
    void Pop();

private:
    /**
     *  \struct  Location
     *  \brief
     */
    struct Location
    {
        TCHAR filePath[MAX_PATH];
        long firstVisibleLine;
        long posInFile;

        inline bool operator==(const Location& loc) const
        {
            return (posInFile == loc.posInFile &&
                    !_tcscmp(filePath, loc.filePath));
        }
    };

    static const unsigned cInitialDepth;
    static DocLocation Instance;

    DocLocation() : _maxDepth(cInitialDepth) {}
    DocLocation(const DocLocation&);
    ~DocLocation() {}

    unsigned _maxDepth;

    Mutex _lock;
    std::vector<Location> _locList;
};
