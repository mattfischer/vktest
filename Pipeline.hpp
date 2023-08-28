#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "Device.hpp"

#include <vulkan/vulkan_core.h>

class Pipeline {
public:
    Pipeline(Device &device);

    VkPipeline vkPipeline() { return mPipeline; }
    VkPipelineLayout vkPipelineLayout() { return mPipelineLayout; }
    VkRenderPass vkRenderPass() { return mRenderPass; }
    VkDescriptorSetLayout vkDescriptorSetLayout() { return mDescriptorSetLayout; }

    VkFormat format();

private:
    Device &mDevice;
    VkShaderModule mVertShaderModule;
    VkShaderModule mFragShaderModule;
    VkRenderPass mRenderPass;
    VkDescriptorSetLayout mDescriptorSetLayout;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipeline;
};

#endif