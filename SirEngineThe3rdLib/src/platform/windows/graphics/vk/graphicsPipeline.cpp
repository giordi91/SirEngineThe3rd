#include "platform/windows/graphics/vk/graphicsPipeline.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/memory/stackAllocator.h"
#include "platform/windows/graphics/vk/vk.h"
#include "vkRootSignatureManager.h"
#include "vkShaderManager.h"
#include <array>

namespace SirEngine::vk {

enum class PSOType { DXR = 0, RASTER, COMPUTE, INVALID };

static const char *PSO_VS_SHADER_ENTRY_POINT = "main";
static const char *PSO_PS_SHADER_ENTRY_POINT = "main";
static const char *PSO_CS_SHADER_ENTRY_POINT = "main";

static const std::string PSO_KEY_GLOBAL_ROOT = "globalRootSignature";
static const std::string PSO_KEY_TYPE = "type";
static const std::string PSO_KEY_TYPE_DXR = "DXR";
static const std::string PSO_KEY_TYPE_RASTER = "RASTER";
static const std::string PSO_KEY_TYPE_COMPUTE = "COMPUTE";
static const std::string PSO_KEY_SHADER_NAME = "shaderName";
static const std::string PSO_KEY_INPUT_LAYOUT = "inputLayout";
static const std::string PSO_KEY_VS_SHADER = "VS";
static const std::string PSO_KEY_PS_SHADER = "PS";
static const std::string PSO_KEY_RASTER_STATE = "rasterState";
static const std::string PSO_KEY_BLEND_STATE = "blendState";
static const std::string PSO_KEY_DEPTH_STENCIL_STATE = "depthStencilState";
static const std::string PSO_KEY_SAMPLE_MASK = "sampleMask";
static const std::string PSO_KEY_TOPOLOGY_TYPE = "topologyType";
static const std::string PSO_KEY_RENDER_TARGETS = "renderTargets";
static const std::string PSO_KEY_RTV_FORMATS = "rtvFormats";
static const std::string PSO_KEY_SAMPLE_DESC_COUNT = "sampleDescCount";
static const std::string PSO_KEY_SAMPLE_DESC_QUALITY = "sampleDescQuality";
static const std::string PSO_KEY_DSV_FORMAT = "dsvFormat";
static const std::string PSO_KEY_DEPTH_STENCIL_CONFIG =
    "depthStencilStateConfig";
static const std::string DEFAULT_STRING = "";
static const int DEFAULT_INT = -1;
static const bool DEFAULT_BOOL = false;
static const std::string DEFAULT_STATE = "default";
static const std::string PSO_KEY_CUSTOM_STATE = "custom";
static const std::string PSO_KEY_DEPTH_ENABLED = "depthEnabled";
static const std::string PSO_KEY_STENCIL_ENABLED = "stencilEnabled";
static const std::string PSO_KEY_DEPTH_COMPARISON_FUNCTION = "depthFunc";
static const std::string PSO_KEY_RASTER_CONFIG = "rasterStateConfig";
static const std::string PSO_KEY_RASTER_CULL_MODE = "cullMode";
static const std::string PSO_KEY_STENCIL_FAIL_OP = "stencilFailOp";
static const std::string PSO_KEY_STENCIL_DEPTH_FAIL_OP = "stencilDepthFailOp";
static const std::string PSO_KEY_STENCIL_PASS_OP = "stencilPassOp";
static const std::string PSO_KEY_STENCIL_COMPARISON_FUNCTION = "stencilFunc";

static constexpr int MAX_SHADER_STAGE_COUNT = 5;

VkSampler STATIC_SAMPLERS[STATIC_SAMPLER_COUNT];
VkDescriptorImageInfo STATIC_SAMPLERS_INFO[STATIC_SAMPLER_COUNT];
VkDescriptorSetLayout STATIC_SAMPLER_LAYOUT;
VkDescriptorSet STATIC_SAMPLER_DESCRIPTOR_SET;

void assertInJson(const nlohmann::json &jobj, const std::string &key) {
  const auto found = jobj.find(key);
  assert(found != jobj.end());
}

const char *STATIC_SAMPLERS_NAMES[STATIC_SAMPLER_COUNT] = {
    "pointWrapSampler",   "pointClampSampler",      "linearWrapSampler",
    "linearClampSampler", "anisotropicWrapSampler", "anisotropicClampSampler",
    "pcfSampler"};

static const std::unordered_map<std::string, PSOType> STRING_TO_PSO_TYPE{
    {PSO_KEY_TYPE_DXR, PSOType::DXR},
    {PSO_KEY_TYPE_COMPUTE, PSOType::COMPUTE},
    {PSO_KEY_TYPE_RASTER, PSOType::RASTER},
};
static const std::unordered_map<std::string, VkPrimitiveTopology>
    STRING_TO_TOPOLOGY = {
        {"triangle", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST},
        {"line", VK_PRIMITIVE_TOPOLOGY_LINE_LIST},
};

static const std::unordered_map<std::string, VkCullModeFlagBits>
    STRING_TO_CULL_MODE_FUNCTION{{"NONE", VK_CULL_MODE_NONE},
                                 {"BACK", VK_CULL_MODE_BACK_BIT},
                                 {"FRONT", VK_CULL_MODE_FRONT_BIT}};

static const std::unordered_map<std::string, VkCompareOp>
    STRING_TO_COMPARISON_FUNCTION{
        {"LESS", VK_COMPARE_OP_LESS},
        {"GREATER", VK_COMPARE_OP_GREATER},
        {"ALWAYS", VK_COMPARE_OP_ALWAYS},
        {"GREATER_EQUAL", VK_COMPARE_OP_GREATER_OR_EQUAL},
        {"EQUAL", VK_COMPARE_OP_EQUAL}};

static const std::unordered_map<std::string, VkStencilOp> STRING_TO_STENCIL_OP{
    {"KEEP", VK_STENCIL_OP_KEEP},
    {"ZERO", VK_STENCIL_OP_ZERO},
    {"REPLACE", VK_STENCIL_OP_REPLACE},
    {"INCR_SAT", VK_STENCIL_OP_INCREMENT_AND_CLAMP},
    {"DECR_SAT", VK_STENCIL_OP_DECREMENT_AND_CLAMP},
    {"INVERT", VK_STENCIL_OP_INVERT},
    {"INCR", VK_STENCIL_OP_INCREMENT_AND_WRAP},
    {"DECR", VK_STENCIL_OP_DECREMENT_AND_WRAP}};

static const std::unordered_map<std::string, VkFormat> STRING_TO_VK_FORMAT{
    {"DXGI_FORMAT_R16G16B16A16_FLOAT", VK_FORMAT_R16G16B16A16_SFLOAT},
    {"DXGI_FORMAT_D32_FLOAT_S8X24_UINT", VK_FORMAT_D32_SFLOAT_S8_UINT},
    {"DXGI_FORMAT_R8G8B8A8_UNORM", VK_FORMAT_R8G8B8A8_UNORM}};

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
  SET_DEBUG_NAME(outSet, VK_OBJECT_TYPE_DESCRIPTOR_SET,
                 "staticSamplersDescriptorSet");
  SET_DEBUG_NAME(layout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                 "staticSamplersDescriptorSetLayout");
}

