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
    int graphicsIndex() { return mGraphicsIndex; }

private:
    VkInstance mInst;
    int32_t mGraphicsIndex;
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice;
    VkQueue mQueue;
};

#endif