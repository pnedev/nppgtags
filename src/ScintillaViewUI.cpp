/**
 *  \file
 *  \brief  GTags result Scintilla view UI
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014 Pavel Nedev
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


#include "ScintillaViewUI.h"
#include "INpp.h"
#include "DBManager.h"
#include "DocLocation.h"
#include <commctrl.h>


// Scintilla user defined styles IDs
enum
{
    SCE_GTAGS_HEADER = 151,
    SCE_GTAGS_PROJECT_PATH,
    SCE_GTAGS_FILE,
    SCE_GTAGS_WORD2SEARCH
};


// Scintilla fold levels
enum
{
    SEARCH_HEADER_LVL = SC_FOLDLEVELBASE + 1,
    FILE_HEADER_LVL,
    RESULT_LVL
};


const TCHAR ScintillaViewUI::cClassName[] = _T("ScintillaViewUI");


using namespace GTags;


/**
 *  \brief
 */
const ScintillaViewUI::Tab&
        ScintillaViewUI::Tab::operator=(const GTags::CmdData& cmd)
{
    _cmdID = cmd.GetID();

    Tools::WtoA(_projectPath, _countof(_projectPath), cmd.GetDBPath());
    Tools::WtoA(_search, _countof(_search), cmd.GetTag());

    return *this;
}


/**
 *  \brief
 */
int ScintillaViewUI::Register()
{
    if (_hWnd)
        return 0;

    WNDCLASS wc         = {0};
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = wndProc;
    wc.hInstance        = HMod;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszClassName    = cClassName;

    RegisterClass(&wc);

    if (composeWindow() == NULL)
        return -1;

    tTbData data        = {0};
    data.hClient        = _hWnd;
    data.pszName        = const_cast<TCHAR*>(cPluginName);
    data.uMask          = 0;
    data.pszAddInfo     = NULL;
    data.uMask          = DWS_DF_CONT_BOTTOM;
    data.pszModuleName  = DllPath.GetFilename();
    data.dlgID          = 0;

    INpp& npp = INpp::Get();
    npp.RegisterWin(_hWnd);
    npp.RegisterDockingWin(data);
    npp.HideDockingWin(_hWnd);

    return 0;
}


/**
 *  \brief
 */
void ScintillaViewUI::Unregister()
{
    if (_hWnd == NULL)
        return;

    INpp& npp = INpp::Get();

    if (_hSci)
    {
        npp.DestroySciHandle(_hSci);
        _hSci = NULL;
    }

    npp.UnregisterWin(_hWnd);
    SendMessage(_hWnd, WM_CLOSE, 0, 0);
    _hWnd = NULL;

    UnregisterClass(cClassName, HMod);
}


/**
 *  \brief
 */
void ScintillaViewUI::Show(const CmdData& cmd)
{
    if (_hWnd == NULL)
        return;

    add(cmd);
}


/**
 *  \brief
 */
void ScintillaViewUI::ResetStyle()
{
    if (_hWnd == NULL)
        return;

    INpp& npp = INpp::Get();

    char font[32];
    npp.GetFontName(font, _countof(font));
    int size = npp.GetFontSize();

    sendSci(SCI_STYLERESETDEFAULT);
    setStyle(STYLE_DEFAULT, cBlack, cWhite, false, false, size, font);
    sendSci(SCI_STYLECLEARALL);

    setStyle(SCE_GTAGS_HEADER, cBlack, RGB(179,217,217), true);
    setStyle(SCE_GTAGS_PROJECT_PATH, cBlack, RGB(179,217,217), true, true);
    setStyle(SCE_GTAGS_FILE, cBlue, cWhite, true);
    setStyle(SCE_GTAGS_WORD2SEARCH, cRed, cWhite, true);
}


/**
 *  \brief
 */
