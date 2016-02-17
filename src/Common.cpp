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
#include <shlobj.h>


namespace
{

/**
 *  \brief
 */
int CALLBACK browseFolderCB(HWND hWnd, UINT uMsg, LPARAM, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED)
        SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
    return 0;
}

} // anonymous namespace


/**
 *  \brief
 */
bool Tools::BrowseForFolder(HWND hOwnerWin, CPath& path)
{
    TCHAR tmp[MAX_PATH];

    BROWSEINFO bi       = {0};
    bi.hwndOwner        = hOwnerWin;
    bi.pszDisplayName   = tmp;
    bi.lpszTitle        = _T("Select the database root (indexed recursively)");
    bi.ulFlags          = BIF_RETURNONLYFSDIRS;
    bi.lpfn             = browseFolderCB;

    if (!path.IsEmpty() && path.Exists())
        bi.lParam = (DWORD)path.C_str();

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (!pidl)
        return false;

    SHGetPathFromIDList(pidl, tmp);

    IMalloc* imalloc = NULL;
    if (SUCCEEDED(SHGetMalloc(&imalloc)))
    {
        imalloc->Free(pidl);
        imalloc->Release();
    }

    path = tmp;
    path += _T("\\");

    return true;
}


/**
 *  \brief
 */
CTextW::CTextW(const wchar_t* str) : _invalidStrLen(false)
{
    if (str)
        _buf.assign(str, str + wcslen(str) + 1);
    else
        _buf.push_back(L'\0');
}


/**
 *  \brief
 */
