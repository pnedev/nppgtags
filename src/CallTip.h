#pragma once

#include <windows.h>
#include <tchar.h>
#include <cstdint>
#include <vector>
#include "Common.h"
#include "NppAPI/Notepad_plus_msgs.h"
#include "NppAPI/Docking.h"
#include "NppAPI/PluginInterface.h"

#include "CmdDefines.h"

class CallTipWin()
{
public:
    static void Register();
    static void Unregister();

    static void Show(const CmdPtr_t& cmd);

    static bool IsShown()
    {
        return (CTW != nullptr);
    }

private:
    static const TCHAR  cClassName[];
    static const int    cBackgroundColor;
    static const int    cWidth;

    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static std::unique_ptr<CallTipWin> CTW;
}