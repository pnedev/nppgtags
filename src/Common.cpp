/**
 *  \file
 *  \brief  Various helper types and functions
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


#include "Common.h"


/**
 *  \brief
 */
CTextW::CTextW(const char* str)
{
    if (str)
    {
        _buf.resize(strlen(str) + 1);
        size_t cnt;
        mbstowcs_s(&cnt, _buf.data(), _buf.size(), str, _TRUNCATE);
    }
    else
    {
        _buf.push_back('\0');
    }
}


/**
 *  \brief
 */
const CTextW& CTextW::operator=(const char* str)
{
    if (str)
    {
        _buf.resize(strlen(str) + 1);
        size_t cnt;
        mbstowcs_s(&cnt, _buf.data(), _buf.size(), str, _TRUNCATE);
    }

    return *this;
}


/**
 *  \brief
 */
void CTextW::operator+=(const wchar_t* str)
{
    if (str)
    {
        unsigned len = wcslen(str);
        if (len)
        {
            _buf.pop_back();
            _buf.insert(_buf.cend(), str, str + len + 1);
        }
    }
}


/**
 *  \brief
 */
void CTextW::operator+=(const char* str)
{
    if (str)
    {
        unsigned len = strlen(str);
        if (len)
        {
            _buf.pop_back();

            unsigned size = _buf.size();
            _buf.resize(size + len + 1);

            size_t cnt;
            mbstowcs_s(&cnt, &_buf[size], len + 1, str, _TRUNCATE);
        }
    }
}


/**
 *  \brief
 */
CTextA::CTextA(const wchar_t* str)
{
    if (str)
    {
        _buf.resize(wcslen(str) + 1);
        size_t cnt;
        wcstombs_s(&cnt, _buf.data(), _buf.size(), str, _TRUNCATE);
    }
    else
    {
        _buf.push_back('\0');
    }
}


/**
 *  \brief
 */
const CTextA& CTextA::operator=(const wchar_t* str)
{
    if (str)
    {
        _buf.resize(wcslen(str) + 1);
        size_t cnt;
        wcstombs_s(&cnt, _buf.data(), _buf.size(), str, _TRUNCATE);
    }

    return *this;
}


/**
 *  \brief
 */
void CTextA::operator+=(const char* str)
{
    if (str)
    {
        unsigned len = strlen(str);
        if (len)
        {
            _buf.pop_back();
            _buf.insert(_buf.cend(), str, str + len + 1);
        }
    }
}


/**
 *  \brief
 */
void CTextA::operator+=(const wchar_t* str)
{
    if (str)
    {
        unsigned len = wcslen(str);
        if (len)
        {
            _buf.pop_back();

            unsigned size = _buf.size();
            _buf.resize(size + len + 1);

            size_t cnt;
            wcstombs_s(&cnt, &_buf[size], len + 1, str, _TRUNCATE);
        }
    }
}


/**
 *  \brief
 */
unsigned CPath::StripFilename()
{
    unsigned len = Len();

    for (; len > 0; len--)
        if (_buf[len] == _T('\\') || _buf[len] == _T('/'))
        {
            len++;
            break;
        }

    _buf.erase(_buf.begin() + len, _buf.end());
    _buf.push_back(_T('\0'));

    return len;
}


/**
 *  \brief
 */
const TCHAR* CPath::GetFilename() const
{
    unsigned len = Len();

    for (; len > 0; len--)
        if (_buf[len] == _T('\\') || _buf[len] == _T('/'))
        {
            len++;
            break;
        }

    return &_buf[len];
}


/**
 *  \brief
 */
unsigned CPath::Up()
{
    unsigned len = Len();
    if (_buf[len] == _T('\\') || _buf[len] == _T('/'))
        len--;

    for (; len > 0; len--)
        if (_buf[len] == _T('\\') || _buf[len] == _T('/'))
        {
            len++;
            break;
        }

    _buf.erase(_buf.begin() + len, _buf.end());
    _buf.push_back(_T('\0'));

    return len;
}


/**
 *  \brief
 */
bool CPath::Contains(const TCHAR* pathStr) const
{
    unsigned len = Len();
    if (len >= _tcslen(pathStr))
        return false;

    return !_tcsncmp(_buf.data(), pathStr, len);
}


/**
 *  \brief
 */
bool CPath::Contains(const CPath& path) const
{
    unsigned len = Len();
    if (len >= path.Len())
        return false;

    return !_tcsncmp(_buf.data(), path._buf.data(), len);
}


/**
 *  \brief
 */
bool CPath::IsContainedIn(const TCHAR* pathStr) const
{
    unsigned len = _tcslen(pathStr);
    if (len >= Len())
        return false;

    return !_tcsncmp(_buf.data(), pathStr, len);
}


/**
 *  \brief
 */
bool CPath::IsContainedIn(const CPath& path) const
{
    unsigned len = path.Len();
    if (len >= Len())
        return false;

    return !_tcsncmp(_buf.data(), path._buf.data(), len);
}


namespace Tools
{

/**
 *  \brief
 */
void ReleaseKey(WORD virtKey, bool onlyIfPressed)
{
    if (!onlyIfPressed || GetKeyState(virtKey))
    {
        INPUT input          = {0};
        input.type           = INPUT_KEYBOARD;
        input.ki.wVk         = virtKey;
        input.ki.dwFlags     = KEYEVENTF_KEYUP;
        input.ki.dwExtraInfo = GetMessageExtraInfo();

        SendInput(1, &input, sizeof(input));
    }
}

} // namespace Tools
