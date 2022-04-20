/**
 *  \file
 *  \brief  GTags result Scintilla view window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2022 Pavel Nedev
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


#pragma comment (lib, "comctl32")


#include "ResultWin.h"
#include "INpp.h"
#include "DbManager.h"
#include "DocLocation.h"
#include "ActivityWin.h"
#include "Cmd.h"
#include "CmdEngine.h"
#include <windowsx.h>
#include <richedit.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include "Common.h"
#include "GTags.h"
#include "NppAPI/dockingResource.h"
#include "StrUniquenessChecker.h"


// Scintilla user defined styles IDs
enum SciStyles_t
{
    SCE_GTAGS_HEADER = 151,
    SCE_GTAGS_PROJECT_PATH,
    SCE_GTAGS_FILE,
    SCE_GTAGS_LINE_NUM,
    SCE_GTAGS_WORD2SEARCH
};


// Scintilla fold levels
enum SciFoldLevels_t
{
    FILE_HEADER_LVL = SC_FOLDLEVELBASE,
    RESULT_LVL
};


namespace GTags
{

const TCHAR ResultWin::cClassName[]         = _T("ResultWin");
const TCHAR ResultWin::cSearchClassName[]   = _T("ResultSearchWin");
const int ResultWin::cSearchBkgndColor      = COLOR_INFOBK;
const unsigned ResultWin::cSearchFontSize   = 10;
const int ResultWin::cSearchWidth           = 420;


ResultWin* ResultWin::RW = NULL;


/**
 *  \brief
 */
int ResultWin::TabParser::Parse(const CmdPtr_t& cmd)
{
    _filesCount = 0;
    _hits = 0;
    _headerStatusLen = 0;

    _fileResults.clear();

    // Add the search header - cmd name + search word + project path
    _buf = cmd->Name();
    _buf += " \"";
    _buf += cmd->Tag().C_str();
    _buf += "\"";

    if (cmd->RegExp() || cmd->IgnoreCase())
    {
        _buf += " (";

        if (cmd->RegExp())
        {
            _buf += "regexp";

            if (cmd->IgnoreCase())
                _buf += ", ";
        }

        if (cmd->IgnoreCase())
            _buf += "ignore case";

        _buf += ")";
    }

    _buf += " in \"";
    _buf += cmd->Db()->GetPath().C_str();
    _buf += "\"";

    const size_t countsPos = _buf.Len();

    int res;

    // parsing command result
    if (cmd->Id() == FIND_FILE)
    {
        res = parseFindFile(cmd);

        // Add results sumary in header
        if (res > 0)
        {
            std::string str = " (";

            if (_filesCount == 1)
            {
                str += "1 hit)";
            }
            else
            {
                str += std::to_string(_filesCount);
                str += " hits)";
            }

            _headerStatusLen = (int)str.size();

            _buf.Insert(countsPos, str.c_str(), str.size());
        }
    }
    else
    {
        res = parseCmd(cmd);

        // Add results sumary in header
        if (res > 0)
        {
            std::string str = " (";

            if (_hits == 1)
            {
                str += "1 hit in 1 file)";
            }
            else
            {
                str += std::to_string(_hits);
                str += " hits in ";

                if (_filesCount == 1)
                {
                    str += "1 file)";
                }
                else
                {
                    str += std::to_string(_filesCount);
                    str += " files)";
                }
            }

            _headerStatusLen = (int)str.size();

            _buf.Insert(countsPos, str.c_str(), str.size());
        }
    }

    return res;
}


/**
 *  \brief
 */
bool ResultWin::TabParser::filterEntry(const DbConfig& cfg, const char* pEntry, size_t len)
{
    if (cfg._usePathFilter)
    {
        CPath currentEntry;
        currentEntry.Append(pEntry, len);

        for (const auto& filter : cfg._pathFilters)
        {
            if (filter.IsParentOf(currentEntry))
                return true;
        }
    }

    return false;
}


/**
 *  \brief
 */
int ResultWin::TabParser::parseFindFile(const CmdPtr_t& cmd)
{
    const char* pSrc = cmd->Result();
    const char* pEol;

    const DbConfig& cfg = cmd->Db()->GetConfig();

    for (;;)
    {
        while (*pSrc == '\n' || *pSrc == '\r' || *pSrc == ' ' || *pSrc == '\t')
            ++pSrc;
        if (*pSrc == 0) break;

        pEol = pSrc;
        while (*pEol != '\n' && *pEol != '\r' && *pEol != 0)
            ++pEol;

        if (!filterEntry(cfg, pSrc, pEol - pSrc))
        {
            _buf += "\n\t";
            _buf.Append(pSrc, pEol - pSrc);

            addResultFile(pSrc, pEol - pSrc);

            ++_filesCount;
        }

        pSrc = pEol;
    }

    return _filesCount;
}


/**
 *  \brief
 */
int ResultWin::TabParser::parseCmd(const CmdPtr_t& cmd)
{
    bool filterReoccurring = false;

    const DbConfig& cfg = cmd->Db()->GetConfig();
    if (cmd->Id() == FIND_DEFINITION && cfg._useLibDb)
    {
        for (const auto& libPath : cfg._libDbPaths)
        {
            if (libPath.IsParentOf(cmd->Db()->GetPath()))
            {
                filterReoccurring = true;
                break;
            }
        }
    }

    StrUniquenessChecker<char> strChecker;

    char*       pSrc = cmd->Result();
    char*       pIdx;

    const char* pLine;
    const char* pPreviousFile = NULL;
    unsigned    previousFileLen = 0;
    bool        previousFileFiltered = false;

    size_t      previousBufLen;

    for (;;)
    {
        while (*pSrc == '\n' || *pSrc == '\r')
            ++pSrc;
        if (*pSrc == 0) break;

        previousBufLen = _buf.Len();
        pLine = pSrc;

        pIdx = pSrc;
        while (*pIdx != ':')
            ++pIdx;

        // Path is absolute (starts with drive letter)
        if ((pIdx - pSrc == 1) && ((*(pIdx + 1) == '\\') || (*(pIdx + 1) == '/')))
            while (*++pIdx != ':');

        // add new file name to the UI buffer only if it is different
        // than the previous one
        if ((pPreviousFile == NULL) || ((unsigned)(pIdx - pSrc) != previousFileLen) ||
            strncmp(pSrc, pPreviousFile, previousFileLen))
        {
            pPreviousFile = pSrc;
            previousFileLen = (unsigned)(pIdx - pSrc);

            if (filterEntry(cfg, pPreviousFile, previousFileLen))
            {
                previousFileFiltered = true;
            }
            else
            {
                _buf += "\n\t";
                _buf.Append(pPreviousFile, previousFileLen);

                addResultFile(pPreviousFile, previousFileLen);

                ++_filesCount;

                previousFileFiltered = false;
            }
        }

        if (previousFileFiltered)
        {
            while (*pSrc != '\n' && *pSrc != '\r')
                ++pSrc;
            continue;
        }

        pSrc = ++pIdx;
        while (*pSrc != ':')
            ++pSrc;

        _buf += "\n\t\tline ";
        _buf.Append(pIdx, pSrc - pIdx);
        _buf += ":\t";

        pIdx = ++pSrc;
        while (*pIdx == ' ' || *pIdx == '\t')
            ++pIdx;

        pSrc = pIdx + 1;
        while (*pSrc != '\n' && *pSrc != '\r')
            ++pSrc;

        if (pSrc == pIdx + 1)
            return -1;

        _buf.Append(pIdx, pSrc - pIdx);

        *pSrc++ = 0;

        if (filterReoccurring && !strChecker.IsUnique(pLine))
            _buf.Resize(previousBufLen);
        else
            ++_hits;
    }

    return _hits;
}


/**
 *  \brief
 */
ResultWin::Tab::Tab(const CmdPtr_t& cmd) :
    _cmdId(cmd->Id()), _regExp(cmd->RegExp()), _ignoreCase(cmd->IgnoreCase()),
    _projectPath(cmd->Db()->GetPath().C_str()), _search(cmd->Tag().C_str()), _currentLine(1), _firstVisibleLine(0),
    _parser(cmd->Parser()), _dirty(false)
{
}


