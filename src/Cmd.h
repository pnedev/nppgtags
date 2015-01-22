/**
 *  \file
 *  \brief  GTags command execution and result classes
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014 Pavel Nedev
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
    inline const TCHAR* GetTag() const { return _tag.C_str(); }
    inline unsigned GetTagLen() const { return _tag.Len(); }
    inline bool Error() const { return _error; }
    inline bool NoResult() const { return (_result == NULL); }
    inline char* GetResult() { return _result; }
    inline const char* GetResult() const { return _result; }
    inline unsigned GetResultLen() const { return _len; }

protected:
    CmdData(CmdID_t id, const TCHAR* name, DBhandle db,
            const TCHAR* tag, const char* result = NULL) :
        _id(id), _error(false), _result(NULL), _len(0), _tag(tag)
    {
        if (db)
            _dbPath = *db;
        _name[0] = 0;
        if (name)
            _tcscpy_s(_name, _countof(_name), name);
        if (result)
        {
            _len = strlen(result);
            _result = new char[_len + 1];
            strcpy_s(_result, _len + 1, result);
        }
    }

    ~CmdData()
    {
        if (_result)
            delete [] _result;
    }

    void SetResult(const char* result)
    {
        if (result == NULL)
            return;

        if (_result)
            delete [] _result;

        _len = strlen(result);
        _result = new char[_len + 1];
        strcpy_s(_result, _len + 1, result);
    }

    void AppendResult(const char* result)
    {
        if (result == NULL)
            return;

        char* oldResult = _result;
        unsigned oldLen = _len;

        _len += strlen(result);
        _result = new char[_len + 1];

        if (oldResult)
        {
            strcpy_s(_result, oldLen + 1, oldResult);
            delete [] oldResult;
        }

        strcpy_s(_result + oldLen, _len + 1 - oldLen, result);
    }

    CmdID_t _id;
    bool _error;
    char* _result;
    unsigned _len;

private:
    friend class Cmd;

    TCHAR _name[32];
    CPath _dbPath;
    const CText _tag;
};


typedef void (*CompletionCB)(CmdData&);


/**
 *  \class  Cmd
 *  \brief
 */
class Cmd
{
public:
    static bool Run(CmdID_t id, const TCHAR* name, DBhandle db,
            const TCHAR* tag, CompletionCB complCB,
            const char* result = NULL);

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

    Cmd(CmdID_t id, const TCHAR* name, DBhandle db, const TCHAR* tag,
            CompletionCB complCB, const char* result = NULL) :
        _cmd(id, name, db, tag, result),
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
