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
    //
    // Vulkan Instance
    //
    if(enableValidationLayers) {
        printf("Validation Layers are ON!\n");
    } else {
        printf("Validation Layers are OFF!\n");
    }


    printf("Size of VulkanState Object: %zu bytes\n", sizeof(VulkanState));

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
    u32 physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(v_state.instance, &physicalDeviceCount, nullptr);
    printf("%d PhysicalDevice(s) found!\n", physicalDeviceCount);

    VkPhysicalDevice* physicalDeviceList = calloc(physicalDeviceCount, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(v_state.instance, &physicalDeviceCount, physicalDeviceList);

    VkPhysicalDeviceProperties props;
    for(u32 i = 0; i < physicalDeviceCount; i++) {
        vkGetPhysicalDeviceProperties(physicalDeviceList[i], &props);
        printf("Device [%d]: %s\n", i, props.deviceName);
    }

    // TODO: ENSURE PHYSICAL DEVICE SUPPORTS REQUIRED FEATURES
    v_state.physicalDevice = physicalDeviceList[0];

    //
    // Create A Surface from the window
    //
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



    //clean up our dynamic memory
    free(queueFamilyProps);
    queueFamilyProps = nullptr;
    free(instanceExtensions);
    instanceExtensions = nullptr;
    free(physicalDeviceList);
    physicalDeviceList = nullptr;

    return ResultOk;
}

void renderer_shutdown() {
    if(enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(v_state.instance, v_state.debugMessenger, nullptr);
    }

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

