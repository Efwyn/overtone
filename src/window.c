// ============================================
// File: src/window.c
// Description: A Window For Rendering the Game
// Author: Morgan Carpenetti
// Created On: 19-04-2026
// ============================================
#include "window.h"
#include "types.h"

#include <stdio.h>
#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define WINDOW_CLASSNAME "OvertoneWindowClass"

typedef struct Window {
    HWND hwnd;
    char title[MAX_TITLE_LENGTH];
    u32 width, height;
    bool shouldClose;
} Window;

typedef struct WindowCallbacks {
    void (*resizeCallback)(u32, u32);
} WindowCallbacks;
WindowCallbacks callbacks;

Window gameWindow;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            gameWindow.width  = LOWORD(lParam);
            gameWindow.height = HIWORD(lParam);
            if(callbacks.resizeCallback != nullptr) {
                callbacks.resizeCallback(gameWindow.width, gameWindow.height);
            }
            return 0;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}


Result window_create(const u32 width, const u32 height, char* const title) {
    assert(title != nullptr);

    WNDCLASSEX wc = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = WindowProc,
        .hInstance = GetModuleHandle(nullptr),
        .lpszClassName = WINDOW_CLASSNAME
    };

    RegisterClassEx(&wc);


    DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    RECT wr = {0, 0, width, height};
    AdjustWindowRect(&wr, windowStyle, false);

    gameWindow.hwnd = CreateWindowEx(
            0,
            WINDOW_CLASSNAME,
            title,
            windowStyle,
            300, 300,
            wr.right - wr.left,
            wr.bottom - wr.top,
            nullptr,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);

    if(!gameWindow.hwnd) {
        printf("ERROR: Failed to create Win32 Window!\n");
        return ResultFailure;
    }

    gameWindow.width = width;
    gameWindow.height = height;
    gameWindow.shouldClose = false;
    if(strcpy_s(gameWindow.title, MAX_TITLE_LENGTH, title) != 0) {
        printf("Error setting window title\n");
        return ResultFailure;
    }

    ShowWindow(gameWindow.hwnd, SW_SHOW);

    return ResultOk;
}

void window_cleanup() {
    DestroyWindow(gameWindow.hwnd);
}

void window_poll_events() {
    MSG msg;

    while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if(msg.message == WM_QUIT) {
            gameWindow.shouldClose = true;
        }
        else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

bool window_should_close() {
    return gameWindow.shouldClose;
}

void window_get_framebuffer_size(u32 *width, u32 *height) {
    *width = gameWindow.width;
    *height = gameWindow.height;
}

void window_set_callback_resize(void (*resizeCallback)(u32, u32)) {
    callbacks.resizeCallback = resizeCallback;
}

VkResult window_create_vulkan_surface(const VkInstance instance, VkSurfaceKHR* surface) {
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = gameWindow.hwnd
    };
    return vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, surface);
#else
    #error PLATFORM NOT SUPPORTED, IMPLEMENT SURFACE CREATION FOR THIS PLATFORM
#endif
}