void ScintillaViewUI::setStyle(int style, COLORREF fore, COLORREF back,
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
HWND ScintillaViewUI::composeWindow()
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
    if (_hSci == NULL || _sciFunc == NULL || _sciPtr == NULL)
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
    MoveWindow(_hSci, 0, 0,
            win.right - win.left, win.bottom - win.top, TRUE);

    sendSci(SCI_SETCODEPAGE, SC_CP_UTF8);
    sendSci(SCI_SETEOLMODE, SC_EOL_CRLF);
    sendSci(SCI_USEPOPUP, false);
    sendSci(SCI_SETUNDOCOLLECTION, false);
    sendSci(SCI_SETCURSOR, SC_CURSORARROW);
    sendSci(SCI_SETCARETSTYLE, CARETSTYLE_INVISIBLE);
    sendSci(SCI_SETCARETLINEBACK, RGB(222,222,238));
    sendSci(SCI_SETCARETLINEVISIBLE, 1);
    sendSci(SCI_SETCARETLINEVISIBLEALWAYS, 1);

    // Implement lexer in the container
    sendSci(SCI_SETLEXER, 0);

    ResetStyle();

    sendSci(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"),
            reinterpret_cast<LPARAM>("1"));

    sendSci(SCI_SETMARGINTYPEN, 1, SC_MARGIN_SYMBOL);
    sendSci(SCI_SETMARGINMASKN, 1, SC_MASK_FOLDERS);
    sendSci(SCI_SETMARGINWIDTHN, 1, 20);
    sendSci(SCI_SETFOLDMARGINCOLOUR, 1, cBlack);
    sendSci(SCI_SETFOLDMARGINHICOLOUR, 1, cBlack);
    sendSci(SCI_SETMARGINSENSITIVEN, 1, 1);
    sendSci(SCI_SETFOLDFLAGS, 0);

    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID,
            SC_MARK_BOXMINUSCONNECTED);

    sendSci(SCI_SETREADONLY, 1);

    ShowWindow(_hSci, SW_SHOWNORMAL);

    return _hWnd;
}


/**
 *  \brief
 */
void ScintillaViewUI::add(const CmdData& cmd)
{
    _tab = cmd;

    // Add the search header - cmd name + search word + project path
    CTextA uiBuf(cmd.GetName());
    uiBuf += " \"";
    uiBuf += _tab._search;
    uiBuf += "\" in \"";
    uiBuf += _tab._projectPath;
    uiBuf += "\"";

    // parsing result buffer and composing UI buffer
    if (_tab._cmdID == FIND_FILE)
        parseFindFile(uiBuf, cmd.GetResult());
    else
        parseCmd(uiBuf, cmd.GetResult());

    AUTOLOCK(_lock);

    sendSci(SCI_SETREADONLY, 0);
    sendSci(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(uiBuf.C_str()));
    sendSci(SCI_SETREADONLY, 1);
    const int line1pos = sendSci(SCI_POSITIONFROMLINE, 1);
    sendSci(SCI_SETSEL, line1pos, line1pos);

    INpp& npp = INpp::Get();
    npp.UpdateDockingWin(_hWnd);
    npp.ShowDockingWin(_hWnd);
    SetFocus(_hWnd);
}


/**
 *  \brief
 */
void ScintillaViewUI::parseCmd(CTextA& dst, const char* src)
{
    const unsigned searchLen = strlen(_tab._search);
    const char* lineRes;
    const char* lineResEnd;
    const char* fileResEnd;
    const char* prevFile = NULL;
    unsigned prevFileLen = 0;

    for (;;)
    {
        while (*src == '\n' || *src == '\r' || *src == ' ' || *src == '\t')
            src++;
        if (*src == 0) break;

        src += searchLen; // skip search word from result buffer
        while (*src == ' ' || *src == '\t')
            src++;

        lineRes = lineResEnd = src;
        while (*lineResEnd != ' ' && *lineResEnd != '\t')
            lineResEnd++;

        src = lineResEnd;
        while (*src == ' ' || *src == '\t')
            src++;

        fileResEnd = src;
        while (*fileResEnd != ' ' && *fileResEnd != '\t')
            fileResEnd++;

        // add new file name to the UI buffer only if it is different
        // than the previous one
        if (prevFile == NULL || (unsigned)(fileResEnd - src) != prevFileLen ||
            strncmp(src, prevFile, prevFileLen))
        {
            prevFile = src;
            prevFileLen = fileResEnd - src;
            dst += "\n\t";
            dst.append(prevFile, prevFileLen);
        }

        dst += "\n\t\tline ";
        dst.append(lineRes, lineResEnd - lineRes);
        dst += ":\t";

        src = fileResEnd;
        while (*src == ' ' || *src == '\t')
            src++;

        lineResEnd = src;
        while (*lineResEnd != '\n' && *lineResEnd != '\r')
            lineResEnd++;

        dst.append(src, lineResEnd - src);
        src = lineResEnd;
    }

    dst += "\n";
}


/**
 *  \brief
 */
void ScintillaViewUI::parseFindFile(CTextA& dst, const char* src)
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

    dst += "\n";
}


/**
 *  \brief
 */
