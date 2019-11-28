#pragma once

#include <vulkan/vulkan.h>

namespace SirEngine::vk {

#define STATIC_SAMPLER_COUNT 7
extern VkSampler STATIC_SAMPLERS[STATIC_SAMPLER_COUNT];

VkPipeline
createGraphicsPipeline(VkDevice logicalDevice, VkShaderModule vs,
                       VkShaderModule ps, VkRenderPass renderPass,
                       VkPipelineVertexInputStateCreateInfo *vertexInfo);
void initStaticSamplers();

}
