// ===========================================
// File: include/window.h
// Description: A Window for Rendering the Game
// Author: Morgan Carpenetti
// Created On: 19-04-2026
// ============================================
#pragma once

#include "types.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MAX_TITLE_LENGTH 64

typedef struct Window {
    HWND hwnd;
    char title[MAX_TITLE_LENGTH];
    u32 width, height;
    bool shouldClose;
} Window;

Result window_create(Window* window, const u32 width, const u32 height, char* const title);
void   window_cleanup(Window* window);
bool   window_poll_events();
void   window_show(const Window* window);
