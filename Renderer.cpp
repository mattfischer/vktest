#include "Renderer.hpp"

static const int kNumVertices = 4;
static const Pipeline::Vertex vertices[] = {
    { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f },
    { -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f },
    {  0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f },
    {  0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f }
};

Renderer::Renderer(HINSTANCE hInstance, HWND hWnd)
{
    mDevice = std::make_unique<Device>();
    mPipeline = std::make_unique<Pipeline>(*mDevice);
    mSwapchain = std::make_unique<Swapchain>(*mDevice, *mPipeline, hInstance, hWnd);

    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = mDevice->graphicsIndex()
    };

    VkResult result = vkCreateCommandPool(mDevice->vkDevice(), &commandPoolCreateInfo, nullptr, &mCommandPool);
    printf("Create command pool: %i\n", result);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    result = vkAllocateCommandBuffers(mDevice->vkDevice(), &commandBufferAllocateInfo, &mCommandBuffer);
    printf("Allocate command buffers: %i\n", result);

    VkDeviceSize uniformBufferSize = sizeof(Uniform);

    VkMemoryPropertyFlagBits propertyFlags = (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    allocateBuffer(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, propertyFlags, &mUniformBuffer, &mUniformDeviceMemory);

    result = vkMapMemory(mDevice->vkDevice(), mUniformDeviceMemory, 0, uniformBufferSize, 0, &mUniformMap);
    printf("Map memory: %i\n", result);

    VkDescriptorPoolSize descriptorPoolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &descriptorPoolSize
    };

    result = vkCreateDescriptorPool(mDevice->vkDevice(), &descriptorPoolCreateInfo, nullptr, &mDescriptorPool);
    printf("Create descriptor pool: %i\n", result);

    VkDescriptorSetLayout descriptorSetLayout = mPipeline->vkDescriptorSetLayout();
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout
    };

    result = vkAllocateDescriptorSets(mDevice->vkDevice(), &descriptorSetAllocateInfo, &mDescriptorSet);
    printf("Allocate descriptor set: %i\n", result);

    VkDescriptorBufferInfo descriptorBufferInfo = {
        .buffer = mUniformBuffer,
        .offset = 0,
        .range = uniformBufferSize
    };

    VkWriteDescriptorSet writeDescriptorSet = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &descriptorBufferInfo
    };

    vkUpdateDescriptorSets(mDevice->vkDevice(), 1, &writeDescriptorSet, 0, nullptr);

    VkDeviceSize vertexBufferSize = sizeof(Pipeline::Vertex) * kNumVertices;
    allocateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, propertyFlags, &mVertexBuffer, &mVertexDeviceMemory);

    void *vertexMap;
    result = vkMapMemory(mDevice->vkDevice(), mVertexDeviceMemory, 0, vertexBufferSize, 0, &vertexMap);
    printf("Map memory: %i\n", result);

    memcpy(vertexMap, vertices, vertexBufferSize);

    vkUnmapMemory(mDevice->vkDevice(), mVertexDeviceMemory);
}

void Renderer::renderFrame(int frame)
{
    Uniform *uniform = (Uniform*)mUniformMap;
    float theta = 2 * 3.14 * frame / 100;
    float sinTheta = sinf(theta);
    float cosTheta = cosf(theta);

    float *t = uniform->transformation;
    t[0] = cosTheta; t[1] = -sinTheta; t[2]  = 0.0f; t[3]  = 0.0f;
    t[4] = sinTheta; t[5] =  cosTheta; t[6]  = 0.0f; t[7]  = 0.0f;
    t[8] = 0.0f;     t[9] =  0.0f;     t[10] = 1.0f; t[11] = 0.0f;

    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult result;

    result = vkBeginCommandBuffer(mCommandBuffer, &commandBufferBeginInfo);

    uint32_t imageIndex;
    result = vkAcquireNextImageKHR(mDevice->vkDevice(), mSwapchain->vkSwapchain(), UINT64_MAX, mSwapchain->imageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);

    VkClearValue clearValue{{{0.0f, 0.0f, 0.0f, 1.0f}}};
    
    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mPipeline->vkRenderPass(),
        .framebuffer = mSwapchain->framebuffers()[imageIndex],
        .renderArea = {
            .offset = { 0, 0 },
            .extent = { mSwapchain->width(), mSwapchain->height() },
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };

    vkCmdBeginRenderPass(mCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline->vkPipeline());
    vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline->vkPipelineLayout(), 0, 1, &mDescriptorSet, 0, nullptr);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)mSwapchain->width(),
        .height = (float)mSwapchain->height(),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = { mSwapchain->width(), mSwapchain->height() }
    };
    vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { mVertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdDraw(mCommandBuffer, 4, 1, 0, 0);
    vkCmdEndRenderPass(mCommandBuffer);

    result = vkEndCommandBuffer(mCommandBuffer);

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore imageAvailableSemaphore = mSwapchain->imageAvailableSemaphore();
    VkSemaphore renderFinishedSemaphore = mSwapchain->renderFinishedSemaphore();

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAvailableSemaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &mCommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinishedSemaphore
    };

    result = vkQueueSubmit(mDevice->vkQueue(), 1, &submitInfo, VK_NULL_HANDLE);

    VkSwapchainKHR swapchain = mSwapchain->vkSwapchain();
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex
    };

    result = vkQueuePresentKHR(mDevice->vkQueue(), &presentInfo);
}

void Renderer::allocateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlagBits propertyFlags, VkBuffer *buffer, VkDeviceMemory *deviceMemory)
{
    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkResult result = vkCreateBuffer(mDevice->vkDevice(), &bufferCreateInfo, nullptr, buffer);
    printf("Create buffer: %i\n", result);

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(mDevice->vkDevice(), *buffer, &memoryRequirements);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(mDevice->vkPhysicalDevice(), &memoryProperties);

    uint32_t memoryTypeIndex = UINT32_MAX;
    for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++) {
        if(memoryRequirements.memoryTypeBits & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
            memoryTypeIndex = i;
            break;
        }
    }

    printf("memory type index: %i\n", memoryTypeIndex);
    VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = size,
        .memoryTypeIndex = memoryTypeIndex
    };

    result = vkAllocateMemory(mDevice->vkDevice(), &allocateInfo, nullptr, deviceMemory);
    printf("Allocate memory: %i\n", result);

    result = vkBindBufferMemory(mDevice->vkDevice(), *buffer, *deviceMemory, 0);
    printf("Bind memory: %i\n", result);
}