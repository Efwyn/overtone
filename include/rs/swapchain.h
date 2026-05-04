// ====================================
// File: swapchain.h
// Author: Morgan Carpenetti
// Description: 
// Created On:  5/4/2026
// ====================================
#pragma once

#include "ec/types.h"
#include <vulkan/vulkan.h>

typedef struct Swapchain {
    VkSwapchainKHR           swapChain;
    VkSurfaceFormatKHR       format;
    VkExtent2D               extents;
    VkImage*                 images;
    VkImageView*             imageViews;
    u32                      length;
} Swapchain;

Result Swapchain_create(Swapchain* swapchain, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface);
void   Swapchain_cleanup(Swapchain* swapchain, VkDevice device);
Result Swapchain_recreate(Swapchain* swapchain, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface);
