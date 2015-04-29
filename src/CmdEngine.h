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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <memory>
#include "Common.h"
#include "DBManager.h"
#include "GTags.h"


namespace GTags
{

enum CmdID_t
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


/**
 *  \class  Cmd
 *  \brief
 */
class Cmd
{
public:
    Cmd(CmdID_t id, const TCHAR* name, DBhandle db = NULL,
            const TCHAR* tag = NULL,
            bool regExp = false, bool matchCase = true);
    ~Cmd() {};

    inline void SetID(CmdID_t id) { _id = id; }
    inline CmdID_t GetID() const { return _id; }

    inline void SetName(const TCHAR* name)
    {
        if (name)
            _tcscpy_s(_name, _countof(_name), name);
    }
    inline const TCHAR* GetName() const { return _name; }

    inline DBhandle GetDBhandle() const { return _db; }
    inline const TCHAR* GetDBPath() const { return _dbPath.C_str(); }

    inline const TCHAR* GetTag() const { return &_tag; }
    inline unsigned GetTagLen() const { return _tag.Len(); }

    inline bool IsRegExp() const { return _regExp; }
    inline bool IsMatchCase() const { return _matchCase; }

    inline bool HasFailed() const { return _fail; }
    inline bool NoResult() const { return (&_result == NULL); }

    inline char* GetResult() { return &_result; }
    inline const char* GetResult() const { return &_result; }
    inline unsigned GetResultLen() const { return _result.Len(); }

protected:
    void SetResult(const char* result);
    void AppendResult(const char* result);

    CmdID_t _id;
    bool    _fail;

private:
    friend class CmdEngine;

    TCHAR       _name[32];
    CTcharArray _tag;
    const bool  _regExp;
    const bool  _matchCase;
    DBhandle    _db;
    const CPath _dbPath;
    CCharArray  _result;
};


typedef void (*CompletionCB)(std::shared_ptr<Cmd>&);


/**
 *  \class  CmdEngine
 *  \brief
 */
class CmdEngine
{
public:
    static bool Run(std::shared_ptr<Cmd>& cmd, CompletionCB complCB = NULL);

private:
    static const TCHAR cCreateDatabaseCmd[];
    static const TCHAR cUpdateSingleCmd[];
    static const TCHAR cAutoComplCmd[];
    static const TCHAR cAutoComplSymCmd[];
    static const TCHAR cAutoComplFileCmd[];
    static const TCHAR cFindFileCmd[];
    static const TCHAR cFindDefinitionCmd[];
    static const TCHAR cFindReferenceCmd[];
    static const TCHAR cFindSymbolCmd[];
    static const TCHAR cGrepCmd[];
    static const TCHAR cVersionCmd[];

    static unsigned __stdcall threadFunc(void* data);

    std::shared_ptr<Cmd>    _cmd;
    CompletionCB const      _complCB;
    HANDLE                  _hThread;

    CmdEngine(std::shared_ptr<Cmd>& cmd, CompletionCB complCB) :
        _cmd(cmd), _complCB(complCB), _hThread(NULL) {}
    ~CmdEngine();

    const TCHAR* getCmdLine();
    void composeCmd(TCHAR* buf, unsigned len);
    unsigned runProcess();
    bool isActive(PROCESS_INFORMATION& pi);
    void terminate(PROCESS_INFORMATION& pi);
};

} // namespace GTags
