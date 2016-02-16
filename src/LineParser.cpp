/**
 *  \file
 *  \brief  Line parser (splitter) class
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2015-2016 Pavel Nedev
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


#include "LineParser.h"
#include "StrUniquenessChecker.h"


namespace GTags
{

/**
 *  \brief
 */
bool LineParser::Parse(const CmdPtr_t& cmd)
{
    const bool filterReoccurring = cmd->Db()->GetConfig()._useLibDb;

    StrUniquenessChecker strChecker;

    _lines.clear();
    _buf = cmd->Result();

    TCHAR* pTmp = NULL;
    for (TCHAR* pToken = _tcstok_s(_buf.C_str(), _T("\n\r"), &pTmp); pToken;
            pToken = _tcstok_s(NULL, _T("\n\r"), &pTmp))
    {
        if (cmd->Id() == FIND_FILE || cmd->Id() == AUTOCOMPLETE_FILE)
            ++pToken;

        if ((!filterReoccurring) || strChecker.IsUnique(pToken))
            _lines.push_back(pToken);
    }

    return true;
}

} // namespace GTags
