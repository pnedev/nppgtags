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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>


#ifdef UNICODE
#define CTcharArray CWcharArray
#else
#define CTcharArray CCharArray
#endif


/**
 *  \class
 *  \brief
 */
class CCharArray
{
public:
    CCharArray(unsigned size = 0) : _ptr(NULL), _size(size)
    {
        if (size)
            _ptr = new char[size];
    }

    ~CCharArray()
    {
        if (_ptr)
            delete [] _ptr;
    }

    char* operator&() { return _ptr; }
    const char* operator&() const { return _ptr; }

    CCharArray& operator()(unsigned size)
    {
        if (_ptr)
            delete [] _ptr;

        if (size)
            _ptr = new char[size];
        else
            _ptr = NULL;

        _size = size;

        return *this;
    }

    CCharArray& operator()(const char* str)
    {
        if (_ptr)
            delete [] _ptr;

        if (str)
        {
            _size = strlen(str) + 1;
            _ptr = new char[_size];
            strcpy_s(_ptr, _size, str);
        }
        else
        {
            _ptr = NULL;
            _size = 0;
        }

        return *this;
    }

    char& operator[](unsigned pos)
    {
        return (pos < _size ? _ptr[pos] : _ptr[0]);
    }

    const char* operator=(const char* array)
    {
        if (_ptr)
            strcpy_s(_ptr, _size, array);
        return _ptr;
    }

    const char* operator+=(const char* array)
    {
        if (_ptr)
            strcat_s(_ptr, _size, array);
        return _ptr;
    }

    bool operator==(const char* array) const
    {
        return (_ptr ? (!strcmp(_ptr, array)) : false);
    }

    unsigned Size() const { return _size; }
    unsigned Len() const { return (_ptr ? strlen(_ptr) : 0); }

private:
    char*       _ptr;
    unsigned    _size;
};


/**
 *  \class
 *  \brief
 */
class CWcharArray
{
public:
    CWcharArray(unsigned size = 0) : _ptr(NULL), _size(size)
    {
        if (size)
            _ptr = new wchar_t[size];
    }

    ~CWcharArray()
    {
        if (_ptr)
            delete [] _ptr;
    }

    wchar_t* operator&() { return _ptr; }
    const wchar_t* operator&() const { return _ptr; }

    CWcharArray& operator()(unsigned size)
    {
        if (_ptr)
            delete [] _ptr;

        if (size)
            _ptr = new wchar_t[size];
        else
            _ptr = NULL;

        _size = size;

        return *this;
    }

    CWcharArray& operator()(const wchar_t* str)
    {
        if (_ptr)
            delete [] _ptr;

        if (str)
        {
            _size = wcslen(str) + 1;
            _ptr = new wchar_t[_size];
            wcscpy_s(_ptr, _size, str);
        }
        else
        {
            _ptr = NULL;
            _size = 0;
        }

        return *this;
    }

    wchar_t& operator[](unsigned pos)
    {
        return (pos < _size ? _ptr[pos] : _ptr[0]);
    }

    const wchar_t* operator=(const wchar_t* array)
    {
        if (_ptr)
            wcscpy_s(_ptr, _size, array);
        return _ptr;
    }

    const wchar_t* operator+=(const wchar_t* array)
    {
        if (_ptr)
            wcscat_s(_ptr, _size, array);
        return _ptr;
    }

    bool operator==(const wchar_t* array) const
    {
        return (_ptr ? (!wcscmp(_ptr, array)) : false);
    }

    unsigned Size() const { return _size; }
    unsigned Len() const { return (_ptr ? wcslen(_ptr) : 0); }

private:
    wchar_t*    _ptr;
    unsigned    _size;
};


/**
 *  \class  CPath
 *  \brief
 */
class CPath
{
public:
    CPath(const TCHAR* pathStr = NULL)
    {
        _str[0] = 0;
        if (pathStr)
            _tcscpy_s(_str, MAX_PATH, pathStr);
    }

    CPath(const char* pathStr)
    {
        _str[0] = 0;
        if (pathStr)
        {
            size_t cnt;
            mbstowcs_s(&cnt, _str, MAX_PATH, pathStr, _TRUNCATE);
        }
    }

    CPath(const CPath& path)
    {
        _tcscpy_s(_str, MAX_PATH, path._str);
    }

    ~CPath() {}

    inline const CPath& operator=(const TCHAR* pathStr)
    {
        _tcscpy_s(_str, MAX_PATH, pathStr);
        return *this;
    }

    inline const CPath& operator=(const CPath& path)
    {
        if (this != &path)
        {
            _tcscpy_s(_str, MAX_PATH, path._str);
        }
        return *this;
    }

    inline bool operator==(const TCHAR* pathStr) const
    {
        return !_tcscmp(_str, pathStr);
    }

    inline bool operator==(const CPath& path) const
    {
        return !_tcscmp(_str, path._str);
    }

