#include "RenderModule.hpp"
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Windows.h>
#include <vector>
#include <iostream>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

VkInstance g_vkInstanse;
VkDevice g_device;
VkPhysicalDevice g_physDevice;
VkDeviceMemory g_buffer1Mem;
VkDeviceMemory g_buffer2Mem;
VkDeviceMemory g_deviceMemory2;
VkDeviceMemory g_deviceMemoryImgSrc;
VkDeviceMemory g_deviceMemoryImgDst;
VkDeviceMemory g_deviceMemoryImgShaderOut;
VkBuffer buffer1, buffer2;
VkDebugUtilsMessengerEXT DebugMessager;
VkSwapchainKHR g_swapChain;
VkSurfaceKHR g_surface;

int g_width = 800;
int g_height = 600;

class Guard
{
public:
    Guard() = default;
    ~Guard()
    {
        if (g_device) vkDeviceWaitIdle(g_device);

        if (buffer1) vkDestroyBuffer(g_device, buffer1, nullptr);
        if (buffer2) vkDestroyBuffer(g_device, buffer2, nullptr);

        if (g_buffer1Mem) vkFreeMemory(g_device, g_buffer1Mem, nullptr);
        if (g_buffer2Mem) vkFreeMemory(g_device, g_buffer2Mem, nullptr);
        if (g_deviceMemory2) vkFreeMemory(g_device, g_deviceMemory2, nullptr);
        if (g_deviceMemoryImgSrc) vkFreeMemory(g_device, g_deviceMemoryImgSrc, nullptr);
        if (g_deviceMemoryImgDst) vkFreeMemory(g_device, g_deviceMemoryImgDst, nullptr);
        if (g_deviceMemoryImgShaderOut) vkFreeMemory(g_device, g_deviceMemoryImgShaderOut, nullptr);
        if (g_swapChain) vkDestroySwapchainKHR(g_device, g_swapChain, nullptr);
        if (g_surface) vkDestroySurfaceKHR(g_vkInstanse, g_surface, nullptr);
        if (g_device) vkDestroyDevice(g_device, nullptr);
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_vkInstanse, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(g_vkInstanse, DebugMessager, nullptr);
        }
        if (g_vkInstanse) vkDestroyInstance(g_vkInstanse, nullptr);
    }
};
Guard g;


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

RenderModule::RenderModule(GLFWwindow* window)
    : EngineModule("RenderLogic")
    , m_window(window)

{}

