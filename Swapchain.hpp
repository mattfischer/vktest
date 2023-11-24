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
    Swapchain(Device &device, VkFormat format, HINSTANCE hInstance, HWND hWnd);

    VkSwapchainKHR vkSwapchain() { return mSwapchain; }
    std::vector<VkImage> &images() { return mSwapchainImages; }
    std::vector<VkImageView> &imageViews() { return mSwapchainImageViews; }
    VkSemaphore imageAvailableSemaphore() { return mImageAvailableSemaphore; }
    VkSemaphore renderFinishedSemaphore() { return mRenderFinishedSemaphore; }

    uint32_t width();
    uint32_t height();

private:
    Device &mDevice;

    VkSurfaceKHR mSurface;
    VkSwapchainKHR mSwapchain;
    VkSemaphore mImageAvailableSemaphore;
    VkSemaphore mRenderFinishedSemaphore;
    std::vector<VkImage> mSwapchainImages;
    std::vector<VkImageView> mSwapchainImageViews;
};

#endif