void destroyStaticSamplers() {
  for (int i = 0; i < STATIC_SAMPLER_COUNT; ++i) {
    vkDestroySampler(vk::LOGICAL_DEVICE, STATIC_SAMPLERS[i], nullptr);
  }
  vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE, STATIC_SAMPLER_LAYOUT,
                               nullptr);
}

PSOType convertStringPSOTypeToEnum(const char *type) {
  const auto found = STRING_TO_PSO_TYPE.find(type);
  return (found != STRING_TO_PSO_TYPE.end() ? found->second : PSOType::INVALID);
}

void getShaderStageCreateInfo(const nlohmann::json &jobj,
                              VkPipelineShaderStageCreateInfo *stages,
                              int &shaderStageCount) {

  // we have a raster pso lets pull out the shader stages
  const std::string vsFile =
      getValueIfInJson(jobj, PSO_KEY_VS_SHADER, DEFAULT_STRING);
  assert(!vsFile.empty());
  int id = shaderStageCount++;
  // fill up shader binding

  // this allows us to change constants at pipeline creation time,
  // this can allow for example to change compute shaders group size
  // and possibly allow brute force benchmarks with different group sizes(CS
  // only) vsInfo.pSpecializationInfo;
  stages[id].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[id].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[id].module = vk::SHADER_MANAGER->getShaderFromName(vsFile);
  stages[id].pName = PSO_VS_SHADER_ENTRY_POINT;

  const std::string psFile =
      getValueIfInJson(jobj, PSO_KEY_PS_SHADER, DEFAULT_STRING);
  if (!psFile.empty()) {
    id = shaderStageCount++;

    stages[id].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[id].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[id].module = vk::SHADER_MANAGER->getShaderFromName(psFile);
    stages[id].pName = PSO_PS_SHADER_ENTRY_POINT;
  }
}

