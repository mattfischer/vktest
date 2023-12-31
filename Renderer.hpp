#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "Device.hpp"
#include "Pipeline.hpp"
#include "Swapchain.hpp"

#include <windows.h>

#include <memory>

class Renderer {
public:
    Renderer(HINSTANCE hInstance, HWND hWnd);

    void renderFrame(int frame);

    struct Uniform {
        float transformation[12];
    };

private:
    void allocateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlagBits propertyFlags, VkBuffer *buffer, VkDeviceMemory *deviceMemory);
    uint32_t findMemoryTypeIndex(VkMemoryRequirements memoryRequirements, VkMemoryPropertyFlagBits propertyFlags);

    VkCommandBuffer mCommandBuffer;
    VkCommandPool mCommandPool;

    VkBuffer mUniformBuffer;
    VkDeviceMemory mUniformDeviceMemory;
    void *mUniformMap;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSet mDescriptorSet;

    VkBuffer mVertexBuffer;
    VkDeviceMemory mVertexDeviceMemory;

    VkImage mTextureImage;
    VkDeviceMemory mTextureMemory;
    VkImageView mTextureImageView;
    VkSampler mSampler;

    VkImage mDepthImage;
    VkDeviceMemory mDepthMemory;
    VkImageView mDepthView;

    std::unique_ptr<Device> mDevice;
    std::unique_ptr<Pipeline> mPipeline;
    std::unique_ptr<Swapchain> mSwapchain;
};

#endif