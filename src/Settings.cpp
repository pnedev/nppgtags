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


#define WIN32_LEAN_AND_MEAN
#include "Settings.h"
#include <fstream>
#include "INpp.h"
#include "Common.h"


namespace GTags
{

static const char Settings::cAutoUpdateKey[]  = "AutoUpdate";
static const char Settings::cLibraryPathKey[] = "LibraryPath";

Settings Settings::Instance;


using namespace std;


/**
 *  \brief
 */
bool Read(const TCHAR* configFile)
{
    if (configFile == NULL)
        return false;

    char cfgFileA[MAX_PATH];
    Tools::WtoA(cfgFileA, _countof(cfgFileA), configFile);

    fstream fs;
    fs.open(cfgFileA, fstream::in);
    if (!fs.is_open())
        return false;

    string line;
    while (getline(fs, line) != eofbit)
    {
        if (!readKey(line))
        {
            fs.close();
            return false;
        }
    }

    fs.close();

    return true;
}


/**
 *  \brief
 */
bool Store(const TCHAR* configFile) const
{
    return true;
}

} // namespace GTags
