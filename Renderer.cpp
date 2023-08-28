#include "Renderer.hpp"

static const int kWidth = 640;
static const int kHeight = 480;

Renderer::Renderer(HINSTANCE hInstance, HWND hWnd)
{
    mDevice = std::make_unique<Device>();
    mPipeline = std::make_unique<Pipeline>(*mDevice);
    mSwapchain = std::make_unique<Swapchain>(*mDevice, *mPipeline, hInstance, hWnd);

    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = mDevice->graphicsIndex();

    VkResult result = vkCreateCommandPool(mDevice->vkDevice(), &commandPoolCreateInfo, nullptr, &mCommandPool);
    printf("Create command pool: %i\n", result);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = mCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(mDevice->vkDevice(), &commandBufferAllocateInfo, &mCommandBuffer);
    printf("Allocate command buffers: %i\n", result);
}

void Renderer::renderFrame(int frame)
{
    *((uint32_t*)mPipeline->uniformMap()) = frame;

    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult result;

    result = vkBeginCommandBuffer(mCommandBuffer, &commandBufferBeginInfo);

    uint32_t imageIndex;
    result = vkAcquireNextImageKHR(mDevice->vkDevice(), mSwapchain->vkSwapchain(), UINT64_MAX, mSwapchain->imageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);

    VkClearValue clearValue{{{0.0f, 0.0f, 0.0f, 1.0f}}};
    
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = mPipeline->vkRenderPass();
    renderPassBeginInfo.framebuffer = mSwapchain->framebuffers()[imageIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = {kWidth, kHeight};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(mCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline->vkPipeline());
    VkDescriptorSet descriptorSet = mPipeline->vkDescriptorSet();
    vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline->vkPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDraw(mCommandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(mCommandBuffer);

    result = vkEndCommandBuffer(mCommandBuffer);

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    VkSemaphore imageAvailableSemaphore = mSwapchain->imageAvailableSemaphore();
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    VkSemaphore renderFinishedSemaphore = mSwapchain->renderFinishedSemaphore();
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;
    result = vkQueueSubmit(mDevice->vkQueue(), 1, &submitInfo, VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    VkSwapchainKHR swapchain = mSwapchain->vkSwapchain();
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(mDevice->vkQueue(), &presentInfo);
}
