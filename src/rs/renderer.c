// =====================================
// File: renderer/renderer.c
// Description: The Core Renderer Source File 
// Author: Morgan Carpenetti
// Created On: 22-04-2026
// ========================================
#include "rs/renderer.h"
#include "rs/debug_util.h"
#include "rs/swapchain.h"
#include "rs/vertex.h"

#include "ec/types.h"
#include "ec/load.h"
#include "ec/timer.h"
#include "pf/window.h"
#include "ec/math/vec_types.h"
#include "ec/math/matrix.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <vulkan/vulkan.h>


#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

//#define PRINT_FRAME_TIMING
#ifdef PRINT_FRAME_TIMING
    const bool printFrameTiming = true;
#else
    const bool printFrameTiming = false;
#endif

#define FRAMES_IN_FLIGHT 2
u32 frameIndex = 0;

typedef struct UBO {
    Mat4 model;
    Mat4 view;
    Mat4 projection;
} UBO;

#define SHADER_NAME  "shaders/flat_textured.spv"
#define TEXTURE_NAME "textures/viking_room.bmp"
#define MESH_PATH    "meshes/viking_room.obj"

Result create_pipeline();
Result create_vertex_buffer(Vertex* vertices, u32 numVertices, VkBuffer* vertexBuffer, VkDeviceMemory* vertexBufferMemory);
Result create_index_buffer(u32* indices, u32 numIndices, VkBuffer* indexBuffer, VkDeviceMemory* indexBufferMemory);
Result create_depth_buffer();
Result recreate_depth_buffer();
Result create_uniform_buffers();
Result create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
Result copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
Result create_descriptor_pools(u32 descriptorCount);
Result create_descriptor_sets(u32 descriptorCount);
Result create_texture(Image fileImage, VkImage* image, VkDeviceMemory* imageMemory);
Result create_imageview(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView);
Result create_texture_sampler();

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
    VkQueue                  graphicsQueue;
    u32                      graphicsQueueIndex;
    bool                     framebufferResized;

    Swapchain                swapchain;

    VkCommandPool            commandPool;
    VkCommandBuffer          commandBuffers[FRAMES_IN_FLIGHT];
    //Synchronization
    VkSemaphore              acquireSemaphores[FRAMES_IN_FLIGHT];
    VkSemaphore*             submitSemaphores; //size based on swapchain image count
    VkFence                  frameFences[FRAMES_IN_FLIGHT];

    // Depth Buffer
    VkImage                  depthBufferImage;
    VkDeviceMemory           depthBufferMemory;
    VkImageView              depthBufferImageView;
    VkFormat                 depthBufferFormat;

    // vertex/index buffers
    MeshData                 meshData;
    VkBuffer                 vertexBuffer;
    VkDeviceMemory           vertexBufferMemory;
    VkBuffer                 indexBuffer;
    VkDeviceMemory           indexBufferMemory;
    // UBO
    VkBuffer*                uniformBuffers;
    VkDeviceMemory*          uniformBuffersMemory;
    void**                   uniformBuffersMapped;

    VkImage                  textureImage;
    VkDeviceMemory           textureImageMemory;
    VkImageView              textureImageView;
    VkSampler                textureSampler;
    // Descriptor Sets
    VkDescriptorSetLayout    descriptorSetLayout;
    VkDescriptorPool         descriptorPool;
    VkDescriptorSet*         descriptorSets;
    // Pipeline
    VkPipelineLayout         pipelineLayout;
    VkPipeline               graphicsPipeline;
} VulkanState;
VulkanState vs = {};

const VkApplicationInfo appInfo = {
    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName   = "Hello Overtone!",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName        = "Overtone",
    .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion         = VK_API_VERSION_1_4
};

