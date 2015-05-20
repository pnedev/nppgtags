/**
 *  \file
 *  \brief  GTags result Scintilla view window
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


#pragma comment (lib, "comctl32")


#include "ResultWin.h"
#include "INpp.h"
#include "DbManager.h"
#include "DocLocation.h"
#include <commctrl.h>


// Scintilla user defined styles IDs
enum
{
    SCE_GTAGS_HEADER = 151,
    SCE_GTAGS_PROJECT_PATH,
    SCE_GTAGS_FILE,
    SCE_GTAGS_LINE_NUM,
    SCE_GTAGS_WORD2SEARCH
};


// Scintilla fold levels
enum
{
    FILE_HEADER_LVL = SC_FOLDLEVELBASE,
    RESULT_LVL
};


namespace GTags
{

const TCHAR ResultWin::cClassName[]   = _T("ResultWin");
const TCHAR ResultWin::cTabFont[]     = _T("Tahoma");


ResultWin* ResultWin::RW = NULL;


/**
 *  \brief
 */
ResultWin::Tab::Tab(const std::shared_ptr<Cmd>& cmd) :
    _cmdId(cmd->Id()), _regExp(cmd->RegExp()), _matchCase(cmd->MatchCase()),
    _outdated(false), _currentLine(1), _firstVisibleLine(0)
{
    Tools::WtoA(_projectPath, _countof(_projectPath), cmd->DbPath());
    Tools::WtoA(_search, _countof(_search), cmd->Tag());

    // Add the search header - cmd name + search word + project path
    _uiBuf = cmd->Name();
    _uiBuf += " \"";
    _uiBuf += _search;
    _uiBuf += "\" (";
    _uiBuf += _regExp ? "regexp, ": "literal, ";
    _uiBuf += _matchCase ? "match case": "ignore case";
    _uiBuf += ") in \"";
    _uiBuf += _projectPath;
    _uiBuf += "\"";

    // parsing result buffer and composing UI buffer
    if (_cmdId == FIND_FILE)
        parseFindFile(_uiBuf, cmd->Result());
    else
        parseCmd(_uiBuf, cmd->Result());
}


/**
 *  \brief
 */
void ResultWin::Tab::parseCmd(CTextA& dst, const char* src)
{
    const char* pLine;
    const char* pPreviousFile = NULL;
    unsigned pPreviousFileLen = 0;

    for (;;)
    {
        while (*src == '\n' || *src == '\r')
            src++;
        if (*src == 0) break;

        pLine = src;
        while (*pLine != ':')
            pLine++;

        // add new file name to the UI buffer only if it is different
        // than the previous one
        if (pPreviousFile == NULL ||
            (unsigned)(pLine - src) != pPreviousFileLen ||
            strncmp(src, pPreviousFile, pPreviousFileLen))
        {
            pPreviousFile = src;
            pPreviousFileLen = pLine - src;
            dst += "\n\t";
            dst.append(pPreviousFile, pPreviousFileLen);
        }

        src = ++pLine;
        while (*src != ':')
            src++;

        dst += "\n\t\tline ";
        dst.append(pLine, src - pLine);
        dst += ":\t";

        pLine = ++src;
        while (*pLine == ' ' || *pLine == '\t')
            pLine++;

        src = pLine + 1;
        while (*src != '\n' && *src != '\r')
            src++;

        if (src == pLine + 1)
        {
            _outdated = true;
            break;
        }

        dst.append(pLine, src - pLine);
    }
}


/**
 *  \brief
 */
void ResultWin::Tab::parseFindFile(CTextA& dst, const char* src)
{
    const char* eol;

    for (;;)
    {
        while (*src == '\n' || *src == '\r' || *src == ' ' || *src == '\t')
            src++;
        if (*src == 0) break;

        eol = src;
        while (*eol != '\n' && *eol != '\r' && *eol != 0)
            eol++;

        dst += "\n\t";
        dst.append(src, eol - src);
        src = eol;
    }
}


