#include "Renderer.hpp"

static const int kNumVertices = 4;
static const Pipeline::Vertex vertices[] = {
    { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    { -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },
    {  0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    {  0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f }
};

Renderer::Renderer(HINSTANCE hInstance, HWND hWnd)
{
    mDevice = std::make_unique<Device>();
    mPipeline = std::make_unique<Pipeline>(*mDevice);

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

    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;

    FILE *file = fopen("bricks.bmp", "rb");
    fread(&bfh, sizeof(bfh), 1, file);
    fread(&bih, sizeof(bih), 1, file);
    fseek(file, bfh.bfOffBits, SEEK_SET);

    uint32_t textureWidth = bih.biWidth;
    uint32_t textureHeight = bih.biHeight;

    unsigned int fileSize = textureWidth * textureHeight * 3;
    std::vector<unsigned char> bits(fileSize);
    fread(&bits[0], fileSize, 1, file);
    fclose(file);

    VkDeviceSize textureSize = textureWidth * textureHeight * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    allocateBuffer(textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, propertyFlags, &stagingBuffer, &stagingMemory);

    uint8_t *stagingMap;
    result = vkMapMemory(mDevice->vkDevice(), stagingMemory, 0, textureSize, 0, (void**)&stagingMap);
    printf("Map memory: %i\n", result);

    for (int i = 0; i < textureWidth * textureHeight; i++) {
        stagingMap[i * 4 + 0] = bits[i * 3 + 2];
        stagingMap[i * 4 + 1] = bits[i * 3 + 1];
        stagingMap[i * 4 + 2] = bits[i * 3 + 0];
        stagingMap[i * 4 + 3] = 0xff;
    }

    vkUnmapMemory(mDevice->vkDevice(), stagingMemory);

    VkImageCreateInfo textureCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .extent = {textureWidth, textureHeight},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    result = vkCreateImage(mDevice->vkDevice(), &textureCreateInfo, nullptr, &mTextureImage);
    printf("Create image: %i\n", result);

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(mDevice->vkDevice(), mTextureImage, &memoryRequirements);

    uint32_t memoryTypeIndex = findMemoryTypeIndex(memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = textureSize,
        .memoryTypeIndex = memoryTypeIndex
    };

    result = vkAllocateMemory(mDevice->vkDevice(), &allocInfo, nullptr, &mTextureMemory);
    printf("Allocate memory: %i\n", result);

    vkBindImageMemory(mDevice->vkDevice(), mTextureImage, mTextureMemory, 0);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo2 = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(mDevice->vkDevice(), &commandBufferAllocateInfo2, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = mTextureImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
    };

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            textureWidth,
            textureHeight,
            1
        }
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        stagingBuffer,
        mTextureImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    VkImageMemoryBarrier barrier2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = mTextureImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
    };

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier2
    );

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    vkQueueSubmit(mDevice->vkQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mDevice->vkQueue());

    vkFreeCommandBuffers(mDevice->vkDevice(), mCommandPool, 1, &commandBuffer);
    vkDestroyBuffer(mDevice->vkDevice(), stagingBuffer, nullptr);
    vkFreeMemory(mDevice->vkDevice(), stagingMemory, nullptr);

    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = mTextureImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    result = vkCreateImageView(mDevice->vkDevice(), &imageViewCreateInfo, nullptr, &mTextureImageView);

    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK
    };
    result = vkCreateSampler(mDevice->vkDevice(), &samplerInfo, nullptr, &mSampler);

    VkDescriptorPoolSize descriptorPoolSizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1
        }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 2,
        .pPoolSizes = descriptorPoolSizes
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

    VkDescriptorImageInfo descriptorImageInfo = {
        .sampler = mSampler,
        .imageView = mTextureImageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet writeDescriptorSets[] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &descriptorBufferInfo
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mDescriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &descriptorImageInfo
        }
    };

    vkUpdateDescriptorSets(mDevice->vkDevice(), 2, writeDescriptorSets, 0, nullptr);

    VkDeviceSize vertexBufferSize = sizeof(Pipeline::Vertex) * kNumVertices;
    allocateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, propertyFlags, &mVertexBuffer, &mVertexDeviceMemory);

    void *vertexMap;
    result = vkMapMemory(mDevice->vkDevice(), mVertexDeviceMemory, 0, vertexBufferSize, 0, &vertexMap);
    printf("Map memory: %i\n", result);

    memcpy(vertexMap, vertices, vertexBufferSize);

    vkUnmapMemory(mDevice->vkDevice(), mVertexDeviceMemory);

    VkImageCreateInfo depthCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_D32_SFLOAT,
        .extent = {640, 640},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    result = vkCreateImage(mDevice->vkDevice(), &depthCreateInfo, nullptr, &mDepthImage);
    printf("Create image: %i\n", result);

    vkGetImageMemoryRequirements(mDevice->vkDevice(), mDepthImage, &memoryRequirements);

    memoryTypeIndex = findMemoryTypeIndex(memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkMemoryAllocateInfo depthAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    result = vkAllocateMemory(mDevice->vkDevice(), &depthAllocInfo, nullptr, &mDepthMemory);
    printf("Allocate memory: %i\n", result);

    vkBindImageMemory(mDevice->vkDevice(), mDepthImage, mDepthMemory, 0);

    VkImageViewCreateInfo depthViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = mDepthImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_D32_SFLOAT,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    result = vkCreateImageView(mDevice->vkDevice(), &depthViewCreateInfo, nullptr, &mDepthView);
    printf("Create image view: %i\n", result);

    mSwapchain = std::make_unique<Swapchain>(*mDevice, *mPipeline, mDepthView, hInstance, hWnd);
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

    VkClearValue clearValues[] = 
    {
        {
            .color = {0.0f, 0.0f, 0.0f, 1.0f}
        },
        {
            .depthStencil = {1.0f, 0}
        }
    };
    
    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mPipeline->vkRenderPass(),
        .framebuffer = mSwapchain->framebuffers()[imageIndex],
        .renderArea = {
            .offset = { 0, 0 },
            .extent = { mSwapchain->width(), mSwapchain->height() },
        },
        .clearValueCount = 2,
        .pClearValues = clearValues
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

uint32_t Renderer::findMemoryTypeIndex(VkMemoryRequirements memoryRequirements, VkMemoryPropertyFlagBits propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(mDevice->vkPhysicalDevice(), &memoryProperties);

    uint32_t memoryTypeIndex = UINT32_MAX;
    for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++) {
        if(memoryRequirements.memoryTypeBits & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
            memoryTypeIndex = i;
            break;
        }
    }

    return memoryTypeIndex;
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

    uint32_t memoryTypeIndex = findMemoryTypeIndex(memoryRequirements, propertyFlags);
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