Result renderer_initialize() {
    if(enableValidationLayers) {
        printf("[RS]: Validation Layers are ON!\n");
    } else {
        printf("[RS]: Validation Layers are OFF!\n");
    }

    //
    // Vulkan Instance
    //
    printf("[RS]: Creating Instance\n");

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

    if(vkCreateInstance(&instanceCreateInfo, nullptr, &vs.instance) != VK_SUCCESS) {
        printf("[RS]: Failed to Create Vulkan Instance!\n");
        return ResultFailure;
    }


    //
    // Setup the Debug Callback (if applicable)
    //

    if(enableValidationLayers) {
        if(setup_debug_util(vs.instance, &vs.debugMessenger) != VK_SUCCESS) {
            printf("[RS]: Failed to setup Debug Callback\n");
            return ResultFailure;
        }
    }

    //
    // Get a PhysicalDevice
    //
    printf("[RS]: Selecting Physical Device\n");

    u32 physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(vs.instance, &physicalDeviceCount, nullptr);
    printf("[RS]: %d PhysicalDevice(s) found!\n", physicalDeviceCount);

    VkPhysicalDevice* physicalDeviceList = calloc(physicalDeviceCount, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(vs.instance, &physicalDeviceCount, physicalDeviceList);

    VkPhysicalDeviceProperties props;
    for(u32 i = 0; i < physicalDeviceCount; i++) {
        vkGetPhysicalDeviceProperties(physicalDeviceList[i], &props);
        printf("[RS]: \tDevice [%d]: %s\n", i, props.deviceName);
    }

    // TODO: ENSURE PHYSICAL DEVICE SUPPORTS REQUIRED FEATURES
    vs.physicalDevice = physicalDeviceList[0];

    //
    // Create A Surface from the window
    //
    printf("[RS]: Creating Surface\n");
    if(window_create_vulkan_surface(vs.instance, &vs.surface) != VK_SUCCESS) {
        printf("[RS]: Failed to Create Surface!\n");
        return ResultFailure;
    }

    //
    // Determine Queue Info
    //
    printf("[RS]: Picking Queue Families\n");

    //Get a List of Properties for each queue family available on the physical device
    u32 queueFamilyCount = 0;
    VkQueueFamilyProperties* queueFamilyProps;
    vkGetPhysicalDeviceQueueFamilyProperties(vs.physicalDevice, &queueFamilyCount, nullptr);
    queueFamilyProps = calloc(queueFamilyCount, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(vs.physicalDevice, &queueFamilyCount, queueFamilyProps);

    //go through each queue family and find on the supports graphics operations and the surface we created
    u32 graphicsQueueIndex = ~0;
    for(u32 qfpIndex = 0; qfpIndex < queueFamilyCount; ++qfpIndex) {
        bool graphicsBit = false;
        VkBool32 surfaceSupported = VK_FALSE;
        graphicsBit = (queueFamilyProps[qfpIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT);
        if(vkGetPhysicalDeviceSurfaceSupportKHR(vs.physicalDevice, qfpIndex, vs.surface, &surfaceSupported) != VK_SUCCESS) {
            printf("[RS]: Unable to Query Physical Device for Surface Support!\n");
            return ResultFailure;
        }

        if(graphicsBit && surfaceSupported) {
            graphicsQueueIndex = qfpIndex;
            break;
        }
    }
    if(graphicsQueueIndex == ~0u) {
        printf("[RS]: Could not find a suitable Queue Family!\n");
        return ResultFailure;
    }

    float queuePriority = 0.5f;
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };
    vs.graphicsQueueIndex = graphicsQueueIndex;

    //
    // Create Logical Device
    //
    printf("[RS]: Creating Logical Device\n");

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
        .features = {
            .samplerAnisotropy = true,
        },
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

    if(vkCreateDevice(vs.physicalDevice, &deviceCreateInfo, nullptr, &vs.device) != VK_SUCCESS) {
        printf("[RS]: Failed to Create Logical Device!\n");
        return ResultFailure;
    }

    vkGetDeviceQueue(vs.device, vs.graphicsQueueIndex, 0, &vs.graphicsQueue);

    //
    // SwapChain Creation
    //
    printf("[RS]: Creating Swap Chain\n");

    u32 width, height;
    window_get_framebuffer_size(&width, &height);
    if(Swapchain_create(&vs.swapchain, vs.physicalDevice,  vs.device, vs.surface) != ResultOk) {
        printf("[RS]: Failed to Create Swap Chain!\n");
        return ResultFailure;
    }

    //
    // Descriptor Set Layout (where we configure out UBOs)
    //
    VkDescriptorSetLayoutBinding layoutBindings[2] = {
        //UBO
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        },
        //sampler
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
    };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = layoutBindings 
    };
    if(vkCreateDescriptorSetLayout(vs.device, &layoutInfo, nullptr, &vs.descriptorSetLayout) != VK_SUCCESS) {
        printf("[RS]: Failed to create descriptor set layout!\n");
        return ResultFailure;
    }

    //
    // Depth Buffer
    //
    if(create_depth_buffer() != ResultOk) {
        printf("[RS]: Failed to create depth buffer!\n");
        return ResultFailure;
    }

    //
    // graphicsPipeline
    //
    printf("[RS]: Creating Pipeline\n");

    if(create_pipeline() != ResultOk) {
        printf("[RS]: Failed to create pipeline!\n");
        return ResultFailure;
    }

    //
    // Command Pool and buffers
    //
    const VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vs.graphicsQueueIndex,
    };

    if(vkCreateCommandPool(vs.device, &commandPoolCreateInfo, nullptr, &vs.commandPool) != VK_SUCCESS) {
        printf("[RS]: Failed to create command pool!\n");
        return ResultFailure;
    }


    printf("[RS]: Loading meshes...\n");
    load_obj_file(MESH_PATH, &vs.meshData);

    if(create_vertex_buffer(vs.meshData.vertices, vs.meshData.vertexCount, &vs.vertexBuffer, &vs.vertexBufferMemory) != ResultOk) {
        printf("[RS]: Failed to create vertex buffer!\n");
        return ResultFailure;
    }
    if(create_index_buffer(vs.meshData.indices, vs.meshData.indexCount, &vs.indexBuffer, &vs.indexBufferMemory) != ResultOk) {
        printf("[RS]: Failed to create index buffer!\n");
        return ResultFailure;
    }

    if(create_uniform_buffers() != ResultOk) {
        printf("[RS]: Failed to create uniform buffers\n");
        return ResultFailure;
    }
    if(create_descriptor_pools(FRAMES_IN_FLIGHT) != ResultOk) {
        printf("[RS]: Failed to create descriptor pool\n");
        return ResultFailure;
    }



    printf("[RS]: Loading textures...\n");
    Image img = {};
    if(load_image(TEXTURE_NAME, &img) != ResultOk) {
        printf("[RS]: Failed to load texture!\n");
        return ResultFailure;
    }

    if(create_texture(img,  &vs.textureImage, &vs.textureImageMemory) != ResultOk) {
        printf("[RS]: Failed to create texture!\n");
        return ResultFailure;
    }

    if(create_imageview(vs.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, &vs.textureImageView) != ResultOk) {
        printf("[RS]: Failed to create texture image view\n");
        return ResultFailure;
    }

    if(create_texture_sampler() != ResultOk) {
        printf("[RS]: Failed to create texture sampler\n");
        return ResultFailure;
    }


    //depends on textureimageview
    if(create_descriptor_sets(FRAMES_IN_FLIGHT) != ResultOk) {
        printf("[RS]: Failed to create descriptor sets!\n");
        return ResultFailure;
    }


    const VkCommandBufferAllocateInfo commandBufferAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vs.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = FRAMES_IN_FLIGHT,
    };

    if(vkAllocateCommandBuffers(vs.device, &commandBufferAllocInfo, vs.commandBuffers) != VK_SUCCESS) {
        printf("[RS]: Failed to Allocate Command Buffers!\n");
        return ResultFailure;
    }

    //
    // Frame Synchronization Objects
    //
    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    const VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        if(vkCreateSemaphore(vs.device, &semaphoreCreateInfo, nullptr, &vs.acquireSemaphores[i]) != VK_SUCCESS) {
            printf("[RS]: Failed to create Semaphore!\n");
            return ResultFailure;
        }
        if(vkCreateFence(vs.device, &fenceCreateInfo, nullptr, &vs.frameFences[i]) != VK_SUCCESS) {
            printf("[RS]: Failed to create fence!\n");
            return ResultFailure;
        }
    }


    // Submission semapphores are instead based on swapchain length
    vs.submitSemaphores = calloc(vs.swapchain.length, sizeof(VkSemaphore));
    for(u32 i = 0; i < vs.swapchain.length; i++) {
        if(vkCreateSemaphore(vs.device, &semaphoreCreateInfo, nullptr, &vs.submitSemaphores[i]) != VK_SUCCESS) {
            printf("[RS]: Failed to create Semaphore!\n");
            return ResultFailure;
        }
    }

    printf("[RS]: Initialization Complete\n");

    //clean up our dynamic memory
    free(img.data);
    free(queueFamilyProps);
    free(instanceExtensions);
    free(physicalDeviceList);
    free(vs.meshData.vertices);
    free(vs.meshData.indices);

    return ResultOk;
}



