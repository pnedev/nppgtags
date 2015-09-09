/**
 *  \file
 *  \brief  Npp API
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


#include "INpp.h"


INpp INpp::Instance;


/**
 *  \brief
 */
std::string INpp::GetWord(bool select) const
{
    long currPos    = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
    long wordStart  = SendMessage(_hSC, SCI_WORDSTARTPOSITION, currPos, true);
    long wordEnd    = SendMessage(_hSC, SCI_WORDENDPOSITION, currPos, true);

    long len = wordEnd - wordStart;
    if (len == 0)
        return std::string();

    if (select)
        SendMessage(_hSC, SCI_SETSEL, wordStart, wordEnd);
    else
        SendMessage(_hSC, SCI_SETSEL, wordEnd, wordEnd);

    std::vector<char> buf;
    buf.resize(len);

    struct TextRange tr = { { wordStart, wordEnd }, buf.data() };
    SendMessage(_hSC, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

    return std::string(buf.cbegin(), buf.cend());
}


/**
 *  \brief
 */
void INpp::ReplaceWord(const char* replText) const
{
    long currPos    = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
    long wordStart  = SendMessage(_hSC, SCI_WORDSTARTPOSITION, currPos, true);
    long wordEnd    = SendMessage(_hSC, SCI_WORDENDPOSITION, currPos, true);

    SendMessage(_hSC, SCI_SETTARGETSTART, wordStart, 0);
    SendMessage(_hSC, SCI_SETTARGETEND, wordEnd, 0);

    SendMessage(_hSC, SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)replText);
    wordEnd = wordStart + strlen(replText);
    SendMessage(_hSC, SCI_SETSEL, wordEnd, wordEnd);
}


/**
 *  \brief
 */
void INpp::SetView(long startPos, long endPos) const
{
    if (endPos == 0)
        endPos = startPos;
    long lineNum = SendMessage(_hSC, SCI_LINEFROMPOSITION, startPos, 0);
    SendMessage(_hSC, SCI_ENSUREVISIBLE, lineNum, 0);

    int linesOnScreen = SendMessage(_hSC, SCI_LINESONSCREEN, 0, 0);
    lineNum = SendMessage(_hSC, SCI_VISIBLEFROMDOCLINE, lineNum, 0) - linesOnScreen / 2;
    if (lineNum < 0)
        lineNum = 0;
    SendMessage(_hSC, SCI_SETFIRSTVISIBLELINE, lineNum, 0);
    SendMessage(_hSC, SCI_SETSEL, startPos, endPos);
}


/**
 *  \brief
 */
bool INpp::SearchText(const char* text, bool matchCase, bool wholeWord, bool regExp,
        long* startPos, long* endPos) const
{
    if (startPos == NULL || endPos == NULL)
        return false;

    if (*startPos < 0)
        *startPos = 0;

    if (*endPos <= 0)
        *endPos = SendMessage(_hSC, SCI_GETLENGTH, 0, 0);

    int searchFlags = 0;
    if (matchCase)
        searchFlags |= SCFIND_MATCHCASE;
    if (wholeWord)
        searchFlags |= SCFIND_WHOLEWORD;
    if (regExp)
        searchFlags |= (SCFIND_REGEXP | SCFIND_POSIX);

    SendMessage(_hSC, SCI_SETSEARCHFLAGS, (WPARAM)searchFlags, 0);
    SendMessage(_hSC, SCI_SETTARGETSTART, *startPos, 0);
    SendMessage(_hSC, SCI_SETTARGETEND, *endPos, 0);

    SendMessage(_hSC, SCI_SETSEL, *startPos, *endPos);
    if (SendMessage(_hSC, SCI_SEARCHINTARGET, strlen(text), (LPARAM)text) < 0)
        return false;

    *startPos = SendMessage(_hSC, SCI_GETTARGETSTART, 0, 0);
    *endPos = SendMessage(_hSC, SCI_GETTARGETEND, 0, 0);

    SetView(*startPos, *endPos);

    return true;
}
