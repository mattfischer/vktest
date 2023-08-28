#include "Device.hpp"

#include <vector>

#include <stdio.h>

Device::Device()
{
    const char *instExtensions[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };

    VkInstanceCreateInfo instCreateInfo{};
    instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instCreateInfo.enabledExtensionCount = 2;
    instCreateInfo.ppEnabledExtensionNames = instExtensions;

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
    
    for(int32_t i=0; i<queueFamilyPropertyCount; i++) {
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            mGraphicsIndex = i;
        }
    }

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = mGraphicsIndex;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char *deviceExtensions[] = {
        "VK_KHR_swapchain"
    };

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    result = vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice);
    printf("Create device: %i\n", result);

    vkGetDeviceQueue(mDevice, mGraphicsIndex, 0, &mQueue);
}