void renderer_shutdown() {
    printf("[RS]: Shutting Down\n");

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(vs.device, vs.frameFences[i], nullptr);
        vkDestroySemaphore(vs.device, vs.acquireSemaphores[i], nullptr);
    }

    for(u32 i = 0; i < vs.swapchain.length; i++) {
        vkDestroySemaphore(vs.device, vs.submitSemaphores[i], nullptr);
    }
    free(vs.submitSemaphores);

    vkFreeCommandBuffers(vs.device, vs.commandPool, FRAMES_IN_FLIGHT, vs.commandBuffers);

    vkDestroyBuffer(vs.device, vs.vertexBuffer, nullptr);
    vkFreeMemory(vs.device, vs.vertexBufferMemory, nullptr);

    vkDestroyBuffer(vs.device, vs.indexBuffer, nullptr);
    vkFreeMemory(vs.device, vs.indexBufferMemory, nullptr);

    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkFreeDescriptorSets(vs.device, vs.descriptorPool, 1, &vs.descriptorSets[i]);
    }
    free(vs.descriptorSets);
    //maybe put elsewhere?
    vkDestroyPipelineLayout(vs.device, vs.pipelineLayout, nullptr);

    vkDestroyDescriptorPool(vs.device, vs.descriptorPool, nullptr);
    for(u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(vs.device, vs.uniformBuffers[i], nullptr);
        vkFreeMemory(vs.device, vs.uniformBuffersMemory[i], nullptr);
    }
    free(vs.uniformBuffers);
    free(vs.uniformBuffersMemory);
    free(vs.uniformBuffersMapped);


    vkDestroyCommandPool(vs.device, vs.commandPool, nullptr);

    vkDestroyPipeline(vs.device, vs.graphicsPipeline, nullptr);

    Swapchain_cleanup(&vs.swapchain, vs.device);

    vkDestroySampler(vs.device, vs.textureSampler, nullptr);
    vkDestroyImageView(vs.device,vs.textureImageView, nullptr);
    vkDestroyImage(vs.device, vs.textureImage, nullptr);
    vkFreeMemory(vs.device, vs.textureImageMemory, nullptr);

    vkDestroyDescriptorSetLayout(vs.device, vs.descriptorSetLayout, nullptr);

    vkDestroyImage(vs.device, vs.depthBufferImage, nullptr);
    vkDestroyImageView(vs.device, vs.depthBufferImageView, nullptr);
    vkFreeMemory(vs.device, vs.depthBufferMemory, nullptr);

    vkDestroyDevice(vs.device, nullptr);
    vkDestroySurfaceKHR(vs.instance, vs.surface, nullptr);

    if(enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(vs.instance, vs.debugMessenger, nullptr);
    }
    vkDestroyInstance(vs.instance, nullptr);
}

