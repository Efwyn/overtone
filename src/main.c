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

int main(int argc, char** argv) {
    Window window;
    if(window_create(&window, 640, 480, "Hello Overtone!") == ResultFailure) {
        printf("ERROR! Failed to Create Window\n");
        return EXIT_FAILURE;
    }

    window_show(&window);


    //Vulkan
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Overtone!",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Overtone",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4
    };

    VkInstanceCreateInfo createInfo = {
        .pApplicationInfo = &appInfo
    };

    if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        printf("ERROR: Failed to Create Vulkan Instance!\n");

        window_cleanup(&window);
        return EXIT_FAILURE;
    }

    //Get a PhysicalDevice
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

    vkDestroyInstance(instance, nullptr);
    window_cleanup(&window);

    return EXIT_SUCCESS;
}
