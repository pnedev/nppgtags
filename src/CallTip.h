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
    // DevNote I don't think overload and fun_start_pos should be apart of the parser,
    // and they could be static variables in CallTipWin but that seems less fitting.
    intptr_t overload = 0;
    intptr_t func_start_pos = 0;
    CallTipParser(intptr_t pOverload, intptr_t pFuncStart) :
        overload(pOverload), func_start_pos(pFuncStart) {}
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
    
    static void DestroyCurrentWin()
    {
        if (CTW != nullptr)
        {
            CTW->_queued_for_deletion = true;
            // SetParent(CTW->_hWnd, NULL); // Set parent wnd as desktop, otherwise npp crashes, idk why.
            SendMessage(CTW->_hWnd, WM_CLOSE, 0, 0);
            // CTW = nullptr;
        }
    }

    CallTipWin(const CmdPtr_t& cmd);
    ~CallTipWin();
private:
    static const TCHAR  cClassName[];
    static const int    cBackgroundColor;
    static const int    cWidth;

    static LRESULT APIENTRY wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static int getDefParamCount(TCHAR* word);

    CallTipWin& operator=(const CallTipWin&) = delete;

    HWND composeWindow(const TCHAR* header);
    int filterLV();
    void resizeLV();
    
    int getItemByName(TCHAR* word);

    void onClick(int item);
    void onDblClick(int item);
    void updateHeader(int overload, int high_overload = -1, TCHAR* header = _T("CallTip"));
    void updateWindow();
    static std::unique_ptr<CallTipWin> CTW;
    int             _selItem;
    bool            _queued_for_deletion;
    HWND            _hWnd;
    HWND            _hLVWnd;
    HFONT           _hFont;
    const CmdId_t   _cmdId;
    const CText     _tag;
    std::shared_ptr<CallTipParser>      _parser;
};

} // namespace GTags
