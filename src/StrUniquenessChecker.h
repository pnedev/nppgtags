/**
 *  \file
 *  \brief  String uniqueness checker class - checks if a string passes through the class for the first time.
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2016 Pavel Nedev
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


#include <string>
#include <map>
#include <functional>


/**
 *  \class  StrUniquenessChecker
 *  \brief
 */
template<typename CharType>
class StrUniquenessChecker
{
public:
    StrUniquenessChecker() {}
    ~StrUniquenessChecker() {}

    bool IsUnique(const CharType* ptr)
    {
        if (!ptr)
            return false;

        std::basic_string<CharType> str(ptr);
        std::hash<std::basic_string<CharType>> hash;

        std::pair<map_t::iterator, bool> ret = _map.insert(elem_t(hash(str), 0));

        return ret.second;
    }

private:
    StrUniquenessChecker(const StrUniquenessChecker&) = delete;
    const StrUniquenessChecker& operator=(const StrUniquenessChecker&) = delete;

    typedef std::pair<std::size_t, char>  elem_t;
    typedef std::map<std::size_t, char>   map_t;

    map_t _map;
};
