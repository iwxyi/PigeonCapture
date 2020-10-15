#ifndef WINDOWSHWND_H
#define WINDOWSHWND_H

#include <windows.h>

enum window_search_mode {
    INCLUDE_MINIMIZED,
    EXCLUDE_MINIMIZED
};

static bool check_window_valid(HWND window, enum window_search_mode mode)
{
    DWORD styles, ex_styles;
    RECT  rect;

    if (/*!IsWindowVisible(window) ||*/
        (mode == EXCLUDE_MINIMIZED && IsIconic(window)))
        return false;

    GetClientRect(window, &rect);
    styles    = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
    ex_styles = (DWORD)GetWindowLongPtr(window, GWL_EXSTYLE);

    if (ex_styles & WS_EX_TOOLWINDOW)
        return false;
    if (styles & WS_CHILD)
        return false;
    if (mode == EXCLUDE_MINIMIZED && (rect.bottom == 0 || rect.right == 0))
        return false;

    return true;
}

static inline HWND next_window(HWND window, enum window_search_mode mode)
{
    while (true) {
        window = GetNextWindow(window, GW_HWNDNEXT);
        if (!window || check_window_valid(window, mode))
            return window;
    }

    return nullptr;
}

static inline HWND first_window(enum window_search_mode mode)
{
    HWND window = GetWindow(GetDesktopWindow(), GW_CHILD);
    if (!check_window_valid(window, mode))
        window = next_window(window, mode);
    return window;
}

#endif // WINDOWSHWND_H
