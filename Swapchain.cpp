#include "Swapchain.hpp"

static const int kWidth = 640;
static const int kHeight = 640;

Swapchain::Swapchain(Device &device, Pipeline &pipeline, HINSTANCE hInstance, HWND hWnd)
: mDevice(device)
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = hInstance;
    surfaceCreateInfo.hwnd = hWnd;

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

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = mSurface;
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
    swapchainCreateInfo.imageFormat = pipeline.format();
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainCreateInfo.imageExtent = {kWidth, kHeight};
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(mDevice.vkDevice(), &swapchainCreateInfo, nullptr, &mSwapchain);
    printf("Create swapchain: %i\n", result);

    uint32_t numSwapchainImages;
    result = vkGetSwapchainImagesKHR(mDevice.vkDevice(), mSwapchain, &numSwapchainImages, nullptr);
    mSwapchainImages.resize(numSwapchainImages);
    result = vkGetSwapchainImagesKHR(mDevice.vkDevice(), mSwapchain, &numSwapchainImages, &mSwapchainImages[0]);
    
    mFramebuffers.resize(numSwapchainImages);
    for(int i=0; i<numSwapchainImages; i++) {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.image = mSwapchainImages[i];
        imageViewCreateInfo.format = pipeline.format();
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        result = vkCreateImageView(mDevice.vkDevice(), &imageViewCreateInfo, nullptr, &imageView);
        printf("Create image view: %i (%i)\n", result, i);

        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = pipeline.vkRenderPass();
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &imageView;
        framebufferCreateInfo.width = kWidth;
        framebufferCreateInfo.height = kHeight;
        framebufferCreateInfo.layers = 1;

        result = vkCreateFramebuffer(mDevice.vkDevice(), &framebufferCreateInfo, nullptr, &mFramebuffers[i]);
        printf("Create framebuffer: %i (%i)\n", result, i);
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