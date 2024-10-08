/**
 *  \file
 *  \brief  Various helper types and functions
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2024 Pavel Nedev
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
#include <objbase.h>
#include <versionhelpers.h>


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
void Tools::ReleaseKeys()
{
    INPUT inputs[4] = {};
    ZeroMemory(inputs, sizeof(inputs));

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_SHIFT;
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_CONTROL;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_MENU;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

    inputs[3].type = INPUT_MOUSE;
    inputs[3].mi.dwFlags = GetSystemMetrics(SM_SWAPBUTTON) ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}


/**
 *  \brief
 */
bool Tools::BrowseForFolder(HWND hOwnerWin, CPath& path, const TCHAR* info, bool onlySubFolders)
{
    TCHAR tmp[MAX_PATH];

    BROWSEINFO bi       = {0};
    bi.hwndOwner        = hOwnerWin;
    bi.pszDisplayName   = tmp;
    bi.lpszTitle        = info ? info : _T("Select the database root (indexed recursively)");
    bi.ulFlags          = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;
    bi.lpfn             = browseFolderCB;

    if (!path.IsEmpty() && path.Exists())
        bi.lParam = (LPARAM)path.C_str();

    if (onlySubFolders)
    {
        LPITEMIDLIST rootPidl;

        if (SHParseDisplayName(path.C_str(), NULL, &rootPidl, 0, NULL) == S_OK)
            bi.pidlRoot = rootPidl;
    }

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (!pidl)
        return false;

    SHGetPathFromIDList(pidl, tmp);
    CoTaskMemFree(pidl);

    path = tmp;
    path += _T("\\");

    return true;
}


/**
 *  \brief
 */
RECT Tools::GetWinRect(HWND hOwner, DWORD styleEx, DWORD style, int width, int height)
{
    RECT maxWin;
    maxWin.left     = GetSystemMetrics(SM_XVIRTUALSCREEN);
    maxWin.top      = GetSystemMetrics(SM_YVIRTUALSCREEN);
    maxWin.right    = GetSystemMetrics(SM_CXVIRTUALSCREEN) + maxWin.left;
    maxWin.bottom   = GetSystemMetrics(SM_CYVIRTUALSCREEN) + maxWin.top;

    POINT center;
    if (hOwner)
    {
        RECT biasWin;
        GetWindowRect(hOwner, &biasWin);
        center.x = (biasWin.right + biasWin.left) / 2;
        center.y = (biasWin.bottom + biasWin.top) / 2;
    }
    else
    {
        center.x = (maxWin.right + maxWin.left) / 2;
        center.y = (maxWin.bottom + maxWin.top) / 2;
    }

    RECT win = maxWin;
    win.right = win.left + width;
    win.bottom = win.top + height;

    AdjustWindowRectEx(&win, style, FALSE, styleEx);

    width = win.right - win.left;
    height = win.bottom - win.top;

    if (width < maxWin.right - maxWin.left)
    {
        win.left = center.x - width / 2;
        if (win.left < maxWin.left)
            win.left = maxWin.left;

        win.right = win.left + width;
        if (win.right > maxWin.right)
        {
            win.right = maxWin.right;
            win.left = win.right - width;
        }
    }
    else
    {
        win.left = maxWin.left;
        win.right = maxWin.right;
    }

    if (height < maxWin.bottom - maxWin.top)
    {
        win.top = center.y - height / 2;
        if (win.top < maxWin.top)
            win.top = maxWin.top;

        win.bottom = win.top + height;
        if (win.bottom > maxWin.bottom)
        {
            win.bottom = maxWin.bottom;
            win.top = win.bottom - height;
        }
    }
    else
    {
        win.top = maxWin.top;
        win.bottom = maxWin.bottom;
    }

    return win;
}


/**
 *  \brief
 */
unsigned Tools::GetFontHeight(HDC hdc, HFONT font)
{
    SIZE fontSize;
    HGDIOBJ oldObj = SelectObject(hdc, font);

    GetTextExtentPoint32(hdc, _T("A"), 1, &fontSize);
    SelectObject(hdc, oldObj);

    return fontSize.cy;
}


/**
 *  \brief
 */
HFONT Tools::CreateFromSystemMessageFont(HDC hdc, unsigned fontHeight)
{
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);

#if (WINVER >= 0x0600)
    if (!IsWindows7OrGreater())
        ncm.cbSize -= sizeof(int);
#endif

    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

    if (hdc && fontHeight)
        ncm.lfMessageFont.lfHeight = -MulDiv(fontHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);

    return CreateFontIndirect(&ncm.lfMessageFont);
}


/**
 *  \brief
 */
HFONT Tools::CreateFromSystemMenuFont(HDC hdc, unsigned fontHeight)
{
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);

