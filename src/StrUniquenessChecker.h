/**
 *  \file
 *  \brief  String uniqueness checker class - checks if a string passes through the class for the first time.
 *          The strings are C-type char arrays that must remain intact for the whole lifetime of the class!!!
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


#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <map>


#ifdef UNICODE
#define StrUniquenessChecker    StrUniquenessCheckerW
#else
#define StrUniquenessChecker    StrUniquenessCheckerA
#endif


/**
 *  \class  StrUniquenessCheckerW
 *  \brief
 */
class StrUniquenessCheckerW
{
public:
    StrUniquenessCheckerW() {}
    ~StrUniquenessCheckerW() {}

    bool IsUnique(const wchar_t* str)
    {
        if (!str)
            return false;

        std::pair<register_t::iterator, bool> ret = _register.insert(elem_t(str, 0));

        return ret.second;
    }

private:
    StrUniquenessCheckerW(const StrUniquenessCheckerW&);
    const StrUniquenessCheckerW& operator=(const StrUniquenessCheckerW&);

    /**
     *  \class  key_t
     *  \brief
     */
    class key_t
    {
    public:
        key_t(const wchar_t* str) : _str(str) {}

        bool operator<(const key_t& rhs) const
        {
            return (wcscmp(_str, rhs._str) < 0);
        }

        bool operator==(const key_t& rhs) const
        {
            return (wcscmp(_str, rhs._str) == 0);
        }

        bool operator!=(const key_t& rhs) const
        {
            return !(operator==(rhs));
        }

    private:
        const wchar_t* _str;
    };

    typedef std::pair<key_t, char>  elem_t;
    typedef std::map<key_t, char>   register_t;

    register_t _register;
};


/**
 *  \class  StrUniquenessCheckerA
 *  \brief
 */
class StrUniquenessCheckerA
{
public:
    StrUniquenessCheckerA() {}
    ~StrUniquenessCheckerA() {}

    bool IsUnique(const char* str)
    {
        if (!str)
            return false;

        std::pair<register_t::iterator, bool> ret = _register.insert(elem_t(str, 0));

        return ret.second;
    }

private:
    StrUniquenessCheckerA(const StrUniquenessCheckerA&);
    const StrUniquenessCheckerA& operator=(const StrUniquenessCheckerA&);

    /**
     *  \class  key_t
     *  \brief
     */
    class key_t
    {
    public:
        key_t(const char* str) : _str(str) {}

        bool operator<(const key_t& rhs) const
        {
            return (strcmp(_str, rhs._str) < 0);
        }

        bool operator==(const key_t& rhs) const
        {
            return (strcmp(_str, rhs._str) == 0);
        }

        bool operator!=(const key_t& rhs) const
        {
            return !(operator==(rhs));
        }

    private:
        const char* _str;
    };

    typedef std::pair<key_t, char>  elem_t;
    typedef std::map<key_t, char>   register_t;

    register_t _register;
};
