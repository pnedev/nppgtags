

#pragma comment (lib, "comctl32")


#include <windows.h>
#include <commctrl.h>
#include "Common.h"
#include "INpp.h"
#include "GTags.h"
#include "CallTip.h"
#include "Cmd.h"
#include "LineParser.h"


namespace GTags
{

const TCHAR CallTipWin::cClassName[]   = _T("CallTipWin");
const int CallTipWin::cBackgroundColor = COLOR_INFOBK;
const int CallTipWin::cWidth           = 400;


std::unique_ptr<CallTipWin> CallTipWin::CTW {nullptr};


/**
 *  \brief
 */
void CallTipWin::Register()
{
    WNDCLASS wc         = {0};
    wc.style            = CS_PARENTDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = wndProc;
    wc.hInstance        = HMod;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetSysColorBrush(cBackgroundColor);
    wc.lpszClassName    = cClassName;

    RegisterClass(&wc);

    INITCOMMONCONTROLSEX icex   = {0};
    icex.dwSize                 = sizeof(icex);
    icex.dwICC                  = ICC_LISTVIEW_CLASSES;

    InitCommonControlsEx(&icex);
}


/**
 *  \brief
 */
void CallTipWin::Unregister()
{
    UnregisterClass(cClassName, HMod);
}


/**
 *  \brief
 */
LRESULT APIENTRY CallTipWin::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        return 0;

        case WM_SETFOCUS:
            SetFocus(CTW->_hLVWnd);
        return 0;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_KILLFOCUS:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                return 0;
                
                // case NM_DBLCLK:
                    // CTW->onDblClick();
                // return  0;
            }
        break;

        case WM_DESTROY:
            CTW = nullptr;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // namespace GTags