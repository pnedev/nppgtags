/**
 *  \file
 *  \brief  GTags settings
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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>


namespace GTags
{

/**
 *  \class  Settings
 *  \brief
 */
class Settings
{
public:
    static Settings& Get() { return Instance; }

    bool Read(const TCHAR* configFile);
    bool Store(const TCHAR* configFile) const;

private:
    static Settings Instance;

    Settings();
    ~Settings();

    Settings(const Settings&);
    const Settings& operator=(const Settings&);
};

} // namespace GTags
