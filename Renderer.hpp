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

private:
    VkCommandBuffer mCommandBuffer;
    VkCommandPool mCommandPool;

    VkBuffer mUniformBuffer;
    VkDeviceMemory mUniformDeviceMemory;
    void *mUniformMap;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSet mDescriptorSet;

    VkBuffer mVertexBuffer;
    VkDeviceMemory mVertexDeviceMemory;

    std::unique_ptr<Device> mDevice;
    std::unique_ptr<Pipeline> mPipeline;
    std::unique_ptr<Swapchain> mSwapchain;
};

#endif