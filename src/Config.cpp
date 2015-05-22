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


#include "Config.h"


namespace GTags
{

const TCHAR CConfig::cDefaultParser[]   = _T("default");
const TCHAR CConfig::cCtagsParser[]     = _T("ctags");
const TCHAR CConfig::cPygmentsParser[]  = _T("pygments");

const TCHAR* CConfig::cParsers[CConfig::PARSER_LIST_END] = {
    CConfig::cDefaultParser,
    CConfig::cCtagsParser,
    CConfig::cPygmentsParser
};

} // namespace GTags
