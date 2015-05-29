/**
 *  \file
 *  \brief  Stores documents locations in a stack to be visited at later time
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


#include "DocLocation.h"
#include "Common.h"
#include "INpp.h"


const unsigned DocLocation::cInitialDepth = 10;
DocLocation DocLocation::Instance;


/**
 *  \brief
 */
void DocLocation::SetDepth(unsigned depth)
{
    AUTOLOCK(_lock);

    if (_locList.capacity() < depth)
    {
        _locList.reserve(depth);
    }
    else
    {
        int diff = _locList.size() - depth;
        if (diff > 0)
        {
            _locList.erase(_locList.begin(), _locList.begin() + diff);
            _locList.resize(depth);
            _backLocIdx = _locList.size() - 1;
        }
    }

    _maxDepth = depth;
}


/**
 *  \brief
 */
void DocLocation::Push()
{
    AUTOLOCK(_lock);

    if (_backLocIdx + 1 < (int)_locList.size())
        _locList.erase(_locList.begin() + _backLocIdx + 1, _locList.end());
    else if (_locList.size() == _maxDepth)
        _locList.erase(_locList.begin());

    Location loc;
    INpp& npp = INpp::Get();
    npp.GetFilePath(loc._filePath);
    loc._posInFile = npp.GetPos();

    if (_locList.empty() || !(loc == _locList.back()))
        _locList.push_back(loc);

    _backLocIdx = _locList.size() - 1;
}


/**
 *  \brief
 */
void DocLocation::Back()
{
    AUTOLOCK(_lock);

    while (_backLocIdx >= 0)
    {
        Location& loc = _locList.at(_backLocIdx--);

        if (Tools::FileExists(loc._filePath))
        {
            swapView(loc);
            break;
        }
    }
}


/**
 *  \brief
 */
void DocLocation::Forward()
{
    AUTOLOCK(_lock);

    while (_backLocIdx + 1 <= (int)_locList.size() - 1)
    {
        Location& loc = _locList.at(++_backLocIdx);

        if (Tools::FileExists(loc._filePath))
        {
            swapView(loc);
            break;
        }
    }
}


/**
 *  \brief
 */
void DocLocation::swapView(Location& loc)
{
    Location newLoc;
    INpp& npp = INpp::Get();

    npp.GetFilePath(newLoc._filePath);
    newLoc._posInFile = npp.GetPos();

    npp.OpenFile(loc._filePath);
    npp.SetView(loc._posInFile);

    loc = newLoc;
}