void transition_image_layout(VkCommandBuffer commandBuffer, VkImage image,
                               const VkImageLayout oldLayout,
                               const VkImageLayout newLayout,
                               const VkAccessFlags2 oldAccessFlags,
                               const VkAccessFlags2 newAccessFlags,
                               const VkPipelineStageFlags2 oldStageFlags,
                               const VkPipelineStageFlags2 newStageFlags,
                               const VkImageAspectFlags aspectFlags) {

    const VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = oldStageFlags,
        .srcAccessMask = oldAccessFlags,
        .dstStageMask = newStageFlags,
        .dstAccessMask = newAccessFlags,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image, 
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
    };

    const VkDependencyInfo dependencyInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}


Result record_command_buffer(VkCommandBuffer commandBuffer, const u32 imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        printf("[RS]: Failed to Begin Command Buffer!\n");
        return ResultFailure;
    }

    transition_image_layout(commandBuffer, vs.swapchain.images[imageIndex], 
                            VK_IMAGE_LAYOUT_UNDEFINED,                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_ACCESS_2_NONE,                                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_IMAGE_ASPECT_COLOR_BIT);

    transition_image_layout(commandBuffer, vs.depthBufferImage, 
                            VK_IMAGE_LAYOUT_UNDEFINED,                       VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,  VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                            VK_IMAGE_ASPECT_DEPTH_BIT);

    const VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    const VkClearValue clearDepth ={{{1.0f, 0}}};

    const VkRenderingAttachmentInfo colorAttachmentInfo = {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = vs.swapchain.imageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue  = clearColor,
    };

    const VkRenderingAttachmentInfo depthAttachmentInfo = {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = vs.depthBufferImageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue  = clearDepth,
    };

    VkRect2D renderArea = {
        .offset = {0, 0},
        .extent = vs.swapchain.extents,
    };
    const VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = renderArea,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo,
    };

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vs.graphicsPipeline);

    VkViewport viewport = {
        .width = vs.swapchain.extents.width,
        .height = vs.swapchain.extents.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &renderArea);

    //Draw the mesh
    VkBuffer vertexBuffers[] = {vs.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, vs.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vs.pipelineLayout, 0, 1, &vs.descriptorSets[frameIndex], 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, vs.meshData.indexCount, 1, 0, 0, 0);

    vkCmdEndRendering(commandBuffer);

    //End Frame
    transition_image_layout(commandBuffer, vs.swapchain.images[imageIndex],
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,          VK_ACCESS_2_NONE,
                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                            VK_IMAGE_ASPECT_COLOR_BIT);


    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        printf("[RS]: Failed to record command buffer!\n");
        return ResultFailure;
    }

    return ResultOk;
}

Result update_uniform_buffers(f32 deltaTime, u32 frameIndex) {
    UBO ubo = {};
    Vec3 rotationAxis   = { 0.0f, 0.0f, 1.0f };

    Vec3 cameraPosition = { 1.0f, 1.0f, 1.5f };
    Vec3 cameraTarget   = { 0.0f, 0.0f, 0.0f };
    Vec3 cameraUp       = { 0.0f, 0.0f, 1.0f };

    f32  aspectRatio = (f32)vs.swapchain.extents.width / (f32)vs.swapchain.extents.height;

    ubo.model = Mat4_rotate(Mat4_Identity, deltaTime * DEG_TO_RAD(20.0f), rotationAxis);
    ubo.view  = Mat4_lookAt(cameraPosition, cameraTarget, cameraUp);
    ubo.projection = Mat4_perspective(DEG_TO_RAD(60.0f), aspectRatio, 0.1f, 100.0f);

    memcpy(vs.uniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
    return ResultOk;
}

Result renderer_draw_frame(f32 deltaTime) {
    TIMESTEP(frameStartTime);

    //
    // Wait on fence
    //
    VkFence frameFence = vs.frameFences[frameIndex];
    if(vkWaitForFences(vs.device, 1, &frameFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        printf("[RS]: Failed to wait for the fence\n");
        return ResultFailure;
    }

    vkResetFences(vs.device, 1, &frameFence);

    u32 imageIndex = 0;
    VkSemaphore acquireSemaphore = vs.acquireSemaphores[frameIndex];
    VkResult acquireResult = vkAcquireNextImageKHR(vs.device, vs.swapchain.swapChain, UINT64_MAX, acquireSemaphore, nullptr, &imageIndex);
    if(acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        printf("[RS]: Swapchain of date\n");
        Result rs = Swapchain_recreate(&vs.swapchain, vs.physicalDevice, vs.device, vs.surface);
        Result rd = recreate_depth_buffer();
        if(rs || rd) {
            return ResultFailure;
        }
        return ResultOk;
    }

    else if(acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        assert(acquireResult == VK_TIMEOUT || acquireResult == VK_NOT_READY);
        printf("[RS]: Failed to retrieve next image!\n");
        return ResultFailure;
    }
    //index submit semaphore by image index rather than in-flight index
    VkSemaphore submitSemaphore = vs.submitSemaphores[imageIndex];


    //
    // Draw and Submit Commands
    //
    TIMESTEP(recordStartTime);
    VkCommandBuffer drawCommandBuffer = vs.commandBuffers[frameIndex];
    record_command_buffer(drawCommandBuffer, imageIndex);
    update_uniform_buffers(deltaTime, frameIndex);


    TIMESTEP(submitStartTime);
    VkPipelineStageFlags waitDestinationStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &acquireSemaphore,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &drawCommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &submitSemaphore
    };

    if(vkQueueSubmit(vs.graphicsQueue, 1, &submitInfo, frameFence) != VK_SUCCESS) {
        printf("[RS]: FAiled to submit command buffer to graphics queue\n");
        return ResultFailure;
    }


    //
    // Frame Presentation
    //
    TIMESTEP(presentStartTime);
    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &submitSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &vs.swapchain.swapChain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr
    };

    VkResult presentResult = vkQueuePresentKHR(vs.graphicsQueue, &presentInfo);
    if(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || vs.framebufferResized) {
        vs.framebufferResized = false;
        u32 width, height;
        window_get_framebuffer_size(&width, &height);
        Swapchain_recreate(&vs.swapchain, vs.physicalDevice, vs.device, vs.surface);
        recreate_depth_buffer();
    }
    else if(presentResult != VK_SUCCESS) {
        printf("[RS]: Failed to present image!\n");
        return ResultFailure;
    }

    TIMESTEP(frameEndTime);
    if(printFrameTiming) {
        printf("[RS] fence: %llu rec: %llu subm: %llu pre: %llu\n",
                recordStartTime - frameStartTime,
                submitStartTime - recordStartTime,
                presentStartTime - submitStartTime,
                frameEndTime - presentStartTime);
    }

    frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
    return ResultOk; 
}


