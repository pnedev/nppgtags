/**
 *  \file
 *  \brief  Npp API
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


#include "INpp.h"


/**
 *  \brief
 */
intptr_t INpp::GetWordSize(bool partial) const
{
    intptr_t currPos    = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
    intptr_t wordStart  = SendMessage(_hSC, SCI_WORDSTARTPOSITION, currPos, true);

    if (partial)
        return currPos - wordStart;

    intptr_t wordEnd = SendMessage(_hSC, SCI_WORDENDPOSITION, currPos, true);

    return wordEnd - wordStart;
}


/**
 *  \brief
 */
void INpp::GetWord(CTextA& word, bool partial, bool select) const
{
    intptr_t currPos    = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
    intptr_t wordStart  = SendMessage(_hSC, SCI_WORDSTARTPOSITION, currPos, true);
    intptr_t wordEnd    = partial ? currPos : SendMessage(_hSC, SCI_WORDENDPOSITION, currPos, true);

    intptr_t len = wordEnd - wordStart;
    if (len == 0)
    {
        word.Clear();
        return;
    }

    if (select)
        SendMessage(_hSC, SCI_SETSEL, wordStart, wordEnd);
    else
        SendMessage(_hSC, SCI_SETSEL, wordEnd, wordEnd);

    word.Resize(len + 1);

    struct Sci_TextRangeFull tr;
    tr.chrg.cpMin   = wordStart;
    tr.chrg.cpMax   = wordEnd;
    tr.lpstrText    = word.C_str();

    SendMessage(_hSC, SCI_GETTEXTRANGEFULL, 0, (LPARAM)&tr);
    word.AutoFit();
}


/**
 *  \brief
 */
void INpp::ReplaceWord(const char* replText, bool partial) const
{
    intptr_t currPos    = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
    intptr_t wordStart  = SendMessage(_hSC, SCI_WORDSTARTPOSITION, currPos, true);
    intptr_t wordEnd    = partial ? currPos : SendMessage(_hSC, SCI_WORDENDPOSITION, currPos, true);

    SendMessage(_hSC, SCI_SETTARGETSTART, wordStart, 0);
    SendMessage(_hSC, SCI_SETTARGETEND, wordEnd, 0);

    SendMessage(_hSC, SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)replText);
    wordEnd = wordStart + strlen(replText);
    SendMessage(_hSC, SCI_SETSEL, wordEnd, wordEnd);
}


/**
 *  \brief
 */
void INpp::EnsureCurrentLineVisible() const
{
    intptr_t lineNum = SendMessage(_hSC, SCI_LINEFROMPOSITION, SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0), 0);
    SendMessage(_hSC, SCI_ENSUREVISIBLE, lineNum, 0);
    lineNum = SendMessage(_hSC, SCI_VISIBLEFROMDOCLINE, lineNum, 0);

    intptr_t linesOnScreen       = SendMessage(_hSC, SCI_LINESONSCREEN, 0, 0);
    intptr_t firstVisibleLine    = SendMessage(_hSC, SCI_GETFIRSTVISIBLELINE, 0, 0);

    if (lineNum < firstVisibleLine || lineNum > firstVisibleLine + linesOnScreen)
    {
        lineNum -= linesOnScreen / 2;
        if (lineNum < 0)
            lineNum = 0;
        SendMessage(_hSC, SCI_SETFIRSTVISIBLELINE, lineNum, 0);
    }
}


/**
 *  \brief
 */
void INpp::SetView(intptr_t startPos, intptr_t endPos) const
{
    if (endPos == 0)
        endPos = startPos;
    intptr_t lineNum = SendMessage(_hSC, SCI_LINEFROMPOSITION, startPos, 0);
    SendMessage(_hSC, SCI_ENSUREVISIBLE, lineNum, 0);

    intptr_t linesOnScreen = SendMessage(_hSC, SCI_LINESONSCREEN, 0, 0);
    lineNum = SendMessage(_hSC, SCI_VISIBLEFROMDOCLINE, lineNum, 0) - linesOnScreen / 2;
    if (lineNum < 0)
        lineNum = 0;
    SendMessage(_hSC, SCI_SETSEL, startPos, endPos);
    SendMessage(_hSC, SCI_SETFIRSTVISIBLELINE, lineNum, 0);
}


/**
 *  \brief
 */
bool INpp::SearchText(const char* text, bool ignoreCase, bool wholeWord, bool regExp,
        intptr_t* startPos, intptr_t* endPos) const
{
    if (startPos == NULL || endPos == NULL)
        return false;

    if (*startPos < 0)
        *startPos = 0;

    if (*endPos <= 0)
        *endPos = SendMessage(_hSC, SCI_GETLENGTH, 0, 0);

    int searchFlags = 0;
    if (!ignoreCase)
        searchFlags |= SCFIND_MATCHCASE;
    if (wholeWord)
        searchFlags |= SCFIND_WHOLEWORD;
    if (regExp)
        searchFlags |= (SCFIND_REGEXP | SCFIND_POSIX);

    SendMessage(_hSC, SCI_SETSEARCHFLAGS, (WPARAM)searchFlags, 0);
    SendMessage(_hSC, SCI_SETTARGETSTART, *startPos, 0);
    SendMessage(_hSC, SCI_SETTARGETEND, *endPos, 0);

    SendMessage(_hSC, SCI_SETSEL, *startPos, *endPos);
    bool isFound = (SendMessage(_hSC, SCI_SEARCHINTARGET, strlen(text), (LPARAM)text) >= 0);

    *startPos = SendMessage(_hSC, SCI_GETTARGETSTART, 0, 0);
    *endPos = SendMessage(_hSC, SCI_GETTARGETEND, 0, 0);

    SetView(*startPos, *endPos);

    return isFound;
}


/**
 *  \brief
 */
bool INpp::isSciWndRegistered(HWND hSciWnd)
{
    for (std::vector<HWND>::iterator iSci = _hSciWndList.begin(); iSci != _hSciWndList.end(); ++iSci)
    {
        if (*iSci == hSciWnd)
            return true;
    }

    return false;
}
