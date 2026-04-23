// ========================================
// File: renderer/renderer.c
// Description: The Core Renderer Source File 
// Author: Morgan Carpenetti
// Created On: 22-04-2026
// ========================================
#include "renderer/renderer.h"

#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>
#include "window.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

inline u32 min_u32(u32 v1, u32 v2) {
    return v1 < v2 ? v1 : v2;
}
inline u32 max_u32(u32 v1, u32 v2) {
    return v1 > v2 ? v1 : v2;
}
inline u32 clamp_u32 (u32 val, u32 min, u32 max) {
    return min_u32(max_u32(val, min), max);
}

//fwd declarations for debug utils
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                       VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks* pAllocator);
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT        type,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void*);

// Vulkan Validation Layers, useful for debugging
#define VALIDATION_LAYER_COUNT 1
char const* validationLayers[VALIDATION_LAYER_COUNT] = {
    "VK_LAYER_KHRONOS_validation"
};

// Vuilkan Extensions this engine will be using no matter what
#define REQUIRED_INSTANCE_EXTENSION_COUNT 2
char const* requiredInstanceExtensions[REQUIRED_INSTANCE_EXTENSION_COUNT] = {
    "VK_KHR_surface",
    "VK_KHR_win32_surface"
};

// Vulkan Physical Device Extensions
#define REQUIRED_DEVICE_EXTENSION_COUNT 1
char const* requiredDeviceExtensions[REQUIRED_DEVICE_EXTENSION_COUNT] = {
    "VK_KHR_swapchain"
};

typedef struct VulkanState {
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice         physicalDevice;
    VkSurfaceKHR             surface;
    VkDevice                 device;
    u32                      graphicsQueueIndex;
    VkQueue                  graphicsQueue;
    //Swapchain
    VkSwapchainKHR           swapChain;
    VkSurfaceFormatKHR       swapChainFormat;
    VkExtent2D               swapChainExtent;
    u32                      swapChainLength;
    VkImage*                 swapChainImages;
    VkImageView*             swapChainImageViews;
    //Pipeline
    VkPipelineLayout         pipelineLayout;
    VkPipeline               graphicsPipeline;
    VkCommandPool            commandPool;
    VkCommandBuffer          commandBuffer;
    //Synchronization
    VkSemaphore              presentCompleteSemaphore;
    VkSemaphore              renderFinishedSemaphore;
    VkFence                  drawFence;
} VulkanState;
VulkanState v_state = {};

const VkApplicationInfo appInfo = {
    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName   = "Hello Overtone!",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName        = "Overtone",
    .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion         = VK_API_VERSION_1_4
};

Result renderer_initialize(const Window* window) {
    if(enableValidationLayers) {
        printf("[Renderer]: Validation Layers are ON!\n");
    } else {
        printf("[Renderer]: Validation Layers are OFF!\n");
    }

    //
    // Vulkan Instance
    //
    printf("[Renderer]: Creating Instance\n");


    // Instance Layers. For now we're either using all or none,
    // but later how this is handled may change
    u32 instanceLayerCount                  = 0;
    const char** instanceLayers             = nullptr;
    if(enableValidationLayers) {
        instanceLayers = validationLayers;
        instanceLayerCount = VALIDATION_LAYER_COUNT;
    }

    // Instance Extensions
    u32 instanceExtensionCount      = REQUIRED_INSTANCE_EXTENSION_COUNT;
    const char** instanceExtensions = nullptr;

    if(enableValidationLayers) instanceExtensionCount += 1;
    instanceExtensions = calloc(instanceExtensionCount, sizeof(char*));
    //copy the required exensions in regardless
    for(u32 i = 0; i < REQUIRED_INSTANCE_EXTENSION_COUNT; i++) {
        instanceExtensions[i] = requiredInstanceExtensions[i];
    }
    //if we're using validation layers append "VK_EXT_debug_utils"
    if(enableValidationLayers) {
        instanceExtensions[instanceExtensionCount - 1] = "VK_EXT_debug_utils";
    }

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = instanceLayerCount,
        .ppEnabledLayerNames     = instanceLayers,
        .enabledExtensionCount   = instanceExtensionCount,
        .ppEnabledExtensionNames = instanceExtensions
    };

    if(vkCreateInstance(&instanceCreateInfo, nullptr, &v_state.instance) != VK_SUCCESS) {
        printf("ERROR: Failed to Create Vulkan Instance!\n");
        return ResultFailure;
    }


    //
    // Setup the Debug Callback (if applicable)
    //

    if(enableValidationLayers) {
    printf("[Renderer]: Setting up Debug Callback\n");
        VkDebugUtilsMessageSeverityFlagsEXT severityFlags = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        VkDebugUtilsMessageTypeFlagsEXT messageTypeFlags  = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

        VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = severityFlags,
                .messageType = messageTypeFlags,
                .pfnUserCallback = debugCallback
        };

        if(CreateDebugUtilsMessengerEXT(v_state.instance, &debugUtilsMessengerCreateInfo, nullptr, &v_state.debugMessenger) != VK_SUCCESS) {
            printf("ERROR: Failed to Create Debug Messenger!\n");
            return ResultFailure;
        }
    }

    //
    // Get a PhysicalDevice
    //
    printf("[Renderer]: Selecting Physical Device\n");

    u32 physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(v_state.instance, &physicalDeviceCount, nullptr);
    printf("[Renderer]: %d PhysicalDevice(s) found!\n", physicalDeviceCount);

    VkPhysicalDevice* physicalDeviceList = calloc(physicalDeviceCount, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(v_state.instance, &physicalDeviceCount, physicalDeviceList);

    VkPhysicalDeviceProperties props;
    for(u32 i = 0; i < physicalDeviceCount; i++) {
        vkGetPhysicalDeviceProperties(physicalDeviceList[i], &props);
        printf("[Renderer]: \tDevice [%d]: %s\n", i, props.deviceName);
    }

    // TODO: ENSURE PHYSICAL DEVICE SUPPORTS REQUIRED FEATURES
    v_state.physicalDevice = physicalDeviceList[0];

    //
    // Create A Surface from the window
    //
    printf("[Renderer]: Creating Surface\n");

#ifdef WIN32
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = window->hwnd
    };
    if(vkCreateWin32SurfaceKHR(v_state.instance, &surfaceCreateInfo, nullptr, &v_state.surface) != VK_SUCCESS) {
        printf("ERROR: Failed to Create Surface!\n");
        return ResultFailure;
    }
