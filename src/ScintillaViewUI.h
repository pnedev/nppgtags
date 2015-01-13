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


#pragma once


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <vector>
#include "Scintilla.h"
#include "Common.h"
#include "AutoLock.h"
#include "GTagsCmd.h"


/**
 *  \class  ScintillaViewUI
 *  \brief
 */
class ScintillaViewUI
{
public:
    static ScintillaViewUI& Get()
    {
        static ScintillaViewUI Instance;
        return Instance;
    }

    // Use to create the static instance of the singleton
    static inline void Init()
    {
        Get();
    }

    void Show(GTags::CmdData& cmd);
    void Update();
    void ResetStyle();

private:
    /**
     *  \class  CmdBranch
     *  \brief
     */
    class CmdBranch
    {
    public:
        CmdBranch(GTags::CmdData& cmd);
        ~CmdBranch() {};

        /**
         *  \struct  Leaf
         *  \brief
         */
        struct Leaf {
            TCHAR* name;
            TCHAR* line;
            TCHAR* preview;
            TCHAR* file;
        };

        bool operator==(const CmdBranch& branch) const
        {
            return (_cmdName == branch._cmdName &&
                    _basePath == branch._basePath);
        }

        const int _cmdID;
        CPath _basePath;
        CText _cmdName;
        char _cmdTag[GTags::cMaxTagLen];
        std::vector<Leaf> _leaves;

    private:
        void parseCmdOutput();

        CText _cmdOutput;
    };

    static const COLORREF cBlack = RGB(0,0,0);
    static const COLORREF cWhite = RGB(255,255,255);

    static const TCHAR cClassName[];

    static LRESULT APIENTRY wndProc(HWND hwnd, UINT umsg,
            WPARAM wparam, LPARAM lparam);

    ScintillaViewUI();
    ScintillaViewUI(const ScintillaViewUI&);
    ~ScintillaViewUI();

    LRESULT sendSci(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        return _sciFunc(_sciPtr, static_cast<unsigned int>(Msg),
                static_cast<uptr_t>(wParam), static_cast<sptr_t>(lParam));
    }

    void setStyle(int style, COLORREF fore = cBlack, COLORREF back = cWhite,
            bool bold = false, int size = 0, const char *font = NULL);

    void composeWindow();
    void add(GTags::CmdData& cmd);
    void remove();
    void removeAll();
    bool openItem(int lineNum);
    void styleSearchWord(int lineNum, int startOffset = 0);
    void onStyleNeeded(SCNotification* notify);
    void onDoubleClick(SCNotification* notify);
    void onMarginClick(SCNotification* notify);
    void onCharAddTry(SCNotification* notify);
    void onResize(int width, int height);

    Mutex _lock;
    HWND _hWnd;
    HWND _hSci;
	SciFnDirect _sciFunc;
	sptr_t _sciPtr;

    // Only one branch possible for now - fix this!
    CmdBranch* _branch;
};
