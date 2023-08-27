#include <windows.h>
#include <stdio.h>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include <vector>

static const int kWidth = 640;
static const int kHeight = 480;

void loadShader(const char *filename, VkShaderModuleCreateInfo *createInfo)
{
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    createInfo->codeSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    createInfo->pCode = (uint32_t*)malloc(createInfo->codeSize);
    fread((void*)createInfo->pCode, 1, createInfo->codeSize, f);
    fclose(f);
}

VkInstance inst;
int32_t graphicsIndex = -1;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkQueue queue;
VkCommandBuffer commandBuffer;
VkCommandPool commandPool;
VkShaderModule vertShaderModule;
VkShaderModule fragShaderModule;
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline pipeline;
VkSurfaceKHR surface;
VkSwapchainKHR swapchain;
VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
std::vector<VkImage> swapchainImages;
std::vector<VkFramebuffer> framebuffers;
VkDescriptorSetLayout descriptorSetLayout;
VkBuffer uniformBuffer;
VkDeviceMemory uniformDeviceMemory;
void *uniformMap;
VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;

void initVulkan(HINSTANCE hInstance, HWND hWnd)
{   
    const char *instExtensions[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };

    VkInstanceCreateInfo instCreateInfo{};
    instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instCreateInfo.enabledExtensionCount = 2;
    instCreateInfo.ppEnabledExtensionNames = instExtensions;

    VkResult result;
    result = vkCreateInstance(&instCreateInfo, nullptr, &inst);
    printf("create instance: %i\n", result);

    uint32_t numPhysicalDevices = 1;
    result = vkEnumeratePhysicalDevices(inst, &numPhysicalDevices, &physicalDevice);

    uint32_t queueFamilyPropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
    printf("queueFamilyPropertyCount: %i\n", queueFamilyPropertyCount);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, &queueFamilyProperties[0]);
    
    for(int32_t i=0; i<queueFamilyPropertyCount; i++) {
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsIndex = i;
        }
    }

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsIndex;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char *deviceExtensions[] = {
        "VK_KHR_swapchain"
    };

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
    printf("Create device: %i\n", result);

    vkGetDeviceQueue(device, graphicsIndex, 0, &queue);

    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = graphicsIndex;

    result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
    printf("Create command pool: %i\n", result);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
    printf("Allocate command buffers: %i\n", result);

    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency subpassDependency{};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
    printf("Create render pass: %i\n", result);

    VkShaderModuleCreateInfo vertShaderModuleCreateInfo{};
    vertShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    loadShader("build/vert.spv", &vertShaderModuleCreateInfo);

    result = vkCreateShaderModule(device, &vertShaderModuleCreateInfo, nullptr, &vertShaderModule);
    printf("Create vertex shader: %i\n", result);

    VkPipelineShaderStageCreateInfo vertShaderStage{};
    vertShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStage.module = vertShaderModule;
    vertShaderStage.pName = "main";

    VkShaderModuleCreateInfo fragShaderModuleCreateInfo{};
    fragShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    loadShader("build/frag.spv", &fragShaderModuleCreateInfo);

    result = vkCreateShaderModule(device, &fragShaderModuleCreateInfo, nullptr, &fragShaderModule);
    printf("Create fragment shader: %i\n", result);

    VkPipelineShaderStageCreateInfo fragShaderStage{};
    fragShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStage.module = fragShaderModule;
    fragShaderStage.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStage, fragShaderStage };

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = kWidth;
    viewport.height = kHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {kWidth, kHeight};

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCreateInfo.minSampleShading = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    VkDescriptorSetLayoutBinding uniformBinding{};
    uniformBinding.binding = 0;
    uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBinding.descriptorCount = 1;
    uniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    descriptorSetLayoutCreateInfo.pBindings = &uniformBinding;

    result = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);
    printf("Create descriptor set layout: %i\n", result);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    result = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
    printf("Create pipeline layout: %i\n", result);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
    printf("Create pipeline: %i\n", result);

    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = hInstance;
    surfaceCreateInfo.hwnd = hWnd;

    result = vkCreateWin32SurfaceKHR(inst, &surfaceCreateInfo, nullptr, &surface);
    printf("Create Win32 Surface: %i\n", result);

    VkBool32 surfaceSupported;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsIndex, surface, &surfaceSupported);
    printf("Surface supported: %s\n", surfaceSupported ? "true" : "false");

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

    uint32_t numSurfaceFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &numSurfaceFormats, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(numSurfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &numSurfaceFormats, &surfaceFormats[0]);
    for(int i=0; i<numSurfaceFormats; i++) {
        printf("Surface format: %i %i\n", surfaceFormats[i].format, surfaceFormats[i].colorSpace);
    }

    uint32_t numPresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &numPresentModes, nullptr);
    std::vector<VkPresentModeKHR> presentModes(numPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &numPresentModes, &presentModes[0]);
    for(int i=0; i<numPresentModes; i++) {
        printf("Present mode: %i\n", presentModes[i]);
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
    swapchainCreateInfo.imageFormat = format;
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

    result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
    printf("Create swapchain: %i\n", result);

    uint32_t numSwapchainImages;
    result = vkGetSwapchainImagesKHR(device, swapchain, &numSwapchainImages, nullptr);
    swapchainImages.resize(numSwapchainImages);
    result = vkGetSwapchainImagesKHR(device, swapchain, &numSwapchainImages, &swapchainImages[0]);
    
    framebuffers.resize(numSwapchainImages);
    for(int i=0; i<numSwapchainImages; i++) {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.format = format;
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
        result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);
        printf("Create image view: %i (%i)\n", result, i);

        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &imageView;
        framebufferCreateInfo.width = kWidth;
        framebufferCreateInfo.height = kHeight;
        framebufferCreateInfo.layers = 1;

        result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffers[i]);
        printf("Create framebuffer: %i (%i)\n", result, i);
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore);
    printf("Create semaphore: %i\n", result);

    result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphore);
    printf("Create semaphore: %i\n", result);

    VkDeviceSize uniformBufferSize = sizeof(uint32_t);

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = uniformBufferSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &uniformBuffer);
    printf("Create buffer: %i\n", result);

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, uniformBuffer, &memoryRequirements);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

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

    result = vkAllocateMemory(device, &allocateInfo, nullptr, &uniformDeviceMemory);
    printf("Allocate memory: %i\n", result);

    result = vkBindBufferMemory(device, uniformBuffer, uniformDeviceMemory, 0);
    printf("Bind memory: %i\n", result);

    result = vkMapMemory(device, uniformDeviceMemory, 0, uniformBufferSize, 0, &uniformMap);
    printf("Map memory: %i\n", result);

    VkDescriptorPoolSize descriptorPoolSize{};
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = 1;
    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;

    result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);
    printf("Create descriptor pool: %i\n", result);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

    result = vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet);
    printf("Allocate descriptor set: %i\n", result);

    VkDescriptorBufferInfo descriptorBufferInfo{};
    descriptorBufferInfo.buffer = uniformBuffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = uniformBufferSize;

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}

