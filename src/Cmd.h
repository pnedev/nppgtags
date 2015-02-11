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
#include "Common.h"
#include "DBManager.h"
#include "GTags.h"


class ReadPipe;


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
    FIND_LITERAL,
    VERSION
};


/**
 *  \class  CmdData
 *  \brief
 */
class CmdData
{
public:
    inline CmdID_t GetID() const { return _id; }
    inline const TCHAR* GetName() const { return _name; }
    inline const TCHAR* GetDBPath() const { return _dbPath.C_str(); }
    inline const TCHAR* GetTag() const { return _tag; }
    inline unsigned GetTagLen() const { return _tcslen(_tag); }
    inline bool Error() const { return _error; }
    inline bool NoResult() const { return (_result == NULL); }
    inline char* GetResult() { return _result; }
    inline const char* GetResult() const { return _result; }
    inline unsigned GetResultLen() const { return _len; }

protected:
    CmdData(CmdID_t id, const TCHAR* name, const TCHAR* tag, DBhandle db,
            const char* result = NULL);
    ~CmdData();

    void SetResult(const char* result);
    void AppendResult(const char* result);

    CmdID_t _id;
    bool _error;

private:
    friend class Cmd;

    TCHAR _name[32];
    TCHAR _tag[cMaxTagLen];
    CPath _dbPath;
    char* _result;
    unsigned _len;
};


typedef void (*CompletionCB)(CmdData&);


/**
 *  \class  Cmd
 *  \brief
 */
class Cmd
{
public:
    static bool Run(CmdID_t id, const TCHAR* name, const TCHAR* tag,
            DBhandle db, CompletionCB complCB, const char* result = NULL);

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
    static const TCHAR cFindLiteralCmd[];
    static const TCHAR cVersionCmd[];

    static unsigned __stdcall threadFunc(void* data);

    CmdData _cmd;
    DBhandle _db;
    CompletionCB const _complCB;
    HANDLE _hThread;

    Cmd(CmdID_t id, const TCHAR* name, const TCHAR* tag, DBhandle db,
            CompletionCB complCB, const char* result = NULL) :
        _cmd(id, name, tag, db, result),
        _db(db), _complCB(complCB), _hThread(NULL) {}
    ~Cmd();

    unsigned thread();
    const TCHAR* getCmdLine();
    void composeCmd(TCHAR* cmd, unsigned len);
    bool runProcess();
    bool isActive(PROCESS_INFORMATION& pi);
    void terminate(PROCESS_INFORMATION& pi);
};

} // namespace GTags