inline VkPrimitiveTopology
convertStringToTopology(const std::string &topology) {
  const auto found = STRING_TO_TOPOLOGY.find(topology);
  if (found != STRING_TO_TOPOLOGY.end()) {
    return found->second;
  }
  assert(0 &&
         "provided string format is not a valid VK topology, or unsupported");
  return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
}

void getAssemblyCreateInfo(
    const nlohmann::json &jobj,
    VkPipelineInputAssemblyStateCreateInfo &inputAssemblyCreateInfo) {

  const std::string topology =
      getValueIfInJson(jobj, PSO_KEY_TOPOLOGY_TYPE, DEFAULT_STRING);
  assert(!topology.empty());

  inputAssemblyCreateInfo.topology = convertStringToTopology(topology);
  assert(inputAssemblyCreateInfo.topology != VK_PRIMITIVE_TOPOLOGY_MAX_ENUM);
  // always false unless geometry shader?
  inputAssemblyCreateInfo.primitiveRestartEnable = false;
}

VkCullModeFlagBits getCullMode(const nlohmann::json &jobj) {
  const std::string funcDefault = "BACK";
  const std::string func =
      getValueIfInJson(jobj, PSO_KEY_RASTER_CULL_MODE, funcDefault);
  const auto found = STRING_TO_CULL_MODE_FUNCTION.find(func);
  assert(found != STRING_TO_CULL_MODE_FUNCTION.end());
  return found->second;
}

void getRasterInfo(const nlohmann::json jobj,
                   VkPipelineRasterizationStateCreateInfo &rasterInfo) {
  rasterInfo.depthClampEnable = false;
  rasterInfo.rasterizerDiscardEnable = false;
  rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
  rasterInfo.lineWidth = 1.0f; // even if we don't use it must be specified
  rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  const std::string rasterStateString =
      getValueIfInJson(jobj, PSO_KEY_RASTER_STATE, DEFAULT_STRING);

  const bool rasterDefault = rasterStateString == DEFAULT_STATE;
  if (rasterDefault) {
    rasterInfo.depthClampEnable = false;
    rasterInfo.rasterizerDiscardEnable = false;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterInfo.lineWidth = 1.0f; // even if we don't use it must be specified
    rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    return;
  }
  // if we are here means we have a custom state
  assert(jobj.find(PSO_KEY_RASTER_CONFIG) != jobj.end());
  const auto config = jobj[PSO_KEY_RASTER_CONFIG];

  rasterInfo.depthClampEnable = false;
  rasterInfo.rasterizerDiscardEnable = false;
  rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterInfo.cullMode = getCullMode(config);
  rasterInfo.lineWidth = 1.0f; // even if we don't use it must be specified
  rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
}

inline VkCompareOp getComparisonFunction(const nlohmann::json &jobj,
                                         const std::string &key) {
  const std::string funcDefault = "LESS";
  const std::string func = getValueIfInJson(jobj, key, funcDefault);
  const auto found = STRING_TO_COMPARISON_FUNCTION.find(func);
  assert(found != STRING_TO_COMPARISON_FUNCTION.end());
  return found->second;
}

inline VkStencilOp getStencilOperationFunction(const nlohmann::json &jobj,
                                               const std::string &key) {
  const std::string funcDefault = "KEEP";
  const std::string func = getValueIfInJson(jobj, key, funcDefault);
  const auto found = STRING_TO_STENCIL_OP.find(func);
  assert(found != STRING_TO_STENCIL_OP.end());
  return found->second;
}

