/**
 *  \file
 *  \brief  GTags config class
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


#pragma once


#include <windows.h>
#include <tchar.h>
#include <vector>
#include "Common.h"


namespace GTags
{

/**
 *  \class  GTagsConfig
 *  \brief
 */
class GTagsConfig
{
public:
    static const TCHAR cCfgFileName[];

    enum
    {
        DEFAULT_PARSER = 0,
        CTAGS_PARSER,
        PYGMENTS_PARSER,
        PARSER_LIST_END
    };

    GTagsConfig();
    ~GTagsConfig() {}

    static const TCHAR* Parser(unsigned idx)
    {
        return (idx < PARSER_LIST_END) ? cParsers[idx] : NULL;
    }

    const TCHAR* Parser() const { return cParsers[_parserIdx]; }

    void SetDefaults();
    bool LoadFromFolder(const CPath& cfgFileFolder, bool isGenericCfg = false);
    bool SaveToFolder(const CPath& cfgFileFolder, bool isGenericCfg = false) const;

    void DbPathsFromBuf(TCHAR* buf, const TCHAR* separators);
    void DbPathsToBuf(CText& buf, TCHAR separator) const;

    const GTagsConfig& operator=(const GTagsConfig& cfg);
    bool operator==(const GTagsConfig& cfg) const;

    int                 _parserIdx;
    bool                _autoUpdate;
    bool                _useLibDb;
    std::vector<CPath>  _libDbPaths;

    bool                _reCache;
    bool                _mcCache;

private:
    static const TCHAR cDefaultParser[];
    static const TCHAR cCtagsParser[];
    static const TCHAR cPygmentsParser[];

    static const TCHAR* cParsers[PARSER_LIST_END];

    static const TCHAR cInfo[];

    static const TCHAR cParserKey[];
    static const TCHAR cAutoUpdateKey[];
    static const TCHAR cUseLibraryKey[];
    static const TCHAR cLibraryPathKey[];
    static const TCHAR cRECacheKey[];
    static const TCHAR cMCCacheKey[];
};

} // namespace GTags
