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


#include <tchar.h>
#include <list>
#include <memory>
#include "Common.h"
#include "DbConfig.h"
#include "CmdDefines.h"
#include "GTags.h"


namespace GTags
{

/**
 *  \class  GTagsDb
 *  \brief
 */
class GTagsDb : public std::enable_shared_from_this<GTagsDb>
{
public:
    ~GTagsDb() {}

    inline const CPath& GetPath() const { return _path; }

    inline const DbConfigPtr_t& GetConfig() const { return _cfg; }
    inline void SetConfig(const DbConfigPtr_t& cfg) { _cfg = cfg; }

    void Update(const CPath& file);
    void ScheduleUpdate(const CPath& file);

private:
    friend class DbManager;

    GTagsDb(const CPath& dbPath, bool writeEn);

    static void dbUpdateCB(const CmdPtr_t& cmd);

    bool lock(bool writeEn);
    bool unlock();

    void runScheduledUpdate();

    CPath           _path;
    DbConfigPtr_t   _cfg;

    int     _readLocks;
    bool    _writeLock;

    std::list<CPath>    _updateList;
};


typedef std::shared_ptr<GTagsDb> DbHandle;


/**
 *  \class  DbManager
 *  \brief
 */
class DbManager
{
public:
    static DbManager& Get() { return Instance; }

    const DbHandle& RegisterDb(const CPath& dbPath);
    bool UnregisterDb(const DbHandle& db);
    DbHandle GetDb(const CPath& filePath, bool writeEn, bool* success);
    void PutDb(const DbHandle& db);
    bool DbExistsInFolder(const CPath& folder);

private:
    static DbManager Instance;

    DbManager() {}
    DbManager(const DbManager&);
    ~DbManager() {}

    bool deleteDb(CPath& dbPath);
    const DbHandle& lockDb(const CPath& dbPath, bool writeEn, bool* success);

    std::list<DbHandle> _dbList;
};

} // namespace GTags
