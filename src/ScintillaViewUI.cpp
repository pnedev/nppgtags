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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include "INpp.h"
#include "DBManager.h"
#include "GTags.h"
#include "ScintillaViewUI.h"
#include "DocLocation.h"


#define SCE_GTAGS_SEARCH_HEADER  151
#define SCE_GTAGS_FILE_HEADER    152
#define SCE_GTAGS_WORD2SEARCH    153


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
void ScintillaViewUI::Show(CmdData& cmd)
{
    AUTOLOCK(_lock);

    add(cmd);

    INpp& npp = INpp::Get();
    npp.UpdateDockingWin(_hWnd);
    npp.ShowDockingWin(_hWnd);
}


/**
 *  \brief
 */
void ScintillaViewUI::Update()
{
    AUTOLOCK(_lock);

    INpp::Get().UpdateDockingWin(_hWnd);
}


/**
 *  \brief
 */
ScintillaViewUI::CmdBranch::CmdBranch(CmdData& cmd) :
    _cmdID(cmd.GetID()), _basePath(cmd.GetDBPath()),
    _cmdName(cmd.GetName()), _cmdOutput(cmd.GetResult())
{
    size_t cnt;
    wcstombs_s(&cnt, _cmdTag, sizeof(_cmdTag), cmd.GetTag(), _TRUNCATE);

    _cmdName.ToUpper();
    _cmdName += _T(" \"");
    _cmdName += _cmdTag;
    _cmdName += _T("\"");
    parseCmdOutput();
}


/**
 *  \brief
 */
void ScintillaViewUI::CmdBranch::parseCmdOutput()
{
    TCHAR* pTmp;

    for (TCHAR* pToken = _tcstok_s(_cmdOutput.C_str(), _T("\n\r"), &pTmp);
        pToken; pToken = _tcstok_s(NULL, _T("\n\r"), &pTmp))
    {
        Leaf leaf = {0};
        if (_cmdID == FIND_FILE)
            leaf.file = pToken;
        else
            leaf.name = pToken;
        _leaves.push_back(leaf);
    }

    if (_cmdID == FIND_FILE)
        return;

    for (unsigned i = 0; i < _leaves.size(); i++)
    {
        Leaf& leaf = _leaves[i];
        unsigned currentResultField = CTAGS_F_TAG;
        for (TCHAR* pToken = _tcstok_s(leaf.name, _T(" \t"), &pTmp);
            pToken; pToken = _tcstok_s(NULL, _T(" \t"), &pTmp))
        {
            switch (currentResultField)
            {
                case CTAGS_F_TAG:
                    leaf.name = pToken;
                    break;

                case CTAGS_F_LINE:
                    leaf.line = pToken;
                    break;

                case CTAGS_F_FILE:
                    leaf.file = pToken;
            }

            if (++currentResultField == CTAGS_F_PREVIEW)
            {
                while (*pTmp == _T(' ') || *pTmp == _T('\t'))
                    pTmp++;
                leaf.preview = pTmp;
                for (int j = _tcslen(leaf.preview) - 1; j > 0; j--)
                    if (leaf.preview[j] == _T('\t'))
                        leaf.preview[j] = _T(' ');
                break;
            }
        }
    }
}


/**
 *  \brief
 */
ScintillaViewUI::ScintillaViewUI() : _branch(NULL)
{
    WNDCLASS wc         = {0};
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = wndProc;
    wc.hInstance        = HInst;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetSysColorBrush(cUIBackgroundColor);
    wc.lpszClassName    = cClassName;

    RegisterClass(&wc);

    createWindow();

    tTbData data        = {0};
    data.hClient        = _hWnd;
    data.pszName        = const_cast<TCHAR*>(cPluginName);
    data.uMask          = 0;
    data.pszAddInfo     = NULL;
    data.uMask          = DWS_DF_CONT_BOTTOM;
    data.pszModuleName  = DllPath.GetFilename_C_str();
    data.dlgID          = 0;

    INpp& npp = INpp::Get();
    npp.RegisterWin(_hWnd);
    npp.RegisterDockingWin(data);
    npp.HideDockingWin(_hWnd);
}


/**
 *  \brief
 */
ScintillaViewUI::~ScintillaViewUI()
{
    removeAll();

    INpp& npp = INpp::Get();
    npp.DestroySciHandle(_hSci);
    npp.UnregisterWin(_hWnd);

    UnregisterClass(cClassName, NULL);
}


/**
 *  \brief
 */
void ScintillaViewUI::setStyle(int style, COLORREF fore, COLORREF back,
        bool bold, int size, const char *font)
{
    sendSci(SCI_STYLESETFORE, style, fore);
    sendSci(SCI_STYLESETBACK, style, back);
    sendSci(SCI_STYLESETBOLD, style, bold);
    if (size >= 1)
        sendSci(SCI_STYLESETSIZE, style, size);
    if (font)
        sendSci(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(font));
}


/**
 *  \brief
 */
