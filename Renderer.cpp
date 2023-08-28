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

    VkDeviceSize uniformBufferSize = sizeof(uint32_t);

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = uniformBufferSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(mDevice->vkDevice(), &bufferCreateInfo, nullptr, &mUniformBuffer);
    printf("Create buffer: %i\n", result);

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(mDevice->vkDevice(), mUniformBuffer, &memoryRequirements);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(mDevice->vkPhysicalDevice(), &memoryProperties);

    int memoryTypeIndex = -1;
    int propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for(int i=0; i<memoryProperties.memoryTypeCount; i++) {
        if(memoryRequirements.memoryTypeBits & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
            memoryTypeIndex = i;
            break;
        }
    }

    printf("memory type index: %i\n", memoryTypeIndex);
    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = uniformBufferSize;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    result = vkAllocateMemory(mDevice->vkDevice(), &allocateInfo, nullptr, &mUniformDeviceMemory);
    printf("Allocate memory: %i\n", result);

    result = vkBindBufferMemory(mDevice->vkDevice(), mUniformBuffer, mUniformDeviceMemory, 0);
    printf("Bind memory: %i\n", result);

    result = vkMapMemory(mDevice->vkDevice(), mUniformDeviceMemory, 0, uniformBufferSize, 0, &mUniformMap);
    printf("Map memory: %i\n", result);

    VkDescriptorPoolSize descriptorPoolSize{};
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = 1;
    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;

    result = vkCreateDescriptorPool(mDevice->vkDevice(), &descriptorPoolCreateInfo, nullptr, &mDescriptorPool);
    printf("Create descriptor pool: %i\n", result);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = mDescriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    VkDescriptorSetLayout descriptorSetLayout = mPipeline->vkDescriptorSetLayout();
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

    result = vkAllocateDescriptorSets(mDevice->vkDevice(), &descriptorSetAllocateInfo, &mDescriptorSet);
    printf("Allocate descriptor set: %i\n", result);

    VkDescriptorBufferInfo descriptorBufferInfo{};
    descriptorBufferInfo.buffer = mUniformBuffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = uniformBufferSize;

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = mDescriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

    vkUpdateDescriptorSets(mDevice->vkDevice(), 1, &writeDescriptorSet, 0, nullptr);
}

void Renderer::renderFrame(int frame)
{
    *((uint32_t*)mUniformMap) = frame;

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
    vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline->vkPipelineLayout(), 0, 1, &mDescriptorSet, 0, nullptr);
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