void RenderModule::init()
{
    { // Init
    //vkCreateInstance
    //vkEnumeratePhysicalDevices
    //vkGetPhysicalDeviceProperties
    //vkGetPhysicalDeviceFeatures
    //vkGetPhysicalDeviceMemoryProperties
    //vkGetPhysicalDeviceQueueFamilyProperties
    //vkEnumerateInstanceExtensionProperties
    //vkEnumerateDeviceExtensionProperties
    //vkCreateDevice
        VkApplicationInfo appInfo;
        appInfo.pApplicationName = "App";
        appInfo.pEngineName = "Engine";
        //appInfo.apiVersion 

        VkInstanceCreateInfo info;
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pNext = nullptr;
        info.pApplicationInfo = nullptr;
        info.flags = 0;
        const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        info.enabledLayerCount = validationLayers.size();
        info.ppEnabledLayerNames = validationLayers.data();

        std::vector<char*> extensions{ "VK_KHR_surface","VK_KHR_win32_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

        info.enabledExtensionCount = extensions.size();
        info.ppEnabledExtensionNames = extensions.data();
        VkResult res = vkCreateInstance(&info, nullptr, &g_vkInstanse);
        if (res != VK_SUCCESS)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional


        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_vkInstanse, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(g_vkInstanse, &createInfo, nullptr, &DebugMessager);
        }
        uint32_t physDeviceCount = 0;
        VkPhysicalDevice* physicalDevices = nullptr;
        vkEnumeratePhysicalDevices(g_vkInstanse, &physDeviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices;
        devices.resize(physDeviceCount);

        vkEnumeratePhysicalDevices(g_vkInstanse, &physDeviceCount, devices.data());

        VkPhysicalDeviceProperties deviceProps = {};
        g_physDevice = devices[0];
        vkGetPhysicalDeviceProperties(devices[0], &deviceProps);
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(devices[0], &features);

        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(devices[0], &memProps);

        uint32_t queueFamilyCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(devices[0], &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[0], &queueFamilyCount, queueFamilyProperties.data());

        uint32_t instanceExt = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExt, nullptr);
        std::vector<VkExtensionProperties> instanceExtProps;
        instanceExtProps.resize(instanceExt);
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExt, instanceExtProps.data());

        uint32_t deviceExt = 0;
        vkEnumerateDeviceExtensionProperties(devices[0], nullptr, &deviceExt, nullptr);
        std::vector<VkExtensionProperties> deviceExtProps;
        deviceExtProps.resize(deviceExt);
        vkEnumerateDeviceExtensionProperties(devices[0], nullptr, &deviceExt, deviceExtProps.data());

        {
            VkDeviceCreateInfo info = {};
            VkDeviceQueueCreateInfo queueInfo = {};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.pNext = nullptr;
            queueInfo.flags = 0;
            queueInfo.pQueuePriorities = nullptr;
            queueInfo.queueCount = 16;
            queueInfo.queueFamilyIndex = 0;

            info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.queueCreateInfoCount = 1;
            info.pQueueCreateInfos = &queueInfo;

            info.enabledLayerCount = 0;
            info.ppEnabledLayerNames = nullptr;

            std::vector<char*> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_NV_sample_mask_override_coverage" };
            info.enabledExtensionCount = extensions.size();
            info.ppEnabledExtensionNames = extensions.data();

            info.pEnabledFeatures = nullptr;
            VkResult res = vkCreateDevice(devices[0], &info, nullptr, &g_device);
            if (res != VK_SUCCESS)
            {
                return;
            }
        }
    } // Init

    { //Memory, resources
        //vkCreateBuffer
        //vkDestroyBuffer
        //vkGetPhysicalDeviceFormatProperties
        //vkGetPhysicalDeviceImageFormatProperties
        //vkCreateImage
        //vkDestroyImage
        //vkGetImageSubresourceLayout
        //vkCreateBufferView
        //vkDestroyBufferView
        //vkCreateImageView
        //vkDestroyImageView
        //vkAllocateMemory
        //vkFreeMemory
        //vkMapMemory
        //vkUnmapMemory
        //vkGetBufferMemoryRequirements
        //vkBindBufferMemory
        //vkGetImageMemoryRequirements
        //vkBindImageMemory
        //SPARSE ???


        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        createInfo.size = sizeof(unsigned int) * 2048 * 2048;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(g_device, &createInfo, nullptr, &buffer1);
        vkCreateBuffer(g_device, &createInfo, nullptr, &buffer2);

        VkFormatProperties out = {};
        vkGetPhysicalDeviceFormatProperties(g_physDevice, VK_FORMAT_R8G8B8A8_UINT, &out);

        VkImageFormatProperties out2 = {};
        vkGetPhysicalDeviceImageFormatProperties(g_physDevice, VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT, &out2);

        VkImage image;
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.pNext = nullptr;
        imageCreateInfo.flags = 0;
        imageCreateInfo.extent = VkExtent3D{ 256, 256, 1 };
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.mipLevels = 7;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        auto res = vkCreateImage(g_device, &imageCreateInfo, nullptr, &image);

        VkImageSubresource subresource = {};
        subresource.arrayLayer = 0;
        subresource.mipLevel = 2;
        subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        VkSubresourceLayout subResLayut = {};
        vkGetImageSubresourceLayout(g_device, image, &subresource, &subResLayut);

        VkBufferView bufferView = VK_NULL_HANDLE;
        VkBufferViewCreateInfo bufferViewCreateInfo = {};
        bufferViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        bufferViewCreateInfo.pNext = nullptr;
        bufferViewCreateInfo.flags = 0;
        bufferViewCreateInfo.buffer = buffer1;
        bufferViewCreateInfo.format = VK_FORMAT_UNDEFINED;
        bufferViewCreateInfo.offset = 0;
        bufferViewCreateInfo.range = sizeof(float) * 32;


        VkMemoryRequirements memoryRequirements = {};

        vkGetBufferMemoryRequirements(g_device, buffer1, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = 9;

        res = vkAllocateMemory(g_device, &allocateInfo, nullptr, &g_buffer1Mem);
        res = vkAllocateMemory(g_device, &allocateInfo, nullptr, &g_buffer2Mem);

        void* data = nullptr;
        res = vkMapMemory(g_device, g_buffer1Mem, 0, VK_WHOLE_SIZE, 0, &data);

        if (data != nullptr)
        {
            for (int i = 0; i < 128; ++i)
            {
                *reinterpret_cast<float*>(data) = 0.5f;
            }
            vkUnmapMemory(g_device, g_buffer1Mem);
        }

        {
            void* data = nullptr;
            vkMapMemory(g_device, g_buffer2Mem, 0, VK_WHOLE_SIZE, 0, &data);

            float sum = 0.0f;

            if (data != nullptr)
            {
                for (int i = 0; i < 128; ++i)
                {
                    float a = *reinterpret_cast<float*>(data);
                    sum += a;
                }
                vkUnmapMemory(g_device, g_buffer2Mem);
            }
        }

        res = vkBindBufferMemory(g_device, buffer1, g_buffer1Mem, 0);
        res = vkBindBufferMemory(g_device, buffer2, g_buffer2Mem, 0);



        VkImageView imgView = VK_NULL_HANDLE;
        VkImageViewCreateInfo imgViewCreateInfo = {};
        VkImageSubresourceRange subRange = {};
        subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subRange.baseArrayLayer = 0;
        subRange.layerCount = 1;
        subRange.baseMipLevel = 0;
        subRange.levelCount = 1;

        imgViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imgViewCreateInfo.pNext = nullptr;
        imgViewCreateInfo.flags = 0;
        imgViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imgViewCreateInfo.image = image;
        imgViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imgViewCreateInfo.subresourceRange = subRange;


        vkGetImageMemoryRequirements(g_device, image, &memoryRequirements);

        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = 1;

        res = vkAllocateMemory(g_device, &allocateInfo, nullptr, &g_deviceMemory2);
        res = vkBindImageMemory(g_device, image, g_deviceMemory2, 0);

        res = vkCreateImageView(g_device, &imgViewCreateInfo, nullptr, &imgView);

        vkDestroyImageView(g_device, imgView, nullptr);

        vkDestroyImage(g_device, image, nullptr);
    }

    { // Command buffers
        //vkGetDeviceQueue
        //vkCreateCommandPool
        //vkDestroyCommandPool
        //vkAllocateCommandBuffers
        //vkBeginCommandBuffer
        //vkEndCommandBuffer
        //vkCmdCopyBuffer
        //vkResetCommandBuffer
        //vkQueueSubmit
        //vkQueueWaitIdle

        VkQueue queue;
        vkGetDeviceQueue(g_device, 0, 0, &queue);

        if (queue == VK_NULL_HANDLE)
        {
            return;
        }

        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.pNext = nullptr;
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = 0;

        VkCommandPool commandPool;
        vkCreateCommandPool(g_device, &commandPoolInfo, nullptr, &commandPool);

        VkCommandBufferAllocateInfo cmdAllocInfo = {};
        cmdAllocInfo.commandPool = commandPool;
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.pNext = nullptr;
        cmdAllocInfo.commandBufferCount = 1;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(g_device, &cmdAllocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = nullptr;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy region = {};
        region.srcOffset = 0;
        region.dstOffset = 0;
        region.size = 128 * sizeof(float);
        vkCmdCopyBuffer(commandBuffer, buffer1, buffer2, 1, &region);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.waitSemaphoreCount = submitInfo.signalSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = submitInfo.pSignalSemaphores = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

        vkQueueWaitIdle(queue);

        void* data = nullptr;
        vkMapMemory(g_device, g_buffer2Mem, 0, VK_WHOLE_SIZE, 0, &data);

        float sum = 0.0f;

        if (data != nullptr)
        {
            for (int i = 0; i < 128; ++i)
            {
                float a = *reinterpret_cast<float*>(data);
                sum += a;
            }
            vkUnmapMemory(g_device, g_buffer2Mem);
        }

        vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

        vkFreeCommandBuffers(g_device, commandPool, 1, &commandBuffer);
        vkDestroyCommandPool(g_device, commandPool, nullptr);
    }
}

void RenderModule::deinit()
{
}

bool RenderModule::needsToRun() const
{
    return false;
}

tf::Task RenderModule::scedule(tf::Taskflow& flow)
{
    tf::Task ret = needsToRun() == false ? flow.emplace([]() {}) : flow.emplace([this]() {
        std::cout << "RenderLogic\n";

        ///////--------------------

        { // 4.Moving Data
  //vkCmdPipelineBarrier
  //vkCmdCopyImageToBuffer
  //vkCmdBlitImage
  //vkCmdCopyBufferToImage

            unsigned char* data = nullptr;
            int imgW, imgH, n2;

            stbi_info("result.png", &imgW, &imgW, &n2);
            data = stbi_load("result.png", &imgW, &imgH, 0, STBI_rgb_alpha);

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBuffMem;
            VkDeviceSize memSize = imgW * imgH * 4;
            {
                VkBufferCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                createInfo.pNext = nullptr;
                createInfo.flags = 0;
                createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                createInfo.size = memSize;
                createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                vkCreateBuffer(g_device, &createInfo, nullptr, &stagingBuffer);

                VkMemoryRequirements memoryRequirements = {};

                vkGetBufferMemoryRequirements(g_device, stagingBuffer, &memoryRequirements);

                VkMemoryAllocateInfo allocateInfo = {};
                allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocateInfo.pNext = nullptr;
                allocateInfo.allocationSize = memoryRequirements.size;
                allocateInfo.memoryTypeIndex = 9;

                auto res = vkAllocateMemory(g_device, &allocateInfo, nullptr, &stagingBuffMem);
                vkBindBufferMemory(g_device, stagingBuffer, stagingBuffMem, 0);
            }

            {
                void* mappedData = nullptr;
                vkMapMemory(g_device, stagingBuffMem, 0, VK_WHOLE_SIZE, 0, &mappedData);

                if (mappedData != nullptr)
                {
                    memcpy(mappedData, (void*)data, memSize);

                    vkUnmapMemory(g_device, stagingBuffMem);
                }
            }

            VkCommandPoolCreateInfo commandPoolInfo = {};
            commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolInfo.pNext = nullptr;
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commandPoolInfo.queueFamilyIndex = 0;

            VkCommandPool commandPool;
            vkCreateCommandPool(g_device, &commandPoolInfo, nullptr, &commandPool);

            auto createSingleTimeCommandBuffer = [&commandPool]()
            {
                VkCommandBufferAllocateInfo cmdAllocInfo = {};
                cmdAllocInfo.commandPool = commandPool;
                cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                cmdAllocInfo.pNext = nullptr;
                cmdAllocInfo.commandBufferCount = 1;
                cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

                VkCommandBuffer commandBuffer;
                vkAllocateCommandBuffers(g_device, &cmdAllocInfo, &commandBuffer);

                VkCommandBufferBeginInfo beginInfo = {};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.pNext = nullptr;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                beginInfo.pInheritanceInfo = nullptr;

                vkBeginCommandBuffer(commandBuffer, &beginInfo);
                return commandBuffer;
            };

            auto endSingleTimeCommandBuffer = [&commandPool](VkCommandBuffer commandBuffer)
            {
                vkEndCommandBuffer(commandBuffer);

                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.pNext = nullptr;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffer;
                submitInfo.waitSemaphoreCount = submitInfo.signalSemaphoreCount = 0;
                submitInfo.pWaitSemaphores = submitInfo.pSignalSemaphores = nullptr;
                submitInfo.pWaitDstStageMask = nullptr;

                VkQueue queue;
                vkGetDeviceQueue(g_device, 0, 0, &queue);
                vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

                vkQueueWaitIdle(queue);
                //vkDeviceWaitIdle(g_device);
                vkFreeCommandBuffers(g_device, commandPool, 1, &commandBuffer);
            };

            VkImage imageSrc;
            VkImage imageDst;
            VkImage imageShaderOut;
            VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.pNext = nullptr;
            imageCreateInfo.flags = 0;
            imageCreateInfo.extent = VkExtent3D{ (uint32_t)imgW, (uint32_t)imgH, 1 };
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;


            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(g_physDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProperties);
            // Mip-chain generation requires support for blit source and destination
            assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
            assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

            auto res = vkCreateImage(g_device, &imageCreateInfo, nullptr, &imageSrc);

            imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            imageCreateInfo.extent = VkExtent3D{ (uint32_t)imgW / 2, (uint32_t)imgH / 2, 1 };
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            res = vkCreateImage(g_device, &imageCreateInfo, nullptr, &imageDst);
            res = vkCreateImage(g_device, &imageCreateInfo, nullptr, &imageShaderOut);


            VkMemoryRequirements memoryRequirements = {};
            vkGetImageMemoryRequirements(g_device, imageSrc, &memoryRequirements);
            VkDeviceSize firstSizeBlock;
            //firstSizeBlock = memoryRequirements.size;

            VkMemoryAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.pNext = nullptr;
            allocateInfo.allocationSize = memoryRequirements.size;
            allocateInfo.memoryTypeIndex = 1;//9 - host visible, 0 - dev local

            res = vkAllocateMemory(g_device, &allocateInfo, nullptr, &g_deviceMemoryImgSrc);
            res = vkBindImageMemory(g_device, imageSrc, g_deviceMemoryImgSrc, 0);

            vkGetImageMemoryRequirements(g_device, imageDst, &memoryRequirements);
            res = vkAllocateMemory(g_device, &allocateInfo, nullptr, &g_deviceMemoryImgDst);
            res = vkBindImageMemory(g_device, imageDst, g_deviceMemoryImgDst, 0);

            res = vkAllocateMemory(g_device, &allocateInfo, nullptr, &g_deviceMemoryImgShaderOut);
            res = vkBindImageMemory(g_device, imageShaderOut, g_deviceMemoryImgShaderOut, 0);


            VkImageView imgViewSrc = VK_NULL_HANDLE;
            VkImageView imgViewDst = VK_NULL_HANDLE;
            VkImageViewCreateInfo imgViewCreateInfo = {};
            VkImageSubresourceRange subRange = {};
            subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subRange.baseArrayLayer = 0;
            subRange.layerCount = 1;
            subRange.baseMipLevel = 0;
            subRange.levelCount = 1;

            imgViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imgViewCreateInfo.pNext = nullptr;
            imgViewCreateInfo.flags = 0;
            imgViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            imgViewCreateInfo.image = imageDst;
            imgViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
            imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imgViewCreateInfo.subresourceRange = subRange;

            vkCreateImageView(g_device, &imgViewCreateInfo, nullptr, &imgViewSrc);
            imgViewCreateInfo.image = imageShaderOut;
            res = vkCreateImageView(g_device, &imgViewCreateInfo, nullptr, &imgViewDst);

            VkSampler imageSampler = VK_NULL_HANDLE;

            VkSamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerCreateInfo.pNext = nullptr;
            samplerCreateInfo.flags = 0;
            samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
            samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.mipLodBias = 0.0f;
            samplerCreateInfo.anisotropyEnable = VK_FALSE;
            samplerCreateInfo.maxAnisotropy = 1.0f;
            samplerCreateInfo.compareEnable = VK_FALSE;
            samplerCreateInfo.compareOp = VK_COMPARE_OP_EQUAL;
            samplerCreateInfo.minLod = 0.0f;
            samplerCreateInfo.maxLod = 7.0f;
            samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
            vkCreateSampler(g_device, &samplerCreateInfo, nullptr, &imageSampler);

            {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = imageSrc;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = 0; // TODO
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // TODO

                vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                endSingleTimeCommandBuffer(cmdBuff);
            }
            {
                auto cmdBuff = createSingleTimeCommandBuffer();
                VkBufferImageCopy region = {};
                region.bufferOffset = 0;
                region.bufferImageHeight = 0;
                region.bufferRowLength = 0;

                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = 0;
                region.imageSubresource.baseArrayLayer = 0;
                region.imageSubresource.layerCount = 1;

                region.imageOffset = { 0, 0, 0 };
                region.imageExtent = {
                    (uint32_t)imgW,
                    (uint32_t)imgH,
                    (uint32_t)1
                };

                vkCmdCopyBufferToImage(cmdBuff, stagingBuffer, imageSrc, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
                endSingleTimeCommandBuffer(cmdBuff);
            }

            {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = imageSrc;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // TODO
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; // TODO

                vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                endSingleTimeCommandBuffer(cmdBuff);
            }

            {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = imageDst;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = 0; // TODO
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // TODO

                vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                endSingleTimeCommandBuffer(cmdBuff);
            }

            {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkImageBlit regions = {};
                regions.srcOffsets[0] = VkOffset3D{ 0,0, 0 };
                regions.srcOffsets[1] = VkOffset3D{ imgW,imgH, 1 };
                regions.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                regions.srcSubresource.baseArrayLayer = 0;
                regions.srcSubresource.mipLevel = 0;
                regions.srcSubresource.layerCount = 1;

                regions.dstOffsets[1] = VkOffset3D{ 0,0, 1 };
                regions.dstOffsets[0] = VkOffset3D{ imgW / 2,imgH / 2, 0 };
                regions.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                regions.dstSubresource.baseArrayLayer = 0;
                regions.dstSubresource.mipLevel = 0;
                regions.dstSubresource.layerCount = 1;

                vkCmdBlitImage(cmdBuff, imageSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imageDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regions, VK_FILTER_LINEAR);
                endSingleTimeCommandBuffer(cmdBuff);
            }

            {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = imageDst;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // TODO
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; // TODO

                vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                endSingleTimeCommandBuffer(cmdBuff);
            }

            {
                auto cmdBuff = createSingleTimeCommandBuffer();
                VkBufferImageCopy region = {};
                region.bufferOffset = 0;
                region.bufferImageHeight = 0;
                region.bufferRowLength = 0;

                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = 0;
                region.imageSubresource.baseArrayLayer = 0;
                region.imageSubresource.layerCount = 1;

                region.imageOffset = { 0, 0, 0 };
                region.imageExtent = {
                    (uint32_t)imgW / 2,
                    (uint32_t)imgH / 2,
                    (uint32_t)1
                };

                vkCmdCopyImageToBuffer(cmdBuff, imageDst, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &region);
                endSingleTimeCommandBuffer(cmdBuff);
            }

            if (0) {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkBufferMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

                barrier.pNext = nullptr;
                barrier.buffer = stagingBuffer;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;//todo
                barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;//todo
                barrier.offset = 0;
                barrier.size = VK_WHOLE_SIZE;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;


                vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

                endSingleTimeCommandBuffer(cmdBuff);
            }

            {
                void* mappedReadData = nullptr;
                vkMapMemory(g_device, stagingBuffMem, 0, VK_WHOLE_SIZE, 0, &mappedReadData);

                if (mappedReadData != nullptr)
                {
                    //memcpy(mappedData, (void*)data, imgW * imgH * n2);
                    stbi_write_png("small.png", imgW / 2, imgH / 2, 4, mappedReadData, imgW / 2 * 4);
                    vkUnmapMemory(g_device, stagingBuffMem);
                }
            }
            // void* mappedData = nullptr;
            // vkMapMemory(g_device, g_deviceMemoryImgSrc, 0, VK_WHOLE_SIZE, 0, &mappedData);
            //
            // if (mappedData != nullptr)
            // {
            //     memcpy(mappedData, (void*)data, imgW * imgH * 4);
            //
            //     vkUnmapMemory(g_device, g_deviceMemoryImgSrc);
            // }
            //
            // {
            //     void* data = nullptr;
            //     res = vkMapMemory(g_device, g_buffer1Mem, 0, VK_WHOLE_SIZE, 0, &data);
            //
            //     if (data != nullptr)
            //     {
            //         memcpy(data, (void*)data, imgW * imgH * 4);
            //         vkUnmapMemory(g_device, g_buffer1Mem);
            //     }
            // }
           //  VkMappedMemoryRange r = {};
           //  r.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
           //  r.pNext = nullptr;
           //  r.memory = g_deviceMemoryImgSrc;
           //  r.offset = 0;
           //  r.size = VK_WHOLE_SIZE;

          //   vkFlushMappedMemoryRanges(g_device, 1, &r);
             //   res = vkCreateImageView(g_device, &imgViewCreateInfo, nullptr, &imgViewSrc);
             //   res = vkCreateImageView(g_device, &imgViewCreateInfo, nullptr, &imgViewDst);


           //  VkCommandBufferAllocateInfo cmdAllocInfo = {};
           //  cmdAllocInfo.commandPool = commandPool;
           //  cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
           //  cmdAllocInfo.pNext = nullptr;
           //  cmdAllocInfo.commandBufferCount = 1;
           //  cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

           //  VkCommandBuffer commandBuffer;
           //  vkAllocateCommandBuffers(g_device, &cmdAllocInfo, &commandBuffer);
           //
           //  VkCommandBufferBeginInfo beginInfo = {};
           //  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
           //  beginInfo.pNext = nullptr;
           //  beginInfo.flags = 0;
           //  beginInfo.pInheritanceInfo = nullptr;
           //
           //  vkBeginCommandBuffer(commandBuffer, &beginInfo);
           //
           //  VkImageBlit regions = {};
           //  //regions.srcOffsets[0] = VkOffset3D{0,0, 0};
           //  regions.srcOffsets[1] = VkOffset3D{ imgW,imgH, 1 };
           //  regions.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
           //  regions.srcSubresource.baseArrayLayer = 0;
           //  regions.srcSubresource.mipLevel = 0;
           //  regions.srcSubresource.layerCount = 1;
           //
           //  //regions.dstOffsets[0] = VkOffset3D{ 0,0, 0 };
           //  regions.dstOffsets[1] = VkOffset3D{ imgW / 2,imgH / 2, 1 };
           //  regions.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
           //  regions.dstSubresource.baseArrayLayer = 0;
           //  regions.dstSubresource.mipLevel = 0;
           //  regions.dstSubresource.layerCount = 1;
           //  VkMemoryBarrier memBarier = {};
           //  memBarier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
           //  memBarier.pNext = nullptr;
           //  memBarier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
           //
           //  memBarier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
           //  //  VkImageMemoryBarrier imageMemoryBarrier = {};
           //  //  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
           //  //  imageMemoryBarrier.pNext = nullptr;
           //  //  imageMemoryBarrier.srcAccessMask = srcAccessMask;
           //  //  imageMemoryBarrier.dstAccessMask = dstAccessMask;
           //  //  imageMemoryBarrier.oldLayout = oldImageLayout;
           //  //  imageMemoryBarrier.newLayout = newImageLayout;
           //  //  imageMemoryBarrier.image = image;
           //  //  imageMemoryBarrier.subresourceRange = subresourceRange;
           //
           //      //vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memBarier, 0, nullptr, 0, &imgMemBarier);
           //  VkClearColorValue val;
           //  val.uint32[0] = 255;
           //  val.uint32[1] = 0;
           //  val.uint32[2] = 0;
           //  val.uint32[3] = 0;
           //
           //  VkImageLayout imLayout = {};
           //  VkBufferImageCopy buffImgCopy = {};
           ////  buffImgCopy.
           // // vkCmdCopyBufferToImage(commandBuffer, buffer1, imageSrc, imLayout, 1, )
           //
           //  //vkCmdClearColorImage(commandBuffer, imageDst, VK_IMAGE_LAYOUT_GENERAL, &val, 1, &subRange);
           //  //VkBufferImageCopy imgCopy = {};
           //  //imgCopy.
           //  //vkCmdCopyImageToBuffer(commandBuffer, imageDst, VK_IMAGE_LAYOUT_GENERAL, buffer1, 1, &region);
           //  vkCmdBlitImage(commandBuffer, imageSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imageDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regions, VK_FILTER_NEAREST);
           //
           //  //vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memBarier, 0, nullptr, 1, &imgMemBarier);
           //
           //  vkEndCommandBuffer(commandBuffer);
           //
           //  VkSubmitInfo submitInfo = {};
           //  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
           //  submitInfo.pNext = nullptr;
           //  submitInfo.commandBufferCount = 1;
           //  submitInfo.pCommandBuffers = &commandBuffer;
           //  submitInfo.waitSemaphoreCount = submitInfo.signalSemaphoreCount = 0;
           //  submitInfo.pWaitSemaphores = submitInfo.pSignalSemaphores = nullptr;
           //  submitInfo.pWaitDstStageMask = nullptr;
           //
           //  VkQueue queue;
           //  vkGetDeviceQueue(g_device, 0, 0, &queue);
           //  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
           //
           //  vkQueueWaitIdle(queue);
           //
           //  void* mappedReadData = nullptr;
           //  vkMapMemory(g_device, g_deviceMemoryImgDst, 0, VK_WHOLE_SIZE, 0, &mappedReadData);
           //
           //  if (mappedReadData != nullptr)
           //  {
           //      //memcpy(mappedData, (void*)data, imgW * imgH * n2);
           //      stbi_write_png("small.png", imgW/2, imgH/2, 3, mappedReadData, imgW/2  *4);
           //      vkUnmapMemory(g_device, g_deviceMemoryImgDst);
           //  }

         //    
         //    vkDestroyImage(g_device, imageSrc, nullptr);
            if (data)
            {
                free(data);
            }
            // }
            //
            //
            // {//Presentation
                 //vkGetPhysicalDeviceWin32PresentationSupportKHR
                 //vkCreateWin32SurfaceKHR
            const bool supportPresentation = vkGetPhysicalDeviceWin32PresentationSupportKHR(g_physDevice, 0);
            if (!supportPresentation)
            {
                return;
            }



            VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
            surfaceInfo.hwnd = glfwGetWin32Window(m_window);;
            surfaceInfo.hinstance = GetModuleHandle(nullptr);;
            surfaceInfo.flags = 0;
            surfaceInfo.pNext = nullptr;
            surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;

            vkCreateWin32SurfaceKHR(g_vkInstanse, &surfaceInfo, nullptr, &g_surface);

            VkSurfaceCapabilitiesKHR surfCaps = {};
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_physDevice, g_surface, &surfCaps);

            VkBool32 sup;
            vkGetPhysicalDeviceSurfaceSupportKHR(g_physDevice, 0, g_surface, &sup);

            VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
            swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapChainCreateInfo.pNext = nullptr;
            swapChainCreateInfo.flags = 0;
            swapChainCreateInfo.surface = g_surface;
            swapChainCreateInfo.minImageCount = 2;
            swapChainCreateInfo.queueFamilyIndexCount = 1;
            uint32_t index(0);
            swapChainCreateInfo.pQueueFamilyIndices = &index;
            swapChainCreateInfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
            swapChainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            swapChainCreateInfo.imageExtent.width = surfCaps.currentExtent.width;
            swapChainCreateInfo.imageExtent.height = surfCaps.currentExtent.height;
            swapChainCreateInfo.imageArrayLayers = 1;
            swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapChainCreateInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            swapChainCreateInfo.clipped = VK_FALSE;
            swapChainCreateInfo.oldSwapchain = g_swapChain;

            auto ret = vkCreateSwapchainKHR(g_device, &swapChainCreateInfo, nullptr, &g_swapChain);

            if (ret != 0)
            {
                return;
            }

            uint32_t numImages = 0;
            vkGetSwapchainImagesKHR(g_device, g_swapChain, &numImages, nullptr);
            std::vector<VkImage> swapChainImages(numImages);
            vkGetSwapchainImagesKHR(g_device, g_swapChain, &numImages, swapChainImages.data());

            uint32_t nextImage = 110;
            vkAcquireNextImageKHR(g_device, g_swapChain, 0, (VkSemaphore)nullptr, (VkFence)nullptr, &nextImage);

            {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = swapChainImages[nextImage];
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = 0; // TODO
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // TODO

                vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                endSingleTimeCommandBuffer(cmdBuff);
            }

            {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = imageShaderOut;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = 0; // TODO
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT; // TODO

                vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                endSingleTimeCommandBuffer(cmdBuff);
            }

            {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = imageDst;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT; // TODO
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // TODO

                vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                endSingleTimeCommandBuffer(cmdBuff);
            }

            ////////////////// SHADER STUFF

            std::ifstream shader("shaders/simple.spv", std::ios::binary);
            shader.seekg(0, std::ios::end);
            size_t filesize = shader.tellg();
            shader.seekg(0, std::ios::beg);
            std::vector<char> buff(filesize);
            shader.read(buff.data(), filesize);
            shader.close();

            VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
            VkShaderModule shaderModule;
            shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderModuleCreateInfo.pNext = nullptr;
            shaderModuleCreateInfo.flags;
            shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(buff.data());
            shaderModuleCreateInfo.codeSize = buff.size();

            vkCreateShaderModule(g_device, &shaderModuleCreateInfo, nullptr, &shaderModule);

            VkPipelineCache pipelineCache;
            VkPipelineCacheCreateInfo pipelineCacheInfo = {};
            pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            pipelineCacheInfo.pNext = nullptr;
            pipelineCacheInfo.flags = 0;
            pipelineCacheInfo.initialDataSize = 0;
            pipelineCacheInfo.pInitialData = nullptr;
            vkCreatePipelineCache(g_device, &pipelineCacheInfo, nullptr, &pipelineCache);

            VkDescriptorSetLayout descriptorSetLayout_1;
            VkDescriptorSetLayout descriptorSetLayout_2;

            /*
            uint32_t              binding;
            VkDescriptorType      descriptorType;
            uint32_t              descriptorCount;
            VkShaderStageFlags    stageFlags;
            const VkSampler*      pImmutableSamplers;
            */
            VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_1[3] = {
                //            { 0, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_ALL , nullptr },
                            { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE , 1, VK_SHADER_STAGE_ALL, nullptr },
                            //            { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_ALL , nullptr }
            };

            VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_2[2]
                = {
                    { 0, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_ALL , nullptr },
                    { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE , 1, VK_SHADER_STAGE_ALL, nullptr }
            };

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo_1 = {};
            descriptorSetLayoutCreateInfo_1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo_1.pNext = nullptr;
            descriptorSetLayoutCreateInfo_1.flags = 0;
            descriptorSetLayoutCreateInfo_1.bindingCount = 1;
            descriptorSetLayoutCreateInfo_1.pBindings = descriptorSetLayoutBinding_1;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo_2 = {};
            descriptorSetLayoutCreateInfo_2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo_2.pNext = nullptr;
            descriptorSetLayoutCreateInfo_2.flags = 0;
            descriptorSetLayoutCreateInfo_2.bindingCount = 2;
            descriptorSetLayoutCreateInfo_2.pBindings = descriptorSetLayoutBinding_2;

            vkCreateDescriptorSetLayout(g_device, &descriptorSetLayoutCreateInfo_1, nullptr, &descriptorSetLayout_1);
            vkCreateDescriptorSetLayout(g_device, &descriptorSetLayoutCreateInfo_2, nullptr, &descriptorSetLayout_2);

            VkPipelineLayout pipelineLayout;
            VkPipelineLayoutCreateInfo pipelineLayoutCreationInfo = {};
            pipelineLayoutCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreationInfo.pNext = nullptr;
            pipelineLayoutCreationInfo.flags = 0;
            pipelineLayoutCreationInfo.pushConstantRangeCount = 0;
            pipelineLayoutCreationInfo.pPushConstantRanges = nullptr;
            pipelineLayoutCreationInfo.setLayoutCount = 2;

            VkDescriptorSetLayout layouts[] = { descriptorSetLayout_2, descriptorSetLayout_1 };
            pipelineLayoutCreationInfo.pSetLayouts = layouts;
            vkCreatePipelineLayout(g_device, &pipelineLayoutCreationInfo, nullptr, &pipelineLayout);


            VkGraphicsPipelineCreateInfo grpCreateInfo;
            //grpCreateInfo.
            /*typedef struct VkSpecializationMapEntry {
                uint32_t    constantID;
                uint32_t    offset;
                size_t      size;
            } */
            VkSpecializationMapEntry entries[4];
            entries[0].constantID = 0;
            entries[0].offset = 0;
            entries[0].size = 1;

            for (int i = 1; i < 4; ++i)
            {
                entries[i].constantID = i;
                entries[i].offset = 4 * i;
                entries[i].size = 4;
            }

            VkComputePipelineCreateInfo computePipelineCreateInfo = {};
            /*
                uint32_t                           mapEntryCount;
        const VkSpecializationMapEntry*    pMapEntries;
        size_t                             dataSize;
        const void*                        pData;
            */
            unsigned char specializationData[30] = {};
            unsigned char* startAddr = specializationData;
            unsigned char* realAddr;
            auto remainder = (unsigned long long)startAddr % 4;
            realAddr = startAddr + remainder;
            bool* bVal = reinterpret_cast<bool*>(&realAddr[0]);
            *bVal = true;


            float* fVal = reinterpret_cast<float*>(&realAddr[4]);
            *fVal = 0.5f;


            float* fValB = reinterpret_cast<float*>(&realAddr[8]);
            *fValB = 0.0;


            float* fValG = reinterpret_cast<float*>(&realAddr[12]);
            *fValG = 1.0;


            VkSpecializationInfo shaderSpecializationInfo = {};
            shaderSpecializationInfo.mapEntryCount = 4;
            shaderSpecializationInfo.pMapEntries = entries;
            shaderSpecializationInfo.dataSize = 16;
            shaderSpecializationInfo.pData = (void*)realAddr;
            // shaderSpecializationInfo.

            computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            computePipelineCreateInfo.pNext = nullptr;
            computePipelineCreateInfo.flags = 0;
            computePipelineCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            computePipelineCreateInfo.stage.pNext = nullptr;
            computePipelineCreateInfo.stage.flags = 0;
            computePipelineCreateInfo.stage.module = shaderModule;
            computePipelineCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            computePipelineCreateInfo.stage.pName = "main";
            computePipelineCreateInfo.stage.pSpecializationInfo = &shaderSpecializationInfo;
            computePipelineCreateInfo.layout = pipelineLayout;

            VkPipeline computePipeline;
            vkCreateComputePipelines(g_device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &computePipeline);

            VkDescriptorPoolSize descriptorPoolSizes[] = {
                { VK_DESCRIPTOR_TYPE_SAMPLER , 10 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE , 10 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE , 10 },
            };
            VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
            VkDescriptorPool descriptorPool;
            descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptorPoolCreateInfo.pNext = nullptr;
            descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            descriptorPoolCreateInfo.maxSets = 128;
            descriptorPoolCreateInfo.poolSizeCount = 3;
            descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;

            vkCreateDescriptorPool(g_device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);

            VkDescriptorSet descriptorSet[10] = {};
            VkDescriptorSetAllocateInfo descriptorSetAllocationInfo = {};
            descriptorSetAllocationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetAllocationInfo.pNext = nullptr;
            descriptorSetAllocationInfo.descriptorPool = descriptorPool;
            descriptorSetAllocationInfo.descriptorSetCount = 2;
            descriptorSetAllocationInfo.pSetLayouts = layouts;
            vkAllocateDescriptorSets(g_device, &descriptorSetAllocationInfo, descriptorSet);

            VkDescriptorImageInfo descrImageInfo = {};
            descrImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            descrImageInfo.imageView = imgViewDst;
            descrImageInfo.sampler = imageSampler;

            VkDescriptorImageInfo descrImageInfo_2 = {};
            descrImageInfo_2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descrImageInfo_2.imageView = imgViewSrc;
            descrImageInfo_2.sampler = imageSampler;

            VkWriteDescriptorSet writeDescriptorSet[3];
            writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet[0].pNext = nullptr;
            writeDescriptorSet[0].dstSet = descriptorSet[1];
            writeDescriptorSet[0].dstBinding = 1;
            writeDescriptorSet[0].dstArrayElement = 0;
            writeDescriptorSet[0].descriptorCount = 1;
            writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writeDescriptorSet[0].pTexelBufferView = nullptr;
            writeDescriptorSet[0].pBufferInfo = nullptr;
            writeDescriptorSet[0].pImageInfo = &descrImageInfo;

            writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet[1].pNext = nullptr;
            writeDescriptorSet[1].dstSet = descriptorSet[0];
            writeDescriptorSet[1].dstBinding = 0;
            writeDescriptorSet[1].dstArrayElement = 0;
            writeDescriptorSet[1].descriptorCount = 1;
            writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            writeDescriptorSet[1].pTexelBufferView = nullptr;
            writeDescriptorSet[1].pBufferInfo = nullptr;
            writeDescriptorSet[1].pImageInfo = &descrImageInfo_2;

            writeDescriptorSet[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet[2].pNext = nullptr;
            writeDescriptorSet[2].dstSet = descriptorSet[0];
            writeDescriptorSet[2].dstBinding = 1;
            writeDescriptorSet[2].dstArrayElement = 0;
            writeDescriptorSet[2].descriptorCount = 1;
            writeDescriptorSet[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            writeDescriptorSet[2].pTexelBufferView = nullptr;
            writeDescriptorSet[2].pBufferInfo = nullptr;
            writeDescriptorSet[2].pImageInfo = &descrImageInfo_2;

            /*
            writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet[1].pNext = nullptr;
            writeDescriptorSet[1].dstSet = descriptorSet[1];
            writeDescriptorSet[1].dstBinding = 1;
            writeDescriptorSet[1].dstArrayElement = 0;
            writeDescriptorSet[1].descriptorCount = 1;
            writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            writeDescriptorSet[1].pTexelBufferView = nullptr;
            writeDescriptorSet[1].pBufferInfo = nullptr;
            writeDescriptorSet[1].pImageInfo = &descrImageInfo;

            writeDescriptorSet[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet[2].pNext = nullptr;
            writeDescriptorSet[2].dstSet = descriptorSet[1];
            writeDescriptorSet[2].dstBinding = 2;
            writeDescriptorSet[2].dstArrayElement = 0;
            writeDescriptorSet[2].descriptorCount = 1;
            writeDescriptorSet[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writeDescriptorSet[2].pTexelBufferView = nullptr;
            writeDescriptorSet[2].pBufferInfo = nullptr;
            writeDescriptorSet[2].pImageInfo = &descrImageInfo_2;
            */
            //writeDescriptorSet[0].pImageInfo->
            vkUpdateDescriptorSets(g_device, 3, writeDescriptorSet, 0, nullptr);

            {
                auto cmdBuff = createSingleTimeCommandBuffer();
                vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
                vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 2, descriptorSet, 0, nullptr);
                vkCmdDispatch(cmdBuff, imgW / 2, imgH / 2, 1);
                endSingleTimeCommandBuffer(cmdBuff);
            }

            ///////////////////// SHADER STUFF END

            {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = imageShaderOut;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; // TODO
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; // TODO

                vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                endSingleTimeCommandBuffer(cmdBuff);
            }

            {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkImageBlit regions = {};
                regions.srcOffsets[0] = VkOffset3D{ 0,0, 0 };
                regions.srcOffsets[1] = VkOffset3D{ imgW / 2,imgH / 2, 1 };
                regions.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                regions.srcSubresource.baseArrayLayer = 0;
                regions.srcSubresource.mipLevel = 0;
                regions.srcSubresource.layerCount = 1;

                regions.dstOffsets[0] = VkOffset3D{ 0,0, 1 };
                regions.dstOffsets[1] = VkOffset3D{ (int)surfCaps.currentExtent.width ,(int)surfCaps.currentExtent.height, 0 };
                regions.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                regions.dstSubresource.baseArrayLayer = 0;
                regions.dstSubresource.mipLevel = 0;
                regions.dstSubresource.layerCount = 1;

                vkCmdBlitImage(cmdBuff, imageShaderOut, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChainImages[nextImage], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regions, VK_FILTER_LINEAR);
                endSingleTimeCommandBuffer(cmdBuff);
            }

            {
                auto cmdBuff = createSingleTimeCommandBuffer();

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = swapChainImages[nextImage];
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // TODO
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT; // TODO

                vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                endSingleTimeCommandBuffer(cmdBuff);
            }

            vkDestroyDescriptorSetLayout(g_device, descriptorSetLayout_1, nullptr);
            vkDestroyDescriptorSetLayout(g_device, descriptorSetLayout_2, nullptr);
            vkFreeDescriptorSets(g_device, descriptorPool, 1, descriptorSet);

            vkDestroyDescriptorPool(g_device, descriptorPool, nullptr);

            vkDestroyShaderModule(g_device, shaderModule, nullptr);

            /*
            vkCmdBindDescriptorSets
            vkCmdPushConstants
            vkCreateSampler
            vkDestroySampler
            */
            vkDestroyPipeline(g_device, computePipeline, nullptr);
            vkDestroyPipelineCache(g_device, pipelineCache, nullptr);
            vkDestroyPipelineLayout(g_device, pipelineLayout, nullptr);
            VkQueue queue;
            vkGetDeviceQueue(g_device, 0, 0, &queue);
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pNext = nullptr;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &g_swapChain;
            presentInfo.waitSemaphoreCount = 0;
            presentInfo.pWaitSemaphores = nullptr;
            presentInfo.pImageIndices = &nextImage;
            vkQueuePresentKHR(queue, &presentInfo);


            vkDestroyImageView(g_device, imgViewDst, nullptr);
            vkDestroyImageView(g_device, imgViewSrc, nullptr);
            vkDestroySampler(g_device, imageSampler, nullptr);
            vkDestroyImage(g_device, imageDst, nullptr);
            vkDestroyImage(g_device, imageShaderOut, nullptr);
            vkDestroyImage(g_device, imageSrc, nullptr);
            vkDestroyBuffer(g_device, stagingBuffer, nullptr);
            vkFreeMemory(g_device, stagingBuffMem, nullptr);
            vkDestroyCommandPool(g_device, commandPool, nullptr);
        }

        ///////----------------------------
    });

    return ret;
}
