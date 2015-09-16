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


#pragma once


#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <vector>


#ifdef UNICODE
#define CText   CTextW
#else
#define CText   CTextA
#endif


namespace Tools
{

void ReleaseKey(WORD virtKey, bool onlyIfPressed = true);


inline unsigned AtoW(wchar_t* dst, unsigned dstSize, const char* src)
{
    size_t cnt;

    mbstowcs_s(&cnt, dst, dstSize, src, _TRUNCATE);

    return cnt;
}


inline bool FileExists(TCHAR* file)
{
    DWORD dwAttrib = GetFileAttributes(file);
    return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}


#ifdef DEVELOPMENT

#ifdef UNICODE
#define Msg(x)  MsgW(x)
#else
#define Msg(x)  MsgA(x)
#endif

inline void MsgW(const wchar_t* msg)
{
    MessageBoxW(NULL, msg, L"", MB_OK);
}


inline void MsgA(const char* msg)
{
    MessageBoxA(NULL, msg, "", MB_OK);
}


inline void MsgNum(int num, int radix = 10)
{
    char buf[128];
    _itoa_s(num, buf, _countof(buf), radix);
    MessageBoxA(NULL, buf, "", MB_OK);
}

#endif

} // namespace Tools


/**
 *  \class  CTextW
 *  \brief
 */
class CTextW
{
protected:
    std::vector<wchar_t> _buf;

public:
    CTextW() { _buf.push_back(L'\0'); }
    CTextW(const CTextW& txt) : _buf(txt._buf) {}

    CTextW(const wchar_t* str)
    {
        if (str)
            _buf.assign(str, str + wcslen(str) + 1);
        else
            _buf.push_back(L'\0');
    }

    CTextW(const char* str);

    CTextW(unsigned size)
    {
        _buf.resize(size + 1, L'\0');
    }

    ~CTextW() {}

    inline const CTextW& operator=(const CTextW& txt)
    {
        if (this != &txt)
            _buf = txt._buf;
        return *this;
    }

    inline const CTextW& operator=(const wchar_t* str)
    {
        if (str)
            _buf.assign(str, str + wcslen(str) + 1);
        return *this;
    }

    const CTextW& operator=(const char* str);

    inline bool operator==(const CTextW& txt) const
    {
        return (_buf == txt._buf);
    }

    inline bool operator==(const wchar_t* str) const
    {
        return !wcscmp(_buf.data(), str);
    }

    inline void operator+=(const CTextW& txt)
    {
        _buf.pop_back();
        _buf.insert(_buf.cend(), txt._buf.begin(), txt._buf.end());
    }

    void operator+=(const wchar_t* str);
    void operator+=(const char* str);

    inline void operator+=(wchar_t letter)
    {
        _buf.pop_back();
        _buf.push_back(letter);
        _buf.push_back(L'\0');
    }

    inline void Append(const wchar_t* data, unsigned len)
    {
        if (data && len)
        {
            _buf.pop_back();
            _buf.insert(_buf.cend(), data, data + len);
            _buf.push_back(L'\0');
        }
    }

    inline void Insert(unsigned at_pos, wchar_t letter)
    {
        if (at_pos < _buf.size())
            _buf.insert(_buf.cbegin() + at_pos, letter);
    }

    inline void Insert(unsigned at_pos, const wchar_t* data, unsigned len)
    {
        if ((at_pos < _buf.size()) && data && len)
            _buf.insert(_buf.cbegin() + at_pos, data, data + len);
    }

    inline const wchar_t* C_str() const { return _buf.data(); }
    inline wchar_t* C_str() { return _buf.data(); }
    inline bool IsEmpty() const { return !(_buf.size() - 1); }
    inline unsigned Size() const { return _buf.size(); }
    inline unsigned Len() const { return (_buf.size() - 1); }
    inline void Resize(unsigned size) { _buf.resize(size + 1, L'\0'); }

    inline void Clear()
    {
        _buf.clear();
        _buf.push_back(L'\0');
    }

#ifdef DEVELOPMENT
    inline void Print() { Tools::MsgW(C_str()); }
#endif
};


/**
 *  \class  CTextA
 *  \brief
 */
class CTextA
{
protected:
    std::vector<char> _buf;

public:
    CTextA() { _buf.push_back('\0'); }
    CTextA(const CTextA& txt) : _buf(txt._buf) {}

    CTextA(const char* str)
    {
        if (str)
            _buf.assign(str, str + strlen(str) + 1);
        else
            _buf.push_back('\0');
    }

    CTextA(const wchar_t* str);

    CTextA(unsigned size)
    {
        _buf.resize(size + 1, '\0');
    }

    ~CTextA() {}

    inline const CTextA& operator=(const CTextA& txt)
    {
        if (this != &txt)
            _buf = txt._buf;
        return *this;
    }

    inline const CTextA& operator=(const char* str)
    {
        if (str)
            _buf.assign(str, str + strlen(str) + 1);
        return *this;
    }

    const CTextA& operator=(const wchar_t* str);

    inline bool operator==(const CTextA& txt) const
    {
        return (_buf == txt._buf);
    }

    inline bool operator==(const char* str) const
    {
        return !strcmp(_buf.data(), str);
    }

    inline void operator+=(const CTextA& txt)
    {
        _buf.pop_back();
        _buf.insert(_buf.cend(), txt._buf.begin(), txt._buf.end());
    }

    void operator+=(const char* str);
    void operator+=(const wchar_t* str);

    inline void operator+=(char letter)
    {
        _buf.pop_back();
        _buf.push_back(letter);
        _buf.push_back('\0');
    }

