// this file is part of notepad++
// Copyright (C)2003 Don HO <donho@altern.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <windows.h>
#include <tchar.h>
#include "NppAPI/PluginInterface.h"
#include "NppAPI/Notepad_plus_msgs.h"
#include "NppAPI/menuCmdID.h"
#include "Common.h"
#include "INpp.h"
#include "ResultWin.h"
#include "GTags.h"


BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reasonForCall, LPVOID )
{
    switch (reasonForCall)
    {
        case DLL_PROCESS_ATTACH:
            return GTags::PluginLoad(hModule);

        case DLL_PROCESS_DETACH:
            GTags::PluginDeInit();
        break;

        case DLL_THREAD_ATTACH:
        break;

        case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}


extern "C" __declspec(dllexport) void setInfo(NppData nppData)
{
    INpp& npp = INpp::Get();
    npp.SetData(nppData);

    GTags::PluginInit();
}


extern "C" __declspec(dllexport) const TCHAR* getName()
{
    return GTags::cPluginName;
}


extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbF)
{
    *nbF = _countof(GTags::Menu);
    return GTags::Menu;
}


extern "C" __declspec(dllexport) void beNotified(SCNotification* notifyCode)
{
    switch (notifyCode->nmhdr.code)
    {
        case NPPN_FILESAVED:
        {
            CPath file;
            INpp::Get().GetFilePathFromBufID(notifyCode->nmhdr.idFrom, file);
            GTags::OnFileChange(file);
        }
        break;

        case NPPN_FILEBEFORERENAME:
        case NPPN_FILEBEFOREDELETE:
        {
            CPath file;
            INpp::Get().GetFilePathFromBufID(notifyCode->nmhdr.idFrom, file);
            GTags::OnFileBeforeChange(file);
        }
        break;

        case NPPN_FILERENAMECANCEL:
        case NPPN_FILEDELETEFAILED:
            GTags::OnFileChangeCancel();
        break;

        case NPPN_FILERENAMED:
        {
            CPath file;
            INpp::Get().GetFilePathFromBufID(notifyCode->nmhdr.idFrom, file);
            GTags::OnFileRename(file);
        }
        break;

        case NPPN_FILEDELETED:
        {
            CPath file;
            INpp::Get().GetFilePathFromBufID(notifyCode->nmhdr.idFrom, file);
            GTags::OnFileDelete(file);
        }
        break;

        case NPPN_WORDSTYLESUPDATED:
        {
            INpp& npp = INpp::Get();
            char font[32];

            npp.GetFontName(STYLE_DEFAULT, font);
            GTags::UIFontName = font;
            GTags::UIFontSize = (unsigned)npp.GetFontSize(STYLE_DEFAULT);
            GTags::ResultWin::ApplyStyle();
        }
        break;

        case SCN_CHARADDED:
        {
            if (notifyCode->characterSource == SC_CHARACTERSOURCE_DIRECT_INPUT)
                GTags::SciAutoComplete(notifyCode->ch);
        }
        break;

        case NPPN_READY:
            GTags::OnNppReady();
        break;

        case NPPN_SHUTDOWN:
            GTags::PluginDeInit();
        break;
    }
}


extern "C" __declspec(dllexport) LRESULT messageProc(UINT , WPARAM , LPARAM )
{
    return TRUE;
}


extern "C" __declspec(dllexport) BOOL isUnicode()
{
#ifdef UNICODE
    return TRUE;
#else
    return FALSE;
#endif
}
