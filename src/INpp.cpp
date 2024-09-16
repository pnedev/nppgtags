/**
 *  \file
 *  \brief  Npp API
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


#include "INpp.h"
#include <map>


/**
 *  \brief
 */
void INpp::MultiSelectBefore(intptr_t len) const
{
    const int selectionsCount = GetSelectionsCount();

    for (int i = 0; i < selectionsCount; ++i)
    {
        const intptr_t pos = SendMessage(_hSC, SCI_GETSELECTIONNCARET, i, 0);

        SendMessage(_hSC, SCI_SETSELECTIONNANCHOR, i, (pos < len) ? 0 : pos - len);
    }
}


/**
 *  \brief
 */
void INpp::ClearSelectionMulti() const
{
    const int selectionsCount = GetSelectionsCount();

    for (int i = 0; i < selectionsCount; ++i)
    {
        const intptr_t pos = SendMessage(_hSC, SCI_GETSELECTIONNCARET, i, 0);

        SendMessage(_hSC, SCI_SETSELECTIONNANCHOR, i, pos);
    }
}


/**
 *  \brief
 */
void INpp::MultiOffsetPos(intptr_t len) const
{
    const int selectionsCount = GetSelectionsCount();

    for (int i = 0; i < selectionsCount; ++i)
    {
        const intptr_t pos = SendMessage(_hSC, SCI_GETSELECTIONNCARET, i, 0);

        SendMessage(_hSC, SCI_SETSELECTIONNCARET, i, pos + len);
    }
}


/**
 *  \brief
 */
void INpp::ClearUnmatchingWordMultiSel(const CTextA& word) const
{
    const size_t len = word.Len();

    CTextA selWord(len + 1);

    int i;
    int selectionsCount;

    do
    {
        selectionsCount = GetSelectionsCount();

        for (i = 0; i < selectionsCount; ++i)
        {
            const intptr_t pos = SendMessage(_hSC, SCI_GETSELECTIONNCARET, i, 0);
            const intptr_t wordStart = SendMessage(_hSC, SCI_WORDSTARTPOSITION, pos, true);

            if (pos - wordStart != static_cast<intptr_t>(len))
            {
                SendMessage(_hSC, SCI_DROPSELECTIONN, i, 0);
                break;
            }
            else
            {
                struct Sci_TextRangeFull tr;
                tr.chrg.cpMin   = pos - len;
                tr.chrg.cpMax   = pos;
                tr.lpstrText    = selWord.C_str();

                SendMessage(_hSC, SCI_GETTEXTRANGEFULL, 0, (LPARAM)&tr);
                selWord.AutoFit();

                if (selWord != word)
                {
                    SendMessage(_hSC, SCI_DROPSELECTIONN, i, 0);
                    break;
                }
            }
        }
    }
    while (i < selectionsCount);
}


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
intptr_t INpp::GetWord(CTextA& word, bool partial, bool select, bool multiSel) const
{
    const intptr_t currPos    = SendMessage(_hSC, SCI_GETCURRENTPOS, 0, 0);
    const intptr_t wordStart  = SendMessage(_hSC, SCI_WORDSTARTPOSITION, currPos, true);
    const intptr_t wordEnd    = partial ? currPos : SendMessage(_hSC, SCI_WORDENDPOSITION, currPos, true);

    const intptr_t len = wordEnd - wordStart;

    if (len == 0)
    {
        word.Clear();
        return len;
    }

    word.Resize(len + 1);

    struct Sci_TextRangeFull tr;
    tr.chrg.cpMin   = wordStart;
    tr.chrg.cpMax   = wordEnd;
    tr.lpstrText    = word.C_str();

    SendMessage(_hSC, SCI_GETTEXTRANGEFULL, 0, (LPARAM)&tr);
    word.AutoFit();

    if (select)
    {
        if (multiSel)
        {
            ClearUnmatchingWordMultiSel(word);

            if (wordEnd != currPos)
                MultiOffsetPos(wordEnd - currPos);

            MultiSelectBefore(len);
        }
        else
        {
            SetMainSelection(wordStart, wordEnd);
        }
    }
    else
    {
        ClearSelectionMulti();
    }

    return len;
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
void INpp::ReplaceWordMulti(const char* replText, bool partial) const
{
    const int selectionsCount = GetSelectionsCount();

    for (int i = 0; i < selectionsCount; ++i)
    {
        const intptr_t pos  = SendMessage(_hSC, SCI_GETSELECTIONNCARET, i, 0);
        intptr_t wordStart  = SendMessage(_hSC, SCI_WORDSTARTPOSITION, pos, true);
        intptr_t wordEnd    = partial ? pos : SendMessage(_hSC, SCI_WORDENDPOSITION, pos, true);

        SendMessage(_hSC, SCI_SETTARGETSTART, wordStart, 0);
        SendMessage(_hSC, SCI_SETTARGETEND, wordEnd, 0);

        SendMessage(_hSC, SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)replText);
        wordEnd = wordStart + strlen(replText);
        SendMessage(_hSC, SCI_SETSELECTIONNANCHOR, i, wordEnd);
        SendMessage(_hSC, SCI_SETSELECTIONNCARET, i, wordEnd);
    }
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
void INpp::InsertTextAtMultiPos(const char* txt) const
{
    const int txtLen = strlen(txt);

    const intptr_t mainPos =
        SendMessage(_hSC, SCI_GETSELECTIONNANCHOR, SendMessage(_hSC, SCI_GETMAINSELECTION, 0, 0), 0);

    std::map<intptr_t, intptr_t> selSorter;

    const int selectionsCount = GetSelectionsCount();

    for (int i = 0; i < selectionsCount; ++i)
        selSorter.emplace(
                SendMessage(_hSC, SCI_GETSELECTIONNANCHOR, i, 0), SendMessage(_hSC, SCI_GETSELECTIONNCARET, i, 0));

    std::vector<std::pair<intptr_t, intptr_t>> selections;
    std::pair<intptr_t, intptr_t> mainSel;

    int selOffset = 0;

    for (auto& sel : selSorter)
    {
        InsertTextAt(sel.second + selOffset, txt);

        if (sel.first != mainPos)
        {
            selections.emplace_back(sel.first + selOffset, sel.second + selOffset + txtLen);
        }
        else
        {
            mainSel.first = sel.first + selOffset;
            mainSel.second = sel.second + selOffset + txtLen;
        }

        selOffset += txtLen;
    }

    selections.emplace_back(mainSel);

    SendMessage(_hSC, SCI_SETSELECTION, selections[0].second, selections[0].first);

    for (size_t i = 1; i < selections.size(); ++i)
        SendMessage(_hSC, SCI_ADDSELECTION, selections[i].second, selections[i].first);
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