Result create_pipeline() {

    BinaryFile shaderFile = {};
    if(load_binary_file(SHADER_NAME, &shaderFile) != ResultOk) {
        printf("[RS]: Failed to load shader file!\n");
        return ResultFailure;
    }

    VkShaderModule shaderModule;
    if(create_shader_module(shaderFile, vs.device, &shaderModule) != ResultOk) {
        printf("[RS]: Failed to Create Shader Module");
        return ResultFailure;
    }

    VkPipelineShaderStageCreateInfo vertexStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = shaderModule,
        .pName = "vertMain",
    };
    VkPipelineShaderStageCreateInfo fragmentStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = shaderModule,
        .pName = "fragMain"
    };
    VkPipelineShaderStageCreateInfo shaderStages[] = { 
        vertexStageInfo,
        fragmentStageInfo
    };


    //Vertex input. The format of the vertex data passed to the vertex shader.
    //In this first example(triangle, the vertex info is in the shader
    //itself

    // bindingDescription and attributeDescriptions will be broken out later to suitable
    // the needs of each pipeline. Here we are describing how a
    // { Vec2 pos, Vec3 color } vertex enters the pipeline

    VkVertexInputBindingDescription bindingDescription = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription attributeDescriptions[2] = {
        { //position
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, pos)
        },
        { // texture coordinates
            .location = 1, 
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(Vertex, texCoord)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = attributeDescriptions,
    };
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };


    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    //Rasterizer settings
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .lineWidth               = 1.0f
    };

    //Multisampling, (MSAA here?)
    // Off for now
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    //Color blending (i.e. how drawn fragments blend with fragments already
    // drawn in the same location). Can be used for alpha blending
    VkPipelineColorBlendAttachmentState blendAttachmentState = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &blendAttachmentState,
    };


    //What parts of the pipeline will be dynamic. Here we're setting
    //the viewport and scissor as dynamic so we can resize the window
    //without recreating the pipeline?
    VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates,
    };

    //Pipeline Layout, this is where we will
    // connect UBOs and push constants
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &vs.descriptorSetLayout,
        .pushConstantRangeCount = 0,
    };

    if(vkCreatePipelineLayout(vs.device, &pipelineLayoutCreateInfo, nullptr, &vs.pipelineLayout) != VK_SUCCESS) {
        printf("[RS]: Failed to create pipelineLayout!\n");
        return ResultFailure;
    }


    VkFormat attachmentFormats = vs.swapchain.format.format;

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &attachmentFormats,
        .depthAttachmentFormat = vs.depthBufferFormat,
    };
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .pDepthStencilState = &depthStencilInfo,
        .layout = vs.pipelineLayout,
        .renderPass = nullptr,
        .pNext = &pipelineRenderingCreateInfo,
    };

    if(vkCreateGraphicsPipelines(vs.device, nullptr, 1, &pipelineCreateInfo, nullptr, &vs.graphicsPipeline) != VK_SUCCESS) {
        printf("[RS]: Unable to create graphics pipeline!");
        return ResultFailure;
    }

    //vkDestroyPipelineLayout(vs.device, pipelineLayout, nullptr);
    vkDestroyShaderModule(vs.device, shaderModule, nullptr);
    free(shaderFile.data);
    return ResultOk;
}

