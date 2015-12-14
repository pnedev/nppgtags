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


#include <windows.h>
#include "DbManager.h"
#include "GTags.h"


namespace GTags
{

DbManager DbManager::Instance;


/**
 *  \brief
 */
void GTagsDb::ScheduleUpdate(const CPath& file)
{
    std::list<CPath>::reverse_iterator iFile;
    for (iFile = _updateList.rbegin(); iFile != _updateList.rend(); ++iFile)
        if (*iFile == file)
            return;

    _updateList.push_back(file);
}


/**
 *  \brief
 */
void GTagsDb::runScheduledUpdate()
{
    if (_updateList.empty())
        return;

    CPath file = *(_updateList.begin());
    _updateList.erase(_updateList.begin());

    UpdateSingleFile(file);
}


/**
 *  \brief
 */
const DbHandle& DbManager::RegisterDb(const CPath& dbPath)
{
    bool success;

    return lockDb(dbPath, true, &success);
}


/**
 *  \brief
 */
bool DbManager::UnregisterDb(const DbHandle& db)
{
    if (!db)
        return false;

    bool ret = false;

    for (std::list<DbHandle>::iterator dbi = _dbList.begin(); dbi != _dbList.end(); ++dbi)
    {
        if (db == *dbi)
        {
            db->unlock();
            if (!db->isLocked())
            {
                ret = deleteDb(db->_path);
                _dbList.erase(dbi);
            }
            break;
        }
    }

    return ret;
}


/**
 *  \brief
 */
DbHandle DbManager::GetDb(const CPath& filePath, bool writeEn, bool* success)
{
    if (!success)
        return NULL;

    *success = false;

    CPath dbPath(filePath);
    int len = dbPath.StripFilename();

    for (; len; len = dbPath.DirUp())
        if (DbExistsInFolder(dbPath))
            break;

    if (len == 0)
        return NULL;

    return lockDb(dbPath, writeEn, success);
}


/**
 *  \brief
 */
void DbManager::PutDb(const DbHandle& db)
{
    if (!db)
        return;

    for (std::list<DbHandle>::iterator dbi = _dbList.begin(); dbi != _dbList.end(); ++dbi)
    {
        if (db == *dbi)
        {
            db->unlock();

            if (!db->isLocked())
                db->runScheduledUpdate();

            break;
        }
    }
}


/**
 *  \brief
 */
bool DbManager::DbExistsInFolder(const CPath& folder)
{
    CPath db(folder);
    db += _T("GTAGS");
    return db.FileExists();
}


/**
 *  \brief
 */
bool DbManager::deleteDb(CPath& dbPath)
{
    dbPath += _T("GTAGS");
    BOOL ret = DeleteFile(dbPath.C_str());
    if (ret)
    {
        dbPath.StripFilename();
        dbPath += _T("GPATH");
        ret = DeleteFile(dbPath.C_str());
    }
    if (ret)
    {
        dbPath.StripFilename();
        dbPath += _T("GRTAGS");
        ret = DeleteFile(dbPath.C_str());
    }

    return ret ? true : false;
}


/**
 *  \brief
 */
const DbHandle& DbManager::lockDb(const CPath& dbPath, bool writeEn, bool* success)
{
    for (std::list<DbHandle>::iterator dbi = _dbList.begin(); dbi != _dbList.end(); ++dbi)
    {
        if ((*dbi)->_path == dbPath)
        {
            *success = (*dbi)->lock(writeEn);
            return *dbi;
        }
    }

    DbHandle newDb(new GTagsDb(dbPath, writeEn));
    _dbList.push_back(newDb);

    *success = true;

    return *(_dbList.rbegin());
}

} // namespace GTags
