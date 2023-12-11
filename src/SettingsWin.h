/**
 *  \file
 *  \brief  GTags config window
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2015-2022 Pavel Nedev
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


#include <windows.h>
#include <tchar.h>
#include "DbManager.h"
#include "Config.h"
#include "CmdDefines.h"
#include "GTags.h"


class CPath;


namespace GTags
{

/**
 *  \class  SettingsWin
 *  \brief
 */
class SettingsWin
{
public:
    static void Show();
    static void Show(const DbHandle& db);

private:
    /**
     *  \struct  Tab
     *  \brief
     */
    struct Tab
    {
        Tab(const DbHandle db = DbHandle(NULL));
        ~Tab();
        Tab& operator=(const Tab&) = delete;

        DbHandle _db;
        DbConfig _cfg;

        bool _updateDb;
    };


    static const TCHAR  cClassName[];
    static const TCHAR  cHeader[];
    static const int    cBackgroundColor;
    static const int    cFontSize;

    static bool createWin();

    static void dbWriteReady(const CmdPtr_t& cmd);
    static void createDefDbCB(const CmdPtr_t& cmd);
    static void createLibDbCB(const CmdPtr_t& cmd);

    static LRESULT CALLBACK keyHookProc(int code, WPARAM wParam, LPARAM lParam);
    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    SettingsWin();
    SettingsWin(const SettingsWin&);
    ~SettingsWin();

    HWND composeWindow(HWND hOwner);

    Tab* getTab(int i = -1);
    bool isDbOpen(const CPath& dbPath);
    void onUpdateDefDb();
    void onUpdateLibDb();
    void onTabChange();
    void onSave();
    void fillTabData();
    void readTabData();
    bool saveDbConfig(Tab* tab);
    bool saveSettings(const Settings& newSettings);
    bool saveTab(Tab* tab);
    void fillDefDb(const CPath& defDb);
    void fillMissing(HWND SettingsWin::*editCtrl, const CText& entry);
    inline void fillLibDb(const CPath& lib);
    inline void fillPathFilter(const CPath& filter);

    bool createDatabase(CPath& dbPath, CompletionCB complCB);

    static SettingsWin* SW;

    Tab*        _activeTab;

    HWND        _hWnd;
    HWND        _hEnDefDb;
    HWND        _hSetDefDb;
    HWND        _hUpdDefDb;
    HWND        _hDefDb;
    HWND        _hTab;
    HWND        _hInfo;
    HWND        _hParserInfo;
    HWND        _hParser;
    HWND        _hAutoUpdDb;
    HWND        _hSciAutoCInfo;
    HWND        _hSciAutoCEn;
    HWND        _hSciAutoCIC;
    HWND        _hSciAutoCFromCharPreInfo;
    HWND        _hSciAutoCFromChar;
    HWND        _hSciAutoCFromCharPostInfo;
    HWND        _hEnLibDb;
    HWND        _hAddLibDb;
    HWND        _hUpdLibDbs;
    HWND        _hLibDbs;
    HWND        _hEnPathFilter;
    HWND        _hAddPathFilter;
    HWND        _hPathFilters;
    HWND        _hSave;
    HWND        _hCancel;

    HHOOK       _hKeyHook;

    HFONT       _hFont;
    HFONT       _hFontInfo;
};

} // namespace GTags
