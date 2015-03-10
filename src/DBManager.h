/**
 *  \file
 *  \brief  GTags database manager
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
#include <tchar.h>
#include <list>
#include "Common.h"
#include "AutoLock.h"


namespace GTags
{

typedef const CPath* DBhandle;


/**
 *  \class  DBManager
 *  \brief
 */
class DBManager
{
public:
    static DBManager& Get() { return Instance; }

    DBhandle RegisterDB(const CPath& dbPath, bool writeEn);
    bool UnregisterDB(DBhandle db);
    DBhandle GetDB(const CPath& filePath, bool writeEn, bool* success);
    bool PutDB(DBhandle db);

private:
    /**
     *  \class  GTagsDB
     *  \brief
     */
    class GTagsDB
    {
    public:
        CPath _path;

        ~GTagsDB() {}

        bool IsLocked()
        {
            return (_writeLock || _readLocks);
        }

        bool Lock(bool writeEn)
        {
            if (writeEn)
            {
                if (_writeLock || _readLocks)
                    return false;
                _writeLock = true;
            }
            else
            {
                if (_writeLock)
                    return false;
                _readLocks++;
            }
            return true;
        }

        void Unlock()
        {
            if (_writeLock)
                _writeLock = false;
            else if (_readLocks > 0)
                _readLocks--;
        }

    protected:
        GTagsDB(const CPath& dbPath, bool writeEn) :
            _path(dbPath), _writeLock(writeEn)
        {
            _readLocks = writeEn ? 0 : 1;
        }

    private:
        friend class DBManager;

        int _readLocks;
        bool _writeLock;
    };

    static DBManager Instance;

    DBManager() {}
    DBManager(const DBManager&);
    ~DBManager() {}

    bool dbExistsInFolder(const CPath& folder);
    bool deleteDB(CPath& dbPath);
    DBhandle addDB(const CPath& dbPath, bool writeEn);
    DBhandle lockDB(const CPath& filePath, bool writeEn, bool* success);

    Mutex _lock;
    std::list<GTagsDB> _dbList;
};

} // namespace GTags
