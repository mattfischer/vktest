#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "Device.hpp"

#include <vulkan/vulkan_core.h>

class Pipeline {
public:
    Pipeline(Device &device);

    VkPipeline vkPipeline() { return mPipeline; }
    VkPipelineLayout vkPipelineLayout() { return mPipelineLayout; }
    VkDescriptorSetLayout vkDescriptorSetLayout() { return mDescriptorSetLayout; }

    VkFormat format();

    struct Vertex {
        float position[3];
        float color[3];
        float tex[2];
    };

private:
    Device &mDevice;
    VkShaderModule mVertShaderModule;
    VkShaderModule mFragShaderModule;
    VkDescriptorSetLayout mDescriptorSetLayout;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipeline;
};

#endif