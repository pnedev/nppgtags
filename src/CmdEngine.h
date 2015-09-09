/**
 *  \file
 *  \brief  GTags command execution and result classes
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2015 Pavel Nedev
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
#include <memory>
#include <vector>
#include "tstring.h"
#include "Common.h"
#include "DbManager.h"


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
    VERSION
};


enum CmdStatus_t
{
    CANCELLED = 0,
    RUN_ERROR,
    FAILED,
    OK
};


/**
 *  \class  Cmd
 *  \brief
 */
class Cmd
{
public:
    Cmd(CmdId_t id, const TCHAR* name, DbHandle db = NULL, const TCHAR* tag = NULL,
            bool regExp = false, bool matchCase = true);
    ~Cmd() {};

    inline void Id(CmdId_t id) { _id = id; }
    inline CmdId_t Id() const { return _id; }

    inline void Name(const TCHAR* name) { if (name) _name = name; }
    inline const TCHAR* Name() const { return _name.c_str(); }

    inline DbHandle Db() const { return _db; }
    inline const TCHAR* DbPath() const { return _dbPath.C_str(); }

    inline void Tag(const TCHAR* tag) { if (tag) _tag = tag; }
    inline const TCHAR* Tag() const { return _tag.c_str(); }
    inline unsigned TagLen() const { return _tag.length(); }

    inline void RegExp(bool re) { _regExp = re; }
    inline bool RegExp() const { return _regExp; }

    inline void MatchCase(bool mc) { _matchCase = mc; }
    inline bool MatchCase() const { return _matchCase; }

    inline void Status(CmdStatus_t stat) { _status = stat; }
    inline CmdStatus_t Status() const { return _status; }

    inline char* Result() { return _result.data(); }
    inline const char* Result() const { return _result.data(); }
    inline unsigned ResultLen() const { return _result.size() - 1; }

private:
    friend class CmdEngine;

    void setResult(const std::vector<char>& result)
    {
        _result.assign(result.cbegin(), result.cend());
    }

    void appendResult(const std::vector<char>& result)
    {
        _result.insert(_result.cend(), result.cbegin(), result.cend());
    }

    CmdId_t             _id;
    tstring             _name;
    DbHandle const      _db;
    CPath               _dbPath;

    tstring             _tag;
    bool                _regExp;
    bool                _matchCase;

    CmdStatus_t         _status;
    std::vector<char>   _result;
};


typedef void (*CompletionCB)(const std::shared_ptr<Cmd>&);


/**
 *  \class  CmdEngine
 *  \brief
 */
class CmdEngine
{
public:
    static bool Run(const std::shared_ptr<Cmd>& cmd,
            CompletionCB complCB = NULL);

private:
    static const TCHAR  cCreateDatabaseCmd[];
    static const TCHAR  cUpdateSingleCmd[];
    static const TCHAR  cAutoComplCmd[];
    static const TCHAR  cAutoComplSymCmd[];
    static const TCHAR  cAutoComplFileCmd[];
    static const TCHAR  cFindFileCmd[];
    static const TCHAR  cFindDefinitionCmd[];
    static const TCHAR  cFindReferenceCmd[];
    static const TCHAR  cFindSymbolCmd[];
    static const TCHAR  cGrepCmd[];
    static const TCHAR  cVersionCmd[];

    static unsigned __stdcall threadFunc(void* data);

    CmdEngine(const std::shared_ptr<Cmd>& cmd, CompletionCB complCB) : _cmd(cmd), _complCB(complCB), _hThread(NULL) {}
    ~CmdEngine();

    const TCHAR* getCmdLine() const;
    void composeCmd(TCHAR* buf, unsigned len) const;
    unsigned runProcess();
    void endProcess(PROCESS_INFORMATION& pi);

    std::shared_ptr<Cmd>    _cmd;
    CompletionCB const      _complCB;
    HANDLE                  _hThread;
};

} // namespace GTags
