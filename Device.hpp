#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <vulkan/vulkan_core.h>

class Device {
public:
    Device();

    VkInstance vkInstance() { return mInst; }
    VkDevice vkDevice() { return mDevice; }
    VkPhysicalDevice vkPhysicalDevice() { return mPhysicalDevice; }
    VkQueue vkQueue() { return mQueue; }
    uint32_t graphicsIndex() { return mGraphicsIndex; }

private:
    VkInstance mInst;
    uint32_t mGraphicsIndex;
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice;
    VkQueue mQueue;
};

#endif