// ========================================
// File: main.c
// Description: Overtone: A Game Engine Written in C
// Author: Morgan Carpenetti
// Created On: 19-04-2026
// ========================================
#include "types.h"
#include "window.h"

#include <stdio.h>
#include <vulkan/vulkan.h>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

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

const VkApplicationInfo appInfo = {
    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName   = "Hello Overtone!",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName        = "Overtone",
    .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion         = VK_API_VERSION_1_4
};

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

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT        type,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void*) {
    if(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        printf("Validation Layer: type %s msg: %s\n", "error", pCallbackData->pMessage);
    }

    if(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        printf("Validation Layer: type %s msg: %s\n", "warning", pCallbackData->pMessage);
    }

    return VK_FALSE;
}



int main(int argc, char** argv) {
    Window window;
    if(window_create(&window, 640, 480, "Hello Overtone!") == ResultFailure) {
        printf("ERROR! Failed to Create Window\n");
        return EXIT_FAILURE;
    }

    window_show(&window);

    //
    // Vulkan Instance
    //
    VkInstance instance             = nullptr;
    VkDebugUtilsMessengerEXT debugMessenger = nullptr;
    VkPhysicalDevice physicalDevice = nullptr;


    if(enableValidationLayers) {
        printf("Validation Layers are ON!\n");
    } else {
        printf("Validation Layers are OFF!\n");
    }

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
        .sType                  = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = instanceLayerCount,
        .ppEnabledLayerNames     = instanceLayers,
        .enabledExtensionCount  = instanceExtensionCount,
        .ppEnabledExtensionNames = instanceExtensions
    };

    if(vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        printf("ERROR: Failed to Create Vulkan Instance!\n");

        window_cleanup(&window);
        return EXIT_FAILURE;
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

        if(CreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            printf("ERROR: Failed to Create Debug Messenger!\n");

            window_cleanup(&window);
            return EXIT_FAILURE;
        }
    }

    //
    // Get a PhysicalDevice
    //
    u32 physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    printf("%d PhysicalDevice(s) found!\n", physicalDeviceCount);

    VkPhysicalDevice* physicalDeviceList = calloc(physicalDeviceCount, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDeviceList);

    VkPhysicalDeviceProperties props;
    for(u32 i = 0; i < physicalDeviceCount; i++) {
        vkGetPhysicalDeviceProperties(physicalDeviceList[i], &props);
        printf("Device [%d]: %s\n", i, props.deviceName);
    }

    //clean up the list we made
    free(physicalDeviceList);
    physicalDeviceList = nullptr;




    bool running = true;
    while(running) {
        if(window_poll_events())
            running = false;

        //Loop Logic
    }

    if(enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
    window_cleanup(&window);

    return EXIT_SUCCESS;
}
