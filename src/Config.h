/**
 *  \file
 *  \brief  GTags config class
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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include "Common.h"


namespace GTags
{

/**
 *  \class  CConfig
 *  \brief
 */
class CConfig
{
public:
    enum
    {
        DEFAULT_PARSER = 0,
        CTAGS_PARSER,
        PYGMENTS_PARSER,
        PARSER_LIST_END
    };

    CConfig(int parserIdx   = DEFAULT_PARSER,
            bool autoUpdate = true,
            bool useLibDb   = false) :
        _parserIdx(parserIdx), _autoUpdate(autoUpdate), _useLibDb(useLibDb) {}

    static const TCHAR* Parser(unsigned idx)
    {
        return (idx < PARSER_LIST_END) ? cParsers[idx] : NULL;
    }

    const TCHAR* Parser() { return cParsers[_parserIdx]; }

    int     _parserIdx;
    bool    _autoUpdate;
    bool    _useLibDb;
    CText   _libDbPath;

private:
    static const TCHAR cDefaultParser[];
    static const TCHAR cCtagsParser[];
    static const TCHAR cPygmentsParser[];

    static const TCHAR* cParsers[PARSER_LIST_END];
};

} // namespace GTags