void renderFrame(int frame)
{
    *((uint32_t*)uniformMap) = frame;

    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult result;

    result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    uint32_t imageIndex;
    result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    VkClearValue clearValue{{{0.0f, 0.0f, 0.0f, 1.0f}}};
    
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = {kWidth, kHeight};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    result = vkEndCommandBuffer(commandBuffer);

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;
    result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(queue, &presentInfo);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static int frame = 0;

    switch(iMsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    
        case WM_TIMER:
            renderFrame(frame++);
            //SetTimer(hWnd, 1, 30, NULL);
            return 0;
    }

    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    setbuf(stdout, NULL);

    WNDCLASSEX wndClass;
    const char *szClassName = "vktest";

    wndClass.cbSize=sizeof(WNDCLASSEX);
    wndClass.style=(UINT)NULL;
    wndClass.lpfnWndProc=WndProc;
    wndClass.cbClsExtra=0;
    wndClass.cbWndExtra=0;
    wndClass.hInstance=hInstance;
    wndClass.hIcon=LoadIcon(0,IDI_APPLICATION);
    wndClass.hCursor=LoadCursor(0,IDC_ARROW);
    wndClass.hbrBackground=(HBRUSH)GetStockObject(NULL_BRUSH);
    wndClass.lpszMenuName=NULL;
    wndClass.lpszClassName=szClassName;
    wndClass.hIconSm=NULL;

    RegisterClassEx(&wndClass);

    HWND hWnd = CreateWindowEx(
        (DWORD)NULL,
        szClassName,
        "Vulkan Test",
        WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION,
        0,
        0,
        kWidth,
        kHeight,
        (HWND)NULL,
        (HMENU)NULL,
        hInstance,
        0
    );

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    initVulkan(hInstance, hWnd);

    SetTimer(hWnd, 1, 0, NULL);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}