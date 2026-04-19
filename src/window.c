// ============================================
// File: src/window.c
// Description: A Window For Rendering the Game
// Author: Morgan Carpenetti
// Created On: 19-04-2026
// ============================================
#include "types.h"
#include "window.h"

#include <stdio.h>
#include <assert.h>

#define WINDOW_CLASSNAME "OvertoneWindowClass"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}


Result window_create(Window* window, const u32 width, const u32 height, char* const title) {
    assert(title != nullptr);

    Window w = {
        .hwnd = NULL,
        .width = width,
        .height = height,
        .shouldClose = false
    };

    WNDCLASSEX wc = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = WindowProc,
        .hInstance = GetModuleHandle(nullptr),
        .lpszClassName = WINDOW_CLASSNAME
    };

    RegisterClassEx(&wc);

    RECT wr = {0, 0, width, height};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);

    HWND hwnd = CreateWindowEx(
            0,
            WINDOW_CLASSNAME,
            title,
            WS_OVERLAPPEDWINDOW,
            300, 300,
            wr.right - wr.left,
            wr.bottom - wr.top,
            nullptr,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);

    if(!hwnd) return ResultFailure;

    window->hwnd = hwnd;
    window->width = width;
    window->height = height;
    window->shouldClose = false;
    if(strcpy_s(window->title, MAX_TITLE_LENGTH, title) != 0) {
        printf("Error setting window title\n");
    }

    return ResultOk;
}

void window_cleanup(Window* window) {
    DestroyWindow(window->hwnd);
    memset(window, 0, sizeof(Window));
}



bool window_poll_events() {
    bool shouldClose = false;
    MSG msg;

    while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if(msg.message == WM_QUIT) {
            printf("Closing Application\n");
            shouldClose = true;
        }
    }

    return shouldClose;
}

void window_show(const Window* window) {
    assert(window->hwnd != NULL);
    ShowWindow(window->hwnd, SW_SHOW);
}