/**
 *  \brief
 */
void ResultWin::Tab::SetFolded(int lineNum)
{
    for (std::vector<int>::iterator i = _expandedLines.begin();
            i != _expandedLines.end(); i++)
        if (*i == lineNum)
        {
            _expandedLines.erase(i);
            break;
        }
}


/**
 *  \brief
 */
void ResultWin::Tab::ClearFolded(int lineNum)
{
    _expandedLines.push_back(lineNum);
}


/**
 *  \brief
 */
bool ResultWin::Tab::IsFolded(int lineNum)
{
    const int size = _expandedLines.size();

    for (int i = 0; i < size; i++)
        if (_expandedLines[i] == lineNum)
            return false;

    return true;
}


/**
 *  \brief
 */
int ResultWin::Register()
{
    if (RW)
        return 0;

    WNDCLASS wc         = {0};
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = wndProc;
    wc.hInstance        = HMod;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszClassName    = cClassName;

    RegisterClass(&wc);

    INITCOMMONCONTROLSEX icex   = {0};
    icex.dwSize                 = sizeof(icex);
    icex.dwICC                  = ICC_STANDARD_CLASSES;

    InitCommonControlsEx(&icex);

    RW = new ResultWin();
    if (RW->composeWindow() == NULL)
    {
        Unregister();
        return -1;
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

    return 0;
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
void ResultWin::show(const std::shared_ptr<Cmd>& cmd)
{
    INpp& npp = INpp::Get();

    if (cmd->ResultLen() > 262144) // 256k
    {
        TCHAR buf[512];
        _sntprintf_s(buf, _countof(buf), _TRUNCATE,
                _T("%s \"%s\": A lot of matches were found, ")
                _T("parsing those will be rather slow.\n")
                _T("Are you sure you want to proceed?"),
                cmd->Name(), cmd->Tag());
        int choice = MessageBox(npp.GetHandle(), buf, cPluginName,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
        if (choice != IDYES)
            return;
    }

    // parsing results happens here
    Tab* tab = new Tab(cmd);

    AUTOLOCK(_lock);

    int i;
    for (i = TabCtrl_GetItemCount(_hTab); i; i--)
    {
        Tab* oldTab = getTab(i - 1);
        if (oldTab && *tab == *oldTab) // same search tab already present?
        {
            if (_activeTab == oldTab) // is this the currently active tab?
                _activeTab = NULL;
            delete oldTab;
            break;
        }
    }

    if (tab->_outdated)
    {
        MessageBox(npp.GetHandle(),
                _T("Database is outdated.")
                _T("\nPlease re-create it and redo the search"),
                cPluginName, MB_OK | MB_ICONEXCLAMATION);
        if (i)
            TabCtrl_DeleteItem(_hTab, --i);
        delete tab;
        tab = NULL;
    }
    else
    {
        if (i == 0) // search is completely new - add new tab
        {
            TCHAR buf[256];
            _sntprintf_s(buf, _countof(buf), _TRUNCATE, _T("%s \"%s\""),
                    cmd->Name(), cmd->Tag());

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
    loadTab(tab);

    npp.UpdateDockingWin(_hWnd);
    showWindow();
}


/**
 *  \brief
 */
void ResultWin::applyStyle()
{
    INpp& npp = INpp::Get();

    char font[32];
    npp.GetFontName(STYLE_DEFAULT, font, _countof(font));
    int size = npp.GetFontSize(STYLE_DEFAULT);
    int caretLineBackColor = npp.GetCaretLineBack();
    int foreColor = npp.GetForegroundColor(STYLE_DEFAULT);
    int backColor = npp.GetBackgroundColor(STYLE_DEFAULT);
    int lineNumColor = npp.GetForegroundColor(STYLE_LINENUMBER);

    if (_hFont)
        DeleteObject(_hFont);

    HDC hdc = GetWindowDC(_hTab);
    _hFont = CreateFont(
            -MulDiv(size - 1, GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH, cTabFont);
    ReleaseDC(_hTab, hdc);

    if (_hFont)
        SendMessage(_hTab, WM_SETFONT, (WPARAM)_hFont, (LPARAM)TRUE);

    sendSci(SCI_STYLERESETDEFAULT);
    setStyle(STYLE_DEFAULT, foreColor, backColor, false, false, size, font);
    sendSci(SCI_STYLECLEARALL);

    setStyle(SCE_GTAGS_HEADER, foreColor, caretLineBackColor, true);
    setStyle(SCE_GTAGS_PROJECT_PATH,
            foreColor, caretLineBackColor, true, true);
    setStyle(SCE_GTAGS_FILE, RGB(0,128,128), backColor, true);
    setStyle(SCE_GTAGS_LINE_NUM, lineNumColor, backColor, false, true);
    setStyle(SCE_GTAGS_WORD2SEARCH, RGB(225,0,0), backColor, true);

    sendSci(SCI_SETCARETLINEBACK, caretLineBackColor);
    sendSci(SCI_STYLESETHOTSPOT, SCE_GTAGS_WORD2SEARCH, true);
    sendSci(SCI_STYLESETUNDERLINE, SCE_GTAGS_HEADER, true);
    sendSci(SCI_STYLESETUNDERLINE, SCE_GTAGS_PROJECT_PATH, true);
}


/**
 *  \brief
 */
ResultWin::~ResultWin()
{
    if (_hWnd)
    {
        closeAllTabs();

        INpp& npp = INpp::Get();

        if (_hSci)
        {
            npp.DestroySciHandle(_hSci);
            _hSci = NULL;
        }

        SendMessage(_hWnd, WM_CLOSE, 0, 0);

        if (_hFont)
        {
            DeleteObject(_hFont);
            _hFont = NULL;
        }
    }
}


/**
 *  \brief
 */
void ResultWin::setStyle(int style, COLORREF fore, COLORREF back,
        bool bold, bool italic, int size, const char *font)
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
    sendSci(SCI_SETCURSOR, SC_CURSORNORMAL);
    sendSci(SCI_SETCARETSTYLE, CARETSTYLE_INVISIBLE);
    sendSci(SCI_SETCARETLINEVISIBLE, true);
    sendSci(SCI_SETCARETLINEVISIBLEALWAYS, true);
    sendSci(SCI_SETHOTSPOTACTIVEUNDERLINE, false);

    sendSci(SCI_SETLAYOUTCACHE, SC_CACHE_DOCUMENT);

    // Implement lexer in the container
    sendSci(SCI_SETLEXER, 0);

    ApplyStyle();

    sendSci(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"),
            reinterpret_cast<LPARAM>("1"));

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
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID,
            SC_MARK_BOXMINUSCONNECTED);
}


/**
 *  \brief
 */
HWND ResultWin::composeWindow()
{
    INpp& npp = INpp::Get();
    HWND hOwner = npp.GetHandle();
    RECT win;
    GetWindowRect(hOwner, &win);

    DWORD style = WS_POPUP | WS_CAPTION | WS_SIZEBOX;

    _hWnd = CreateWindow(cClassName, cPluginName,
            style, win.left, win.top,
            win.right - win.left, win.bottom - win.top,
            hOwner, NULL, HMod, (LPVOID)this);
    if (_hWnd == NULL)
        return NULL;

    _hSci = npp.CreateSciHandle(_hWnd);
    if (_hSci)
    {
        _sciFunc =
                (SciFnDirect)::SendMessage(_hSci, SCI_GETDIRECTFUNCTION, 0, 0);
        _sciPtr = (sptr_t)::SendMessage(_hSci, SCI_GETDIRECTPOINTER, 0, 0);
    }
    if (_hSci == NULL || _sciFunc == NULL || _sciPtr == 0)
    {
        SendMessage(_hWnd, WM_CLOSE, 0, 0);
        _hWnd = NULL;
        _hSci = NULL;
        return NULL;
    }

    AdjustWindowRect(&win, style, FALSE);
    MoveWindow(_hWnd, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);
    GetClientRect(_hWnd, &win);

    _hTab = CreateWindowEx(0, WC_TABCONTROL, NULL,
            WS_CHILD | WS_VISIBLE | TCS_BUTTONS | TCS_FOCUSNEVER,
            0, 0, win.right - win.left, win.bottom - win.top,
            _hWnd, NULL, HMod, NULL);

    TabCtrl_SetExtendedStyle(_hTab, TCS_EX_FLATSEPARATORS);

    TabCtrl_AdjustRect(_hTab, FALSE, &win);
    MoveWindow(_hSci, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);

    configScintilla();

    ShowWindow(_hSci, SW_SHOWNORMAL);

    return _hWnd;
}


/**
 *  \brief
 */
void ResultWin::showWindow()
{
    if (_hKeyHook == NULL)
        _hKeyHook = SetWindowsHookEx(WH_KEYBOARD, keyHookProc, NULL,
                _nppThreadId);

    INpp::Get().ShowDockingWin(_hWnd);
    SetFocus(_hWnd);
}


/**
 *  \brief
 */
void ResultWin::hideWindow()
{
    if (_hKeyHook)
    {
        UnhookWindowsHookEx(_hKeyHook);
        _hKeyHook = NULL;
    }

    INpp& npp = INpp::Get();
    npp.HideDockingWin(_hWnd);
    SetFocus(npp.GetSciHandle());
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
void ResultWin::loadTab(ResultWin::Tab* tab)
{
    sendSci(SCI_SETCURSOR, SC_CURSORWAIT);

    // store current view if there is one
    if (_activeTab)
    {
        _activeTab->_currentLine =
                sendSci(SCI_LINEFROMPOSITION, sendSci(SCI_GETCURRENTPOS));
        _activeTab->_firstVisibleLine = sendSci(SCI_GETFIRSTVISIBLELINE);
    }

    _activeTab = NULL;

    sendSci(SCI_SETREADONLY, 0);
    sendSci(SCI_CLEARALL);

    _activeTab = tab;

    sendSci(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(tab->_uiBuf.C_str()));
    sendSci(SCI_SETREADONLY, 1);

    sendSci(SCI_SETFIRSTVISIBLELINE, tab->_firstVisibleLine);
    sendSci(SCI_GOTOLINE, tab->_currentLine);

    sendSci(SCI_SETCURSOR, SC_CURSORNORMAL);
}


/**
 *  \brief
 */
bool ResultWin::openItem(int lineNum, unsigned matchNum)
{
    sendSci(SCI_GOTOLINE, lineNum);

    int lineLen = sendSci(SCI_LINELENGTH, lineNum);
    if (lineLen <= 0)
        return false;

    CCharArray lineTxt(lineLen + 1);

    sendSci(SCI_GETLINE, lineNum, reinterpret_cast<LPARAM>(&lineTxt));

    int line = 0;
    int i;

    if (_activeTab->_cmdId != FIND_FILE)
    {
        for (i = 7; i <= lineLen && lineTxt[i] != ':'; i++);
        lineTxt[i] = 0;
        line = atoi(&lineTxt[7]) - 1;

        lineNum = sendSci(SCI_GETFOLDPARENT, lineNum);
        if (lineNum == -1)
            return false;

        lineLen = sendSci(SCI_LINELENGTH, lineNum);
        if (lineLen <= 0)
            return false;

        lineTxt(lineLen + 1);
        sendSci(SCI_GETLINE, lineNum, reinterpret_cast<LPARAM>(&lineTxt));
    }

    lineTxt[lineLen] = 0;
    for (i = 1; i <= lineLen && lineTxt[i] != '\r' && lineTxt[i] != '\n'; i++);
    lineTxt[i] = 0;

    CPath file(_activeTab->_projectPath);
    CText str(&lineTxt[1]);
    file += str.C_str();

    INpp& npp = INpp::Get();
    if (!file.FileExists())
    {
        MessageBox(npp.GetHandle(),
                _T("File not found, present results are outdated.")
                _T("\nPlease redo the search"),
                cPluginName, MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    DocLocation::Get().Push();
    npp.OpenFile(file.C_str());
    SetFocus(npp.GetSciHandle());

    if (_activeTab->_cmdId == FIND_FILE)
        return true;

    const long endPos = npp.LineEndPosition(line);

    const bool wholeWord = (_activeTab->_cmdId != GREP);

    // Highlight the corresponding match number if there are more than one
    // matches on single result line
    for (long findBegin = npp.PositionFromLine(line), findEnd = endPos;
        matchNum; findBegin = findEnd, findEnd = endPos, matchNum--)
    {
        if (!npp.SearchText(_activeTab->_search, _activeTab->_matchCase,
                wholeWord, _activeTab->_regExp, &findBegin, &findEnd))
        {
            MessageBox(npp.GetHandle(),
                    _T("Look-up mismatch, present results are outdated.")
                    _T("\nSave all modified files and redo the search"),
                    cPluginName, MB_OK | MB_ICONEXCLAMATION);
            return false;
        }
    }

    return true;
}


/**
 *  \brief
 */
bool ResultWin::findString(const char* str, int* startPos, int* endPos,
        bool matchCase, bool wholeWord, bool regExp)
{
    int searchFlags = 0;
    if (matchCase)
        searchFlags |= SCFIND_MATCHCASE;
    if (wholeWord)
        searchFlags |= SCFIND_WHOLEWORD;
    if (regExp)
        searchFlags |= (SCFIND_REGEXP | SCFIND_POSIX);

    sendSci(SCI_SETSEARCHFLAGS, searchFlags);
    sendSci(SCI_SETTARGETSTART, *startPos);
    sendSci(SCI_SETTARGETEND, *endPos);

    if (sendSci(SCI_SEARCHINTARGET, strlen(str),
            reinterpret_cast<LPARAM>(str)) >= 0)
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
void ResultWin::toggleFolding(int lineNum)
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
void ResultWin::onStyleNeeded(SCNotification* notify)
{
    if (_activeTab == NULL)
        return;

    if (!_lock.TryLock())
        return;

    int lineNum = sendSci(SCI_LINEFROMPOSITION, sendSci(SCI_GETENDSTYLED));
    const int endStylingPos = notify->position;

    for (int startPos = sendSci(SCI_POSITIONFROMLINE, lineNum);
        endStylingPos > startPos;
        startPos = sendSci(SCI_POSITIONFROMLINE, ++lineNum))
    {
        const int lineLen = sendSci(SCI_LINELENGTH, lineNum);
        if (lineLen <= 0)
            continue;

        const int endPos = startPos + lineLen;

        sendSci(SCI_STARTSTYLING, startPos, 0xFF);

        if ((char)sendSci(SCI_GETCHARAT, startPos) != '\t')
        {
            int pathLen = strlen(_activeTab->_projectPath);

            // 2 * '"' + LF + CR = 4
            sendSci(SCI_SETSTYLING, lineLen - pathLen - 4, SCE_GTAGS_HEADER);
            sendSci(SCI_SETSTYLING, pathLen + 4, SCE_GTAGS_PROJECT_PATH);
        }
        else
        {
            if ((char)sendSci(SCI_GETCHARAT, startPos + 1) != '\t')
            {
                if (_activeTab->_cmdId == FIND_FILE)
                {
                    int findBegin = startPos;
                    int findEnd = endPos;

                    if (findString(_activeTab->_search, &findBegin, &findEnd,
                        _activeTab->_matchCase, false, _activeTab->_regExp))
                    {
                        // Highlight all matches in a single result line
                        do
                        {
                            if (findBegin - startPos)
                                sendSci(SCI_SETSTYLING, findBegin - startPos,
                                        SCE_GTAGS_FILE);
                            sendSci(SCI_SETSTYLING, findEnd - findBegin,
                                    SCE_GTAGS_WORD2SEARCH);
                            findBegin = startPos = findEnd;
                            findEnd = endPos;
                        } while (findString(_activeTab->_search,
                                &findBegin, &findEnd, _activeTab->_matchCase,
                                false, _activeTab->_regExp));

                        if (endPos - startPos)
                            sendSci(SCI_SETSTYLING, endPos - startPos,
                                    SCE_GTAGS_FILE);
                    }
                    else
                    {
                        sendSci(SCI_SETSTYLING, lineLen, SCE_GTAGS_FILE);
                    }
                }
                else
                {
                    sendSci(SCI_SETSTYLING, lineLen, SCE_GTAGS_FILE);
                    sendSci(SCI_SETFOLDLEVEL, lineNum,
                            FILE_HEADER_LVL | SC_FOLDLEVELHEADERFLAG);
                    if (_activeTab->IsFolded(lineNum))
                        sendSci(SCI_FOLDLINE, lineNum, SC_FOLDACTION_CONTRACT);
                }
            }
            else
            {
                // "\t\tline: Num" - 'N' is at position 8
                int previewPos = startPos + 8;
                for (; (char)sendSci(SCI_GETCHARAT, previewPos) != '\t';
                        previewPos++);

                int findBegin = previewPos;
                int findEnd = endPos;

                bool wholeWord = (_activeTab->_cmdId != GREP);

                if (findString(_activeTab->_search, &findBegin, &findEnd,
                    _activeTab->_matchCase, wholeWord, _activeTab->_regExp))
                {
                    sendSci(SCI_SETSTYLING, previewPos - startPos,
                            SCE_GTAGS_LINE_NUM);
                    // Highlight all matches in a single result line
                    do
                    {
                        if (findBegin - previewPos)
                            sendSci(SCI_SETSTYLING, findBegin - previewPos,
                                    STYLE_DEFAULT);
                        sendSci(SCI_SETSTYLING, findEnd - findBegin,
                                SCE_GTAGS_WORD2SEARCH);
                        findBegin = previewPos = findEnd;
                        findEnd = endPos;
                    } while (findString(_activeTab->_search,
                            &findBegin, &findEnd, _activeTab->_matchCase,
                            wholeWord, _activeTab->_regExp));

                    if (endPos - previewPos)
                        sendSci(SCI_SETSTYLING, endPos - previewPos,
                                STYLE_DEFAULT);
                }
                else
                {
                    sendSci(SCI_SETSTYLING, lineLen, STYLE_DEFAULT);
                }

                sendSci(SCI_SETFOLDLEVEL, lineNum, RESULT_LVL);
            }
        }
    }

    _lock.Unlock();
}


/**
 *  \brief
 */
void ResultWin::onHotspotClick(SCNotification* notify)
{
    if (!_lock.TryLock())
        return;

    const int lineNum = sendSci(SCI_LINEFROMPOSITION, notify->position);
    unsigned matchNum = 1;

    if (_activeTab->_cmdId != FIND_FILE)
    {
        const int endLine = sendSci(SCI_GETLINEENDPOSITION, lineNum);

        // "\t\tline: Num" - 'N' is at position 8
        int findBegin = sendSci(SCI_POSITIONFROMLINE, lineNum) + 8;
        for (; (char)sendSci(SCI_GETCHARAT, findBegin) != '\t'; findBegin++);

        bool wholeWord = (_activeTab->_cmdId != GREP);

        // Find which hotspot was clicked in case there are more than one
        // matches on single result line
        for (int findEnd = endLine;
                findString(_activeTab->_search, &findBegin, &findEnd,
                    _activeTab->_matchCase, wholeWord, _activeTab->_regExp);
                findBegin = findEnd, findEnd = endLine, matchNum++)
            if (notify->position >= findBegin && notify->position <= findEnd)
                break;
    }

    openItem(lineNum, matchNum);

    // Clear false selection in results win when visiting result location
    sendSci(SCI_GOTOPOS, notify->position);

    _lock.Unlock();
}


/**
 *  \brief
 */
void ResultWin::onDoubleClick(int pos)
{
    if (!_lock.TryLock())
        return;

    int lineNum = sendSci(SCI_LINEFROMPOSITION, pos);

    if (lineNum == 0)
    {
        pos = sendSci(SCI_GETCURRENTPOS);
        if (pos == sendSci(SCI_POSITIONAFTER, pos)) // end of document
        {
            lineNum = sendSci(SCI_LINEFROMPOSITION, pos);
            int foldLine = sendSci(SCI_GETFOLDPARENT, lineNum);
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

    if (lineNum > 0)
    {
        if (sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG)
            toggleFolding(lineNum);
        else
            openItem(lineNum);
    }

    _lock.Unlock();
}


/**
 *  \brief
 */
void ResultWin::onMarginClick(SCNotification* notify)
{
    if (!_lock.TryLock())
        return;

    int lineNum = sendSci(SCI_LINEFROMPOSITION, notify->position);

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

    _lock.Unlock();
}


/**
 *  \brief
 */
bool ResultWin::onKeyPress(WORD keyCode)
{
    if (!_lock.TryLock())
        return false;

    bool handled = true;
    int lineNum = sendSci(SCI_LINEFROMPOSITION, sendSci(SCI_GETCURRENTPOS));

    switch (keyCode)
    {
        case VK_ADD:
            if (!(sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG))
                lineNum = sendSci(SCI_GETFOLDPARENT, lineNum);
            if (lineNum > 0 && !sendSci(SCI_GETFOLDEXPANDED, lineNum))
                toggleFolding(lineNum);
        break;

        case VK_SUBTRACT:
            if (!(sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG))
                lineNum = sendSci(SCI_GETFOLDPARENT, lineNum);
            if (lineNum > 0 && sendSci(SCI_GETFOLDEXPANDED, lineNum))
                toggleFolding(lineNum);
        break;

        case VK_LEFT:
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
        }
        break;

        case VK_RIGHT:
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
        }
        break;

        default:
            handled = false;
    }

    _lock.Unlock();

    return handled;
}


/**
 *  \brief
 */
void ResultWin::onTabChange()
{
    if (!_lock.TryLock())
        return;

    Tab* tab = getTab();
    if (tab)
        loadTab(tab);

    _lock.Unlock();
}


/**
 *  \brief
 */
void ResultWin::onCloseTab()
{
    if (!_lock.TryLock())
        return;

    int i = TabCtrl_GetCurSel(_hTab);
    delete _activeTab;
    _activeTab = NULL;
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

        INpp::Get().UpdateDockingWin(_hWnd);
        hideWindow();
    }

    _lock.Unlock();
}


/**
 *  \brief
 */
void ResultWin::closeAllTabs()
{
    _activeTab = NULL;

    for (int i = TabCtrl_GetItemCount(_hTab); i; i--)
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
    MoveWindow(_hSci, win.left, win.top,
            win.right - win.left, win.bottom - win.top, TRUE);
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
            // Key is pressed
            if (!(lParam & (1 << 31)))
            {
                if (wParam == VK_ESCAPE)
                {
                    RW->onCloseTab();
                    return 1;
                }
                if (wParam == VK_RETURN || wParam == VK_SPACE)
                {
                    RW->onDoubleClick(0);
                    return 1;
                }
                if (RW->onKeyPress(wParam))
                {
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
LRESULT APIENTRY ResultWin::wndProc(HWND hWnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_SETFOCUS:
            SetFocus(RW->_hSci);
        return 0;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case SCN_STYLENEEDED:
                    RW->onStyleNeeded((SCNotification*)lParam);
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
            }
        break;

        case WM_CONTEXTMENU:
            RW->onCloseTab();
        break;

        case WM_SIZE:
            RW->onResize(LOWORD(lParam), HIWORD(lParam));
        return 0;

        case WM_DESTROY:
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags
