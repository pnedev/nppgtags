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

    virtual int Parse(const CmdPtr_t&) = 0;
    virtual const CTextA& GetText() const { return _buf; }
    virtual const std::vector<TCHAR*>& GetList() const { return _lines; }

protected:
    CTextA              _buf;
    std::vector<TCHAR*> _lines;
};


/**
 *  \class  Cmd
 *  \brief
 */
class Cmd
{
public:
    static const TCHAR* CmdName[];

    Cmd(CmdId_t id, DbHandle db = NULL, ParserPtr_t parser = ParserPtr_t(NULL),
            const TCHAR* tag = NULL, bool ignoreCase = false, bool regExp = false);
    ~Cmd() {}

    inline void Id(CmdId_t id) { _id = id; }
    inline CmdId_t Id() const { return _id; }

    inline const TCHAR* Name() const { return CmdName[_id]; }

    inline DbHandle Db() const { return _db; }

    inline void Tag(const CText& tag) { _tag = tag; }
    inline const CText& Tag() const { return _tag; }

    inline void Parser(const ParserPtr_t& parser) { _parser = parser; }
    inline const ParserPtr_t& Parser() const { return _parser; }

    inline void RegExp(bool re) { _regExp = re; }
    inline bool RegExp() const { return _regExp; }

    inline void IgnoreCase(bool ic) { _ignoreCase = ic; }
    inline bool IgnoreCase() const { return _ignoreCase; }

    inline void SkipLibs(bool skipLibs) { _skipLibs = skipLibs; }
    inline bool SkipLibs() const { return _skipLibs; }

    inline void Status(CmdStatus_t stat) { _status = stat; }
    inline CmdStatus_t Status() const { return _status; }

    inline char* Result() { return _result.data(); }
    inline const char* Result() const { return _result.data(); }
    inline size_t ResultLen() const { return _result.size() - 1; }

    void AppendToResult(const std::vector<char>& data);
    void SetResult(const std::vector<char>& data)
    {
        _result.assign(data.begin(), data.end());
    }

private:
    friend class CmdEngine;

    CmdId_t             _id;
    DbHandle            _db;

    CText               _tag;
    ParserPtr_t         _parser;
    bool                _ignoreCase;
    bool                _regExp;
    bool                _skipLibs;

    CmdStatus_t         _status;
    std::vector<char>   _result;
};

} // namespace GTags