/**
 *  \brief
 */
inline void ResultWin::Tab::SetFolded(intptr_t lineNum)
{
    _expandedLines.erase(lineNum);
}


/**
 *  \brief
 */
inline void ResultWin::Tab::SetAllFolded()
{
    _expandedLines.clear();
}


/**
 *  \brief
 */
inline void ResultWin::Tab::ClearFolded(intptr_t lineNum)
{
    _expandedLines.insert(lineNum);
}


/**
 *  \brief
 */
inline bool ResultWin::Tab::IsFolded(intptr_t lineNum)
{
    return (_expandedLines.find(lineNum) == _expandedLines.end());
}


/**
 *  \brief
 */
inline void ResultWin::Tab::MoveFolded(Tab& tab)
{
    _expandedLines = std::move(tab._expandedLines);
}


/**
 *  \brief
 */
HWND ResultWin::Register()
{
    if (RW)
        return NULL;

    WNDCLASS wc         = {0};
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = wndProc;
    wc.hInstance        = HMod;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszClassName    = cClassName;

    RegisterClass(&wc);

    wc.lpfnWndProc      = searchWndProc;
    wc.hbrBackground    = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName    = cSearchClassName;

    RegisterClass(&wc);

    INITCOMMONCONTROLSEX icex   = {0};
    icex.dwSize                 = sizeof(icex);
    icex.dwICC                  = ICC_STANDARD_CLASSES;

    InitCommonControlsEx(&icex);

    // Scintilla uses that, make sure it is not accidentally unloaded
    LoadLibrary(_T("Riched20.dll"));

    RW = new ResultWin();
    if (RW->composeWindow() == NULL)
    {
        Unregister();
        return NULL;
    }

    tTbData data        = {0};
    data.hClient        = RW->_hWnd;
    data.pszName        = const_cast<TCHAR*>(cPluginName);
    data.uMask          = 0;
    data.pszAddInfo     = NULL;
    data.uMask          = DWS_DF_CONT_BOTTOM;
    data.pszModuleName  = DllPath.GetFilename();
    data.dlgID          = 0;

    INpp& npp = INpp::Get();
    npp.RegisterDockingWin(data);
    npp.HideDockingWin(RW->_hWnd);

    return RW->_hWnd;
}


/**
 *  \brief
 */
void ResultWin::Unregister()
{
    if (RW)
    {
        delete RW;
        RW = NULL;
    }

    UnregisterClass(cClassName, HMod);
    UnregisterClass(cSearchClassName, HMod);
}


/**
 *  \brief
 */
void ResultWin::show()
{
    INpp& npp = INpp::Get();

    if (GetFocus() != npp.ReadSciHandle())
        SetFocus(npp.GetSciHandle());
    else if (TabCtrl_GetItemCount(_hTab))
        showWindow();
}


/**
 *  \brief
 */
void ResultWin::show(const CmdPtr_t& cmd)
{
    Tab* tab = new Tab(cmd);

    bool isNewTab = false;
    HWND hFocus = NULL;

    int i;
    for (i = TabCtrl_GetItemCount(_hTab); i; --i)
    {
        Tab* oldTab = getTab(i - 1);

        if (oldTab && (*tab == *oldTab)) // same search tab already present?
        {
            tab->_currentLine       = oldTab->_currentLine;
            tab->_firstVisibleLine  = oldTab->_firstVisibleLine;

            tab->MoveFolded(*oldTab);

            if (_activeTab == oldTab) // is this the currently active tab?
            {
                _activeTab = NULL;
                hFocus = GetFocus();
            }

            delete oldTab;
            break;
        }
    }

    if (i == 0) // search is completely new - add new tab or visit single result
    {
        if (visitSingleResult(tab))
        {
            delete tab;
            return;
        }

        TCHAR buf[64];
        _sntprintf_s(buf, _countof(buf), _TRUNCATE, _T("%s \"%s\""), cmd->Name(), cmd->Tag().C_str());

        TCITEM tci  = {0};
        tci.mask    = TCIF_TEXT | TCIF_PARAM;
        tci.pszText = buf;
        tci.lParam  = (LPARAM)tab;

        i = TabCtrl_InsertItem(_hTab, TabCtrl_GetItemCount(_hTab), &tci);
        if (i == -1)
        {
            delete tab;
            return;
        }

        isNewTab = true;
    }
    else // same search tab exists - reuse it, just update results
    {
        TCITEM tci  = {0};
        tci.mask    = TCIF_PARAM;
        tci.lParam  = (LPARAM)tab;

        if (!TabCtrl_SetItem(_hTab, --i, &tci))
        {
            TabCtrl_DeleteItem(_hTab, i);
            delete tab;
            tab = NULL;
        }
    }

    if (tab == NULL)
    {
        if (_activeTab)
            return;

        int cnt = TabCtrl_GetItemCount(_hTab);
        if (cnt == 0)
        {
            closeAllTabs();
            return;
        }

        if (i > cnt - 1)
            i = cnt - 1;
        tab = getTab(i);
    }

    TabCtrl_SetCurSel(_hTab, i);
    loadTab(tab, isNewTab);

    showWindow(hFocus);
}


/**
 *  \brief
 */
void ResultWin::close(const CmdPtr_t& cmd)
{
    int count = TabCtrl_GetItemCount(_hTab);
    if (count == 0)
        return;

    CmdId_t cmdId       = cmd->Id();
    CTextA  projectPath = cmd->Db()->GetPath().C_str();
    CTextA  search      = cmd->Tag().C_str();

    for (int i = count; i; --i)
    {
        Tab* oldTab = getTab(i - 1);

        if (oldTab && ((cmdId == oldTab->_cmdId) ||
            ((cmdId == FIND_SYMBOL) && ((oldTab->_cmdId == FIND_DEFINITION) || (oldTab->_cmdId == FIND_REFERENCE)))) &&
            (projectPath == oldTab->_projectPath) && (search == oldTab->_search)) // same search tab already present?
        {
            --i;

            if (_activeTab == oldTab) // is this the currently active tab?
                _activeTab = NULL;
            delete oldTab;

            TabCtrl_DeleteItem(_hTab, i);

            if (--count)
            {
                i = i ? i - 1 : 0;
                Tab* tab = getTab(i);
                TabCtrl_SetCurSel(_hTab, i);
                if (tab)
                    loadTab(tab);
            }
            else
            {
                sendSci(SCI_SETREADONLY, 0);
                sendSci(SCI_CLEARALL);
                sendSci(SCI_SETREADONLY, 1);

                hideWindow();
            }

            return;
        }
    }
}


/**
 *  \brief
 */
void ResultWin::reRunCmd()
{
    if (!_activeTab)
        return;

    DbHandle db = getDatabaseAt(CPath(_activeTab->_projectPath.C_str()));
    if (!db)
        return;

    CmdPtr_t cmd(new Cmd(_activeTab->_cmdId, db, _activeTab->_parser, NULL,
            _activeTab->_ignoreCase, _activeTab->_regExp));

    cmd->Tag(CText(_activeTab->_search.C_str()));
    CmdEngine::Run(cmd, showResultCB);

    _activeTab->_dirty = false;
}


/**
 *  \brief
 */
void ResultWin::applyStyle()
{
    INpp& npp = INpp::Get();

    char font[32];
    npp.GetFontName(STYLE_DEFAULT, font);
    int size = npp.GetFontSize(STYLE_DEFAULT);
    COLORREF caretLineBackColor = npp.GetCaretLineBack();
    COLORREF foreColor = npp.GetForegroundColor(STYLE_DEFAULT);
    COLORREF backColor = npp.GetBackgroundColor(STYLE_DEFAULT);
    COLORREF lineNumColor = npp.GetForegroundColor(STYLE_LINENUMBER);

    COLORREF fileForeColor =
            RGB(GetRValue(backColor) ^ 0xFF, GetGValue(backColor) ^ 0x7F, GetBValue(backColor) ^ 0x7F);
    COLORREF findForeColor =
            RGB(GetRValue(backColor) ^ 0x1C, GetGValue(backColor) ^ 0xFF, GetBValue(backColor) ^ 0xFF);

    HFONT hFont = Tools::CreateFromSystemMenuFont();
    if (hFont)
        SendMessage(_hTab, WM_SETFONT, (WPARAM)hFont, TRUE);

    sendSci(SCI_STYLERESETDEFAULT);
    setStyle(STYLE_DEFAULT, foreColor, backColor, false, false, size, font);
    sendSci(SCI_STYLECLEARALL);

    setStyle(SCE_GTAGS_HEADER, foreColor, backColor, true);
    setStyle(SCE_GTAGS_PROJECT_PATH, foreColor, backColor, true, true);
    setStyle(SCE_GTAGS_FILE, fileForeColor, backColor, true);
    setStyle(SCE_GTAGS_LINE_NUM, lineNumColor, backColor, false, true);
    setStyle(SCE_GTAGS_WORD2SEARCH, findForeColor, backColor, true);

    sendSci(SCI_SETCARETLINEBACK, caretLineBackColor);
    sendSci(SCI_STYLESETHOTSPOT, SCE_GTAGS_WORD2SEARCH, true);
}


/**
 *  \brief
 */
void ResultWin::notifyDBUpdate(const CmdPtr_t& cmd)
{
    // TODO: Hardcoded to true for now. Maybe make it a setting in the future
    const bool alwaysUpdate = true || (cmd->Id() == CREATE_DATABASE);

    const CTextA tag(cmd->Tag().C_str());
    CTextA lastProjPath;

    std::string updatedFile;

    for (int i = TabCtrl_GetItemCount(_hTab); i; --i)
    {
        Tab* tab = getTab(i - 1);

        if (alwaysUpdate)
        {
            if (CPath(tab->_projectPath.C_str()) == cmd->Db()->GetPath())
                tab->_dirty = true;
        }
        else
        {
            if (!(lastProjPath == tab->_projectPath))
            {
                lastProjPath = tab->_projectPath;

                if (!(CPath(tab->_projectPath.C_str()) == cmd->Db()->GetPath()))
                    continue;

                if (tag.Len() <= tab->_projectPath.Len())
                    continue;

                updatedFile = tag.C_str() + tab->_projectPath.Len();

                size_t j = 0;
                while ((j = updatedFile.find('\\', j)) != std::string::npos)
                    updatedFile[j] = '/';
            }

            const TabParser* parser = dynamic_cast<TabParser*>(tab->_parser.get());

            if (parser->isFileInResults(updatedFile))
                tab->_dirty = true;
        }
    }

    if (_activeTab && _activeTab->_dirty)
        reRunCmd();
}


/**
 *  \brief
 */
ResultWin::~ResultWin()
{
    if (_hWnd)
    {
        closeAllTabs();

        if (_hSci)
        {
            INpp::Get().DestroySciHandle(_hSci);
            _hSci = NULL;
        }

        SendMessage(_hWnd, WM_CLOSE, 0, 0);
    }
}


/**
 *  \brief
 */
void ResultWin::setStyle(int style, COLORREF fore, COLORREF back, bool bold, bool italic, int size, const char *font)
{
    sendSci(SCI_STYLESETEOLFILLED, style, 1);
    sendSci(SCI_STYLESETFORE, style, fore);
    sendSci(SCI_STYLESETBACK, style, back);
    sendSci(SCI_STYLESETBOLD, style, bold);
    sendSci(SCI_STYLESETITALIC, style, italic);

    if (size >= 1)
        sendSci(SCI_STYLESETSIZE, style, size);
    if (font)
        sendSci(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(font));
}


/**
 *  \brief
 */
void ResultWin::configScintilla()
{
    sendSci(SCI_SETCODEPAGE, SC_CP_UTF8);
    sendSci(SCI_SETEOLMODE, SC_EOL_CRLF);
    sendSci(SCI_USEPOPUP, false);
    sendSci(SCI_SETUNDOCOLLECTION, false);
    sendSci(SCI_SETCARETSTYLE, CARETSTYLE_LINE);
    sendSci(SCI_SETCARETLINEVISIBLE, true);
    sendSci(SCI_SETCARETLINEVISIBLEALWAYS, true);
    sendSci(SCI_SETHOTSPOTACTIVEUNDERLINE, false);

    sendSci(SCI_SETWRAPMODE, SC_WRAP_WORD);
    sendSci(SCI_SETWRAPVISUALFLAGS, SC_WRAPVISUALFLAG_START);
    sendSci(SCI_SETWRAPVISUALFLAGSLOCATION, SC_WRAPVISUALFLAGLOC_DEFAULT);
    sendSci(SCI_SETWRAPINDENTMODE, SC_WRAPINDENT_FIXED);
    sendSci(SCI_SETWRAPSTARTINDENT, 24);
    sendSci(SCI_SETLAYOUTCACHE, SC_CACHE_CARET);

    // Implement lexer in the container
    sendSci(SCI_SETLEXER, 0);

    ApplyStyle();

    sendSci(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<LPARAM>("1"));

    sendSci(SCI_SETMARGINTYPEN, 1, SC_MARGIN_SYMBOL);
    sendSci(SCI_SETMARGINMASKN, 1, SC_MASK_FOLDERS);
    sendSci(SCI_SETMARGINWIDTHN, 1, 20);
    sendSci(SCI_SETFOLDMARGINCOLOUR, 1, cBlack);
    sendSci(SCI_SETFOLDMARGINHICOLOUR, 1, cBlack);
    sendSci(SCI_SETMARGINSENSITIVEN, 1, true);
    sendSci(SCI_SETFOLDFLAGS, 0);

    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
}


/**
 *  \brief
 */
HWND ResultWin::composeWindow()
{
    INpp& npp = INpp::Get();
    HWND hOwner = npp.GetHandle();

    DWORD style = WS_POPUP | WS_CAPTION | WS_SIZEBOX | WS_CLIPCHILDREN;

    _hWnd = CreateWindow(cClassName, cPluginName, style,
            0, 0, 10, 10, hOwner, NULL, HMod, NULL);
    if (_hWnd == NULL)
        return NULL;

    _hSci = npp.CreateSciHandle(_hWnd);
    if (_hSci)
    {
        _sciFunc    = (SciFnDirect)::SendMessage(_hSci, SCI_GETDIRECTFUNCTION, 0, 0);
        _sciPtr     = (sptr_t)::SendMessage(_hSci, SCI_GETDIRECTPOINTER, 0, 0);
    }

    if (_hSci == NULL || _sciFunc == NULL || _sciPtr == 0)
    {
        SendMessage(_hWnd, WM_CLOSE, 0, 0);
        _hWnd = NULL;
        _hSci = NULL;
        return NULL;
    }

    _hTab = CreateWindowEx(0, WC_TABCONTROL, NULL,
            WS_CHILD | WS_VISIBLE | TCS_BUTTONS | TCS_FOCUSNEVER,
            0, 0, 10, 10, _hWnd, NULL, HMod, NULL);

    configScintilla();

    ShowWindow(_hSci, SW_SHOWNORMAL);

    return _hWnd;
}


/**
 *  \brief
 */
void ResultWin::createSearchWindow()
{
    if (_hSearch)
    {
        SetFocus(_hSearchTxt);
        Edit_SetSel(_hSearchTxt, 0, -1);
        return;
    }

    const DWORD styleEx = WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW;
    const DWORD style   = WS_POPUP | WS_CAPTION;

    CreateWindowEx(styleEx, cSearchClassName, _T("Search in results"), style, 0, 0, 10, 10, _hWnd, NULL, HMod, NULL);
}


/**
 *  \brief
 */