bool ScintillaViewUI::openItem(int lineNum)
{
    int lineLen = sendSci(SCI_LINELENGTH, lineNum);
    char* lineTxt = new char[lineLen + 1];

    sendSci(SCI_GETLINE, lineNum, reinterpret_cast<LPARAM>(lineTxt));

    long line = -1;
    int i;

    if (lineTxt[1] == '\t')
    {
        for (i = 7; lineTxt[i] != ':'; i++);
        lineTxt[i] = 0;
        line = atoi(&lineTxt[7]) - 1;
        delete [] lineTxt;

        lineNum = sendSci(SCI_GETFOLDPARENT, lineNum);
        if (lineNum == -1)
            return false;

        lineLen = sendSci(SCI_LINELENGTH, lineNum);
        lineTxt = new char[lineLen + 1];
        sendSci(SCI_GETLINE, lineNum, reinterpret_cast<LPARAM>(lineTxt));
    }

    lineTxt[lineLen] = 0;
    for (i = 1; lineTxt[i] != '\r' && lineTxt[i] != '\n'; i++);
    lineTxt[i] = 0;

    CPath file(_tab._projectPath);
    CText str(&lineTxt[1]);
	file += str.C_str();
    delete [] lineTxt;

    INpp& npp = INpp::Get();
    if (!file.FileExists())
    {
        MessageBox(npp.GetHandle(),
                _T("File not found, update database and search again"),
                cPluginName, MB_OK | MB_ICONEXCLAMATION);
        return true;
    }

    DocLocation::Get().Push();
    npp.OpenFile(file.C_str());
    SetFocus(npp.ReadSciHandle());

    // GTags command is FIND_FILE
    if (line == -1)
    {
        npp.ClearSelection();
        return true;
    }

    bool wholeWord =
            (_tab._cmdID != GREP && _tab._cmdID != FIND_LITERAL);

    if (!npp.SearchText(_tab._search, true, wholeWord,
            npp.PositionFromLine(line), npp.LineEndPosition(line)))
    {
        MessageBox(npp.GetHandle(),
                _T("Look-up mismatch, update database and search again"),
                cPluginName, MB_OK | MB_ICONINFORMATION);
    }

    return true;
}


/**
 *  \brief
 */
void ScintillaViewUI::styleString(int styleID, const char* str,
        int lineNum, int lineOffset, bool matchCase, bool wholeWord)
{
    struct TextToFind ttf = {0};
    ttf.lpstrText = const_cast<char*>(str);
    ttf.chrg.cpMin = sendSci(SCI_POSITIONFROMLINE, lineNum) + lineOffset;
    ttf.chrg.cpMax = sendSci(SCI_GETLINEENDPOSITION, lineNum);

    int searchFlags = 0;
    if (matchCase)
        searchFlags |= SCFIND_MATCHCASE;
    if (wholeWord)
        searchFlags |= SCFIND_WHOLEWORD;

    if (sendSci(SCI_FINDTEXT, searchFlags,
            reinterpret_cast<LPARAM>(&ttf)) != -1)
    {
        sendSci(SCI_STARTSTYLING, ttf.chrgText.cpMin, 0xFF);
        sendSci(SCI_SETSTYLING, ttf.chrgText.cpMax - ttf.chrgText.cpMin,
                styleID);
    }
}


/**
 *  \brief
 */
void ScintillaViewUI::onStyleNeeded(SCNotification* notify)
{
    int lineNum = sendSci(SCI_LINEFROMPOSITION,
            sendSci(SCI_GETENDSTYLED));
    const int endPos = notify->position;

    for (int startPos = sendSci(SCI_POSITIONFROMLINE, lineNum);
        endPos > startPos;
        startPos = sendSci(SCI_POSITIONFROMLINE, ++lineNum))
    {
        int lineLen = sendSci(SCI_LINELENGTH, lineNum);
        if (lineLen == 0)
            continue;

        if ((char)sendSci(SCI_GETCHARAT, startPos) != '\t')
        {
            sendSci(SCI_STARTSTYLING, startPos, 0xFF);
            sendSci(SCI_SETSTYLING, lineLen, SCE_GTAGS_HEADER);

            int pathLen = strlen(_tab._projectPath);
            startPos = sendSci(SCI_GETLINEENDPOSITION, lineNum) - pathLen - 1;

            sendSci(SCI_STARTSTYLING, startPos, 0xFF);
            sendSci(SCI_SETSTYLING, pathLen, SCE_GTAGS_PROJECT_PATH);
            sendSci(SCI_SETFOLDLEVEL, lineNum,
                    SEARCH_HEADER_LVL | SC_FOLDLEVELHEADERFLAG);
        }
        else
        {
            if ((char)sendSci(SCI_GETCHARAT, startPos + 1) != '\t')
            {
                sendSci(SCI_STARTSTYLING, startPos, 0xFF);
                sendSci(SCI_SETSTYLING, lineLen, SCE_GTAGS_FILE);
                if (_tab._cmdID == FIND_FILE)
                {
                    styleString(SCE_GTAGS_WORD2SEARCH, _tab._search, lineNum);
                    sendSci(SCI_SETFOLDLEVEL, lineNum, RESULT_LVL);
                }
                else
                {
                    sendSci(SCI_SETFOLDLEVEL, lineNum,
                            FILE_HEADER_LVL | SC_FOLDLEVELHEADERFLAG);
                    sendSci(SCI_FOLDLINE, lineNum, SC_FOLDACTION_CONTRACT);
                }
            }
            else
            {
                bool wholeWord =
                        (_tab._cmdID == GREP || _tab._cmdID == FIND_LITERAL) ?
                        false : true;
                styleString(SCE_GTAGS_WORD2SEARCH, _tab._search, lineNum,
                        7, true, wholeWord);
                sendSci(SCI_SETFOLDLEVEL, lineNum, RESULT_LVL);
            }
        }
    }
}


