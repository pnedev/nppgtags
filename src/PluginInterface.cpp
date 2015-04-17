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


#include "PluginInterface.h"
#include "Notepad_plus_msgs.h"
#include "menuCmdID.h"
#include "Common.h"
#include "INpp.h"
#include "GTags.h"
#include "ResultWin.h"


namespace
{

const int cFuncCount = 16;
FuncItem InterfaceFunc[cFuncCount];

CPathContainer StoredPath;


/**
 *  \brief
 */
void addMenuItem(const TCHAR* itemName = NULL,
        PFUNCPLUGINCMD cmdFunc = NULL, bool initCheckMark = false)
{
    static int i = 0;

    if (itemName != NULL)
        _tcscpy_s(InterfaceFunc[i]._itemName, PLUGIN_ITEM_SIZE, itemName);
    InterfaceFunc[i]._pFunc = cmdFunc;
    InterfaceFunc[i++]._init2Check = initCheckMark;
}

}


BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reasonForCall,
        LPVOID lpReserved)
{
    switch (reasonForCall)
    {
        case DLL_PROCESS_ATTACH:
            return GTags::PluginInit(hModule);

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

    char font[32];
    npp.GetFontName(STYLE_DEFAULT, font, _countof(font));
    Tools::AtoW(GTags::UIFontName, _countof(GTags::UIFontName), font);
    GTags::UIFontSize = (unsigned)npp.GetFontSize(STYLE_DEFAULT);

    if (GTags::ResultWin::Get().Register())
        MessageBox(npp.GetHandle(),
            _T("ResultWin init failed, plugin will not be operational"),
            GTags::cPluginName, MB_OK | MB_ICONERROR);

    ZeroMemory(InterfaceFunc, sizeof(InterfaceFunc));

    addMenuItem(GTags::cAutoCompl, GTags::AutoComplete);
    addMenuItem(GTags::cAutoComplFile, GTags::AutoCompleteFile);
    addMenuItem(GTags::cFindFile, GTags::FindFile);
    addMenuItem(GTags::cFindDefinition, GTags::FindDefinition);
    addMenuItem(GTags::cFindReference, GTags::FindReference);
    addMenuItem(GTags::cGrep, GTags::Grep);
    addMenuItem(); // separator
    addMenuItem(_T("Go Back"), GTags::GoBack);
    addMenuItem(_T("Go Forward"), GTags::GoForward);
    addMenuItem(); // separator
    addMenuItem(GTags::cCreateDatabase, GTags::CreateDatabase);
    addMenuItem(_T("Delete Database"), GTags::DeleteDatabase);
    addMenuItem(); // separator
    addMenuItem(_T("Settings"), GTags::SettingsCfg);
    addMenuItem(); // separator
    addMenuItem(GTags::cVersion, GTags::About);
}


extern "C" __declspec(dllexport) const TCHAR* getName()
{
    return GTags::cPluginName;
}


extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbF)
{
    *nbF = cFuncCount;
    return InterfaceFunc;
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
                StoredPath = file;
            }
        break;

        case NPPN_FILERENAMECANCEL:
        case NPPN_FILEDELETEFAILED:
            StoredPath.Delete();
        break;

        case NPPN_FILERENAMED:
            if (GTags::Config._autoUpdate)
            {
                TCHAR file[MAX_PATH];
                INpp::Get().GetFilePathFromBufID(
                        notifyCode->nmhdr.idFrom, file);
                GTags::UpdateSingleFile(file);

                if (StoredPath())
                {
                    GTags::UpdateSingleFile(StoredPath()->C_str());
                    StoredPath.Delete();
                }
            }
        break;

        case NPPN_FILEDELETED:
            if (GTags::Config._autoUpdate)
            {
                if (StoredPath())
                {
                    GTags::UpdateSingleFile(StoredPath()->C_str());
                    StoredPath.Delete();
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
            GTags::ResultWin::Get().ApplyStyle();
        }
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


#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
