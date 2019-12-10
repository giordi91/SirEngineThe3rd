#pragma once

#include "volk.h"

namespace SirEngine::vk {


#define STATIC_SAMPLER_COUNT 7
extern VkSampler STATIC_SAMPLERS[STATIC_SAMPLER_COUNT];
extern VkDescriptorImageInfo STATIC_SAMPLERS_INFO[STATIC_SAMPLER_COUNT];
extern VkDescriptorSetLayout STATIC_SAMPLER_LAYOUT;
extern VkDescriptorSet
    STATIC_SAMPLER_DESCRIPTOR_SET; // used in case you want to manually update
                                   // the samplers and not bound them as static

VkPipeline
createGraphicsPipeline(const char *psoPath, VkDevice logicalDevice,
                       VkRenderPass renderPass,
                       VkPipelineVertexInputStateCreateInfo *vertexInfo);
void initStaticSamplers();
void createStaticSamplerDescriptorSet(VkDescriptorPool &pool,
                                      VkDescriptorSet &outSet,
                                      VkDescriptorSetLayout &layout);
void destroyStaticSamplers();

} // namespace SirEngine::vk