void ScintillaViewUI::createWindow()
{
    INpp& npp = INpp::Get();
    HWND hOwnerWnd = npp.GetHandle();
    RECT win;
    GetWindowRect(hOwnerWnd, &win);
    DWORD style = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_SIZEBOX;
    _hWnd = CreateWindow(cClassName, cPluginName,
            style, win.left, win.top,
            win.right - win.left, win.bottom - win.top,
            hOwnerWnd, NULL, HInst, (LPVOID)this);

    _hSci = npp.CreateSciHandle(_hWnd);
	_sciFunc = (SciFnDirect)::SendMessage(_hSci, SCI_GETDIRECTFUNCTION, 0, 0);
	_sciPtr = (sptr_t)::SendMessage(_hSci, SCI_GETDIRECTPOINTER, 0, 0);

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
    sendSci(SCI_SETCARETLINEBACK, RGB(212,255,127));
    sendSci(SCI_SETCARETLINEVISIBLE, true);
    sendSci(SCI_SETCARETWIDTH);

    char font[32];
    npp.GetFontNameA(font, sizeof(font));
    int size = npp.GetFontSize();

    // Implement lexer in the container
    sendSci(SCI_SETLEXER, 0);

    sendSci(SCI_STYLERESETDEFAULT);
    setStyle(STYLE_DEFAULT, cBlack, cWhite, false, size, font);
    sendSci(SCI_STYLECLEARALL);

    setStyle(SCE_GTAGS_SEARCH_HEADER, cBlack, cWhite, true);
    setStyle(SCE_GTAGS_FILE_HEADER, RGB(10,100,100), cWhite, true);
    setStyle(SCE_GTAGS_WORD2SEARCH, RGB(200,0,30), cWhite, true);

    sendSci(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"),
            reinterpret_cast<LPARAM>("1"));

    sendSci(SCI_SETMARGINTYPEN, 1, SC_MARGIN_SYMBOL);
    sendSci(SCI_SETMARGINMASKN, 1, SC_MASK_FOLDERS);
    sendSci(SCI_SETMARGINWIDTHN, 1, 20);

    sendSci(SCI_SETFOLDMARGINCOLOUR, 1, cBlack);
    sendSci(SCI_SETFOLDMARGINHICOLOUR, 1, cBlack);

    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
    sendSci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID,
            SC_MARK_BOXMINUSCONNECTED);

    sendSci(SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED);
    sendSci(SCI_SETMARGINSENSITIVEN, 1, 1);

    sendSci(SCI_SETREADONLY, true);

    ShowWindow(_hSci, SW_SHOW);
}


/**
 *  \brief
 */
void ScintillaViewUI::add(CmdData& cmd)
{
    remove();

    _branch = new CmdBranch(cmd);

    char str[512];
    size_t cnt;

    sendSci(SCI_SETSEL);
    sendSci(SCI_SETREADONLY, false);

    if (sendSci(SCI_GETLINECOUNT) > 1)
    {
        sendSci(SCI_ADDTEXT, 1, reinterpret_cast<LPARAM>("\n"));
        sendSci(SCI_SETSEL);
    }

    wcstombs_s(&cnt, str, 512, _branch->_cmdName.C_str(), _TRUNCATE);
    sendSci(SCI_ADDTEXT, strlen(str), reinterpret_cast<LPARAM>(str));

    unsigned leaves_cnt = _branch->_leaves.size();
    for (unsigned i = 0; i < leaves_cnt; i++)
    {
        CmdBranch::Leaf& leaf = _branch->_leaves[i];

        sendSci(SCI_ADDTEXT, 2, reinterpret_cast<LPARAM>("\n\t"));
        wcstombs_s(&cnt, str, 512, leaf.file, _TRUNCATE);
        sendSci(SCI_ADDTEXT, strlen(str), reinterpret_cast<LPARAM>(str));

        if (_branch->_cmdID != FIND_FILE)
        {
            for (; i < leaves_cnt; i++)
            {
                CmdBranch::Leaf& next = _branch->_leaves[i];
                if (_tcscmp(next.file, leaf.file))
                {
                    i--;
                    break;
                }

                sendSci(SCI_ADDTEXT, 8,
                        reinterpret_cast<LPARAM>("\n\t\tline "));
                wcstombs_s(&cnt, str, 512, next.line, _TRUNCATE);
                sendSci(SCI_ADDTEXT, strlen(str),
                        reinterpret_cast<LPARAM>(str));
                sendSci(SCI_ADDTEXT, 2,
                        reinterpret_cast<LPARAM>(":\t"));
                wcstombs_s(&cnt, str, 512, next.preview, _TRUNCATE);
                sendSci(SCI_ADDTEXT, strlen(str),
                        reinterpret_cast<LPARAM>(str));
            }
        }
    }

    sendSci(SCI_SETREADONLY, true);
    sendSci(SCI_SETSEL);
}


/**
 *  \brief
 */