CTextW::CTextW(const char* str) : _invalidStrLen(false)
{
    if (str)
    {
        _buf.resize(strlen(str) + 1, L'\0');
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
const CTextW& CTextW::operator=(const CTextW& txt)
{
    if (this != &txt)
    {
        _buf = txt._buf;
        _invalidStrLen = txt._invalidStrLen;
    }
    return *this;
}


/**
 *  \brief
 */
const CTextW& CTextW::operator=(const wchar_t* str)
{
    if (str)
    {
        _buf.assign(str, str + wcslen(str) + 1);
        _invalidStrLen = false;
    }
    return *this;
}


/**
 *  \brief
 */
const CTextW& CTextW::operator=(const char* str)
{
    if (str)
    {
        _buf.resize(strlen(str) + 1, L'\0');
        size_t cnt;
        mbstowcs_s(&cnt, _buf.data(), _buf.size(), str, _TRUNCATE);
        _invalidStrLen = false;
    }

    return *this;
}


/**
 *  \brief
 */
void CTextW::operator+=(const CTextW& txt)
{
    AutoFit();

    _buf.pop_back();
    _buf.insert(_buf.cend(), txt._buf.begin(), txt._buf.begin() + txt.Len() + 1);
}


/**
 *  \brief
 */
void CTextW::operator+=(const wchar_t* str)
{
    AutoFit();

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
    AutoFit();

    if (str)
    {
        unsigned len = strlen(str);
        if (len)
        {
            _buf.pop_back();

            unsigned size = _buf.size();
            _buf.resize(size + len + 1, L'\0');

            size_t cnt;
            mbstowcs_s(&cnt, &_buf[size], len + 1, str, _TRUNCATE);
        }
    }
}


/**
 *  \brief
 */
void CTextW::operator+=(wchar_t letter)
{
    AutoFit();

    _buf.pop_back();
    _buf.push_back(letter);
    _buf.push_back(L'\0');
}


/**
 *  \brief
 */
void CTextW::Append(const wchar_t* data, unsigned len)
{
    AutoFit();

    if (data && len)
    {
        _buf.pop_back();
        _buf.insert(_buf.cend(), data, data + len);
        _buf.push_back(L'\0');
    }
}


/**
 *  \brief
 */
void CTextW::Insert(unsigned at_pos, wchar_t letter)
{
    AutoFit();

    if (at_pos < _buf.size())
        _buf.insert(_buf.cbegin() + at_pos, letter);
}


/**
 *  \brief
 */
void CTextW::Insert(unsigned at_pos, const wchar_t* data, unsigned len)
{
    AutoFit();

    if ((at_pos < _buf.size()) && data && len)
        _buf.insert(_buf.cbegin() + at_pos, data, data + len);
}


/**
 *  \brief
 */
void CTextW::Clear()
{
    _buf.clear();
    _buf.push_back(L'\0');
    _invalidStrLen = false;
}


/**
 *  \brief
 */
void CTextW::Resize(unsigned size)
{
    unsigned len = Len();

    _buf.resize(size);
    _buf.push_back(L'\0');
    _invalidStrLen = _invalidStrLen || (size > len);
}


/**
 *  \brief
 */
CTextA::CTextA(const char* str) : _invalidStrLen(false)
{
    if (str)
        _buf.assign(str, str + strlen(str) + 1);
    else
        _buf.push_back('\0');
}


/**
 *  \brief
 */
CTextA::CTextA(const wchar_t* str) : _invalidStrLen(false)
{
    if (str)
    {
        _buf.resize(wcslen(str) + 1, '\0');
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
const CTextA& CTextA::operator=(const CTextA& txt)
{
    if (this != &txt)
    {
        _buf = txt._buf;
        _invalidStrLen = txt._invalidStrLen;
    }
    return *this;
}


/**
 *  \brief
 */
const CTextA& CTextA::operator=(const char* str)
{
    if (str)
    {
        _buf.assign(str, str + strlen(str) + 1);
        _invalidStrLen = false;
    }
    return *this;
}


/**
 *  \brief
 */
const CTextA& CTextA::operator=(const wchar_t* str)
{
    if (str)
    {
        _buf.resize(wcslen(str) + 1, '\0');
        size_t cnt;
        wcstombs_s(&cnt, _buf.data(), _buf.size(), str, _TRUNCATE);
        _invalidStrLen = false;
    }

    return *this;
}


/**
 *  \brief
 */
void CTextA::operator+=(const CTextA& txt)
{
    AutoFit();

    _buf.pop_back();
    _buf.insert(_buf.cend(), txt._buf.begin(), txt._buf.begin() + txt.Len() + 1);
}


/**
 *  \brief
 */
void CTextA::operator+=(const char* str)
{
    AutoFit();

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
    AutoFit();

    if (str)
    {
        unsigned len = wcslen(str);
        if (len)
        {
            _buf.pop_back();

            unsigned size = _buf.size();
            _buf.resize(size + len + 1, '\0');

            size_t cnt;
            wcstombs_s(&cnt, &_buf[size], len + 1, str, _TRUNCATE);
        }
    }
}


/**
 *  \brief
 */
void CTextA::operator+=(char letter)
{
    AutoFit();

    _buf.pop_back();
    _buf.push_back(letter);
    _buf.push_back('\0');
}


/**
 *  \brief
 */
void CTextA::Append(const char* data, unsigned len)
{
    AutoFit();

    if (data && len)
    {
        _buf.pop_back();
        _buf.insert(_buf.cend(), data, data + len);
        _buf.push_back('\0');
    }
}


/**
 *  \brief
 */
void CTextA::Insert(unsigned at_pos, char letter)
{
    AutoFit();

    if (at_pos < _buf.size())
        _buf.insert(_buf.cbegin() + at_pos, letter);
}


/**
 *  \brief
 */
void CTextA::Insert(unsigned at_pos, const char* data, unsigned len)
{
    AutoFit();

    if ((at_pos < _buf.size()) && data && len)
        _buf.insert(_buf.cbegin() + at_pos, data, data + len);
}


/**
 *  \brief
 */
void CTextA::Clear()
{
    _buf.clear();
    _buf.push_back('\0');
    _invalidStrLen = false;
}


/**
 *  \brief
 */
void CTextA::Resize(unsigned size)
{
    unsigned len = Len();

    _buf.resize(size);
    _buf.push_back('\0');
    _invalidStrLen = _invalidStrLen || (size > len);
}


/**
 *  \brief
 */
unsigned CPath::StripTrailingSpaces()
{
    AutoFit();

    unsigned len = Len();

    for (; len > 0; --len)
        if (_buf[len - 1] != _T(' ') && _buf[len - 1] != _T('\t') &&
                _buf[len - 1] != _T('\r') && _buf[len - 1] != _T('\n'))
            break;

    _buf.erase(_buf.begin() + len, _buf.end());
    _buf.push_back(_T('\0'));

    return len;
}


/**
 *  \brief
 */
unsigned CPath::StripFilename()
{
    AutoFit();

    unsigned len = Len();

    for (; len > 0; --len)
        if (_buf[len - 1] == _T('\\') || _buf[len - 1] == _T('/'))
            break;

    _buf.erase(_buf.begin() + len, _buf.end());
    _buf.push_back(_T('\0'));

    return len;
}


/**
 *  \brief
 */
unsigned CPath::DirUp()
{
    AutoFit();

    unsigned len = Len();
    if (_buf[len - 1] == _T('\\') || _buf[len - 1] == _T('/'))
        --len;

    for (; len > 0; --len)
        if (_buf[len - 1] == _T('\\') || _buf[len - 1] == _T('/'))
            break;

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

    for (; len > 0; --len)
        if (_buf[len - 1] == _T('\\') || _buf[len - 1] == _T('/'))
        {
            ++len;
            break;
        }

    return &_buf[len];
}


/**
 *  \brief
 */
bool CPath::IsParentOf(const CPath& path) const
{
    unsigned len = Len();
    if (len > path.Len())
        return false;

    return !_tcsncmp(_buf.data(), path._buf.data(), len);
}


/**
 *  \brief
 */
bool CPath::IsParentOf(const TCHAR* pathStr) const
{
    unsigned len = Len();
    if (len > _tcslen(pathStr))
        return false;

    return !_tcsncmp(_buf.data(), pathStr, len);
}


/**
 *  \brief
 */
bool CPath::IsSubpathOf(const CPath& path) const
{
    unsigned len = path.Len();
    if (len > Len())
        return false;

    return !_tcsncmp(_buf.data(), path._buf.data(), len);
}


/**
 *  \brief
 */
bool CPath::IsSubpathOf(const TCHAR* pathStr) const
{
    unsigned len = _tcslen(pathStr);
    if (len > Len())
        return false;

    return !_tcsncmp(_buf.data(), pathStr, len);
}
