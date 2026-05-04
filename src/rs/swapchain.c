// ====================================
// File: swapchain.c
// Author: Morgan Carpenetti
// Description: 
// Created On:  5/4/2026
// ====================================
#include "rs/swapchain.h"
#include "pf/window.h"

#include <stdlib.h>
#include <stdio.h>


const VkFormat              preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
const VkColorSpaceKHR   preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
const VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
//const VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

Result Swapchain_create(Swapchain* swapchain, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    u32 availableFormatsCount = 0;
    VkSurfaceFormatKHR* availableSurfaceFormats;
    u32 availablePresentModesCount = 0;
    VkPresentModeKHR* availablePresentModes;

    if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS) {
        printf("ERROR: Failed to Get Surface Capabilites!\n");
        return ResultFailure;
    }

    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &availableFormatsCount, nullptr);
    availableSurfaceFormats = calloc(availableFormatsCount, sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &availablePresentModesCount, nullptr);
    availablePresentModes = calloc(availablePresentModesCount, sizeof(VkPresentModeKHR));

    if(availableFormatsCount == 0 || availablePresentModesCount == 0) {
        printf("ERROR: Failed to find any Surface Formats or Present Modes!\n");
        return ResultFailure;
    }

    //populate the arrays
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &availableFormatsCount, availableSurfaceFormats);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &availablePresentModesCount, availablePresentModes);


    //Pick A Surface Format
    // We'll go with the first that supports B8R8G8A8_SRGB and SRGB nonlinear 
    u32 formatIndex = 0;
    VkSurfaceFormatKHR chosenSurfaceFormat = availableSurfaceFormats[0];
    while(formatIndex < availableFormatsCount) {
        if(availableSurfaceFormats[formatIndex].format == VK_FORMAT_B8G8R8A8_SRGB &&
           availableSurfaceFormats[formatIndex].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenSurfaceFormat = availableSurfaceFormats[formatIndex];
            break;
        }
        formatIndex++;
    }

    if(formatIndex == availableFormatsCount)
        printf("Failed to find preferred format, using first available instead\n");


    //Pick a Present Mode, preferred is Mailbox, but we'll use fifo if we can't find it
    //VK_PRESENT_MODE_IMMEDIATE_KHR is Vsync off
    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(u32 i = 0; i < availablePresentModesCount; ++i) {
        if(availablePresentModes[i] == preferredPresentMode) {
            chosenPresentMode = preferredPresentMode;
            break;
        }
    }

    // Set the extents of the swapchain
    // if capabilites.currentExtent already set, use that,
    // otherwise clamp the window size to the capabilites max/min
    u32 fbWidth, fbHeight;
    window_get_framebuffer_size(&fbWidth, &fbHeight);
    VkExtent2D swapChainExtent;
    if(surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        swapChainExtent = surfaceCapabilities.currentExtent;
    }
    else {
        swapChainExtent.width = clamp_u32(fbWidth, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        swapChainExtent.height = clamp_u32(fbHeight, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }

    // Set the number of images we will have in the swapchain
    // at least 1 more than the minimum, but don't exceed
    // the max provided in surfaceCapabilites
    // 0 in capabilities means no max given
    const u32 defaultSwapchainImageCount = 3;
    u32 minImageCount = max_u32(defaultSwapchainImageCount, surfaceCapabilities.minImageCount + 1);
    if(0 < surfaceCapabilities.maxImageCount && surfaceCapabilities.maxImageCount < minImageCount) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }


    VkSwapchainCreateInfoKHR swapChainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = minImageCount,
        .imageFormat = chosenSurfaceFormat.format,
        .imageColorSpace = chosenSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = chosenPresentMode,
        .clipped = true,
        .oldSwapchain = nullptr
    };

    if(vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapchain->swapChain) != VK_SUCCESS) {
        printf("ERROR: Failed to Create Swapchain!\n");
        return ResultFailure;
    }

    if(vkGetSwapchainImagesKHR(device, swapchain->swapChain, &swapchain->length, nullptr) != VK_SUCCESS) {
        printf("ERROR: Failed to Get Swapchain Images!\n");
        return ResultFailure;
    }

    //lifetime is until the end of program or we remake the swapchain with a new length
    swapchain->images = calloc(swapchain->length, sizeof(VkImage));
    if(!swapchain->images) {
        printf("ERROR: Failed to allocate memory for swapchain images!\n");
        return ResultFailure;
    }

    if(vkGetSwapchainImagesKHR(device, swapchain->swapChain, &swapchain->length, swapchain->images) != VK_SUCCESS) {
        printf("ERROR: Failed to Get Swapchain Images!\n");
        return ResultFailure;
    }

    swapchain->format = chosenSurfaceFormat;
    swapchain->extents = swapChainExtent;

    //Swapchain Image Views
    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchain->format.format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        },
    };

    //lifetime is until the end of program or we remake the swapchain with a new length
    swapchain->imageViews = calloc(swapchain->length, sizeof(VkImageView));
    if(!swapchain->imageViews) {
        printf("ERROR: Failed to allocate memory for Swapchain VkImageViews!\n");
        return ResultFailure;
    }

    for(u32 i = 0; i < swapchain->length; i++) {
        imageViewCreateInfo.image = swapchain->images[i];
        if(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchain->imageViews[i]) != VK_SUCCESS) {
            printf("ERROR: Failed to Create Swapchain VkImageView!\n");
            return ResultFailure;
        }
    }

    free(availableSurfaceFormats);
    free(availablePresentModes);
    return ResultOk;
}

void Swapchain_cleanup(Swapchain* swapchain, VkDevice device) {
    for(u32 i = 0; i < swapchain->length; i++) {
        vkDestroyImageView(device, swapchain->imageViews[i], nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain->swapChain, nullptr); 

    free(swapchain->imageViews);
    free(swapchain->images);
}

Result Swapchain_recreate(Swapchain* swapchain, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface) {
    vkDeviceWaitIdle(device);
    Swapchain_cleanup(swapchain, device);

    return Swapchain_create(swapchain, physicalDevice, device, surface);
}

