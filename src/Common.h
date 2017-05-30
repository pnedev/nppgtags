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


/**
 *  \class  CTextW
 *  \brief
 */
class CTextW
{
protected:
    std::vector<wchar_t>    _buf;
    bool                    _invalidStrLen;

public:
    CTextW() : _invalidStrLen(false) { _buf.push_back(0); }
    CTextW(unsigned size) : _invalidStrLen(true) { _buf.resize(size + 1, 0); }

    CTextW(const wchar_t* str);
    CTextW(const char* str);

    CTextW(const CTextW& txt) : _buf(txt._buf), _invalidStrLen(txt._invalidStrLen) {}

    ~CTextW() {}

    inline void AutoFit()
    {
        if (_invalidStrLen)
        {
            _buf.resize(wcslen(_buf.data()) + 1);
            _invalidStrLen = false;
        }
    }

    const CTextW& operator=(const CTextW& txt);
    const CTextW& operator=(const wchar_t* str);
    const CTextW& operator=(const char* str);

    inline bool operator==(const CTextW& txt) const { return (_buf == txt._buf); }
    inline bool operator==(const wchar_t* str) const { return !wcscmp(_buf.data(), str); }

    void operator+=(const CTextW& txt);
    void operator+=(const wchar_t* str);
    void operator+=(const char* str);
    void operator+=(wchar_t letter);

    void Append(const wchar_t* data, unsigned len);
    void Append(const char* data, unsigned len);
    void Insert(unsigned at_pos, wchar_t letter);
    void Insert(unsigned at_pos, const wchar_t* data, unsigned len);
    void Erase(unsigned from_pos, unsigned len);

    void Clear();
    void Resize(unsigned size);

    inline unsigned Len() const { return (_invalidStrLen) ? wcslen(_buf.data()) : (_buf.size() - 1); }
    inline bool IsEmpty() const { return (Len() == 0); }
    inline const std::vector<wchar_t>& Vector() const { return _buf; }
    inline const wchar_t* C_str() const { return _buf.data(); }
    inline wchar_t* C_str() { return _buf.data(); }
    inline unsigned Size() const { return _buf.size(); }
};


/**
 *  \class  CTextA
 *  \brief
 */
class CTextA
{
protected:
    std::vector<char>   _buf;
    bool                _invalidStrLen;

public:
    CTextA() : _invalidStrLen(false) { _buf.push_back(0); }
    CTextA(unsigned size) : _invalidStrLen(true) { _buf.resize(size + 1, 0); }

    CTextA(const char* str);
    CTextA(const wchar_t* str);

    CTextA(const CTextA& txt) : _buf(txt._buf), _invalidStrLen(txt._invalidStrLen) {}

    ~CTextA() {}

    inline void AutoFit()
    {
        if (_invalidStrLen)
        {
            _buf.resize(strlen(_buf.data()) + 1);
            _invalidStrLen = false;
        }
    }

    const CTextA& operator=(const CTextA& txt);
    const CTextA& operator=(const char* str);
    const CTextA& operator=(const wchar_t* str);

    inline bool operator==(const CTextA& txt) const { return (_buf == txt._buf); }
    inline bool operator==(const char* str) const { return !strcmp(_buf.data(), str); }

    void operator+=(const CTextA& txt);
    void operator+=(const char* str);
    void operator+=(const wchar_t* str);
    void operator+=(char letter);

    void Append(const char* data, unsigned len);
    void Append(const wchar_t* data, unsigned len);
    void Insert(unsigned at_pos, char letter);
    void Insert(unsigned at_pos, const char* data, unsigned len);
    void Erase(unsigned from_pos, unsigned len);

    void Clear();
    void Resize(unsigned size);

    inline unsigned Len() const { return (_invalidStrLen) ? strlen(_buf.data()) : (_buf.size() - 1); }
    inline bool IsEmpty() const { return (Len() == 0); }
    inline const std::vector<char>& Vector() const { return _buf; }
    inline const char* C_str() const { return _buf.data(); }
    inline char* C_str() { return _buf.data(); }
    inline unsigned Size() const { return _buf.size(); }
};


/**
 *  \class  CPath
 *  \brief
 */
class CPath : public CText
{
public:
	CPath() : CText() {}
    CPath(const CPath& path) : CText(static_cast<const CText&>(path)) {}
	CPath(const char* pathStr) : CText(pathStr) {}
	CPath(const wchar_t* pathStr) : CText(pathStr) {}
    CPath(unsigned size) : CText(size) {}
    ~CPath() {}

    inline bool Exists() const
    {
        if (IsEmpty())
            return false;
        DWORD dwAttrib = GetFileAttributes(C_str());
        return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES);
    }

    inline bool FileExists() const
    {
        if (IsEmpty())
            return false;
        DWORD dwAttrib = GetFileAttributes(C_str());
        return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }

    void AsFolder();
    unsigned StripFilename();
    unsigned DirUp();

    const TCHAR* GetFilename() const;
    bool IsParentOf(const CPath& path) const;
    bool IsParentOf(const TCHAR* pathStr) const;
    bool IsSubpathOf(const CPath& path) const;
    bool IsSubpathOf(const TCHAR* pathStr) const;

private:
    bool pathMatches(const TCHAR* pathStr, unsigned len) const;
};


namespace Tools
{

bool BrowseForFolder(HWND hOwnerWin, CPath& path, const TCHAR* info = NULL, bool onlySubFolders = false);
RECT GetWinRect(HWND hOwner, DWORD styleEx, DWORD style, int width, int height);
unsigned GetWindowsVersion();


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
