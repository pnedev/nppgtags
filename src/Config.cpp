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


#include <fstream>
#include <string>
#include "Common.h"
#include "INpp.h"
#include "Config.h"
#include "GTags.h"


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

const TCHAR CConfig::cParserKey[]       = _T("Parser = ");
const TCHAR CConfig::cAutoUpdateKey[]   = _T("AutoUpdate = ");
const TCHAR CConfig::cUseLibraryKey[]   = _T("UseLibrary = ");
const TCHAR CConfig::cLibraryPathKey[]  = _T("LibraryPath = ");


/**
 *  \brief
 */
void CConfig::GetDefaultCfgFile(CPath& cfgFile)
{
    TCHAR cfgDir[MAX_PATH];
    INpp::Get().GetPluginsConfDir(_countof(cfgDir), cfgDir);

    cfgFile = cfgDir;
    cfgFile += _T("\\");
    cfgFile += cPluginName;
    cfgFile += _T(".cfg");
}


/**
 *  \brief
 */
bool CConfig::LoadFromFile(const TCHAR* file)
{
    CPath cfgFile(file);
    if (file == NULL)
        GetDefaultCfgFile(cfgFile);

    std::tifstream ifs(cfgFile.C_str());
    if (!ifs.good())
        return false;

    std::tstring line;
    while (std::getline(ifs, line))
    {

    }

    return true;
}


/**
 *  \brief
 */
bool CConfig::SaveToFile(const TCHAR* file) const
{
    CPath cfgFile(file);
    if (file == NULL)
        GetDefaultCfgFile(cfgFile);

    std::tofstream ofs(cfgFile.C_str());
    if (!ofs.good())
        return false;

    ofs << cParserKey << Parser() << std::endl;
    ofs << cAutoUpdateKey << (_autoUpdate ? _T("yes") : _T("no")) << std::endl;
    ofs << cUseLibraryKey << (_useLibDb ? _T("yes") : _T("no")) << std::endl;
    ofs << cLibraryPathKey << _libDbPath.C_str() << std::endl;

    return ofs.good();
}

} // namespace GTags