void getDepthStencilState(
    const nlohmann::json jobj,
    VkPipelineDepthStencilStateCreateInfo &depthStencilState) {
  const std::string depthStencilStateString =
      getValueIfInJson(jobj, PSO_KEY_DEPTH_STENCIL_STATE, DEFAULT_STRING);

  const bool isDefault = depthStencilStateString == DEFAULT_STATE;
  if (isDefault) {
    depthStencilState.depthTestEnable = true;
    depthStencilState.depthWriteEnable = true;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER;
    depthStencilState.stencilTestEnable = false;
    return;
  }
  assert(jobj.find(PSO_KEY_DEPTH_STENCIL_CONFIG) != jobj.end());
  const auto dssObj = jobj[PSO_KEY_DEPTH_STENCIL_CONFIG];

  const bool depthEnabled =
      getValueIfInJson(dssObj, PSO_KEY_DEPTH_ENABLED, DEFAULT_BOOL);
  depthStencilState.depthTestEnable = depthEnabled;

  depthStencilState.depthCompareOp =
      getComparisonFunction(dssObj, PSO_KEY_DEPTH_COMPARISON_FUNCTION);
  const bool stencilEnabled =
      getValueIfInJson(dssObj, PSO_KEY_STENCIL_ENABLED, DEFAULT_BOOL);
  depthStencilState.stencilTestEnable = stencilEnabled;
  const VkStencilOp stencilFail =
      getStencilOperationFunction(dssObj, PSO_KEY_STENCIL_FAIL_OP);
  const VkStencilOp depthFail =
      getStencilOperationFunction(dssObj, PSO_KEY_STENCIL_DEPTH_FAIL_OP);
  const VkStencilOp stencilPass =
      getStencilOperationFunction(dssObj, PSO_KEY_STENCIL_PASS_OP);
  const VkCompareOp stencilFunction =
      getComparisonFunction(dssObj, PSO_KEY_STENCIL_COMPARISON_FUNCTION);

  depthStencilState.front.compareOp = stencilFunction;
  depthStencilState.front.failOp = stencilFail;
  depthStencilState.front.passOp = stencilPass;
  depthStencilState.front.depthFailOp = depthFail;
  depthStencilState.front.compareMask = 0xff;
  depthStencilState.front.writeMask = 0xff;
  depthStencilState.back = depthStencilState.front;
}

inline VkFormat convertStringToTextureFormat(const std::string &format) {
  const auto found = STRING_TO_VK_FORMAT.find(format);
  if (found != STRING_TO_VK_FORMAT.end()) {
    return found->second;
  }
  assert(0 && "provided string format is not a valid VK format");
  return VK_FORMAT_UNDEFINED;
}