// returns index on success, UINT32_MAX on failure
u32 query_memory_type(u32 typeFilter, VkMemoryPropertyFlags desiredProperties) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vs.physicalDevice, &memoryProperties);
    for(u32 i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if(typeFilter & (1 << i) &&
          (memoryProperties.memoryTypes[i].propertyFlags & desiredProperties) == desiredProperties) {
            return i;
        }
    }
    return UINT32_MAX;
}

Result create_buffer(VkDeviceSize size,
                   VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags memoryProperties,
                   VkBuffer* buffer,
                   VkDeviceMemory* bufferMemory) {
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if(vkCreateBuffer(vs.device, &bufferInfo, nullptr, buffer) != VK_SUCCESS) {
        printf("[RS]: Failed to create buffer!\n");
        return ResultFailure;
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(vs.device, *buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = query_memory_type(memoryRequirements.memoryTypeBits, memoryProperties),
    };

    if(vkAllocateMemory(vs.device, &allocateInfo, nullptr, bufferMemory) != VK_SUCCESS) {
        printf("[RS]: Failed to allocate buffer memory\n");
        return ResultFailure;
    }
    if(vkBindBufferMemory(vs.device, *buffer, *bufferMemory, 0) != VK_SUCCESS) {
        printf("[RS]: Failed to bind buffer memory\n");
        return ResultFailure;
    }

    return ResultOk;
}

// Two helper functions for one-time commands.
//  NOTE: since this is allocating
//  every time you want to do a command, it may be more effective to keep
//  it around in the future
Result begin_single_time_commands(VkCommandBuffer *commandBuffer) {
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = vs.commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    if(vkAllocateCommandBuffers(vs.device, &allocateInfo, commandBuffer) != VK_SUCCESS) {
        //critical failure, no recovery here
        printf("[RS]: Failed to create copy command buffer\n");
        return ResultFailure;
    }

    VkCommandBufferBeginInfo commandBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(*commandBuffer, &commandBeginInfo);

    return ResultOk;
}

void end_single_time_commands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    vkQueueSubmit(vs.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vs.graphicsQueue);

    vkFreeCommandBuffers(vs.device, vs.commandPool, 1, &commandBuffer);
}

// Copy data from one VkBuffer to another
// notes: maybe can do en masse?
// consider keeping the copy buffer around longer than just this function
Result copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer copyCommandBuffer;
    begin_single_time_commands(&copyCommandBuffer);

    VkBufferCopy copyRegion = {
        .size      = size,
    };
    vkCmdCopyBuffer(copyCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    end_single_time_commands(copyCommandBuffer);
    return ResultOk;
}

Result create_vertex_buffer(Vertex* vertices, u32 numVertices, VkBuffer* vertexBuffer, VkDeviceMemory* vertexBufferMemory) {
    printf("[RS]: Creating Vertex Buffer, %d vertices\n", numVertices);
    VkDeviceSize bufferSize = sizeof(Vertex) * numVertices;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    void* stagingData = nullptr;

    //First create a staging buffer
    if(create_buffer(bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &stagingBuffer,
                    &stagingBufferMemory) != ResultOk) {
        printf("[RS]: Failed to create staging Buffer\n");
        return ResultFailure;
    }

    //first allocate the memory in vulkan and set data to point to it
    vkMapMemory(vs.device, stagingBufferMemory, 0, bufferSize, 0, &stagingData);
    memcpy(stagingData, vertices, bufferSize);
    vkUnmapMemory(vs.device, stagingBufferMemory);

    if(create_buffer(bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    vertexBuffer,
                    vertexBufferMemory) != ResultOk) {
        printf("[RS]: Failed to create Vertex Buffer!\n");
        return ResultFailure;
    }

    //copy the data from the staging buffer to the GPU-local buffer
    copy_buffer(stagingBuffer, *vertexBuffer, bufferSize);

    vkDestroyBuffer(vs.device, stagingBuffer, nullptr);
    vkFreeMemory(vs.device, stagingBufferMemory, nullptr);

    return ResultOk;
}

Result create_index_buffer(u32* indices, u32 numIndices, VkBuffer* indexBuffer, VkDeviceMemory* indexBufferMemory) {
    printf("[RS]: Creating Index Buffer, %d indices\n", numIndices);
    VkDeviceSize bufferSize = sizeof(u32) * numIndices;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    void* stagingData = nullptr;

    //First create a staging buffer
    if(create_buffer(bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &stagingBuffer,
                    &stagingBufferMemory) != ResultOk) {
        printf("[RS]: Failed to create staging Buffer\n");
        return ResultFailure;
    }

    //allocate the memory in vulkan and set data to point to it
    vkMapMemory(vs.device, stagingBufferMemory, 0, bufferSize, 0, &stagingData);
    memcpy(stagingData, indices, bufferSize);
    vkUnmapMemory(vs.device, stagingBufferMemory);

    if(create_buffer(bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    indexBuffer,
                    indexBufferMemory) != ResultOk) {
        printf("[RS]: Failed to create Index Buffer!\n");
        return ResultFailure;
    }

    //copy the data from the staging buffer to the GPU-local buffer
    copy_buffer(stagingBuffer, *indexBuffer, bufferSize);

    vkDestroyBuffer(vs.device, stagingBuffer, nullptr);
    vkFreeMemory(vs.device, stagingBufferMemory, nullptr);

    return ResultOk;
}

