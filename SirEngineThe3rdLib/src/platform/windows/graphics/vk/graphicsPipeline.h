#pragma once

#include <vulkan/vulkan.h>

namespace SirEngine::vk {

#define STATIC_SAMPLER_COUNT 7
extern VkSampler STATIC_SAMPLERS[STATIC_SAMPLER_COUNT];
extern VkDescriptorImageInfo STATIC_SAMPLERS_INFO[STATIC_SAMPLER_COUNT];

VkPipeline
createGraphicsPipeline(VkDevice logicalDevice, VkShaderModule vs,
                       VkShaderModule ps, VkRenderPass renderPass,
                       VkPipelineVertexInputStateCreateInfo *vertexInfo);
void initStaticSamplers();
void createStaticSamplerDescriptorSet(VkDescriptorPool& pool ,VkDescriptorSet& outSet,VkDescriptorSetLayout& layout );
void destroyStaticSamplers();

}
