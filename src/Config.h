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
 *  \class  DbConfig
 *  \brief
 */
class DbConfig
{
public:
    enum
    {
        DEFAULT_PARSER = 0,
        CTAGS_PARSER,
        PYGMENTS_PARSER,
        PARSER_LIST_END
    };

    DbConfig();
    ~DbConfig() {}

    static bool IsOptionRecognized(const TCHAR* option);

    static const TCHAR* Parser(unsigned idx)
    {
        return (idx < PARSER_LIST_END) ? cParsers[idx] : NULL;
    }

    const TCHAR* Parser() const { return cParsers[_parserIdx]; }

    void SetDefaults();
    bool LoadFromFolder(const CPath& cfgFileFolder);
    bool SaveToFolder(const CPath& cfgFileFolder) const;

    void DbPathsFromBuf(TCHAR* buf, const TCHAR* separators);
    void DbPathsToBuf(CText& buf, TCHAR separator) const;

    const DbConfig& operator=(const DbConfig& cfg);
    bool operator==(const DbConfig& cfg) const;

    int                 _parserIdx;
    bool                _autoUpdate;
    bool                _useLibDb;
    std::vector<CPath>  _libDbPaths;

protected:
    bool Write(FILE* fp) const;

private:
    static const TCHAR cInfo[];

    static const TCHAR cParserKey[];
    static const TCHAR cAutoUpdateKey[];
    static const TCHAR cUseLibDbKey[];
    static const TCHAR cLibDbPathsKey[];

    static const TCHAR cDefaultParser[];
    static const TCHAR cCtagsParser[];
    static const TCHAR cPygmentsParser[];

    static const TCHAR* cParsers[PARSER_LIST_END];

    friend class Settings;
};


/**
 *  \class  Settings
 *  \brief
 */
class Settings
{
public:
    Settings();
    ~Settings() {}

    static bool IsOptionRecognized(const TCHAR* option);

    void SetDefaults();
    bool Load();
    bool Save() const;

    bool    _useDefDb;
    CPath   _defDbPath;
    bool    _re;
    bool    _mc;

    DbConfig    _genericDbCfg;

private:
    static const TCHAR cInfo[];

    static const TCHAR cUseDefDbKey[];
    static const TCHAR cDefDbPathKey[];
    static const TCHAR cREOptionKey[];
    static const TCHAR cMCOptionKey[];
};

} // namespace GTags
