/**
 *  \file
 *  \brief  GTags command class
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


#include "Cmd.h"


namespace GTags
{

const TCHAR* Cmd::CmdName[] = {
    _T("Create Database"),              // CREATE_DATABASE
    _T("Database Single File Update"),  // UPDATE_SINGLE
    _T("AutoComplete"),                 // AUTOCOMPLETE
    _T("AutoComplete"),                 // AUTOCOMPLETE_SYMBOL
    _T("AutoComplete File Name"),       // AUTOCOMPLETE_FILE
    _T("Find File"),                    // FIND_FILE
    _T("Find Definition"),              // FIND_DEFINITION
    _T("Find Reference"),               // FIND_REFERENCE
    _T("Find Symbol"),                  // FIND_SYMBOL
    _T("Search in Source Files"),       // GREP
    _T("Search in Other Files"),        // GREP_TEXT
    _T("About"),                        // VERSION
    _T("About CTags")                   // CTAGS_VERSION
};


/**
 *  \brief
 */
Cmd::Cmd(CmdId_t id, DbHandle db, ParserPtr_t parser,
        const TCHAR* tag, bool ignoreCase, bool regExp, bool autorun) :
        _id(id), _db(db), _parser(parser),
        _ignoreCase(ignoreCase), _regExp(regExp), _autorun(autorun), _skipLibs(false), _status(CANCELLED)
{
    if (tag)
        _tag = tag;
}


void Cmd::AppendToResult(const std::vector<char>& data)
{
    // remove \0 string termination
    if (!_result.empty())
        _result.pop_back();
    _result.insert(_result.cend(), data.begin(), data.end());
}

} // namespace GTags
