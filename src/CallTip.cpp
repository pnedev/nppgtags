



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
            SetFocus(ACW->_hLVWnd);
        return 0;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_KILLFOCUS:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                return 0;

                case LVN_KEYDOWN:
                    if (ACW->onKeyDown(((LPNMLVKEYDOWN)lParam)->wVKey))
                        return 1;
                break;

                case NM_DBLCLK:
                    ACW->onDblClick();
                return 0;
            }
        break;

        case WM_DESTROY:
            ACW = nullptr;
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}