Result create_uniform_buffers() {
    vs.uniformBuffers       = calloc(FRAMES_IN_FLIGHT, sizeof(VkBuffer));
    vs.uniformBuffersMemory = calloc(FRAMES_IN_FLIGHT, sizeof(VkDeviceMemory));
    vs.uniformBuffersMapped = calloc(FRAMES_IN_FLIGHT, sizeof(UBO));
 
    VkDeviceSize bufferSize = sizeof(UBO);
    for(size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        create_buffer(bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &vs.uniformBuffers[i], &vs.uniformBuffersMemory[i]);
        if(vkMapMemory(vs.device, vs.uniformBuffersMemory[i], 0, bufferSize, 0, &vs.uniformBuffersMapped[i]) != VK_SUCCESS) {
            printf("[RS]: Failed to map UBO memory!\n");
            return ResultFailure;
        }
    }
    return ResultOk;
}

// Create a pool of drescriptorsetLayout, one per frame in flight
Result create_descriptor_pools(u32 descriptorCount) {
    VkDescriptorPoolSize poolSizes[2] = {
        {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = descriptorCount,
        },
        {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = descriptorCount,
        },
    };
    VkDescriptorPoolCreateInfo poolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = descriptorCount,
        .poolSizeCount = 2,
        .pPoolSizes = poolSizes,
    };
    if(vkCreateDescriptorPool(vs.device, &poolCreateInfo, nullptr, &vs.descriptorPool) != VK_SUCCESS) {
        printf("[RS]: Failed to create descriptor pools!\n");
        return ResultFailure;
    }
    return ResultOk;
}

Result create_descriptor_sets(u32 descriptorCount) {
    //vkAllocateDescriptorSets expects an array of layouts the same size as the pool
    // so we need to allocate an array of layouts dynamically here
    VkDescriptorSetLayout* layouts;
    layouts = calloc(descriptorCount, sizeof(VkDescriptorSetLayout));
    for(u32 i = 0; i < descriptorCount; i++) {
        layouts[i] = vs.descriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vs.descriptorPool,
        .descriptorSetCount = descriptorCount,
        .pSetLayouts = layouts,
    };

    vs.descriptorSets = calloc(descriptorCount, sizeof(VkDescriptorSet)); 
    if(vs.descriptorSets == nullptr ||
       vkAllocateDescriptorSets(vs.device, &allocInfo, vs.descriptorSets) != VK_SUCCESS) {
        printf("[RS]: Failed to allocate descriptorSets!\n");
        return ResultFailure;
    }

    for(size_t i = 0; i < descriptorCount; i++) {
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = vs.uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UBO), //can use VK_WHOLE_SIZE if we're using the whole buffer
        };
        VkDescriptorImageInfo imageInfo = {
            .sampler = vs.textureSampler,
            .imageView = vs.textureImageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkWriteDescriptorSet descriptorWrites[2] = {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = vs.descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &bufferInfo,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = vs.descriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo,
            },
        };

        vkUpdateDescriptorSets(vs.device, 2, descriptorWrites, 0, nullptr);
    }

    free(layouts);
    return ResultOk;
}

void renderer_signal_framebuffer_resized(u32, u32) {
    vs.framebufferResized = true;
}

void renderer_wait_idle() {
    vkDeviceWaitIdle(vs.device);
}

Result create_image(u32 width, u32 height,
                    VkFormat format,
                    VkImageTiling tiling,
                    VkImageUsageFlags usage,
                    VkMemoryPropertyFlags properties,
                    VkImage* image,
                    VkDeviceMemory* imageMemory) {
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {
            width,
            height,
            1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage, 
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    vkCreateImage(vs.device, &imageInfo, nullptr, image); 
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vs.device, *image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = query_memory_type(memRequirements.memoryTypeBits, properties),
    };

    if(vkAllocateMemory(vs.device, &allocInfo, nullptr, imageMemory) != VK_SUCCESS) {
        printf("[RS]: Failed to allocate memory for image!\n");
        return ResultFailure;
    }

    vkBindImageMemory(vs.device, *image, *imageMemory, 0);

    return ResultOk;
}

void copy_buffer_to_image(VkBuffer buffer, VkImage image, u32 width, u32 height) {
    VkCommandBuffer cmdBuf;
    begin_single_time_commands(&cmdBuf);

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0, 0, 0 },
        .imageExtent = {
            width, height, 1,
        },
    };
    vkCmdCopyBufferToImage(cmdBuf, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    end_single_time_commands(cmdBuf);
}