/**
 *  \brief
 */
void ScintillaViewUI::onDoubleClick(SCNotification* notify)
{
    if (!_lock.TryLock())
        return;

    const int lineNum = sendSci(SCI_LINEFROMPOSITION, notify->position);
    sendSci(SCI_GOTOLINE, lineNum); // Clear double-click auto-selection

    if (sendSci(SCI_LINELENGTH, lineNum))
    {
        if (sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG)
            sendSci(SCI_TOGGLEFOLD, lineNum);
        else
            openItem(lineNum);
    }

    _lock.Unlock();
}


/**
 *  \brief
 */
void ScintillaViewUI::onMarginClick(SCNotification* notify)
{
    if (!_lock.TryLock())
        return;

    if (notify->margin == 1)
    {
        const int lineNum = sendSci(SCI_LINEFROMPOSITION, notify->position);
        if (sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG)
            sendSci(SCI_TOGGLEFOLD, lineNum);
    }

    _lock.Unlock();
}


/**
 *  \brief
 */
void ScintillaViewUI::onCharAddTry(SCNotification* notify)
{
    if (!_lock.TryLock())
        return;

    if (notify->ch == ' ')
    {
        const int lineNum =
                sendSci(SCI_LINEFROMPOSITION, sendSci(SCI_GETCURRENTPOS));
        if (sendSci(SCI_LINELENGTH, lineNum))
        {
            if (sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG)
                sendSci(SCI_TOGGLEFOLD, lineNum);
            else
                openItem(lineNum);
        }
    }

    _lock.Unlock();
}


/**
 *  \brief
 */
void ScintillaViewUI::onClose()
{
    if (!_lock.TryLock())
        return;

    sendSci(SCI_SETREADONLY, 0);
    sendSci(SCI_CLEARALL);
    sendSci(SCI_SETREADONLY, 1);

    INpp& npp = INpp::Get();
    npp.UpdateDockingWin(_hWnd);
    npp.HideDockingWin(_hWnd);
    SetFocus(npp.ReadSciHandle());

    _lock.Unlock();
}


/**
 *  \brief
 */
void ScintillaViewUI::onResize(int width, int height)
{
    MoveWindow(_hSci, 0, 0, width, height, TRUE);
}


/**
 *  \brief
 */
LRESULT APIENTRY ScintillaViewUI::wndProc(HWND hwnd, UINT umsg,
        WPARAM wparam, LPARAM lparam)
{
    ScintillaViewUI* ui;

    switch (umsg)
    {
        case WM_CREATE:
            ui = (ScintillaViewUI*)((LPCREATESTRUCT)lparam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToUlong(ui));
            return 0;

        case WM_SETFOCUS:
            ui = reinterpret_cast<ScintillaViewUI*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            SetFocus(ui->_hSci);
            return 0;

        case WM_NOTIFY:
            ui = reinterpret_cast<ScintillaViewUI*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));

            switch (((LPNMHDR)lparam)->code)
            {
                case SCN_STYLENEEDED:
                    ui->onStyleNeeded((SCNotification*)lparam);
                    return 0;

                case SCN_DOUBLECLICK:
                    ui->onDoubleClick((SCNotification*)lparam);
                    return 0;

                case SCN_MARGINCLICK:
                    ui->onMarginClick((SCNotification*)lparam);
                    return 0;

                case SCN_CHARADDED:
                    ui->onCharAddTry((SCNotification*)lparam);
                    return 0;
            }
        break;

        case WM_CONTEXTMENU:
            ui = reinterpret_cast<ScintillaViewUI*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            ui->onClose();
        break;

        case WM_SIZE:
            ui = reinterpret_cast<ScintillaViewUI*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            ui->onResize(LOWORD(lparam), HIWORD(lparam));
            return 0;

        case WM_DESTROY:
            return 0;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}
