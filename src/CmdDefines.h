/**
 *  \file
 *  \brief  GTags command forward declarations
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


#pragma once


#include <memory>


namespace GTags
{

enum CmdId_t
{
    CREATE_DATABASE = 0,
    UPDATE_SINGLE,
    AUTOCOMPLETE,
    AUTOCOMPLETE_SYMBOL,
    AUTOCOMPLETE_FILE,
    FIND_FILE,
    FIND_DEFINITION,
    FIND_REFERENCE,
    FIND_SYMBOL,
    GREP,
    GREP_TEXT,
    VERSION,
    CTAGS_VERSION
};


enum CmdStatus_t
{
    CANCELLED = 0,
    RUN_ERROR,
    FAILED,
    PARSE_ERROR,
    PARSE_EMPTY,
    OK
};


class Cmd;
class ResultParser;


typedef std::shared_ptr<Cmd> CmdPtr_t;
typedef std::shared_ptr<ResultParser> ParserPtr_t;
typedef void (*CompletionCB)(const CmdPtr_t&);

} // namespace GTags