#else
    #error PLATFORM NOT SUPPORTED, IMPLEMENT SURFACE CREATION FOR THIS PLATFORM
#endif

    //
    // Determine Queue Info
    //
    printf("[Renderer]: Picking Queue Families\n");

    //Get a List of Properties for each queue family available on the physical device
    u32 queueFamilyCount = 0;
    VkQueueFamilyProperties* queueFamilyProps;
    vkGetPhysicalDeviceQueueFamilyProperties(v_state.physicalDevice, &queueFamilyCount, nullptr);
    queueFamilyProps = calloc(queueFamilyCount, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(v_state.physicalDevice, &queueFamilyCount, queueFamilyProps);

    //go through each queue family and find on the supports graphics operations and the surface we created
    u32 graphicsQueueIndex = ~0;
    for(u32 qfpIndex = 0; qfpIndex < queueFamilyCount; ++qfpIndex) {
        bool graphicsBit = false;
        VkBool32 surfaceSupported = VK_FALSE;
        graphicsBit = (queueFamilyProps[qfpIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT);
        if(vkGetPhysicalDeviceSurfaceSupportKHR(v_state.physicalDevice, qfpIndex, v_state.surface, &surfaceSupported) != VK_SUCCESS) {
            printf("ERROR: Unable to Query Physical Device for Surface Support!\n");
            return ResultFailure;
        }

        if(graphicsBit && surfaceSupported) {
            graphicsQueueIndex = qfpIndex;
            break;
        }
    }
    if(graphicsQueueIndex == ~0u) {
        printf("ERROR: Could not find a suitable Queue Family!\n");
        return ResultFailure;
    }

    float queuePriority = 0.5f;
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };
    v_state.graphicsQueueIndex = graphicsQueueIndex;

    //
    // Create Logical Device
    //
    printf("[Renderer]: Creating Logical Device\n");

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .pNext = nullptr,
        .extendedDynamicState = true,
    };
    VkPhysicalDeviceVulkan13Features vulkan13Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &dynamicStateFeatures,
        .synchronization2 = true,
        .dynamicRendering = true,
    };
    VkPhysicalDeviceVulkan11Features vulkan11Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &vulkan13Features,
        .shaderDrawParameters = true,
    };
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vulkan11Features,
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &physicalDeviceFeatures2, //going to extend with more stuff to for example enable dynamic rendering
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = REQUIRED_DEVICE_EXTENSION_COUNT,
        .ppEnabledExtensionNames = requiredDeviceExtensions,
    };

    if(vkCreateDevice(v_state.physicalDevice, &deviceCreateInfo, nullptr, &v_state.device) != VK_SUCCESS) {
        printf("ERROR: Failed to Create Logical Device!\n");
        return ResultFailure;
    }

    vkGetDeviceQueue(v_state.device, v_state.graphicsQueueIndex, 0, &v_state.graphicsQueue);

    //
    // SwapChain Creation
    //
    printf("[Renderer]: Creating Swap Chain\n");

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    u32 availableFormatsCount = 0;
    VkSurfaceFormatKHR* availableSurfaceFormats;
    u32 availablePresentModesCount = 0;
    VkPresentModeKHR* availablePresentModes;
    if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(v_state.physicalDevice, v_state.surface, &surfaceCapabilities) != VK_SUCCESS) {
        printf("ERROR: Failed to Get Surface Capabilites!\n");
        return ResultFailure;
    }

    vkGetPhysicalDeviceSurfaceFormatsKHR(v_state.physicalDevice, v_state.surface, &availableFormatsCount, nullptr);
    availableSurfaceFormats = calloc(availableFormatsCount, sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(v_state.physicalDevice, v_state.surface, &availablePresentModesCount, nullptr);
    availablePresentModes = calloc(availablePresentModesCount, sizeof(VkPresentModeKHR));

    if(availableFormatsCount == 0 || availablePresentModesCount == 0) {
        printf("ERROR: Failed to find any Surface Formats or Present Modes!\n");
        return ResultFailure;
    }

    //populate the arrays
    vkGetPhysicalDeviceSurfaceFormatsKHR(v_state.physicalDevice, v_state.surface, &availableFormatsCount, availableSurfaceFormats);
    vkGetPhysicalDeviceSurfacePresentModesKHR(v_state.physicalDevice, v_state.surface, &availablePresentModesCount, availablePresentModes);


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
    const VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
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
    VkExtent2D swapChainExtent;
    if(surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        swapChainExtent = surfaceCapabilities.currentExtent;
    }
    else {
        swapChainExtent.width = clamp_u32(window->width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        swapChainExtent.height = clamp_u32(window->height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
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
        .surface = v_state.surface,
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

    if(vkCreateSwapchainKHR(v_state.device, &swapChainCreateInfo, nullptr, &v_state.swapChain) != VK_SUCCESS) {
        printf("ERROR: Failed to Create Swapchain!\n");
        return ResultFailure;
    }

    if(vkGetSwapchainImagesKHR(v_state.device, v_state.swapChain, &v_state.swapChainLength, nullptr) != VK_SUCCESS) {
        printf("ERROR: Failed to Get Swapchain Images!\n");
        return ResultFailure;
    }

    //lifetime is until the end of program or we remake the swapchain with a new length
    v_state.swapChainImages = calloc(v_state.swapChainLength, sizeof(VkImage));
    if(!v_state.swapChainImages) {
        printf("ERROR: Failed to allocate memory for swapchain images!\n");
        return ResultFailure;
    }

    if(vkGetSwapchainImagesKHR(v_state.device, v_state.swapChain, &v_state.swapChainLength, v_state.swapChainImages) != VK_SUCCESS) {
        printf("ERROR: Failed to Get Swapchain Images!\n");
        return ResultFailure;
    }

    v_state.swapChainFormat = chosenSurfaceFormat;
    v_state.swapChainExtent = swapChainExtent;

    //Swapchain Image Views
    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = v_state.swapChainFormat.format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        },
    };

    //lifetime is until the end of program or we remake the swapchain with a new length
    v_state.swapChainImageViews = calloc(v_state.swapChainLength, sizeof(VkImageView));
    if(!v_state.swapChainImageViews) {
        printf("ERROR: Failed to allocate memory for Swapchain VkImageViews!\n");
        return ResultFailure;
    }

    for(u32 i = 0; i < v_state.swapChainLength; i++) {
        imageViewCreateInfo.image = v_state.swapChainImages[i];
        if(vkCreateImageView(v_state.device, &imageViewCreateInfo, nullptr, &v_state.swapChainImageViews[i]) != VK_SUCCESS) {
            printf("ERROR: Failed to Create Swapchain VkImageView!\n");
            return ResultFailure;
        }
    }

    //clean up our dynamic memory
    free(availableSurfaceFormats);
    availableSurfaceFormats = nullptr;
    free(availablePresentModes);
    availablePresentModes = nullptr;
    free(queueFamilyProps);
    queueFamilyProps = nullptr;
    free(instanceExtensions);
    instanceExtensions = nullptr;
    free(physicalDeviceList);
    physicalDeviceList = nullptr;

    return ResultOk;
}

void renderer_shutdown() {
    printf("[Renderer]: Shutting Down\n");
    if(enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(v_state.instance, v_state.debugMessenger, nullptr);
    }

    for(u32 i = 0; i < v_state.swapChainLength; i++) {
        vkDestroyImageView(v_state.device, v_state.swapChainImageViews[i], nullptr);
    }
    free(v_state.swapChainImageViews);
    free(v_state.swapChainImages);

    vkDestroySwapchainKHR(v_state.device, v_state.swapChain, nullptr); 
    vkDestroyDevice(v_state.device, nullptr);
    vkDestroySurfaceKHR(v_state.instance, v_state.surface, nullptr);
    vkDestroyInstance(v_state.instance, nullptr);
}

void renderer_draw_frame() {
    //Draw one frame
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                       VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks* pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    func(instance, debugMessenger, pAllocator);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT        type,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void*) {
    if(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        printf("Validation Layer: severity: %s, type %x, msg: %s\n", "error", type, pCallbackData->pMessage);
    }

    if(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        printf("Validation Layer: severity: %s, type %x, msg: %s\n", "warning", type, pCallbackData->pMessage);
    }

    return VK_FALSE;
}