VkRenderPass getRenderPass(const nlohmann::json &jobj) {
  const uint32_t renderTargets =
      getValueIfInJson(jobj, PSO_KEY_RENDER_TARGETS, DEFAULT_INT);

  assertInJson(jobj, PSO_KEY_RTV_FORMATS);

  // TODO change this to maximum number of attachments
  VkAttachmentDescription attachments[10] = {};
  VkAttachmentReference attachmentsRefs[10] = {};
  int count = 0;
  for (auto &format : jobj[PSO_KEY_RTV_FORMATS]) {
    VkFormat currentFormat =
        convertStringToTextureFormat(format.get<std::string>());
    assert(currentFormat != VK_FORMAT_UNDEFINED && "Unsupported render format");

    attachments[count].format = currentFormat;
    // TODO no MSAA yet
    attachments[count].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[count].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // TODO not really sure what to do about the stencil...
    // for now set to load and store, should leave it untouched
    attachments[count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[count].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[count].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // we have only one subpass and uses all attachments
    attachmentsRefs[count].attachment = count;
    attachmentsRefs[count].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    count++;
  }
  assert(renderTargets == count &&
         "number of render targets and provided formats don't match");

  VkRenderPass renderPass{};

  VkSubpassDescription subPass{};
  subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subPass.colorAttachmentCount = count;
  subPass.pColorAttachments = attachmentsRefs;

  VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  createInfo.attachmentCount = count;
  createInfo.pAttachments = attachments;
  createInfo.subpassCount = 1;
  createInfo.pSubpasses = &subPass;

  vkCreateRenderPass(vk::LOGICAL_DEVICE, &createInfo, nullptr, &renderPass);
  return renderPass;

}

VkPipeline processRasterPSO(nlohmann::json::const_reference jobj,
                            VkRenderPass renderPass,
                            VkPipelineVertexInputStateCreateInfo *vertexInfo) {
  // load root signature
  const std::string rootFile =
      getValueIfInJson(jobj, PSO_KEY_GLOBAL_ROOT, DEFAULT_STRING);
  assert(!rootFile.empty());

  RSHandle layoutHandle = vk::PIPELINE_LAYOUT_MANAGER->loadSignatureFile(
      rootFile.c_str(), vk::STATIC_SAMPLER_LAYOUT);
  // TODO fix this should not be global anymore
  vk::PIPELINE_LAYOUT =
      vk::PIPELINE_LAYOUT_MANAGER->getLayoutFromHandle(layoutHandle);

  // load shader stage
  // here we define all the stages of the pipeline
  VkPipelineShaderStageCreateInfo stages[MAX_SHADER_STAGE_COUNT] = {};
  int shaderStageCount = 0;
  getShaderStageCreateInfo(jobj, stages, shaderStageCount);

  // TODO right now we are not using a vertex input, this needs to be fixed and
  // read from PSO
  VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  // input assembler info
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  getAssemblyCreateInfo(jobj, inputAssemblyCreateInfo);

  VkPipelineViewportStateCreateInfo viewportCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewportCreateInfo.viewportCount = 1;
  viewportCreateInfo.scissorCount = 1;

  // process raster info
  VkPipelineRasterizationStateCreateInfo rasterInfo{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  getRasterInfo(jobj, rasterInfo);

  // TODO support MSAA
  VkPipelineMultisampleStateCreateInfo multisampleState{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // TODO fix depth test
  VkPipelineDepthStencilStateCreateInfo depthStencilState = {
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  getDepthStencilState(jobj, depthStencilState);
  // TODO remove this
  depthStencilState.depthTestEnable = false;

  const std::string blendStateString =
      getValueIfInJson(jobj, PSO_KEY_BLEND_STATE, DEFAULT_STRING);
  assert(blendStateString == "default" &&
         "no supported blend state other than default");

  VkRenderPass pass = getRenderPass(jobj);

  VkPipelineColorBlendAttachmentState attachState{};
  attachState.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo blendState{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  blendState.attachmentCount = 1;
  blendState.pAttachments = &attachState;

  // always support dynamic viewport and scissor such that we can resize without
  // recompile PSOs
  VkDynamicState dynStateFlags[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = {
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynamicState.dynamicStateCount =
      sizeof(dynStateFlags) / sizeof(dynStateFlags[0]);
  dynamicState.pDynamicStates = dynStateFlags;

  // create the actual pipline
  VkGraphicsPipelineCreateInfo createInfo{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  createInfo.stageCount = shaderStageCount;
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
  VkResult status = vkCreateGraphicsPipelines(vk::LOGICAL_DEVICE, nullptr, 1,
                                              &createInfo, nullptr, &pipeline);
  assert(status == VK_SUCCESS);
  assert(pipeline);
  return pipeline;
};

VkPipeline
createGraphicsPipeline(const char *psoPath, VkDevice logicalDevice,
                       VkRenderPass renderPass,
                       VkPipelineVertexInputStateCreateInfo *vertexInfo) {

  auto jobj = getJsonObj(psoPath);

  const std::string typeString =
      getValueIfInJson(jobj, PSO_KEY_TYPE, DEFAULT_STRING);
  assert(!typeString.empty());
  PSOType type = convertStringPSOTypeToEnum(typeString.c_str());

  switch (type) {
  case PSOType::DXR: {
    assert(0 && "Unsupported PSO type");
    break;
  }
  case PSOType::RASTER: {
    return processRasterPSO(jobj, renderPass, vertexInfo);
    break;
  }
  case PSOType::COMPUTE: {
    assert(0 && "Unsupported PSO type");
    break;
  }
  case PSOType::INVALID: {
    assert(0 && "Unsupported PSO type");
    break;
  }
  default:;
  }
  return nullptr;
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
                                   STATIC_SAMPLER_DESCRIPTOR_SET,
                                   STATIC_SAMPLER_LAYOUT);
}

} // namespace SirEngine::vk