void ResultWin::onSearchWindowCreate(HWND hWnd)
{
    _hSearch = hWnd;

    HDC hdc = GetWindowDC(_hSearch);

    _hSearchFont    = Tools::CreateFromSystemMessageFont(hdc, cSearchFontSize);
    _hBtnFont       = Tools::CreateFromSystemMenuFont(hdc, cSearchFontSize - 1);

    _searchTxtHeight    = Tools::GetFontHeight(hdc, _hSearchFont) + 1;
    int btnHeight       = Tools::GetFontHeight(hdc, _hBtnFont) + 2;

    ReleaseDC(_hSearch, hdc);

    const DWORD styleTxtEx  = WS_EX_CLIENTEDGE;
    const DWORD styleTxt    = WS_CHILD | WS_VISIBLE | WS_HSCROLL | ES_AUTOHSCROLL;

    {
        RECT win { 0, 0, cSearchWidth, _searchTxtHeight };
        AdjustWindowRectEx(&win, styleTxt, FALSE, styleTxtEx);
        _searchTxtHeight = win.bottom - win.top;
    }

    RECT win = Tools::GetWinRect(_hWnd,
            (DWORD)GetWindowLongPtr(_hSearch, GWL_EXSTYLE), (DWORD)GetWindowLongPtr(_hSearch, GWL_STYLE),
            cSearchWidth + 1, _searchTxtHeight + btnHeight + 1);
    RECT ownerWin;
    GetWindowRect(_hWnd, &ownerWin);

    MoveWindow(_hSearch,
            ownerWin.right - (win.right - win.left) -
            GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXFIXEDFRAME) - GetSystemMetrics(SM_CXVSCROLL),
            ownerWin.top +
            GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYSIZE),
            win.right - win.left, win.bottom - win.top, TRUE);

    GetClientRect(_hSearch, &win);

    int btnWidth = (win.right - win.left - 120) / 3;

    _hRE = CreateWindowEx(0, _T("BUTTON"), _T("RegExp"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            1, 0, btnWidth, btnHeight,
            _hSearch, NULL, HMod, NULL);

    _hIC = CreateWindowEx(0, _T("BUTTON"), _T("IgnoreCase"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            btnWidth + 1, 0, btnWidth, btnHeight,
            _hSearch, NULL, HMod, NULL);

    _hWW = CreateWindowEx(0, _T("BUTTON"), _T("WholeWord"),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            2 * btnWidth + 1, 0, btnWidth, btnHeight,
            _hSearch, NULL, HMod, NULL);

    _hUp = CreateWindowEx(0, _T("BUTTON"), _T("Up"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            3 * btnWidth + 1, 0, 60, btnHeight,
            _hSearch, NULL, HMod, NULL);

    _hDown = CreateWindowEx(0, _T("BUTTON"), _T("Down"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            3 * btnWidth + 61, 0, 60, btnHeight,
            _hSearch, NULL, HMod, NULL);

    if (_hBtnFont)
    {
        SendMessage(_hRE, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
        SendMessage(_hIC, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
        SendMessage(_hWW, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
        SendMessage(_hUp, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
        SendMessage(_hDown, WM_SETFONT, (WPARAM)_hBtnFont, TRUE);
    }

    Button_SetCheck(_hRE, _lastRE ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(_hIC, _lastIC ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(_hWW, _lastWW ? BST_CHECKED : BST_UNCHECKED);

    _hSearchTxt = CreateWindowEx(styleTxtEx, RICHEDIT_CLASS, NULL, styleTxt,
            0, btnHeight + 1, win.right - win.left, _searchTxtHeight,
            _hSearch, NULL, HMod, NULL);

    SendMessage(_hSearchTxt, EM_SETBKGNDCOLOR, 0, GetSysColor(cSearchBkgndColor));

    if (_hSearchFont)
        SendMessage(_hSearchTxt, WM_SETFONT, (WPARAM)_hSearchFont, TRUE);

    SendMessage(_hSearchTxt, EM_SETEVENTMASK, 0, ENM_REQUESTRESIZE);

    INpp& npp = INpp::Get();

    HWND nppHSci = npp.ReadSciHandle();

    npp.SetSciHandle(_hSci);

    CTextA selectionA;
    if (!npp.IsSelectionVertical())
        npp.GetSelection(selectionA);

    npp.SetSciHandle(nppHSci);

    if (!selectionA.IsEmpty())
    {
        const CText selection(selectionA.C_str());

        Edit_SetText(_hSearchTxt, selection.C_str());
        Edit_SetSel(_hSearchTxt, 0, -1);
    }
    else if (!_lastSearchTxt.IsEmpty())
    {
        Edit_SetText(_hSearchTxt, _lastSearchTxt.C_str());
        Edit_SetSel(_hSearchTxt, 0, -1);
    }
    else
    {
        Edit_SetText(_hSearchTxt, _T(""));
    }

    ShowWindow(_hSearch, SW_SHOWNORMAL);
    SetFocus(_hSearchTxt);
}


/**
 *  \brief
 */
void ResultWin::showWindow(HWND hFocus)
{
    INpp::Get().ShowDockingWin(_hWnd);

    ActivityWin::UpdatePositions();

    if (hFocus)
        SetFocus(hFocus);
    else
        SetFocus(_hWnd);
}


/**
 *  \brief
 */
void ResultWin::hideWindow()
{
    if (_hSearch)
        SendMessage(_hSearch, WM_CLOSE, 0, 0);

    if (_hKeyHook)
    {
        UnhookWindowsHookEx(_hKeyHook);
        _hKeyHook = NULL;
    }

    INpp& npp = INpp::Get();
    npp.HideDockingWin(_hWnd);

    ActivityWin::UpdatePositions();

    if (GetFocus() != npp.ReadSciHandle())
        SetFocus(npp.GetSciHandle());
}


/**
 *  \brief
 */
void ResultWin::releaseKeys()
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
    inputs[3].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}


/**
 *  \brief
 */
ResultWin::Tab* ResultWin::getTab(int i)
{
    if (i == -1)
    {
        i = TabCtrl_GetCurSel(_hTab);
        if (i == -1)
            return NULL;
    }

    TCITEM tci  = {0};
    tci.mask    = TCIF_PARAM;

    if (!TabCtrl_GetItem(_hTab, i, &tci))
        return NULL;

    return (Tab*)tci.lParam;
}


/**
 *  \brief
 */
void ResultWin::loadTab(ResultWin::Tab* tab, bool firstTimeLoad)
{
    // store current view if there is one
    if (_activeTab)
    {
        _activeTab->_currentLine = sendSci(SCI_LINEFROMPOSITION, sendSci(SCI_GETCURRENTPOS));
        _activeTab->_firstVisibleLine = sendSci(SCI_GETFIRSTVISIBLELINE);
    }

    _activeTab = NULL;

    sendSci(SCI_SETREADONLY, 0);
    sendSci(SCI_CLEARALL);

    _activeTab = tab;

    sendSci(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(tab->_parser->GetText().C_str()));
    sendSci(SCI_SETREADONLY, 1);

    sendSci(SCI_GOTOLINE, tab->_currentLine);
    sendSci(SCI_SETFIRSTVISIBLELINE, tab->_firstVisibleLine);

    if (tab->_dirty)
    {
        reRunCmd();
    }
    else if (firstTimeLoad && tab->_cmdId != FIND_FILE)
    {
        const TabParser* parser = dynamic_cast<TabParser*>(tab->_parser.get());

        if (parser->getFilesCount() == 1)
            foldAll(SC_FOLDACTION_EXPAND);
    }
}


/**
 *  \brief
 */
bool ResultWin::visitSingleResult(ResultWin::Tab* tab)
{
    const TabParser* parser = dynamic_cast<TabParser*>(tab->_parser.get());

    if (parser->getHitsCount() != 1)
        return false;

    releaseKeys();

    intptr_t line = -1;

    if (tab->_cmdId != FIND_FILE)
    {
        const std::string results(parser->GetText().C_str());

        const size_t pos = results.rfind("\t\tline ");
        if (pos != std::string::npos)
        {
            const std::string lineNum = results.substr(pos + 7);
            line = std::stoll(lineNum) - 1;
        }
    }

    CPath file = tab->_projectPath.C_str();
    file += parser->getFirstFileResult().c_str();

    INpp& npp = INpp::Get();
    if (!file.FileExists())
    {
        MessageBox(npp.GetHandle(),
                _T("File not found, database seems outdated.")
                _T("\nPlease re-create it and redo the search."),
                cPluginName, MB_OK | MB_ICONEXCLAMATION);
        return true;
    }

    DocLocation::Get().Push();
    npp.OpenFile(file.C_str());
    UpdateWindow(npp.GetHandle());

    if (tab->_cmdId == FIND_FILE)
        return true;

    const bool wholeWord    = (tab->_cmdId != GREP && tab->_cmdId != GREP_TEXT);
    intptr_t findBegin      = npp.PositionFromLine(line);
    intptr_t findEnd        = npp.LineEndPosition(line);

    if (!npp.SearchText(tab->_search.C_str(), tab->_ignoreCase, wholeWord, tab->_regExp, &findBegin, &findEnd))
    {
        MessageBox(npp.GetHandle(),
                _T("Look-up mismatch, database seems outdated.")
                _T("\nSave all modified files, re-create it and redo the search."),
                cPluginName, MB_OK | MB_ICONEXCLAMATION);
    }

    return true;
}


/**
 *  \brief
 */
bool ResultWin::openItem(intptr_t lineNum, unsigned matchNum)
{
    releaseKeys();

    intptr_t lineLen = sendSci(SCI_GETCURRENTPOS, 0, 0);
    sendSci(SCI_SETSEL, lineLen, lineLen);

    lineLen = sendSci(SCI_LINELENGTH, lineNum);
    if (lineLen <= 0)
        return false;

    std::vector<char> lineTxt;
    lineTxt.resize(lineLen + 1, 0);

    sendSci(SCI_GETLINE, lineNum, reinterpret_cast<LPARAM>(lineTxt.data()));

    intptr_t line = 0;
    intptr_t i;

    if (_activeTab->_cmdId != FIND_FILE)
    {
        for (i = 7; i <= lineLen && lineTxt[i] != ':'; ++i);
        lineTxt[i] = 0;
        line = atoll(&lineTxt[7]) - 1;

        lineNum = sendSci(SCI_GETFOLDPARENT, lineNum);
        if (lineNum == -1)
            return false;

        lineLen = sendSci(SCI_LINELENGTH, lineNum);
        if (lineLen <= 0)
            return false;

        lineTxt.resize(lineLen + 1, 0);
        sendSci(SCI_GETLINE, lineNum, reinterpret_cast<LPARAM>(lineTxt.data()));
    }

    for (i = 1; (i <= lineLen) && (lineTxt[i] != '\r') && (lineTxt[i] != '\n'); ++i);
    lineTxt[i] = 0;

    CPath file;

    // Path is not absolute (does not start with drive letter)
    if ((lineLen < 5) || (lineTxt[2] != ':'))
        file = _activeTab->_projectPath.C_str();

    file += &lineTxt[1];

    INpp& npp = INpp::Get();
    if (!file.FileExists())
    {
        MessageBox(npp.GetHandle(),
                _T("File not found, present results are outdated.")
                _T("\nPlease re-create database and redo the search."),
                cPluginName, MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    DocLocation::Get().Push();
    npp.OpenFile(file.C_str());
    UpdateWindow(npp.GetHandle());

    if (_activeTab->_cmdId == FIND_FILE)
        return true;

    const intptr_t endPos = npp.LineEndPosition(line);

    const bool wholeWord = (_activeTab->_cmdId != GREP && _activeTab->_cmdId != GREP_TEXT);

    // Highlight the corresponding match number if there are more than one
    // matches on single result line
    for (intptr_t findBegin = npp.PositionFromLine(line), findEnd = endPos;
        matchNum; findBegin = findEnd, findEnd = endPos, --matchNum)
    {
        if (!npp.SearchText(_activeTab->_search.C_str(), _activeTab->_ignoreCase, wholeWord, _activeTab->_regExp,
                &findBegin, &findEnd))
        {
            MessageBox(npp.GetHandle(),
                    _T("Look-up mismatch, present results are outdated.")
                    _T("\nSave all modified files and redo the search."),
                    cPluginName, MB_OK | MB_ICONEXCLAMATION);
            return false;
        }
    }

    return true;
}


/**
 *  \brief
 */
bool ResultWin::findString(const char* str, intptr_t* startPos, intptr_t* endPos,
    bool ignoreCase, bool wholeWord, bool regExp)
{
    int searchFlags = 0;
    if (!ignoreCase)
        searchFlags |= SCFIND_MATCHCASE;
    if (wholeWord)
        searchFlags |= SCFIND_WHOLEWORD;
    if (regExp)
        searchFlags |= (SCFIND_REGEXP | SCFIND_POSIX);

    sendSci(SCI_SETSEARCHFLAGS, searchFlags);
    sendSci(SCI_SETTARGETSTART, *startPos);
    sendSci(SCI_SETTARGETEND, *endPos);

    if (sendSci(SCI_SEARCHINTARGET, strlen(str), reinterpret_cast<LPARAM>(str)) >= 0)
    {
        *startPos = sendSci(SCI_GETTARGETSTART);
        *endPos = sendSci(SCI_GETTARGETEND);
        return true;
    }

    return false;
}


/**
 *  \brief
 */
void ResultWin::toggleFolding(intptr_t lineNum)
{
    sendSci(SCI_GOTOLINE, lineNum);
    sendSci(SCI_TOGGLEFOLD, lineNum);

    if (sendSci(SCI_GETFOLDEXPANDED, lineNum))
        _activeTab->ClearFolded(lineNum);
    else
        _activeTab->SetFolded(lineNum);
}


/**
 *  \brief
 */
void ResultWin::foldAll(int foldAction)
{
    sendSci(SCI_FOLDALL, foldAction);

    const intptr_t linesCount = sendSci(SCI_GETLINECOUNT);

    intptr_t lineNum = 1;
    for (; lineNum < linesCount && !(sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG); ++lineNum);

    if (lineNum == linesCount)
        return;

    if (sendSci(SCI_GETFOLDEXPANDED, lineNum))
    {
        for (; lineNum < linesCount; ++lineNum)
        {
            if (sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG)
            {
                _activeTab->ClearFolded(lineNum);
            }
        }
    }
    else
    {
        _activeTab->SetAllFolded();
    }
}


/**
 *  \brief
 */
void ResultWin::onStyleNeeded(SCNotification* notify)
{
    if (_activeTab == NULL)
        return;

    intptr_t lineNum = sendSci(SCI_LINEFROMPOSITION, sendSci(SCI_GETENDSTYLED));
    const intptr_t endStylingPos = notify->position;

    for (intptr_t startPos = sendSci(SCI_POSITIONFROMLINE, lineNum); endStylingPos > startPos;
            startPos = sendSci(SCI_POSITIONFROMLINE, ++lineNum))
    {
        const intptr_t lineLen = sendSci(SCI_LINELENGTH, lineNum);
        if (lineLen <= 0)
            continue;

        const intptr_t endPos = startPos + lineLen;

        sendSci(SCI_STARTSTYLING, startPos, 0xFF);

        if ((char)sendSci(SCI_GETCHARAT, startPos) != '\t')
        {
            size_t pathLen = _activeTab->_projectPath.Len();
            const TabParser* parser = dynamic_cast<TabParser*>(_activeTab->_parser.get());

            // 2 * '"' + LF + CR = 4 so length to style is modified with -4 and +1 (as it is length)
            sendSci(SCI_SETSTYLING, lineLen - pathLen - parser->getHeaderStatusLen() - 3, SCE_GTAGS_HEADER);
            sendSci(SCI_SETSTYLING, pathLen + 2, SCE_GTAGS_PROJECT_PATH);
            sendSci(SCI_SETSTYLING, parser->getHeaderStatusLen(), SCE_GTAGS_HEADER);
        }
        else
        {
            if ((char)sendSci(SCI_GETCHARAT, startPos + 1) != '\t')
            {
                if (_activeTab->_cmdId == FIND_FILE)
                {
                    intptr_t findBegin = startPos;
                    intptr_t findEnd = endPos;

                    if (findString(_activeTab->_search.C_str(), &findBegin, &findEnd,
                        _activeTab->_ignoreCase, false, _activeTab->_regExp))
                    {
                        // Highlight all matches in a single result line
                        do
                        {
                            if (findBegin - startPos)
                                sendSci(SCI_SETSTYLING, findBegin - startPos, SCE_GTAGS_FILE);

                            sendSci(SCI_SETSTYLING, findEnd - findBegin, SCE_GTAGS_WORD2SEARCH);

                            findBegin = startPos = findEnd;
                            findEnd = endPos;
                        }
                        while (findString(_activeTab->_search.C_str(), &findBegin, &findEnd,
                                _activeTab->_ignoreCase, false, _activeTab->_regExp));

                        if (endPos - startPos)
                            sendSci(SCI_SETSTYLING, endPos - startPos, SCE_GTAGS_FILE);
                    }
                    else
                    {
                        sendSci(SCI_SETSTYLING, lineLen, SCE_GTAGS_FILE);
                    }
                }
                else
                {
                    sendSci(SCI_SETSTYLING, lineLen, SCE_GTAGS_FILE);
                    sendSci(SCI_SETFOLDLEVEL, lineNum, FILE_HEADER_LVL | SC_FOLDLEVELHEADERFLAG);

                    if (_activeTab->IsFolded(lineNum))
                        sendSci(SCI_FOLDLINE, lineNum, SC_FOLDACTION_CONTRACT);
                }
            }
            else
            {
                // "\t\tline: Num" - 'N' is at position 8
                intptr_t previewPos = startPos + 8;
                for (; (char)sendSci(SCI_GETCHARAT, previewPos) != '\t'; ++previewPos);

                intptr_t findBegin = previewPos;
                intptr_t findEnd = endPos;

                const bool wholeWord = (_activeTab->_cmdId != GREP && _activeTab->_cmdId != GREP_TEXT);

                if (findString(_activeTab->_search.C_str(), &findBegin, &findEnd,
                    _activeTab->_ignoreCase, wholeWord, _activeTab->_regExp))
                {
                    sendSci(SCI_SETSTYLING, previewPos - startPos, SCE_GTAGS_LINE_NUM);

                    // Highlight all matches in a single result line
                    do
                    {
                        if (findBegin - previewPos)
                            sendSci(SCI_SETSTYLING, findBegin - previewPos, STYLE_DEFAULT);

                        sendSci(SCI_SETSTYLING, findEnd - findBegin, SCE_GTAGS_WORD2SEARCH);

                        findBegin = previewPos = findEnd;
                        findEnd = endPos;
                    }
                    while (findString(_activeTab->_search.C_str(), &findBegin, &findEnd,
                            _activeTab->_ignoreCase, wholeWord, _activeTab->_regExp));

                    if (endPos - previewPos)
                        sendSci(SCI_SETSTYLING, endPos - previewPos, STYLE_DEFAULT);
                }
                else
                {
                    sendSci(SCI_SETSTYLING, lineLen, STYLE_DEFAULT);
                }

                sendSci(SCI_SETFOLDLEVEL, lineNum, RESULT_LVL);
            }
        }
    }
}


/**
 *  \brief
 */
void ResultWin::onNewPosition()
{
    const intptr_t pos = sendSci(SCI_GETCURRENTPOS);

    if (pos == sendSci(SCI_POSITIONAFTER, pos)) // end of document
    {
        const intptr_t foldLine = sendSci(SCI_GETFOLDPARENT, sendSci(SCI_LINEFROMPOSITION, pos));
        if (!sendSci(SCI_GETFOLDEXPANDED, foldLine))
            sendSci(SCI_GOTOLINE, foldLine);
    }
}


/**
 *  \brief
 */
void ResultWin::onHotspotClick(SCNotification* notify)
{
    const intptr_t lineNum = sendSci(SCI_LINEFROMPOSITION, notify->position);
    unsigned matchNum = 1;

    if (_activeTab->_cmdId != FIND_FILE)
    {
        const intptr_t endLine = sendSci(SCI_GETLINEENDPOSITION, lineNum);

        // "\t\tline: Num" - 'N' is at position 8
        intptr_t findBegin = sendSci(SCI_POSITIONFROMLINE, lineNum) + 8;
        for (; (char)sendSci(SCI_GETCHARAT, findBegin) != '\t'; ++findBegin);

        const bool wholeWord = (_activeTab->_cmdId != GREP && _activeTab->_cmdId != GREP_TEXT);

        // Find which hotspot was clicked in case there are more than one
        // matches on single result line
        for (intptr_t findEnd = endLine; findString(_activeTab->_search.C_str(), &findBegin, &findEnd,
                    _activeTab->_ignoreCase, wholeWord, _activeTab->_regExp);
                findBegin = findEnd, findEnd = endLine, ++matchNum)
            if (notify->position >= findBegin && notify->position <= findEnd)
                break;
    }

    openItem(lineNum, matchNum);

    // Clear false selection in results win when visiting result location
    sendSci(SCI_GOTOPOS, notify->position);
}


/**
 *  \brief
 */
void ResultWin::onDoubleClick(intptr_t pos)
{
    intptr_t lineNum = sendSci(SCI_LINEFROMPOSITION, pos);

    if (lineNum == 0)
    {
        pos = sendSci(SCI_GETCURRENTPOS);
        if (pos == sendSci(SCI_POSITIONAFTER, pos)) // end of document
        {
            lineNum = sendSci(SCI_LINEFROMPOSITION, pos);
            const intptr_t foldLine = sendSci(SCI_GETFOLDPARENT, lineNum);
            if (!sendSci(SCI_GETFOLDEXPANDED, foldLine))
            {
                lineNum = foldLine;
                pos = sendSci(SCI_POSITIONFROMLINE, lineNum);
            }
        }
        else
        {
            lineNum = sendSci(SCI_LINEFROMPOSITION, pos);
        }
    }

    if (lineNum == 0)
    {
        sendSci(SCI_GOTOLINE, 0); // Clear double click selection
        foldAll(SC_FOLDACTION_TOGGLE);
    }
    else if (lineNum > 0)
    {
        if (sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG)
            toggleFolding(lineNum);
        else
            openItem(lineNum);
    }
}


/**
 *  \brief
 */
void ResultWin::onMarginClick(SCNotification* notify)
{
    intptr_t lineNum = sendSci(SCI_LINEFROMPOSITION, notify->position);

    if (lineNum == 0)
    {
        foldAll(SC_FOLDACTION_TOGGLE);
    }
    else
    {
        if (!(sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG))
            lineNum = sendSci(SCI_GETFOLDPARENT, lineNum);

        if (lineNum > 0)
        {
            toggleFolding(lineNum);
        }
        else
        {
            lineNum = sendSci(SCI_LINEFROMPOSITION, notify->position);
            sendSci(SCI_GOTOLINE, lineNum);
        }
    }
}


/**
 *  \brief
 */
bool ResultWin::onKeyPress(WORD keyCode, bool alt)
{
    const intptr_t currentPos = sendSci(SCI_GETCURRENTPOS);
    intptr_t lineNum = sendSci(SCI_LINEFROMPOSITION, currentPos);

    switch (keyCode)
    {
        case VK_UP:
        {
            intptr_t linePosOffset = currentPos - sendSci(SCI_POSITIONFROMLINE, lineNum);

            if (--lineNum >= 0)
            {
                if (!(sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG))
                {
                    const intptr_t foldLine = sendSci(SCI_GETFOLDPARENT, lineNum);
                    if (!sendSci(SCI_GETFOLDEXPANDED, foldLine))
                        lineNum = foldLine;
                }

                const intptr_t lineStart = sendSci(SCI_POSITIONFROMLINE, lineNum);
                const intptr_t lineEnd = sendSci(SCI_GETLINEENDPOSITION, lineNum);

                if (linePosOffset > lineEnd - lineStart)
                    linePosOffset = lineEnd - lineStart;

                sendSci(SCI_GOTOPOS, lineStart + linePosOffset);
            }
        }
        return true;

        case VK_DOWN:
        {
            intptr_t linePosOffset = currentPos - sendSci(SCI_POSITIONFROMLINE, lineNum);

            if (++lineNum < sendSci(SCI_GETLINECOUNT))
            {
                if (!(sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG))
                {
                    const intptr_t foldLine = sendSci(SCI_GETFOLDPARENT, lineNum);
                    if (!sendSci(SCI_GETFOLDEXPANDED, foldLine))
                    {
                        const intptr_t nextFold =
                                sendSci(SCI_GETLASTCHILD, foldLine, sendSci(SCI_GETFOLDLEVEL, foldLine)) + 1;
                        lineNum = (nextFold < sendSci(SCI_GETLINECOUNT)) ? nextFold : foldLine;
                    }
                }

                const intptr_t lineStart = sendSci(SCI_POSITIONFROMLINE, lineNum);
                const intptr_t lineEnd = sendSci(SCI_GETLINEENDPOSITION, lineNum);

                if (linePosOffset > lineEnd - lineStart)
                    linePosOffset = lineEnd - lineStart;

                sendSci(SCI_GOTOPOS, lineStart + linePosOffset);
            }
        }
        return true;

        case VK_LEFT:
            if (alt)
            {
                int i = TabCtrl_GetCurSel(_hTab);

                if (i > 0)
                {
                    Tab* tab = getTab(--i);
                    if (tab)
                    {
                        TabCtrl_SetCurSel(_hTab, i);
                        loadTab(tab);
                    }
                }

                return true;
            }
        break;

        case VK_RIGHT:
            if (alt)
            {
                int i = TabCtrl_GetCurSel(_hTab);

                if (i < TabCtrl_GetItemCount(_hTab) - 1)
                {
                    Tab* tab = getTab(++i);
                    if (tab)
                    {
                        TabCtrl_SetCurSel(_hTab, i);
                        loadTab(tab);
                    }
                }

                return true;
            }
        break;

        case VK_PRIOR:
        {
            const intptr_t linesOnScreen = sendSci(SCI_LINESONSCREEN);
            sendSci(SCI_LINESCROLL, 0, -linesOnScreen);
            lineNum = sendSci(SCI_DOCLINEFROMVISIBLE, sendSci(SCI_GETFIRSTVISIBLELINE));
            sendSci(SCI_GOTOLINE, lineNum);
        }
        return true;

        case VK_NEXT:
        {
            const intptr_t linesOnScreen     = sendSci(SCI_LINESONSCREEN);
            const intptr_t firstVisibleLine  = sendSci(SCI_GETFIRSTVISIBLELINE);
            sendSci(SCI_LINESCROLL, 0, linesOnScreen);
            const intptr_t newFirstVisible   = sendSci(SCI_GETFIRSTVISIBLELINE);

            if (newFirstVisible - firstVisibleLine >= linesOnScreen)
            {
                lineNum = sendSci(SCI_DOCLINEFROMVISIBLE, newFirstVisible);
            }
            else
            {
                lineNum = sendSci(SCI_GETLINECOUNT) - 1;
                const intptr_t foldLine = sendSci(SCI_GETFOLDPARENT, lineNum);
                if (!sendSci(SCI_GETFOLDEXPANDED, foldLine))
                    lineNum = foldLine;
            }

            sendSci(SCI_GOTOLINE, lineNum);
        }
        return true;

        case VK_ADD:
            if (alt || lineNum == 0)
            {
                foldAll(SC_FOLDACTION_EXPAND);
            }
            else
            {
                if (!(sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG))
                    lineNum = sendSci(SCI_GETFOLDPARENT, lineNum);
                if (lineNum > 0 && !sendSci(SCI_GETFOLDEXPANDED, lineNum))
                    toggleFolding(lineNum);
            }
        return true;

        case VK_SUBTRACT:
            if (alt || lineNum == 0)
            {
                if (!(sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG) && lineNum)
                {
                    const intptr_t foldLine = sendSci(SCI_GETFOLDPARENT, lineNum);
                    if (foldLine > 0)
                        sendSci(SCI_GOTOLINE, foldLine);
                }
                foldAll(SC_FOLDACTION_CONTRACT);
            }
            else
            {
                if (!(sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG))
                    lineNum = sendSci(SCI_GETFOLDPARENT, lineNum);
                if (lineNum > 0 && sendSci(SCI_GETFOLDEXPANDED, lineNum))
                    toggleFolding(lineNum);
            }
        return true;

        default:;
    }

    return false;
}


/**
 *  \brief
 */
void ResultWin::onTabChange()
{
    Tab* tab = getTab();
    if (tab)
        loadTab(tab);
}


/**
 *  \brief
 */
void ResultWin::onCloseTab()
{
    int i = TabCtrl_GetCurSel(_hTab);

    if (_activeTab)
    {
        delete _activeTab;
        _activeTab = NULL;
    }

    TabCtrl_DeleteItem(_hTab, i);

    if (TabCtrl_GetItemCount(_hTab))
    {
        i = i ? i - 1 : 0;
        Tab* tab = getTab(i);
        TabCtrl_SetCurSel(_hTab, i);
        if (tab)
            loadTab(tab);
    }
    else
    {
        sendSci(SCI_SETREADONLY, 0);
        sendSci(SCI_CLEARALL);
        sendSci(SCI_SETREADONLY, 1);

        hideWindow();
    }
}


/**
 *  \brief
 */
void ResultWin::closeAllTabs()
{
    _activeTab = NULL;

    for (int i = TabCtrl_GetItemCount(_hTab); i; --i)
    {
        Tab* tab = getTab(i - 1);
        if (tab)
            delete tab;
        TabCtrl_DeleteItem(_hTab, i - 1);
    }

    sendSci(SCI_SETREADONLY, 0);
    sendSci(SCI_CLEARALL);
    sendSci(SCI_SETREADONLY, 1);

    hideWindow();
}


/**
 *  \brief
 */
void ResultWin::onResize(int width, int height)
{
    RECT win = {0, 0, width, height};

    MoveWindow(_hTab, 0, 0, width, height, TRUE);
    TabCtrl_AdjustRect(_hTab, FALSE, &win);
    MoveWindow(_hSci, win.left, win.top, win.right - win.left, win.bottom - win.top, TRUE);

    onMove();
}


/**
 *  \brief
 */
void ResultWin::onMove()
{
    if (_hSearch)
    {
        RECT ownerWin;
        GetWindowRect(_hWnd, &ownerWin);

        RECT win;
        GetWindowRect(_hSearch, &win);

        RECT txtWin;
        GetWindowRect(_hSearchTxt, &txtWin);

        const int hScrollSize =
                (GetWindowLongPtr(_hSearchTxt, GWL_STYLE) & WS_HSCROLL) ? GetSystemMetrics(SM_CYHSCROLL) : 0;

        MoveWindow(_hSearch,
                ownerWin.right - (win.right - win.left) -
                GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXFIXEDFRAME) - GetSystemMetrics(SM_CXVSCROLL),
                ownerWin.top +
                GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYSIZE),
                win.right - win.left,
                win.bottom - win.top - (txtWin.bottom - txtWin.top) + _searchTxtHeight + hScrollSize, TRUE);

        GetClientRect(_hSearch, &win);

        MoveWindow(_hSearchTxt,
                0, win.bottom - _searchTxtHeight - hScrollSize,
                win.right - win.left, _searchTxtHeight + hScrollSize, TRUE);
    }
}


/**
 *  \brief
 */
void ResultWin::onSearch(bool reverseDir, bool keepFocus)
{
    if (_hSearch)
    {
        _lastRE = (Button_GetCheck(_hRE) == BST_CHECKED);
        _lastIC = (Button_GetCheck(_hIC) == BST_CHECKED);
        _lastWW = (Button_GetCheck(_hWW) == BST_CHECKED);

        int txtLen = Edit_GetTextLength(_hSearchTxt);
        if (txtLen == 0)
        {
            SendMessage(_hSearch, WM_CLOSE, 0, 0);
            SetFocus(_hSci);
            return;
        }

        _lastSearchTxt.Resize(txtLen);
        Edit_GetText(_hSearchTxt, _lastSearchTxt.C_str(), (int)_lastSearchTxt.Size());
    }
    else if (_lastSearchTxt.IsEmpty())
    {
        return;
    }

    CTextA txt(_lastSearchTxt.C_str());

    const intptr_t docEnd = sendSci(SCI_GETLENGTH);

    intptr_t startPos = sendSci(SCI_GETCURRENTPOS);
    intptr_t endPos = docEnd;

    if (reverseDir)
    {
        if (startPos)
            startPos -= 1;
        else
            startPos = docEnd;

        endPos = 0;
    }

    bool found = findString(txt.C_str(), &startPos, &endPos, _lastIC, _lastWW, _lastRE);

    if (!found && ((!reverseDir && startPos) || (reverseDir && startPos != docEnd)))
    {
        startPos = reverseDir ? docEnd : 0;

        found = findString(txt.C_str(), &startPos, &endPos, _lastIC, _lastWW, _lastRE);

        FLASHWINFO fi {0};
        fi.cbSize       = sizeof(fi);
        fi.hwnd         = INpp::Get().GetHandle();
        fi.dwFlags      = FLASHW_ALL;
        fi.uCount       = 3;
        fi.dwTimeout    = 80;

        FlashWindowEx(&fi);
    }

    if (found)
    {
        if (!keepFocus)
            SetFocus(_hSci);

        sendSci(SCI_SETSEL, startPos, endPos);
        sendSci(SCI_ENSUREVISIBLEENFORCEPOLICY, sendSci(SCI_LINEFROMPOSITION, startPos));
    }
    else
    {
        Edit_SetSel(_hSearchTxt, 0, -1);
    }
}


/**
 *  \brief
 */
LRESULT CALLBACK ResultWin::keyHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0)
    {
        HWND hWnd = GetFocus();

        if (RW->_hWnd == hWnd || IsChild(RW->_hWnd, hWnd))
        {
            const unsigned keyDownMask = (1 << (sizeof(SHORT) * 8 - 1));

            const bool ctrl     = ((GetKeyState(VK_CONTROL) & keyDownMask) != 0);
            const bool alt      = ((GetKeyState(VK_MENU) & keyDownMask) != 0);
            const bool shift    = ((GetKeyState(VK_SHIFT) & keyDownMask) != 0);

            // Key is pressed
            if (!(lParam & (1 << 31)))
            {
                if (!ctrl)
                {
                    if (!alt)
                    {
                        if ((wParam == VK_RETURN && RW->_hSearch) || wParam == VK_F3)
                        {
                            RW->onSearch(shift);
                            return 1;
                        }

                        if (!shift)
                        {
                            if (wParam == VK_ESCAPE)
                            {
                                if (RW->_hSearch)
                                {
                                    SendMessage(RW->_hSearch, WM_CLOSE, 0, 0);
                                    SetFocus(RW->_hSci);
                                }
                                else
                                {
                                    RW->onCloseTab();
                                }

                                return 1;
                            }
                            if (wParam == VK_RETURN || wParam == VK_SPACE)
                            {
                                RW->onDoubleClick(0);
                                return 1;
                            }
                            if (wParam == VK_F5)
                            {
                                RW->reRunCmd();
                                return 1;
                            }
                        }
                    }

                    if (!shift && RW->onKeyPress(static_cast<WORD>(wParam), alt))
                    {
                        return 1;
                    }
                }
                else if (wParam == 0x46 && !alt && !shift) // 'F'
                {
                    RW->createSearchWindow();
                    return 1;
                }
            }
        }
        else if (RW->_hSearch && (RW->_hSearch == hWnd || IsChild(RW->_hSearch, hWnd)))
        {
            const unsigned keyDownMask = (1 << (sizeof(SHORT) * 8 - 1));

            const bool ctrl     = ((GetKeyState(VK_CONTROL) & keyDownMask) != 0);
            const bool alt      = ((GetKeyState(VK_MENU) & keyDownMask) != 0);
            const bool shift    = ((GetKeyState(VK_SHIFT) & keyDownMask) != 0);

            // Key is pressed and no CTRL or ALT
            if (!(lParam & (1 << 31)) && !ctrl && !alt)
            {
                if (wParam == VK_RETURN || wParam == VK_F3)
                {
                    RW->onSearch(shift);
                    return 1;
                }
                if (wParam == VK_ESCAPE && !shift)
                {
                    SendMessage(RW->_hSearch, WM_CLOSE, 0, 0);
                    SetFocus(RW->_hSci);
                    return 1;
                }
            }
        }
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}


/**
 *  \brief
 */
LRESULT APIENTRY ResultWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_SETFOCUS:
            RW->hookKeyboard();
            SetFocus(RW->_hSci);
        return 0;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case SCN_STYLENEEDED:
                    RW->onStyleNeeded((SCNotification*)lParam);
                return 0;

                case SCN_UPDATEUI:
                    if (((SCNotification*)lParam)->updated & SC_UPDATE_SELECTION)
                        RW->onNewPosition();
                return 0;

                case SCN_HOTSPOTRELEASECLICK:
                    RW->onHotspotClick((SCNotification*)lParam);
                return 0;

                case SCN_DOUBLECLICK:
                    RW->onDoubleClick(((SCNotification*)lParam)->position);
                return 0;

                case SCN_MARGINCLICK:
                    RW->onMarginClick((SCNotification*)lParam);
                return 0;

                case TCN_SELCHANGE:
                    RW->onTabChange();
                return 0;

                case DMN_CLOSE:
                    RW->closeAllTabs();
                return 0;
            }
        break;

        case WM_CONTEXTMENU:
            RW->onCloseTab();
        break;

        case WM_SIZE:
            RW->onResize(LOWORD(lParam), HIWORD(lParam));
        return 0;

        case WM_MOVE:
            RW->onMove();
        return 0;

        case WM_DESTROY:
        return 0;

        // Below are WM_USER messages for DLL threads synchronization

        case WM_RUN_CMD_CALLBACK:
        {
            CompletionCB    complCB = reinterpret_cast<CompletionCB>(wParam);
            CmdPtr_t        cmd(*(reinterpret_cast<CmdPtr_t*>(lParam)));

            ReplyMessage(0);

            if (complCB && cmd)
                complCB(cmd);
        }
        return 0;

        case WM_OPEN_ACTIVITY_WIN:
        {
            TCHAR* header   = reinterpret_cast<TCHAR*>(wParam);
            HANDLE hCancel  = reinterpret_cast<HANDLE>(lParam);

            if (hCancel)
                ActivityWin::Show(header, hCancel);
        }
        return 0;

        case WM_CLOSE_ACTIVITY_WIN:
        {
            HANDLE hCancel = reinterpret_cast<HANDLE>(lParam);

            if (hCancel)
            {
                HWND hActivityWin = ActivityWin::GetHwnd(hCancel);

                if (hActivityWin)
                    SendMessage(hActivityWin, WM_CLOSE, 0, 0);
            }
        }
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


/**
 *  \brief
 */
LRESULT APIENTRY ResultWin::searchWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            RW->onSearchWindowCreate(hWnd);
        return 0;

        case WM_SETFOCUS:
            SetFocus(RW->_hSearchTxt);
        return 0;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == EN_REQUESTRESIZE)
            {
                RW->onMove();
                return 0;
            }
        break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                if ((HWND)lParam == RW->_hDown)
                {
                    RW->onSearch(false, true);
                    return 0;
                }
                else if ((HWND)lParam == RW->_hUp)
                {
                    RW->onSearch(true, true);
                    return 0;
                }
            }
        break;

        case WM_DESTROY:
            if (RW->_hSearchFont)
            {
                DeleteObject(RW->_hSearchFont);
                RW->_hSearchFont = NULL;
            }

            if (RW->_hBtnFont)
            {
                DeleteObject(RW->_hBtnFont);
                RW->_hBtnFont = NULL;
            }

            RW->_hSearch = NULL;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