#if (WINVER >= 0x0600)
    if (!IsWindows7OrGreater())
        ncm.cbSize -= sizeof(int);
#endif

    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

    if (hdc && fontHeight)
        ncm.lfMenuFont.lfHeight = -MulDiv(fontHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);

    return CreateFontIndirect(&ncm.lfMenuFont);
}


/**
 *  \brief
 */
CTextW::CTextW(const wchar_t* str) : _invalidStrLen(false)
{
    if (str)
        _buf.assign(str, str + wcslen(str) + 1);
    else
        _buf.push_back(0);
}


/**
 *  \brief
 */
CTextW::CTextW(const char* str) : _invalidStrLen(false)
{
    if (str)
    {
        _buf.resize(strlen(str) + 1, 0);
        size_t cnt;
        mbstowcs_s(&cnt, _buf.data(), _buf.size(), str, _TRUNCATE);
    }
    else
    {
        _buf.push_back(0);
    }
}


/**
 *  \brief
 */
CTextW& CTextW::operator=(const CTextW& txt)
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
CTextW& CTextW::operator=(const wchar_t* str)
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
CTextW& CTextW::operator=(const char* str)
{
    if (str)
    {
        _buf.resize(strlen(str) + 1, 0);
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
        size_t len = wcslen(str);
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
        size_t len = strlen(str);
        if (len)
        {
            _buf.pop_back();

            size_t size = _buf.size();
            _buf.resize(size + len + 1, 0);

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
    _buf.push_back(0);
}


/**
 *  \brief
 */
void CTextW::Append(const wchar_t* data, size_t len)
{
    AutoFit();

    if (data && len)
    {
        _buf.pop_back();
        _buf.insert(_buf.cend(), data, data + len);
        _buf.push_back(0);
    }
}


/**
 *  \brief
 */
void CTextW::Append(const char* data, size_t len)
{
    AutoFit();

    if (data && len)
    {
        const size_t currentLen = Len();
        _buf.resize(currentLen + len + 1, 0);
        size_t cnt;
        mbstowcs_s(&cnt, _buf.data() + currentLen, len + 1, data, _TRUNCATE);
    }
}


/**
 *  \brief
 */
void CTextW::Insert(size_t at_pos, wchar_t letter)
{
    AutoFit();

    if (at_pos < _buf.size())
        _buf.insert(_buf.cbegin() + at_pos, letter);
}


/**
 *  \brief
 */
void CTextW::Insert(size_t at_pos, const wchar_t* data, size_t len)
{
    AutoFit();

    if ((at_pos < _buf.size()) && data && len)
        _buf.insert(_buf.cbegin() + at_pos, data, data + len);
}


/**
 *  \brief
 */
void CTextW::Erase(size_t from_pos, size_t len)
{
    AutoFit();

    if ((from_pos < _buf.size()) && len)
    {
        if (len > _buf.size())
            len = _buf.size();

        _buf.erase(_buf.cbegin() + from_pos, _buf.cbegin() + from_pos + len);
    }
}


/**
 *  \brief
 */
void CTextW::Clear()
{
    _buf.clear();
    _buf.push_back(0);
    _invalidStrLen = false;
}


/**
 *  \brief
 */
void CTextW::Resize(size_t size)
{
    const size_t len = Len();

    _buf.resize(size);
    _buf.push_back(0);
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
        _buf.push_back(0);
}


/**
 *  \brief
 */
CTextA::CTextA(const wchar_t* str) : _invalidStrLen(false)
{
    if (str)
    {
        _buf.resize(wcslen(str) + 1, 0);
        size_t cnt;
        wcstombs_s(&cnt, _buf.data(), _buf.size(), str, _TRUNCATE);
    }
    else
    {
        _buf.push_back(0);
    }
}


/**
 *  \brief
 */
CTextA& CTextA::operator=(const CTextA& txt)
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
CTextA& CTextA::operator=(const char* str)
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
CTextA& CTextA::operator=(const wchar_t* str)
{
    if (str)
    {
        _buf.resize(wcslen(str) + 1, 0);
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
        size_t len = strlen(str);
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
        size_t len = wcslen(str);
        if (len)
        {
            _buf.pop_back();

            size_t size = _buf.size();
            _buf.resize(size + len + 1, 0);

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
    _buf.push_back(0);
}


/**
 *  \brief
 */
void CTextA::Append(const char* data, size_t len)
{
    AutoFit();

    if (data && len)
    {
        _buf.pop_back();
        _buf.insert(_buf.cend(), data, data + len);
        _buf.push_back(0);
    }
}


/**
 *  \brief
 */
void CTextA::Append(const wchar_t* data, size_t len)
{
    AutoFit();

    if (data && len)
    {
        const size_t currentLen = Len();
        _buf.resize(currentLen + len + 1, 0);
        size_t cnt;
        wcstombs_s(&cnt, _buf.data() + currentLen, len + 1, data, _TRUNCATE);
    }
}


/**
 *  \brief
 */
void CTextA::Insert(size_t at_pos, char letter)
{
    AutoFit();

    if (at_pos < _buf.size())
        _buf.insert(_buf.cbegin() + at_pos, letter);
}


/**
 *  \brief
 */
void CTextA::Insert(size_t at_pos, const char* data, size_t len)
{
    AutoFit();

    if ((at_pos < _buf.size()) && data && len)
        _buf.insert(_buf.cbegin() + at_pos, data, data + len);
}


/**
 *  \brief
 */
void CTextA::Erase(size_t from_pos, size_t len)
{
    AutoFit();

    if ((from_pos < _buf.size()) && len)
    {
        if (len > _buf.size())
            len = _buf.size();

        _buf.erase(_buf.cbegin() + from_pos, _buf.cbegin() + from_pos + len);
    }
}


/**
 *  \brief
 */
void CTextA::Clear()
{
    _buf.clear();
    _buf.push_back(0);
    _invalidStrLen = false;
}


/**
 *  \brief
 */
void CTextA::Resize(size_t size)
{
    const size_t len = Len();

    _buf.resize(size);
    _buf.push_back(0);
    _invalidStrLen = _invalidStrLen || (size > len);
}


/**
 *  \brief
 */
void CPath::NormalizePathSlashes()
{
    AutoFit();

    for (size_t i = 0; i < Len(); ++i)
        if (_buf[i] == _T('/'))
            _buf[i] = _T('\\');
}


/**
 *  \brief
 */
void CPath::AsFolder()
{
    AutoFit();

    size_t len = Len();

    for (; len > 0; --len)
        if (_buf[len - 1] != _T(' ') && _buf[len - 1] != _T('\t') &&
                _buf[len - 1] != _T('\r') && _buf[len - 1] != _T('\n'))
            break;

    _buf.erase(_buf.begin() + len, _buf.end());

    if (len > 0 && _buf[len - 1] != _T('\\') && _buf[len - 1] != _T('/'))
        _buf.push_back(_T('\\'));

    _buf.push_back(0);
}


/**
 *  \brief
 */
size_t CPath::StripFilename()
{
    AutoFit();

    size_t len = Len();

    for (; len > 0; --len)
        if (_buf[len - 1] == _T('\\') || _buf[len - 1] == _T('/'))
            break;

    _buf.erase(_buf.begin() + len, _buf.end());
    _buf.push_back(0);

    return len;
}


/**
 *  \brief
 */
size_t CPath::DirUp()
{
    AutoFit();

    size_t len = Len();
    if (_buf[len - 1] == _T('\\') || _buf[len - 1] == _T('/'))
        --len;

    for (; len > 0; --len)
        if (_buf[len - 1] == _T('\\') || _buf[len - 1] == _T('/'))
            break;

    _buf.erase(_buf.begin() + len, _buf.end());
    _buf.push_back(0);

    return len;
}


/**
 *  \brief
 */
const TCHAR* CPath::GetFilename() const
{
    size_t len = Len();

    for (; len > 0; --len)
        if (_buf[len - 1] == _T('\\') || _buf[len - 1] == _T('/'))
            break;

    return &_buf[len];
}


/**
 *  \brief
 */
bool CPath::pathMatches(const TCHAR* pathStr, size_t len) const
{
    if (len > Len())
        return false;

    for (int i = (int)len - 1; i >= 0; --i)
    {
        if ((_buf[i] != pathStr[i]) &&
            !(_buf[i] == _T('\\') && pathStr[i] == _T('/')) && !(_buf[i] == _T('/') && pathStr[i] == _T('\\')))
        return false;
    }

    return true;
}


/**
 *  \brief
 */
bool CPath::IsParentOf(const CPath& path) const
{
    size_t len = Len();
    if (len > path.Len())
        return false;

    return pathMatches(path._buf.data(), len);
}


/**
 *  \brief
 */
bool CPath::IsParentOf(const TCHAR* pathStr) const
{
    size_t len = Len();
    if (len > _tcslen(pathStr))
        return false;

    return pathMatches(pathStr, len);
}


/**
 *  \brief
 */
bool CPath::IsSubpathOf(const CPath& path) const
{
    size_t len = path.Len();
    if (len > Len())
        return false;

    return pathMatches(path._buf.data(), len);
}


/**
 *  \brief
 */
bool CPath::IsSubpathOf(const TCHAR* pathStr) const
{
    size_t len = _tcslen(pathStr);
    if (len > Len())
        return false;

    return pathMatches(pathStr, len);
}
