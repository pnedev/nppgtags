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

    int diff = _locList.size() - depth;
    if (_locList.capacity() < depth)
    {
        _locList.reserve(depth);
    }
    else if (diff > 0)
    {
        _locList.erase(_locList.begin(), _locList.begin() + diff);
        _locList.resize(depth);
    }
    _maxDepth = depth;
}


/**
 *  \brief
 */
void DocLocation::Push()
{
    AUTOLOCK(_lock);

    if (_locList.size() == _maxDepth)
        _locList.erase(_locList.begin());

    INpp& npp = INpp::Get();
    Location loc;

    npp.GetFilePath(loc.filePath);
    npp.GetView(&loc.firstVisibleLine, &loc.posInFile);

    if (!_locList.empty() && (loc == _locList.back()))
        return;

    _locList.push_back(loc);
}


/**
 *  \brief
 */
void DocLocation::Pop()
{
    AUTOLOCK(_lock);

    if (_locList.empty())
        return;

    Location& loc = _locList.back();

    if (Tools::FileExists(loc.filePath))
    {
        INpp& npp = INpp::Get();
        npp.OpenFile(loc.filePath);
        npp.SetView(loc.firstVisibleLine, loc.posInFile);
    }

    _locList.pop_back();
}
