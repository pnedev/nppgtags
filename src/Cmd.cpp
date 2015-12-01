/**
 *  \file
 *  \brief  GTags command class
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2015 Pavel Nedev
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


#include "Cmd.h"


namespace GTags
{

/**
 *  \brief
 */
Cmd::Cmd(CmdId_t id, const TCHAR* name, DbHandle db, ParserPtr_t parser,
        const TCHAR* tag, bool regExp, bool matchCase) :
        _id(id), _db(db), _parser(parser),
        _regExp(regExp), _matchCase(matchCase), _skipLibs(false), _status(CANCELLED)
{
    if (db)
        _dbPath = *db;

    if (name)
        _name = name;

    if (tag)
        _tag = tag;
}

} // namespace GTags