Result create_texture(Image fileImage, VkImage* image, VkDeviceMemory* imageMemory) {

    //create a staging buffer for loading the image into gpu memory
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize bufferSize = fileImage.width * fileImage.height * 4;
    void* stagingData = nullptr;

    //First create a staging buffer
    if(create_buffer(bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &stagingBuffer,
                    &stagingBufferMemory) != ResultOk) {
        printf("[RS]: Failed to create staging Buffer\n");
        return ResultFailure;
    }

    //first allocate the memory in vulkan and set data to point to it
    vkMapMemory(vs.device, stagingBufferMemory, 0, bufferSize, 0, &stagingData);
    memcpy(stagingData, fileImage.data, bufferSize);
    vkUnmapMemory(vs.device, stagingBufferMemory);


    if(create_image(fileImage.width, fileImage.height,
                 VK_FORMAT_R8G8B8A8_SRGB,
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 image,
                 imageMemory) != ResultOk) {
        printf("[RS]: Failed to create temp texture image\n");
        return ResultFailure;
    }


    VkCommandBuffer transitionCommandBuffer;
    begin_single_time_commands(&transitionCommandBuffer);
    transition_image_layout(transitionCommandBuffer, *image,
            VK_IMAGE_LAYOUT_UNDEFINED,           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,        //layout
            VK_ACCESS_2_NONE,                    VK_ACCESS_2_TRANSFER_WRITE_BIT,              //access flags
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,            //stage flags
            VK_IMAGE_ASPECT_COLOR_BIT); 
    end_single_time_commands(transitionCommandBuffer);

    copy_buffer_to_image(stagingBuffer, *image, fileImage.width, fileImage.height);

    begin_single_time_commands(&transitionCommandBuffer);
    transition_image_layout(transitionCommandBuffer, *image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,       VK_ACCESS_2_SHADER_READ_BIT, //access flags
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,     VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, // | VK_PIPELINE_STAGE_2_COPY_BIT); //stage flags
            VK_IMAGE_ASPECT_COLOR_BIT);
    end_single_time_commands(transitionCommandBuffer);


    vkDestroyBuffer(vs.device, stagingBuffer, nullptr);
    vkFreeMemory(vs.device, stagingBufferMemory, nullptr);
    return ResultOk; 
}

Result create_imageview(const VkImage image, const VkFormat format, const VkImageAspectFlags aspectFlags, VkImageView* imageView) {
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    if(vkCreateImageView(vs.device, &viewInfo, nullptr, imageView) != VK_SUCCESS) {
        printf("[RS]: Failed to create image view for texture!\n");
        return ResultFailure;
    }

    return ResultOk;
}

Result create_texture_sampler() {
    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(vs.physicalDevice, &props);
    VkSamplerCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        //mipmapping
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        //how to u,v,w, coordinate addressing behaves
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT, //repeat texture if u goes beyond texture bounds
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        //anisotropic filtering
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = props.limits.maxSamplerAnisotropy,
        //result can be compared to a value and filtered
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,

        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    if(vkCreateSampler(vs.device, &createInfo, nullptr, &vs.textureSampler) != VK_SUCCESS) {
        printf("[RS]: Failed to create sampler for texture!\n");
        return ResultFailure;
    }

    return ResultOk;
}

Result find_suitable_format(const VkFormat* candidates, const u32 candidate_count, const VkImageTiling tiling, const VkFormatFeatureFlags features, VkFormat* chosenFormat) {
    for(u32 i = 0; i < candidate_count; i++) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(vs.physicalDevice, candidates[i], &props);
        if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features)) {
            *chosenFormat = candidates[i];
            return ResultOk;

        } else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features)) {
            *chosenFormat = candidates[i];
            return ResultOk;
        }
    }

    printf("[RS]: Failed to find a suitable depth buffer format!\n");
    return ResultFailure;
}

Result create_depth_buffer() {
#define DEPTH_BUFFER_FORMAT_CANDIDATE_COUNT 3
    const VkFormat formatCandidates[DEPTH_BUFFER_FORMAT_CANDIDATE_COUNT] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };
    if(find_suitable_format(formatCandidates,
                            DEPTH_BUFFER_FORMAT_CANDIDATE_COUNT, 
                            VK_IMAGE_TILING_OPTIMAL,
                            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
                            &vs.depthBufferFormat) != ResultOk) {
        printf("[RS]: Failed to find a suitable format for depth buffer!\n");
        return ResultFailure;
    };


    if(create_image(vs.swapchain.extents.width, vs.swapchain.extents.height,
                     vs.depthBufferFormat, VK_IMAGE_TILING_OPTIMAL,
                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     &vs.depthBufferImage, &vs.depthBufferMemory) != ResultOk) {
        printf("[RS]: Failed to create depth buffer image\n");
        return ResultFailure;
    }

    if(create_imageview(vs.depthBufferImage,
                        vs.depthBufferFormat,
                        VK_IMAGE_ASPECT_DEPTH_BIT,
                        &vs.depthBufferImageView) != ResultOk) {
        printf("[RS]: Failed to create depth buffer image view\n");
        return ResultFailure;
    }

    printf("[RS]: Depth buffer created\n");
    return ResultOk;
}

Result recreate_depth_buffer() {
        vkDestroyImage(vs.device, vs.depthBufferImage, nullptr);
        vkDestroyImageView(vs.device, vs.depthBufferImageView, nullptr);
        vkFreeMemory(vs.device, vs.depthBufferMemory, nullptr);
        return create_depth_buffer();
}
