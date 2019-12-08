#include "platform/windows/graphics/vk/graphicsPipeline.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/memory/stackAllocator.h"
#include "platform/windows/graphics/vk/vk.h"
#include "vkRootSignatureManager.h"
#include <array>

namespace SirEngine::vk {

VkSampler STATIC_SAMPLERS[STATIC_SAMPLER_COUNT];
VkDescriptorImageInfo STATIC_SAMPLERS_INFO[STATIC_SAMPLER_COUNT];
VkDescriptorSetLayout STATIC_SAMPLER_LAYOUT;
VkDescriptorSet STATIC_SEMPLER_DESCRIPTOR_SET;

const char *STATIC_SAMPLERS_NAMES[STATIC_SAMPLER_COUNT] = {
    "pointWrapSampler",   "pointClampSampler",      "linearWrapSampler",
    "linearClampSampler", "anisotropicWrapSampler", "anisotropicClampSampler",
    "pcfSampler"};

std::array<const VkSamplerCreateInfo, STATIC_SAMPLER_COUNT>
getStaticSamplersCreateInfo() {
  // Applications usually only need a handful of samplers.  So just define them
  // all up front and keep them available as part of the root signature.

  const VkSamplerCreateInfo pointWrap{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                      nullptr,
                                      0,
                                      VK_FILTER_NEAREST,
                                      VK_FILTER_NEAREST,
                                      VK_SAMPLER_MIPMAP_MODE_NEAREST,
                                      VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      0.0f,
                                      VK_FALSE,
                                      1};
  const VkSamplerCreateInfo pointClamp{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                       nullptr,
                                       0,
                                       VK_FILTER_NEAREST,
                                       VK_FILTER_NEAREST,
                                       VK_SAMPLER_MIPMAP_MODE_NEAREST,
                                       VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                       VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                       VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                       0.0f,
                                       VK_FALSE,
                                       1};
  const VkSamplerCreateInfo linearWrap{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                       nullptr,
                                       0,
                                       VK_FILTER_LINEAR,
                                       VK_FILTER_LINEAR,
                                       VK_SAMPLER_MIPMAP_MODE_NEAREST,
                                       VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                       VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                       VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                       0.0f,
                                       VK_FALSE,
                                       1};
  const VkSamplerCreateInfo linearClamp{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                        nullptr,
                                        0,
                                        VK_FILTER_LINEAR,
                                        VK_FILTER_LINEAR,
                                        VK_SAMPLER_MIPMAP_MODE_NEAREST,
                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                        0.0f,
                                        VK_FALSE,
                                        1};
  const VkSamplerCreateInfo anisotropicWrap{
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      nullptr,
      0,
      VK_FILTER_LINEAR,
      VK_FILTER_LINEAR,
      VK_SAMPLER_MIPMAP_MODE_NEAREST,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      0.0f,
      VK_TRUE,
      16};
  const VkSamplerCreateInfo anisotropicClamp{
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      nullptr,
      0,
      VK_FILTER_LINEAR,
      VK_FILTER_LINEAR,
      VK_SAMPLER_MIPMAP_MODE_NEAREST,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      0.0f,
      VK_TRUE,
      8};
  const VkSamplerCreateInfo shadowPCFClamp{
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      nullptr,
      0,
      VK_FILTER_LINEAR,
      VK_FILTER_LINEAR,
      VK_SAMPLER_MIPMAP_MODE_NEAREST,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      0.0f,
      VK_FALSE,
      1,
      VK_TRUE,
      VK_COMPARE_OP_GREATER,
      0.0,
      0.0,
      VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK};

  return {pointWrap,       pointClamp,       linearWrap,    linearClamp,
          anisotropicWrap, anisotropicClamp, shadowPCFClamp};
}

void createStaticSamplerDescriptorSet(VkDescriptorPool &pool,
                                      VkDescriptorSet &outSet,
                                      VkDescriptorSetLayout &layout) {
  // here we are are creating the layout, but we are using static samplers
  // so we are passing immutable samplers directly in the layout that
  // gets built in the graphics pipeline
  VkDescriptorSetLayoutBinding resourceBinding[1] = {};
  resourceBinding[0].binding = 0;
  resourceBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  resourceBinding[0].descriptorCount = STATIC_SAMPLER_COUNT;
  resourceBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  resourceBinding[0].pImmutableSamplers = STATIC_SAMPLERS;

  VkDescriptorSetLayoutCreateInfo resourceLayoutInfo[1] = {};
  resourceLayoutInfo[0].sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  resourceLayoutInfo[0].pNext = NULL;
  resourceLayoutInfo[0].bindingCount = 1;
  resourceLayoutInfo[0].pBindings = resourceBinding;

  VK_CHECK(vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, resourceLayoutInfo,
                                       NULL, &layout));

  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = pool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &layout; // the layout we defined for the set,
                                      // so it also knows the size
  VK_CHECK(
      vkAllocateDescriptorSets(vk::LOGICAL_DEVICE, &allocateInfo, &outSet));
  SET_DEBUG_NAME(outSet,VK_OBJECT_TYPE_DESCRIPTOR_SET,"staticSamplersDescriptorSet");
  SET_DEBUG_NAME(layout,VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,"staticSamplersDescriptorSetLayout");
}

