// ===========================================
// File: include/window.h
// Description: A Window for Rendering the Game
// Author: Morgan Carpenetti
// Created On: 19-04-2026
// ============================================
#pragma once

#include "types.h"

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#define MAX_TITLE_LENGTH 64


typedef struct Window {
    HWND hwnd;
    char title[MAX_TITLE_LENGTH];
    u32 width, height;
    bool shouldClose;
} Window;

Result     window_create(const u32 width, const u32 height, char* const title, Window** windowHandle);
void       window_cleanup();
void       window_poll_events();
void       window_show();
void       window_get_framebuffer_size(u32* width, u32* height);
void       window_set_callback_resize(void (*resizeCallback)(u32, u32));
VkResult   window_create_vulkan_surface(const VkInstance instance, VkSurfaceKHR* surface);
