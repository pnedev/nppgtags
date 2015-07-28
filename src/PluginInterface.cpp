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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <memory>
#include "PluginInterface.h"
#include "Notepad_plus_msgs.h"
#include "menuCmdID.h"
#include "Common.h"
#include "INpp.h"
#include "Config.h"
#include "ResultWin.h"
#include "GTags.h"


namespace
{

std::unique_ptr<CPath> ChangedFile;

}


BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reasonForCall,
        LPVOID lpReserved)
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
            if (GTags::Config._autoUpdate)
            {
                TCHAR file[MAX_PATH];
                INpp::Get().GetFilePathFromBufID(
                        notifyCode->nmhdr.idFrom, file);
                GTags::UpdateSingleFile(file);
            }
        break;

        case NPPN_FILEBEFORERENAME:
        case NPPN_FILEBEFOREDELETE:
            if (GTags::Config._autoUpdate)
            {
                TCHAR file[MAX_PATH];
                INpp::Get().GetFilePathFromBufID(
                        notifyCode->nmhdr.idFrom, file);
                ChangedFile.reset(new CPath(file));
            }
        break;

        case NPPN_FILERENAMECANCEL:
        case NPPN_FILEDELETEFAILED:
            ChangedFile.reset();
        break;

        case NPPN_FILERENAMED:
            if (GTags::Config._autoUpdate)
            {
                TCHAR file[MAX_PATH];
                INpp::Get().GetFilePathFromBufID(
                        notifyCode->nmhdr.idFrom, file);
                GTags::UpdateSingleFile(file);

                if (ChangedFile)
                {
                    GTags::UpdateSingleFile(ChangedFile->C_str());
                    ChangedFile.reset();
                }
            }
        break;

        case NPPN_FILEDELETED:
            if (GTags::Config._autoUpdate)
            {
                if (ChangedFile)
                {
                    GTags::UpdateSingleFile(ChangedFile->C_str());
                    ChangedFile.reset();
                }
                else
                {
                    TCHAR file[MAX_PATH];
                    INpp::Get().GetFilePathFromBufID(
                            notifyCode->nmhdr.idFrom, file);
                    GTags::UpdateSingleFile(file);
                }
            }
        break;

        case NPPN_WORDSTYLESUPDATED:
        {
            INpp& npp = INpp::Get();
            char font[32];
            npp.GetFontName(STYLE_DEFAULT, font, _countof(font));
            Tools::AtoW(GTags::UIFontName, _countof(GTags::UIFontName), font);
            GTags::UIFontSize = (unsigned)npp.GetFontSize(STYLE_DEFAULT);
            GTags::ResultWin::ApplyStyle();
        }
        break;

        case NPPN_READY:
            GTags::PluginInit();
        break;

        case NPPN_SHUTDOWN:
            GTags::PluginDeInit();
        break;
    }
}


extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message,
        WPARAM wParam, LPARAM lParam)
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
