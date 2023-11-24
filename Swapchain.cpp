#include "Swapchain.hpp"

static const int kWidth = 640;
static const int kHeight = 640;

Swapchain::Swapchain(Device &device, VkFormat format, HINSTANCE hInstance, HWND hWnd)
: mDevice(device)
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = hInstance,
        .hwnd = hWnd
    };

    VkResult result = vkCreateWin32SurfaceKHR(mDevice.vkInstance(), &surfaceCreateInfo, nullptr, &mSurface);
    printf("Create Win32 Surface: %i\n", result);

    VkBool32 surfaceSupported;
    vkGetPhysicalDeviceSurfaceSupportKHR(mDevice.vkPhysicalDevice(), mDevice.graphicsIndex(), mSurface, &surfaceSupported);
    printf("Surface supported: %s\n", surfaceSupported ? "true" : "false");

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mDevice.vkPhysicalDevice(), mSurface, &surfaceCapabilities);

    uint32_t numSurfaceFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mDevice.vkPhysicalDevice(), mSurface, &numSurfaceFormats, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(numSurfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(mDevice.vkPhysicalDevice(), mSurface, &numSurfaceFormats, &surfaceFormats[0]);
    for(int i=0; i<numSurfaceFormats; i++) {
        printf("Surface format: %i %i\n", surfaceFormats[i].format, surfaceFormats[i].colorSpace);
    }

    uint32_t numPresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(mDevice.vkPhysicalDevice(), mSurface, &numPresentModes, nullptr);
    std::vector<VkPresentModeKHR> presentModes(numPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(mDevice.vkPhysicalDevice(), mSurface, &numPresentModes, &presentModes[0]);
    for(int i=0; i<numPresentModes; i++) {
        printf("Present mode: %i\n", presentModes[i]);
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = mSurface,
        .minImageCount = surfaceCapabilities.minImageCount + 1,
        .imageFormat = format,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = {kWidth, kHeight},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    result = vkCreateSwapchainKHR(mDevice.vkDevice(), &swapchainCreateInfo, nullptr, &mSwapchain);
    printf("Create swapchain: %i\n", result);

    uint32_t numSwapchainImages;
    result = vkGetSwapchainImagesKHR(mDevice.vkDevice(), mSwapchain, &numSwapchainImages, nullptr);
    mSwapchainImages.resize(numSwapchainImages);
    result = vkGetSwapchainImagesKHR(mDevice.vkDevice(), mSwapchain, &numSwapchainImages, &mSwapchainImages[0]);
    
    mSwapchainImageViews.resize(numSwapchainImages);
    for(int i=0; i<numSwapchainImages; i++) {
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = mSwapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components = { 
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        result = vkCreateImageView(mDevice.vkDevice(), &imageViewCreateInfo, nullptr, &mSwapchainImageViews[i]);
        printf("Create image view: %i (%i)\n", result, i);
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    result = vkCreateSemaphore(mDevice.vkDevice(), &semaphoreCreateInfo, nullptr, &mImageAvailableSemaphore);
    printf("Create semaphore: %i\n", result);

    result = vkCreateSemaphore(mDevice.vkDevice(), &semaphoreCreateInfo, nullptr, &mRenderFinishedSemaphore);
    printf("Create semaphore: %i\n", result);
}

uint32_t Swapchain::width()
{
    return kWidth;
}

uint32_t Swapchain::height()
{
    return kHeight;
}