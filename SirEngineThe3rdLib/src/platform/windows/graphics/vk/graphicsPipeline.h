#pragma once

#include <vulkan/vulkan.h>

namespace SirEngine::vk {
VkPipeline
createGraphicsPipeline(VkDevice logicalDevice, VkShaderModule vs,
                       VkShaderModule ps, VkRenderPass renderPass,
                       VkPipelineVertexInputStateCreateInfo *vertexInfo);

}
