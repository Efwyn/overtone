// ========================================
// File: renderer/renderer.h
// Description: The Core Renderer Header 
// Author: Morgan Carpenetti
// Created On: 22-04-2026
// ========================================
#pragma once

#include "types.h"
#include "window.h"

Result renderer_initialize(Window* window);
void   renderer_shutdown();
Result renderer_draw_frame();
void   renderer_wait_idle();
void   renderer_signal_framebuffer_resized(u32 width, u32 height);
