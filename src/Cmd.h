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


#pragma once


#include <windows.h>
#include <tchar.h>
#include <vector>
#include "Common.h"
#include "CmdDefines.h"
#include "DbManager.h"


namespace GTags
{

class CmdEngine;


/**
 *  \class  ResultParser
 *  \brief
 */
class ResultParser
{
public:
    virtual ~ResultParser() {}

    virtual bool Parse(const CmdPtr_t&) = 0;
};


/**
 *  \class  Cmd
 *  \brief
 */
class Cmd
{
public:
    Cmd(CmdId_t id, const TCHAR* name, DbHandle db = NULL, ParserPtr_t parser = ParserPtr_t(NULL),
            const TCHAR* tag = NULL, bool regExp = false, bool matchCase = true);
    ~Cmd() {}

    inline void Id(CmdId_t id) { _id = id; }
    inline CmdId_t Id() const { return _id; }

    inline void Name(const TCHAR* name) { if (name) _name = name; }
    inline const TCHAR* Name() const { return _name.C_str(); }

    inline DbHandle Db() const { return _db; }
    inline const CPath& DbPath() const { return _dbPath; }

    inline void Tag(const CText& tag) { _tag = tag; }
    inline const CText& Tag() const { return _tag; }

    inline void Parser(const ParserPtr_t& parser) { _parser = parser; }
    inline const ParserPtr_t& Parser() const { return _parser; }

    inline void RegExp(bool re) { _regExp = re; }
    inline bool RegExp() const { return _regExp; }

    inline void MatchCase(bool mc) { _matchCase = mc; }
    inline bool MatchCase() const { return _matchCase; }

    inline void SkipLibs(bool skipLibs) { _skipLibs = skipLibs; }
    inline bool SkipLibs() const { return _skipLibs; }

    inline void Status(CmdStatus_t stat) { _status = stat; }
    inline CmdStatus_t Status() const { return _status; }

    inline char* Result() { return _result.data(); }
    inline const char* Result() const { return _result.data(); }
    inline unsigned ResultLen() const { return _result.size() - 1; }

private:
    friend class CmdEngine;

    void setResult(const std::vector<char>& data)
    {
        _result.assign(data.begin(), data.end());
    }

    void appendResult(const std::vector<char>& data)
    {
        // remove \0 string termination
        if (!_result.empty())
            _result.pop_back();
        _result.insert(_result.cend(), data.begin(), data.end());
    }

    CmdId_t             _id;
    CText               _name;
    DbHandle const      _db;
    CPath               _dbPath;

    CText               _tag;
    ParserPtr_t         _parser;
    bool                _regExp;
    bool                _matchCase;
    bool                _skipLibs;

    CmdStatus_t         _status;
    std::vector<char>   _result;
};

} // namespace GTags
