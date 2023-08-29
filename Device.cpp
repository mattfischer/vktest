#include "Device.hpp"

#include <vector>

#include <stdio.h>

Device::Device()
{
    const char *instExtensions[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };

    VkInstanceCreateInfo instCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = instExtensions
    };

    VkResult result;
    result = vkCreateInstance(&instCreateInfo, nullptr, &mInst);
    printf("create instance: %i\n", result);

    uint32_t numPhysicalDevices = 1;
    result = vkEnumeratePhysicalDevices(mInst, &numPhysicalDevices, &mPhysicalDevice);

    uint32_t queueFamilyPropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyPropertyCount, nullptr);
    printf("queueFamilyPropertyCount: %i\n", queueFamilyPropertyCount);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyPropertyCount, &queueFamilyProperties[0]);
    
    for(uint32_t i=0; i<queueFamilyPropertyCount; i++) {
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            mGraphicsIndex = i;
        }
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = mGraphicsIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    const char *deviceExtensions[] = {
        "VK_KHR_swapchain"
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = deviceExtensions
    };

    result = vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice);
    printf("Create device: %i\n", result);

    vkGetDeviceQueue(mDevice, mGraphicsIndex, 0, &mQueue);
}