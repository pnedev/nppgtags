#pragma once

#include <windows.h>
#include <tchar.h>
#include <memory>
#include "Common.h"
#include "CmdDefines.h"


namespace GTags
{

class CallTipWin
{
public:
    static void Register();
    static void Unregister();

    static void Show(const CmdPtr_t& cmd);

    static bool IsShown()
    {
        return (CTW != nullptr);
    }

    CallTipWin(const CmdPtr_t& cmd);
    CallTipWin(const CallTipWin&);
    ~CallTipWin();

private:
    static const TCHAR  cClassName[];
    static const int    cBackgroundColor;
    static const int    cWidth;

    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    CallTipWin& operator=(const CallTipWin&) = delete;

    HWND composeWindow(const TCHAR* header);
    int filterLV(const CText& filter);
    void resizeLV();

    void onDblClick();

    static std::unique_ptr<CallTipWin> CTW;

    HWND            _hWnd;
    HWND            _hLVWnd;
    HFONT           _hFont;
    const CmdId_t   _cmdId;
    const bool      _ic;
    const int       _cmdTagLen;
    ParserPtr_t     _completion;
};

} // namespace GTags