    inline void Append(const char* data, unsigned len)
    {
        if (data && len)
        {
            _buf.pop_back();
            _buf.insert(_buf.cend(), data, data + len);
            _buf.push_back('\0');
        }
    }

    inline void Insert(unsigned at_pos, char letter)
    {
        if (at_pos < _buf.size())
            _buf.insert(_buf.cbegin() + at_pos, letter);
    }

    inline void Insert(unsigned at_pos, const char* data, unsigned len)
    {
        if ((at_pos < _buf.size()) && data && len)
            _buf.insert(_buf.cbegin() + at_pos, data, data + len);
    }

    inline const char* C_str() const { return _buf.data(); }
    inline char* C_str() { return _buf.data(); }
    inline bool IsEmpty() const { return !(_buf.size() - 1); }
    inline unsigned Size() const { return _buf.size(); }
    inline unsigned Len() const { return (_buf.size() - 1); }
    inline void Resize(unsigned size) { _buf.resize(size + 1, '\0'); }

    inline void Clear()
    {
        _buf.clear();
        _buf.push_back('\0');
    }

#ifdef DEVELOPMENT
    inline void Print() { Tools::MsgA(C_str()); }
#endif
};


/**
 *  \class  CPath
 *  \brief
 */
class CPath : public CText
{
public:
    CPath(const TCHAR* pathStr) : CText(pathStr) {}
    CPath(const char* pathStr) : CText(pathStr) {}
    ~CPath() {}

    inline const CPath& operator=(const TCHAR* pathStr)
    {
        ((CText*)this)->operator=(pathStr);
        return *this;
    }

    inline const CPath& operator=(const CPath& path)
    {
        if (this != &path)
            _buf = path._buf;
        return *this;
    }

    inline bool operator==(const TCHAR* pathStr) const
    {
        return ((CText*)this)->operator==(pathStr);
    }

    inline bool operator==(const CPath& path) const
    {
        return !_tcscmp(_buf, path._buf);
    }

    inline const CPath& operator+=(const TCHAR* str)
    {
        _tcscat_s(_buf, MAX_PATH, str);
        return *this;
    }

    inline const CPath& operator+=(const CPath& path)
    {
        _tcscat_s(_buf, MAX_PATH, path._buf);
        return *this;
    }

    inline const TCHAR* C_str() const { return _buf; }
    inline unsigned Len() const { return _tcslen(_buf); }

    inline bool Exists() const
    {
        DWORD dwAttrib = GetFileAttributes(_buf);
        return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES);
    }

    inline bool FileExists() const
    {
        DWORD dwAttrib = GetFileAttributes(_buf);
        return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }

    unsigned StripFilename();
    const TCHAR* GetFilename() const;
    unsigned Up();
    bool Contains(const TCHAR* pathStr) const;
    bool Contains(const CPath& path) const;
    bool IsContainedIn(const TCHAR* pathStr) const;
    bool IsContainedIn(const CPath& path) const;
};


/**
 *  \class  CPath
 *  \brief
 */
// class CPath
// {
// public:
    // CPath(const TCHAR* pathStr = NULL)
    // {
        // _buf[0] = 0;
        // if (pathStr)
            // _tcscpy_s(_buf, MAX_PATH, pathStr);
    // }

    // CPath(const char* pathStr)
    // {
        // _buf[0] = 0;
        // if (pathStr)
        // {
            // size_t cnt;
            // mbstowcs_s(&cnt, _buf, MAX_PATH, pathStr, _TRUNCATE);
        // }
    // }

    // CPath(const CPath& path)
    // {
        // _tcscpy_s(_buf, MAX_PATH, path._buf);
    // }

    // ~CPath() {}

    // inline const CPath& operator=(const TCHAR* pathStr)
    // {
        // _tcscpy_s(_buf, MAX_PATH, pathStr);
        // return *this;
    // }

    // inline const CPath& operator=(const CPath& path)
    // {
        // if (this != &path)
        // {
            // _tcscpy_s(_buf, MAX_PATH, path._buf);
        // }
        // return *this;
    // }

    // inline bool operator==(const TCHAR* pathStr) const
    // {
        // return !_tcscmp(_buf, pathStr);
    // }

    // inline bool operator==(const CPath& path) const
    // {
        // return !_tcscmp(_buf, path._buf);
    // }

    // inline const CPath& operator+=(const TCHAR* str)
    // {
        // _tcscat_s(_buf, MAX_PATH, str);
        // return *this;
    // }

    // inline const CPath& operator+=(const CPath& path)
    // {
        // _tcscat_s(_buf, MAX_PATH, path._buf);
        // return *this;
    // }

    // inline const TCHAR* C_str() const { return _buf; }
    // inline unsigned Len() const { return _tcslen(_buf); }

    // inline bool Exists() const
    // {
        // DWORD dwAttrib = GetFileAttributes(_buf);
        // return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES);
    // }

    // inline bool FileExists() const
    // {
        // DWORD dwAttrib = GetFileAttributes(_buf);
        // return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    // }

    // unsigned StripFilename();
    // const TCHAR* GetFilename() const;
    // unsigned Up();
    // bool Contains(const TCHAR* pathStr) const;
    // bool Contains(const CPath& path) const;
    // bool IsContainedIn(const TCHAR* pathStr) const;
    // bool IsContainedIn(const CPath& path) const;

// #ifdef DEVELOPMENT
    // inline void Print() { Tools::Msg(C_str()); }
// #endif

// private:
    // TCHAR _buf[MAX_PATH];
// };
