#include "vkRootSignatureManager.h"

#include "SirEngine/graphics/materialMetadata.h"
#include "SirEngine/io/binaryFile.h"
#include "SirEngine/io/fileUtils.h"
#include "SirEngine/log.h"
#include "SirEngine/materialManager.h"
#include "platform/windows/graphics/vk/vk.h"
#include "platform/windows/graphics/vk/vkBindingTableManager.h"

namespace SirEngine::vk {

VkSampler STATIC_SAMPLERS[STATIC_SAMPLER_COUNT];
VkDescriptorImageInfo STATIC_SAMPLERS_INFO[STATIC_SAMPLER_COUNT];
VkDescriptorSetLayout STATIC_SAMPLERS_LAYOUT;
VkDescriptorSet STATIC_SAMPLERS_DESCRIPTOR_SET;
VkDescriptorSetLayout PER_FRAME_LAYOUT = nullptr;
DescriptorHandle PER_FRAME_DATA_HANDLE;
DescriptorHandle STATIC_SAMPLERS_HANDLE;
VkPipelineLayout ENGINE_PIPELINE_LAYOUT;

const char *STATIC_SAMPLERS_NAMES[STATIC_SAMPLER_COUNT] = {
    "pointWrapSampler",   "pointClampSampler",      "linearWrapSampler",
    "linearClampSampler", "anisotropicWrapSampler", "anisotropicClampSampler",
    "pcfSampler"};

std::array<const VkSamplerCreateInfo, STATIC_SAMPLER_COUNT>
getStaticSamplersCreateInfo() {
  // Applications usually only need a handful of samplers.  So just define them
  // all up front and keep them available as part of the root signature.

  const VkSamplerCreateInfo pointWrap{
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
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
      1,
      false,
      VK_COMPARE_OP_NEVER,  // should not matter since compare is off
      0.0,
      20.0};
  const VkSamplerCreateInfo pointClamp{
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
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
      1,
      false,
      VK_COMPARE_OP_NEVER,  // should not matter since compare is off
      0.0,
      20.0};
  const VkSamplerCreateInfo linearWrap{
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      nullptr,
      0,
      VK_FILTER_LINEAR,
      VK_FILTER_LINEAR,
      VK_SAMPLER_MIPMAP_MODE_LINEAR,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT,
      0.0f,
      VK_FALSE,
      1,
      false,
      VK_COMPARE_OP_NEVER,  // should not matter since compare is off
      0.0,
      20.0};
  const VkSamplerCreateInfo linearClamp{
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      nullptr,
      0,
      VK_FILTER_LINEAR,
      VK_FILTER_LINEAR,
      VK_SAMPLER_MIPMAP_MODE_LINEAR,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      0.0f,
      VK_FALSE,
      1,
      false,
      VK_COMPARE_OP_NEVER,  // should not matter since compare is off
      0.0,
      20.0};
  const VkSamplerCreateInfo anisotropicWrap{
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      nullptr,
      0,
      VK_FILTER_LINEAR,
      VK_FILTER_LINEAR,
      VK_SAMPLER_MIPMAP_MODE_LINEAR,
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
      VK_SAMPLER_MIPMAP_MODE_LINEAR,
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

void createPerFrameDataDescriptorSet(VkDescriptorSetLayout &layout) {
  // here we are are creating the layout, but we are using static samplers
  // so we are passing immutable samplers directly in the layout that
  // gets built in the graphics pipeline
  VkDescriptorSetLayoutBinding resourceBinding[2] = {};
  resourceBinding[0].binding = 0;
  resourceBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  resourceBinding[0].descriptorCount = 1;
  resourceBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT |
                                  VK_SHADER_STAGE_COMPUTE_BIT;
  resourceBinding[0].pImmutableSamplers = nullptr;

  resourceBinding[1].binding = 1;
  resourceBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  resourceBinding[1].descriptorCount = 1;
  resourceBinding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT |
                                  VK_SHADER_STAGE_COMPUTE_BIT;
  resourceBinding[1].pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo resourceLayoutInfo[1] = {};
  resourceLayoutInfo[0].sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  resourceLayoutInfo[0].pNext = nullptr;
  resourceLayoutInfo[0].bindingCount = 2;
  resourceLayoutInfo[0].pBindings = resourceBinding;

  VK_CHECK(vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, resourceLayoutInfo,
                                       NULL, &layout));
  SET_DEBUG_NAME(layout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                 "perFrameDataDescriptorSetLayout");

  PER_FRAME_DATA_HANDLE = vk::DESCRIPTOR_MANAGER->allocate(
      layout, graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED,
      "perFrameDataDescriptor");
}

void createStaticSamplerDescriptorSet() {
  // new
  VkDescriptorSetLayoutBinding resourceBinding[STATIC_SAMPLER_COUNT] = {};
  for (int i = 0; i < STATIC_SAMPLERS_COUNT; ++i) {
    resourceBinding[i].binding = i;
    resourceBinding[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    resourceBinding[i].descriptorCount = 1;
    resourceBinding[i].stageFlags =
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
    resourceBinding[i].pImmutableSamplers = &STATIC_SAMPLERS[i];
  }
  VkDescriptorSetLayoutCreateInfo resourceLayoutInfo[1] = {};
  resourceLayoutInfo[0].sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  resourceLayoutInfo[0].pNext = nullptr;
  resourceLayoutInfo[0].bindingCount = STATIC_SAMPLERS_COUNT;
  resourceLayoutInfo[0].pBindings = resourceBinding;

  VK_CHECK(vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, resourceLayoutInfo,
                                       NULL, &STATIC_SAMPLERS_LAYOUT));
  SET_DEBUG_NAME(STATIC_SAMPLERS_LAYOUT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                 "staticSamplersDescriptorSetLayout");

  STATIC_SAMPLERS_HANDLE = vk::DESCRIPTOR_MANAGER->allocate(
      STATIC_SAMPLERS_LAYOUT,
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_NONE, "staticSamplers");
  STATIC_SAMPLERS_DESCRIPTOR_SET =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(STATIC_SAMPLERS_HANDLE);

  /*
  // here we are are creating the layout, but we are using static samplers
  // so we are passing immutable samplers directly in the layout that
  // gets built in the graphics pipeline
  VkDescriptorSetLayoutBinding resourceBinding[1] = {};
  resourceBinding[0].binding = 0;
  resourceBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  resourceBinding[0].descriptorCount = STATIC_SAMPLER_COUNT;
  resourceBinding[0].stageFlags =
      VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
  //resourceBinding[0].pImmutableSamplers = STATIC_SAMPLERS;

  VkDescriptorSetLayoutCreateInfo resourceLayoutInfo[1] = {};
  resourceLayoutInfo[0].sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  resourceLayoutInfo[0].pNext = nullptr;
  resourceLayoutInfo[0].bindingCount = 1;
  resourceLayoutInfo[0].pBindings = resourceBinding;

  VK_CHECK(vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, resourceLayoutInfo,
                                       NULL, &STATIC_SAMPLERS_LAYOUT));
  SET_DEBUG_NAME(STATIC_SAMPLERS_LAYOUT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                 "staticSamplersDescriptorSetLayout");

  STATIC_SAMPLERS_HANDLE = vk::DESCRIPTOR_MANAGER->allocate(
      STATIC_SAMPLERS_LAYOUT,
      graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_NONE, "staticSamplers");
  STATIC_SAMPLERS_DESCRIPTOR_SET =
      vk::DESCRIPTOR_MANAGER->getDescriptorSet(STATIC_SAMPLERS_HANDLE);
      */
}

void destroyStaticSamplers() {
  for (int i = 0; i < STATIC_SAMPLER_COUNT; ++i) {
    vkDestroySampler(vk::LOGICAL_DEVICE, STATIC_SAMPLERS[i], nullptr);
  }
  vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE, STATIC_SAMPLERS_LAYOUT,
                               nullptr);
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
  createStaticSamplerDescriptorSet();
}

void initPerFrameDataDescriptor() {
  createPerFrameDataDescriptorSet(PER_FRAME_LAYOUT);
};

const std::string ROOT_KEY_CONFIG = "config";
const std::string ROOT_KEY_PASS_CONFIG = "passConfig";
const std::string ROOT_KEY_NAME = "name";
const std::string ROOT_KEY_TYPE = "type";
const std::string ROOT_KEY_RESOURCE = "underlyingResource";
const std::string ROOT_KEY_NUM_DESCRIPTOR = "numDescriptors";
const std::string ROOT_KEY_SET = "sets";
const std::string ROOT_KEY_REGISTER = "register";
const std::string ROOT_KEY_BASE_REGISTER = "baseRegister";
const std::string ROOT_KEY_SIZE_IN_32_BIT_VALUES = "sizeIn32BitValues";
const std::string ROOT_KEY_VISIBILITY = "visibility";
const std::string ROOT_KEY_FLAGS = "flags";
const std::string ROOT_KEY_FLAGS_LOCAL = "local";
const std::string ROOT_EMPTY = "empty";
const std::string ROOT_DEFAULT_STRING = "";
const std::string ROOT_KEY_STATIC_SAMPLERS = "staticSamplers";

const std::unordered_map<std::string, VkDescriptorType>
    STRING_TO_DESCRIPTOR_TYPE{
        {"UAV-buffer", VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
        {"SRV-texture", VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
        {"SRV-buffer", VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
        {"CBV-buffer", VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
    };

const std::unordered_map<std::string, VkShaderStageFlags>
    STRING_TO_SHADER_FLAGS{
        {"VS", VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT},
        {"PS", VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT},
        {"CS", VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT},
        {"ALL", VkShaderStageFlagBits::VK_SHADER_STAGE_ALL},
    };

VkDescriptorSetLayout getEmptyLayout(const char *name) {
  VkDescriptorSetLayoutCreateInfo descriptorInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  descriptorInfo.bindingCount = 0;
  descriptorInfo.pBindings = nullptr;
  VkDescriptorSetLayout emptyLayout;
  vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, &descriptorInfo, nullptr,
                              &emptyLayout);

  SET_DEBUG_NAME(emptyLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, name);
  return emptyLayout;
}

VkDescriptorType getDescriptorType(const graphics::MATERIAL_RESOURCE_TYPE type,
                                   const VkShaderStageFlags flags) {
  switch (type) {
    case graphics::MATERIAL_RESOURCE_TYPE::BUFFER: {
      return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    case graphics::MATERIAL_RESOURCE_TYPE::TEXTURE: {
      if ((flags & VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT) > 0) {
        return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      }
      return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }
    case graphics::MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER: {
      return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
    default: {
      assert(0 && "unsupported material resource type");
      return VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
  }
}

VkShaderStageFlags getVisibilityFlags2(
    const GRAPHIC_RESOURCE_VISIBILITY visibility) {
  VkShaderStageFlags flags = 0;

  flags |= (visibility & GRAPHICS_RESOURCE_VISIBILITY_VERTEX) > 0
               ? VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT
               : 0;
  flags |= (visibility & GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT) > 0
               ? VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT
               : 0;
  flags |= (visibility & GRAPHICS_RESOURCE_VISIBILITY_COMPUTE) > 0
               ? VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT
               : 0;
  return flags;
}

void VkPipelineLayoutManager::initialize() {
  vk::initPerFrameDataDescriptor();
  vk::initStaticSamplers();
}

void VkPipelineLayoutManager::cleanup() {
  destroyStaticSamplers();
  int count = m_rootRegister.binCount();
  for (int i = 0; i < count; ++i) {
    if (m_rootRegister.isBinUsed(i)) {
      const char *key = m_rootRegister.getKeyAtBin(i);
      RSHandle handle{};
      m_rootRegister.get(key, handle);
      // now that we have the handle we can get the data
      assertMagicNumber(handle);
      const uint32_t index = getIndexFromHandle(handle);
      const LayoutData &data = m_rsPool.getConstRef(index);
      vkDestroyPipelineLayout(vk::LOGICAL_DEVICE, data.layout, nullptr);
      vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE, data.descriptorSetLayout,
                                   nullptr);
      vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE, data.passSetLayout,
                                   nullptr);
    }
  }
}

void processConfig(const graphics::MaterialResource &meta,
                   VkDescriptorSetLayoutBinding *binding) {
  binding->stageFlags = getVisibilityFlags2(meta.visibility);
  VkDescriptorType descriptorType =
      getDescriptorType(meta.type, binding->stageFlags);
  assert(descriptorType != VK_DESCRIPTOR_TYPE_MAX_ENUM);

  binding->binding = meta.binding;
  binding->descriptorType = descriptorType;
  binding->descriptorCount = 1;
}

RSHandle VkPipelineLayoutManager::loadSignatureFile(
    const char *name, graphics::MaterialMetadata *metadata) {
  // From spec:  The pipeline layout represents a sequence of descriptor sets
  // with each having a specific layout.
  // this is the same as root signature in DX12
  VkPipelineLayoutCreateInfo layoutInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

  VkPushConstantRange pushRange{};
  // check the number of bindings
  bool hasConfig = metadata->objectResourceCount != 0;
  VkDescriptorSetLayout descriptorLayout = nullptr;
  if (hasConfig) {
    int pushConstantOffset = metadata->hasObjectPushConstant() ? 1 : 0;
    int allocSize =
        sizeof(VkDescriptorSetLayoutBinding) * metadata->objectResourceCount -
        pushConstantOffset;
    // allocating enough memory of the set layout binding
    auto *bindings = reinterpret_cast<VkDescriptorSetLayoutBinding *>(
        globals::FRAME_ALLOCATOR->allocate(allocSize));
    // zeroing out
    memset(bindings, 0, allocSize);

    // we support at most 3 descriptor set
    // 0 = per frame data
    // 1 = static samplers
    // 2 = per pass data
    // 3 = per object data
    // everything in the config bindings will be for per object data, the
    // object the per  frame data is defined by the engine and will be a
    // different descriptor set

    for (uint32_t i = 0; i < metadata->objectResourceCount - pushConstantOffset;
         ++i) {
      // if we have a push constant we are going to skip it
      const auto &meta = metadata->objectResources[i + pushConstantOffset];
      processConfig(meta, &bindings[i]);
    }
    // passing in the "root signature"
    VkDescriptorSetLayoutCreateInfo descriptorInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};

    descriptorInfo.bindingCount =
        metadata->objectResourceCount - pushConstantOffset;
    descriptorInfo.pBindings = bindings;

    // can be freed
    vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, &descriptorInfo, nullptr,
                                &descriptorLayout);
    SET_DEBUG_NAME(descriptorLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                   frameConcatenation(name, "DescriptorLayout"));

    // setup push constants
    if (pushConstantOffset != 0) {
      // if we are here it means we have push constant to deal with
      const graphics::MaterialResource &meta = metadata->objectResources[0];
      assert(meta.type == graphics::MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER);
      layoutInfo.pushConstantRangeCount = 1;
      layoutInfo.pPushConstantRanges = &pushRange;
      pushRange.size = meta.extension.uniform.structSize;
      pushRange.offset = 0;
      pushRange.stageFlags = 0;
      pushRange.stageFlags |=
          (meta.visibility & GRAPHICS_RESOURCE_VISIBILITY_VERTEX) > 0
              ? VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT
              : 0;
      pushRange.stageFlags |=
          (meta.visibility & GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT) > 0
              ? VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT
              : 0;
      pushRange.stageFlags |=
          (meta.visibility & GRAPHICS_RESOURCE_VISIBILITY_COMPUTE) > 0
              ? VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT
              : 0;
    }
  } else {
    const char *passname =
        frameConcatenation(name, "PerObjectDescriptorLayoutEmpty");
    descriptorLayout = getEmptyLayout(passname);
  }

  bool passFound = metadata->passResourceCount != 0;
  VkDescriptorSetLayout passDescriptorLayout = nullptr;
  bool usePassEmpty = true;
  if (passFound) {
    const auto passConfigLen = metadata->passResourceCount;

    if (passConfigLen > 0) {
      // if we have both a passConfig and is not empty we are going to process
      // it and not use an empty pass config
      usePassEmpty = false;
      int passAllocSize = sizeof(VkDescriptorSetLayoutBinding) * passConfigLen;
      // allocating enough memory of the set layout binding
      auto *passBindings = static_cast<VkDescriptorSetLayoutBinding *>(
          globals::FRAME_ALLOCATOR->allocate(passAllocSize));
      // zeroing out
      memset(passBindings, 0, passAllocSize);
      for (uint32_t i = 0; i < passConfigLen; ++i) {
        const auto &currentConfigJ = metadata->passResources[i];
        processConfig(currentConfigJ, &passBindings[i]);
      }

      // passing in the "root signature"
      VkDescriptorSetLayoutCreateInfo passDescriptorInfo{
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};

      passDescriptorInfo.bindingCount = passConfigLen;
      passDescriptorInfo.pBindings = passBindings;

      vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, &passDescriptorInfo,
                                  nullptr, &passDescriptorLayout);
      SET_DEBUG_NAME(passDescriptorLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                     frameConcatenation(name, "PassDescriptorLayout"));
    }
  }

  if (usePassEmpty) {
    const char *passname =
        frameConcatenation(name, "PassDescriptorLayoutEmpty");
    passDescriptorLayout = getEmptyLayout(passname);
  }

  // we always bind the samplers, worst case we don't bind them, we might
  // want to optimize this in the future
  bool useStaticSamplers = true;
  VkDescriptorSetLayout layouts[4] = {
      PER_FRAME_LAYOUT,
      STATIC_SAMPLERS_LAYOUT,
      passDescriptorLayout,
      descriptorLayout,
  };
  layoutInfo.setLayoutCount = 3 + (useStaticSamplers ? 1 : 0);
  layoutInfo.pSetLayouts = layouts;

  // generate the handle
  uint32_t index;
  LayoutData &rsdata = m_rsPool.getFreeMemoryData(index);
  vkCreatePipelineLayout(vk::LOGICAL_DEVICE, &layoutInfo, nullptr,
                         &rsdata.layout);
  SET_DEBUG_NAME(rsdata.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT,
                 frameConcatenation(name, "PipelineLayout"));

  const RSHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
  rsdata.magicNumber = MAGIC_NUMBER_COUNTER;
  rsdata.descriptorSetLayout = descriptorLayout;
  rsdata.usesStaticSamplers = useStaticSamplers;
  rsdata.passSetLayout = passDescriptorLayout;
  m_rootRegister.insert(name, handle);
  ++MAGIC_NUMBER_COUNTER;

  return handle;
}

RSHandle VkPipelineLayoutManager::getHandleFromName(const char *name) const {
  bool found = m_rootRegister.containsKey(name);
  // assert();
  if (!found) {
    SE_CORE_ERROR("could not find RS handle for {0}", name);
  }
  RSHandle value;
  m_rootRegister.get(name, value);
  return value;
}

VkPipelineLayout VkPipelineLayoutManager::createEngineLayout(
    const VkDescriptorSetLayout perFrameLayout,
    const VkDescriptorSetLayout samplersLayout) {
  // From spec:  The pipeline layout represents a sequence of descriptor sets
  // with each having a specific layout.
  // this is the same as root signature in DX12
  VkPipelineLayoutCreateInfo layoutInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

  // always allocating two layouts, and dynamically changing how many layouts
  // we use depending on user config passing in the "root signature"
  VkDescriptorSetLayout layouts[2] = {perFrameLayout, samplersLayout};
  layoutInfo.setLayoutCount = 2;
  layoutInfo.pSetLayouts = layouts;

  VkPipelineLayout layout;
  vkCreatePipelineLayout(vk::LOGICAL_DEVICE, &layoutInfo, nullptr, &layout);
  return layout;
}
}  // namespace SirEngine::vk
