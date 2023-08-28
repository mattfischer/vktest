#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include "Device.hpp"
#include "Pipeline.hpp"

#include <windows.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include <vector>

class Swapchain {
public:
    Swapchain(Device &device, Pipeline &pipeline, HINSTANCE hInstance, HWND hWnd);

    VkSwapchainKHR vkSwapchain() { return mSwapchain; }
    std::vector<VkFramebuffer> &framebuffers() { return mFramebuffers; }
    VkSemaphore imageAvailableSemaphore() { return mImageAvailableSemaphore; }
    VkSemaphore renderFinishedSemaphore() { return mRenderFinishedSemaphore; }

private:
    Device &mDevice;

    VkSurfaceKHR mSurface;
    VkSwapchainKHR mSwapchain;
    VkSemaphore mImageAvailableSemaphore;
    VkSemaphore mRenderFinishedSemaphore;
    std::vector<VkImage> mSwapchainImages;
    std::vector<VkFramebuffer> mFramebuffers;
};

#endif