void ScintillaViewUI::remove()
{
    sendSci(SCI_SETREADONLY, false);
    sendSci(SCI_CLEARALL);
    sendSci(SCI_SETREADONLY, true);

    if (_branch)
    {
        delete _branch;
        _branch = NULL;
    }

    INpp& npp = INpp::Get();
    npp.UpdateDockingWin(_hWnd);
    npp.HideDockingWin(_hWnd);
    SetFocus(npp.ReadSciHandle());
}


/**
 *  \brief
 */
void ScintillaViewUI::removeAll()
{
    AUTOLOCK(_lock);

    sendSci(SCI_SETREADONLY, false);
    sendSci(SCI_CLEARALL);
    sendSci(SCI_SETREADONLY, true);

    if (_branch)
    {
        delete _branch;
        _branch = NULL;
    }

    INpp& npp = INpp::Get();
    npp.UpdateDockingWin(_hWnd);
    npp.HideDockingWin(_hWnd);
    SetFocus(npp.ReadSciHandle());
}


/**
 *  \brief
 */
bool ScintillaViewUI::openItem(int lineNum)
{
    int lineLen = sendSci(SCI_LINELENGTH, lineNum);
    char* lineTxt = new char[lineLen];

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
        lineTxt = new char[lineLen];
        sendSci(SCI_GETLINE, lineNum, reinterpret_cast<LPARAM>(lineTxt));
    }

    for (i = 1; lineTxt[i] != '\r' && lineTxt[i] != '\n'; i++);
    lineTxt[i] = 0;

    CPath file(_branch->_basePath);
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
    npp.OpenFile(file);
    SetFocus(npp.ReadSciHandle());

    // GTags command was FIND_FILE
    if (line == -1)
    {
        npp.ClearSelection();
        return true;
    }

    bool wholeWord =
            (_branch->_cmdID != GREP && _branch->_cmdID != FIND_LITERAL);

    if (!npp.SearchText(_branch->_cmdTag, true, wholeWord,
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
        if (lineLen > 0)
        {
            char firstChar = sendSci(SCI_GETCHARAT, startPos);
            char secondChar = sendSci(SCI_GETCHARAT, startPos + 1);

            if (firstChar == '\t')
            {
                if (secondChar == '\t')
                {
                    struct TextToFind ttf = {0};
                    ttf.lpstrText = const_cast<char*>(_branch->_cmdTag);
                    ttf.chrg.cpMin = startPos + 2;
                    ttf.chrg.cpMax = startPos + lineLen;

                    if (sendSci(SCI_FINDTEXT, SCFIND_MATCHCASE,
                        reinterpret_cast<LPARAM>(&ttf)) != -1)
                    {
                        sendSci(SCI_STARTSTYLING, ttf.chrgText.cpMin, 0xFF);
                        sendSci(SCI_SETSTYLING,
                                ttf.chrgText.cpMax - ttf.chrgText.cpMin,
                                SCE_GTAGS_WORD2SEARCH);
                        sendSci(SCI_SETFOLDLEVEL, lineNum, RESULT_LVL);
                    }
                }
                else
                {
                    sendSci(SCI_STARTSTYLING, startPos, 0xFF);
                    sendSci(SCI_SETSTYLING, lineLen, SCE_GTAGS_FILE_HEADER);
                    sendSci(SCI_SETFOLDLEVEL, lineNum,
                            FILE_HEADER_LVL |
                            (_branch->_cmdID == FIND_FILE ?
                            0 : SC_FOLDLEVELHEADERFLAG));
                }
            }
            else
            {
                sendSci(SCI_STARTSTYLING, startPos, 0xFF);
                sendSci(SCI_SETSTYLING, lineLen, SCE_GTAGS_SEARCH_HEADER);
                sendSci(SCI_SETFOLDLEVEL, lineNum,
                        SEARCH_HEADER_LVL | SC_FOLDLEVELHEADERFLAG);
            }
        }
    }
}


/**
 *  \brief
 */
void ScintillaViewUI::onDoubleClick(SCNotification* notify)
{
    AUTOLOCK(_lock);

    const int lineNum = sendSci(SCI_LINEFROMPOSITION, notify->position);

    if (sendSci(SCI_GETFOLDLEVEL, lineNum) & SC_FOLDLEVELHEADERFLAG)
        sendSci(SCI_TOGGLEFOLD, lineNum);
    else
        openItem(lineNum);
}


/**
 *  \brief
 */
void ScintillaViewUI::onMarginClick(SCNotification* notify)
{
    AUTOLOCK(_lock);

    const int lineNum = sendSci(SCI_LINEFROMPOSITION, notify->position);

    if (notify->margin == 1)
        sendSci(SCI_TOGGLEFOLD, lineNum);
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
            }
            break;

        case WM_CONTEXTMENU:
            ui = reinterpret_cast<ScintillaViewUI*>(static_cast<LONG_PTR>
                    (GetWindowLongPtr(hwnd, GWLP_USERDATA)));
            ui->removeAll();
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
