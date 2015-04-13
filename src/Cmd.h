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
 *  \class  CmdData
 *  \brief
 */
class CmdData
{
public:
    CmdData(CmdID_t id, const TCHAR* name, DBhandle db = NULL,
            const TCHAR* tag = NULL,
            bool regExp = false, bool matchCase = true);
    ~CmdData();

    inline void SetID(CmdID_t id) { _id = id; }
    inline CmdID_t GetID() const { return _id; }

    inline void SetName(const TCHAR* name)
    {
        if (name)
            _tcscpy_s(_name, _countof(_name), name);
    }
    inline const TCHAR* GetName() const { return _name; }

    inline void SetDB(DBhandle db)
    {
        if (db)
            _dbPath = *db;
    }
    inline const TCHAR* GetDBPath() const { return _dbPath.C_str(); }

    inline const TCHAR* GetTag() const { return _tag; }
    inline unsigned GetTagLen() const { return _tcslen(_tag); }

    inline bool IsRegExp() const { return _regExp; }
    inline bool IsMatchCase() const { return _matchCase; }

    inline bool Error() const { return _error; }
    inline bool NoResult() const { return (_result == NULL); }

    inline char* GetResult() { return _result; }
    inline const char* GetResult() const { return _result; }
    inline unsigned GetResultLen() const { return _len; }

protected:
    void SetResult(const char* result);
    void AppendResult(const char* result);

    CmdID_t _id;
    bool _error;

private:
    friend class Cmd;

    TCHAR _name[32];
    TCHAR* _tag;
    bool _regExp;
    bool _matchCase;
    CPath _dbPath;
    char* _result;
    unsigned _len;
};


typedef void (*CompletionCB)(std::shared_ptr<CmdData>&);


/**
 *  \class  Cmd
 *  \brief
 */
class Cmd
{
public:
    static bool Run(std::shared_ptr<CmdData>& cmdData,
            CompletionCB complCB, DBhandle db = NULL);

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

    std::shared_ptr<CmdData> _cmd;
    CompletionCB const _complCB;
    DBhandle _db;
    HANDLE _hThread;

    Cmd(std::shared_ptr<CmdData>& cmdData, CompletionCB complCB,
            DBhandle db = NULL) :
        _cmd(cmdData), _complCB(complCB), _db(db), _hThread(NULL) {}
    ~Cmd();

    unsigned thread();
    const TCHAR* getCmdLine();
    void composeCmd(TCHAR* cmd, unsigned len);
    bool runProcess();
    bool isActive(PROCESS_INFORMATION& pi);
    void terminate(PROCESS_INFORMATION& pi);
};

} // namespace GTags
