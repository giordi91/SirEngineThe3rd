#include "platform/windows/graphics/vk/graphicsPipeline.h"
#include "platform/windows/graphics/vk/vk.h"
#include <array>
namespace SirEngine::vk {

VkSampler STATIC_SAMPLERS[STATIC_SAMPLER_COUNT];
VkDescriptorImageInfo STATIC_SAMPLERS_INFO[STATIC_SAMPLER_COUNT];
const char *STATIC_SAMPLERS_NAMES[STATIC_SAMPLER_COUNT] = {
    "pointWrapSampler",   "pointClampSampler",      "linearWrapSampler",
    "linearClampSampler", "anisotropicWrapSampler", "anisotropicClampSampler", "pcfSampler"};

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
  VkDescriptorSetLayoutBinding resource_binding[1] = {};
  resource_binding[0].binding = 0;
  resource_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  resource_binding[0].descriptorCount = STATIC_SAMPLER_COUNT;
  resource_binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  resource_binding[0].pImmutableSamplers = STATIC_SAMPLERS;

  /*
  //Not static samplers
  VkDescriptorSetLayoutBinding resource_binding[1] = {};
  resource_binding[0].binding = 0;
  resource_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  resource_binding[0].descriptorCount = STATIC_SAMPLER_COUNT;
  resource_binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  resource_binding[0].pImmutableSamplers = NULL;
  */

  VkDescriptorSetLayoutCreateInfo resource_layout_info[1] = {};
  resource_layout_info[0].sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  resource_layout_info[0].pNext = NULL;
  resource_layout_info[0].bindingCount = 1;
  resource_layout_info[0].pBindings = resource_binding;

  VK_CHECK(vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, resource_layout_info,
                                       NULL, &layout));

  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = pool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &layout; // the layout we defined for the set,
                                      // so it also knows the size
  VK_CHECK(
      vkAllocateDescriptorSets(vk::LOGICAL_DEVICE, &allocateInfo, &outSet));


	
}

void destroyStaticSamplers() {
  for (int i = 0; i < STATIC_SAMPLER_COUNT; ++i) {
    vkDestroySampler(vk::LOGICAL_DEVICE, STATIC_SAMPLERS[i], nullptr);
  }
}

VkPipeline
createGraphicsPipeline(VkDevice logicalDevice, VkShaderModule vs,
                       VkShaderModule ps, VkRenderPass renderPass,
                       VkPipelineVertexInputStateCreateInfo *vertexInfo, VkDescriptorSetLayout samplersLayout) {

  // From spec:  The pipeline layout represents a sequence of descriptor sets
  // with each having a specific layout.
  // this is the same as root signature in DX12
  VkPipelineLayoutCreateInfo layoutInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

  //multiple set, with separated samplers
  // to do so we declare a set of layout bindings, basically an array telling us
  // how many elements we have, this will be coming from a json file
  VkDescriptorSetLayoutBinding bindings[2]{};
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  //bindings[2].binding = 2;
  //bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  //bindings[2].descriptorCount = STATIC_SAMPLER_COUNT;
  //bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  //bindings[2].pImmutableSamplers = vk::STATIC_SAMPLERS;
  /*
  // to do so we declare a set of layout bindings, basically an array telling us
  // how many elements we have, this will be coming from a json file
  VkDescriptorSetLayoutBinding bindings[3]{};
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  bindings[2].descriptorCount = STATIC_SAMPLER_COUNT;
  bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[2].pImmutableSamplers = vk::STATIC_SAMPLERS;
  */

  // passing in the "root signature"
  VkDescriptorSetLayoutCreateInfo descriptorInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};

  descriptorInfo.bindingCount = ARRAYSIZE(bindings);
  descriptorInfo.pBindings = bindings;

  // creating the layout/root signature
  VkDescriptorSetLayout descriptorLayout;
  vkCreateDescriptorSetLayout(logicalDevice, &descriptorInfo, nullptr,
                              &descriptorLayout);

  //mulitple layouts
  // now we know how many layouts we have, and we can create the pipelien layout
  VkDescriptorSetLayout layouts[] = {descriptorLayout, samplersLayout};
  layoutInfo.setLayoutCount = 2;
  layoutInfo.pSetLayouts = layouts;


  //single layout
  // now we know how many layouts we have, and we can create the pipeline layout
  //layoutInfo.setLayoutCount = 1;
  //layoutInfo.pSetLayouts = &descriptorLayout;

  // TODO need a manager
  vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &PIPELINE_LAYOUT);

  LAYOUTS_TO_DELETE.push_back(descriptorLayout);

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
	//setting debug name
    SET_DEBUG_NAME(STATIC_SAMPLERS[i], VK_OBJECT_TYPE_SAMPLER,
                   STATIC_SAMPLERS_NAMES[i]);

    STATIC_SAMPLERS_INFO[i] = {};
    STATIC_SAMPLERS_INFO[i].sampler = STATIC_SAMPLERS[i];
  }
}

} // namespace SirEngine::vk
