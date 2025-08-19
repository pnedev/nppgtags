#pragma once

#include <windows.h>
#include <tchar.h>
#include <cstdint>
#include <vector>
#include <memory>
#include "Common.h"
#include "CmdDefines.h"
#include "Cmd.h"


namespace GTags
{


class CallTipParser : public ResultParser
{
public:

    intptr_t overload = 0;
    CallTipParser(intptr_t pOverload) : overload(pOverload) {}
    virtual ~CallTipParser() {}

    virtual intptr_t Parse(const CmdPtr_t&);
    virtual const std::vector<TCHAR*>& GetListPaths() const { return _paths; }
    

protected:
    std::vector<TCHAR*> _paths;

private:
    CText _buf;
};

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
    ~CallTipWin();
private:
    static const TCHAR  cClassName[];
    static const int    cBackgroundColor;
    static const int    cWidth;

    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    CallTipWin& operator=(const CallTipWin&) = delete;

    HWND composeWindow(const TCHAR* header);
    int filterLV();
    void resizeLV();

    void onDblClick();

    static std::unique_ptr<CallTipWin> CTW;

    HWND            _hWnd;
    HWND            _hLVWnd;
    HFONT           _hFont;
    const CmdId_t   _cmdId;
    const bool      _ic;
    const int       _cmdTagLen;
    std::shared_ptr<CallTipParser>      _parser;
};

} // namespace GTags
