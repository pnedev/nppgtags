/**
 *  \file
 *  \brief  Line parser (splitter) class
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2015-2022 Pavel Nedev
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
#include <cstdint>
#include <vector>
#include <memory>
#include "Common.h"
#include "Cmd.h"


namespace GTags
{

/**
 *  \class  LineParser
 *  \brief
 */
class LineParser : public ResultParser
{
public:
    LineParser() {}
    virtual ~LineParser() {}

    virtual intptr_t Parse(const CmdPtr_t&);

private:
    CText _buf;
};

} // namespace GTags
