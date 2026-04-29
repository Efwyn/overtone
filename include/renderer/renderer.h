// ========================================
// File: renderer/renderer.h
// Description: The Core Renderer Header 
// Author: Morgan Carpenetti
// Created On: 22-04-2026
// ========================================
#pragma once

#include "types.h"

Result renderer_initialize();
void   renderer_shutdown();
Result renderer_draw_frame();

// Wait for queued frames to finish processing
void   renderer_wait_idle();

// Notify the renderer that the window has resized so
//  it can recreate the swapchain
void   renderer_signal_framebuffer_resized(u32 width, u32 height);