    inline const CPath& operator+=(const TCHAR* str)
    {
        _tcscat_s(_str, MAX_PATH, str);
        return *this;
    }

    inline const CPath& operator+=(const CPath& path)
    {
        _tcscat_s(_str, MAX_PATH, path._str);
        return *this;
    }

    inline const TCHAR* C_str() const { return _str; }
    inline unsigned Len() const { return _tcslen(_str); }

    inline bool Exists() const
    {
        DWORD dwAttrib = GetFileAttributes(_str);
        return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES);
    }

    inline bool FileExists() const
    {
        DWORD dwAttrib = GetFileAttributes(_str);
        return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }

    unsigned StripFilename();
    const TCHAR* GetFilename() const;
    unsigned Up();
    bool Contains(const TCHAR* pathStr) const;
    bool Contains(const CPath& path) const;
    bool IsContainedIn(const TCHAR* pathStr) const;
    bool IsContainedIn(const CPath& path) const;

private:
    TCHAR _str[MAX_PATH];
};


#ifdef UNICODE
#define CText     CTextW
#else
#define CText     CTextA
#endif


/**
 *  \class  CTextW
 *  \brief
 */
class CTextW
{
private:
    enum
    {
        ALLOC_CHUNK_SIZE = 4096
    };

    void resize(unsigned newLen);
    unsigned expand(unsigned newLen);

    unsigned _size;
    unsigned _len;
    wchar_t *_str;
    wchar_t _buf[ALLOC_CHUNK_SIZE];

public:
    CTextW() : _size(ALLOC_CHUNK_SIZE), _len(0), _str(_buf) { _buf[0] = 0; }
    CTextW(unsigned size);
    CTextW(const wchar_t* str);
    CTextW(const char* str);
    CTextW(const CTextW& txt);
    ~CTextW()
    {
        if (_str != _buf)
            delete [] _str;
    }

    const CTextW& operator=(const wchar_t* str);
    const CTextW& operator=(const char* str);
    const CTextW& operator=(const CTextW& txt);

    inline bool operator==(const wchar_t* str) const
    {
        return !wcscmp(_str, str);
    }

    inline bool operator==(const CTextW& txt) const
    {
        return !wcscmp(_str, txt._str);
    }

    const CTextW& operator+=(const wchar_t* str);
    const CTextW& operator+=(const char* str);
    const CTextW& operator+=(const CTextW& txt);
    const CTextW& append(const wchar_t* str, unsigned len);

    inline const wchar_t* C_str() const { return _str; }
    inline unsigned Size() const { return _size; }
    inline unsigned Len() const { return _len; }
};


/**
 *  \class  CTextA
 *  \brief
 */
class CTextA
{
private:
    enum
    {
        ALLOC_CHUNK_SIZE = 4096
    };

    void resize(unsigned newLen);
    unsigned expand(unsigned newLen);

    unsigned _size;
    unsigned _len;
    char *_str;
    char _buf[ALLOC_CHUNK_SIZE];

public:
    CTextA() : _size(ALLOC_CHUNK_SIZE), _len(0), _str(_buf) { _buf[0] = 0; }
    CTextA(unsigned size);
    CTextA(const char* str);
    CTextA(const wchar_t* str);
    CTextA(const CTextA& txt);
    ~CTextA()
    {
        if (_str != _buf)
            delete [] _str;
    }

    const CTextA& operator=(const char* str);
    const CTextA& operator=(const wchar_t* str);
    const CTextA& operator=(const CTextA& txt);

    inline bool operator==(const char* str) const
    {
        return !strcmp(_str, str);
    }

    inline bool operator==(const CTextA& txt) const
    {
        return !strcmp(_str, txt._str);
    }

    const CTextA& operator+=(const char* str);
    const CTextA& operator+=(const wchar_t* str);
    const CTextA& operator+=(const CTextA& txt);
    const CTextA& append(const char* str, unsigned len);

    inline const char* C_str() const { return _str; }
    inline unsigned Size() const { return _size; }
    inline unsigned Len() const { return _len; }
};


namespace Tools
{

void ReleaseKey(WORD virtKey);


inline unsigned WtoA(char* dst, unsigned dstSize, const wchar_t* src)
{
    size_t cnt;

    wcstombs_s(&cnt, dst, dstSize, src, _TRUNCATE);

    return cnt;
}


inline unsigned AtoW(wchar_t* dst, unsigned dstSize, const char* src)
{
    size_t cnt;

    mbstowcs_s(&cnt, dst, dstSize, src, _TRUNCATE);

    return cnt;
}


inline bool FileExists(TCHAR* file)
{
    DWORD dwAttrib = GetFileAttributes(file);
    return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}


#ifdef DEVELOPMENT

#ifdef UNICODE
#define Msg(x)     MsgW(x)
#else
#define Msg(x)     MsgA(x)
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
