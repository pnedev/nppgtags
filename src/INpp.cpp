/**
 *  \file
 *  \brief  Npp API
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


#include "INpp.h"


INpp INpp::Instance;


/**
 *  \brief
 */
long INpp::GetWord(char* buf, int bufSize, bool select) const
{
    long currPos = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
    long wordStart = SendMessage(_hSC, SCI_WORDSTARTPOSITION,
            (WPARAM)currPos, (LPARAM)true);
    long wordEnd = SendMessage(_hSC, SCI_WORDENDPOSITION,
            (WPARAM)currPos, (LPARAM)true);

    long len = wordEnd - wordStart;
    if (len != 0)
    {
        if (select)
            SendMessage(_hSC, SCI_SETSEL, (WPARAM)wordStart, (LPARAM)wordEnd);
        else
            SendMessage(_hSC, SCI_SETSEL, (WPARAM)wordEnd, (LPARAM)wordEnd);

        if (bufSize > len)
        {
            struct TextRange tr = { { wordStart, wordEnd }, buf };
            SendMessage(_hSC, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);
        }
    }

    return len;
}


/**
 *  \brief
 */
void INpp::ReplaceWord(const char* replText) const
{
    long currPos = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
    long wordStart = SendMessage(_hSC, SCI_WORDSTARTPOSITION,
            (WPARAM)currPos, (LPARAM)true);
    long wordEnd = SendMessage(_hSC, SCI_WORDENDPOSITION,
            (WPARAM)currPos, (LPARAM)true);

    SendMessage(_hSC, SCI_SETTARGETSTART, (WPARAM)wordStart, 0);
    SendMessage(_hSC, SCI_SETTARGETEND, (WPARAM)wordEnd, 0);

    SendMessage(_hSC, SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)replText);
    wordEnd = wordStart + strlen(replText);
    SendMessage(_hSC, SCI_SETSEL, (WPARAM)wordEnd, (LPARAM)wordEnd);
}


/**
 *  \brief
 */
bool INpp::SearchText(const char* text,
        bool matchCase, bool wholeWord, bool regExpr,
        long startPos = 0, long endPos = 0) const
{
    if (startPos < 0)
        startPos = 0;

    if (endPos <= 0)
        endPos = SendMessage(_hSC, SCI_GETLENGTH, 0, 0);

    int searchFlags = 0;
    if (regExpr)
    {
        searchFlags |= (SCFIND_REGEXP | SCFIND_POSIX);
    }
    else
    {
        if (matchCase)
            searchFlags |= SCFIND_MATCHCASE;
        if (wholeWord)
            searchFlags |= SCFIND_WHOLEWORD;
    }

    SendMessage(_hSC, SCI_SETSEARCHFLAGS, (WPARAM)searchFlags, 0);
    SendMessage(_hSC, SCI_SETTARGETSTART, (WPARAM)startPos, 0);
    SendMessage(_hSC, SCI_SETTARGETEND, (WPARAM)endPos, 0);

    SendMessage(_hSC, SCI_SETSEL, (WPARAM)startPos, (LPARAM)endPos);
    if (SendMessage(_hSC, SCI_SEARCHINTARGET, strlen(text),
            reinterpret_cast<LPARAM>(text)) < 0)
        return false;

    startPos = SendMessage(_hSC, SCI_GETTARGETSTART, 0, 0);
    endPos = SendMessage(_hSC, SCI_GETTARGETEND, 0, 0);

    long lineNum =
        SendMessage(_hSC, SCI_LINEFROMPOSITION, (WPARAM)startPos, 0) - 5;
    if (lineNum < 0)
        lineNum = 0;
    SendMessage(_hSC, SCI_SETFIRSTVISIBLELINE, (WPARAM)lineNum, 0);
    SendMessage(_hSC, SCI_SETSEL, (WPARAM)startPos, (LPARAM)endPos);

    return true;
}