void destroyStaticSamplers() {
  for (int i = 0; i < STATIC_SAMPLER_COUNT; ++i) {
    vkDestroySampler(vk::LOGICAL_DEVICE, STATIC_SAMPLERS[i], nullptr);
  }
  vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE,STATIC_SAMPLER_LAYOUT,nullptr);
}

void getPipelineLayout(VkDevice logicalDevice,
                       VkDescriptorSetLayout samplersLayout,
                       VkDescriptorSetLayout &descriptorLayout,
                       const char *rootFile, VkPipelineLayout *outLayout) {
  // temporary hardcoded load root signature
}

VkPipeline
createGraphicsPipeline(VkDevice logicalDevice, VkShaderModule vs,
                       VkShaderModule ps, VkRenderPass renderPass,
                       VkPipelineVertexInputStateCreateInfo *vertexInfo) {
  const char *rootFile = "../data/rs/forwardPhongRS.json";

  RSHandle layoutHandle =
      vk::PIPELINE_LAYOUT_MANAGER->loadSignatureFile(rootFile, vk::STATIC_SAMPLER_LAYOUT);
  // TODO fix this should not be global anymore
  vk::PIPELINE_LAYOUT =
      vk::PIPELINE_LAYOUT_MANAGER->getLayoutFromHandle(layoutHandle);

  // here we define all the stages of the pipeline
  VkPipelineShaderStageCreateInfo stages[2] = {};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vs;
  stages[0].pName = "main";
  // this allows us to change constants at pipeline creation time,
  // this can allow for example to change compute shaders group size
  // and possibly allow brute force benchmarks with different sizes
  // vsInfo.pSpecializationInfo;
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = ps;
  stages[1].pName = "main";

  VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyCreateInfo.primitiveRestartEnable = false;

  VkPipelineViewportStateCreateInfo viewportCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewportCreateInfo.viewportCount = 1;
  viewportCreateInfo.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterInfo{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterInfo.depthClampEnable = false;
  rasterInfo.rasterizerDiscardEnable = false;
  rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
  rasterInfo.lineWidth = 1.0f; // even if we don't use it must be specified
  rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  ;

  VkPipelineMultisampleStateCreateInfo multisampleState{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkGraphicsPipelineCreateInfo createInfo{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  VkPipelineDepthStencilStateCreateInfo depthStencilState = {
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  depthStencilState.depthTestEnable = false;

  VkPipelineColorBlendAttachmentState attachState{};
  attachState.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo blendState{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  blendState.attachmentCount = 1;
  blendState.pAttachments = &attachState;

  VkDynamicState dynStateFlags[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = {
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamicState.dynamicStateCount =
      sizeof(dynStateFlags) / sizeof(dynStateFlags[0]);
  dynamicState.pDynamicStates = dynStateFlags;

  createInfo.stageCount = 2;
  createInfo.pStages = stages;
  createInfo.pVertexInputState =
      vertexInfo != nullptr ? vertexInfo : &vertexInputCreateInfo;
  createInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
  createInfo.pViewportState = &viewportCreateInfo;
  createInfo.pRasterizationState = &rasterInfo;
  createInfo.pMultisampleState = &multisampleState;
  createInfo.pDepthStencilState = &depthStencilState;
  createInfo.pColorBlendState = &blendState;
  createInfo.pDynamicState = &dynamicState;
  createInfo.renderPass = renderPass;
  createInfo.layout = PIPELINE_LAYOUT;

  VkPipeline pipeline = nullptr;
  VkResult status = vkCreateGraphicsPipelines(logicalDevice, nullptr, 1,
                                              &createInfo, nullptr, &pipeline);
  assert(status == VK_SUCCESS);
  assert(pipeline);
  return pipeline;
}

void initStaticSamplers() {
  auto createInfos = getStaticSamplersCreateInfo();
  for (int i = 0; i < STATIC_SAMPLER_COUNT; ++i) {

    VK_CHECK(vkCreateSampler(vk::LOGICAL_DEVICE, &createInfos[i], NULL,
                             &STATIC_SAMPLERS[i]));
    // setting debug name
    SET_DEBUG_NAME(STATIC_SAMPLERS[i], VK_OBJECT_TYPE_SAMPLER,
                   STATIC_SAMPLERS_NAMES[i]);

    STATIC_SAMPLERS_INFO[i] = {};
    STATIC_SAMPLERS_INFO[i].sampler = STATIC_SAMPLERS[i];
  }
  createStaticSamplerDescriptorSet(vk::DESCRIPTOR_POOL,
                                   STATIC_SEMPLER_DESCRIPTOR_SET,
                                   STATIC_SAMPLER_LAYOUT);
}

} // namespace SirEngine::vk
