#include "vkRootSignatureManager.h"

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"

namespace SirEngine::vk {

const std::string ROOT_KEY_CONFIG = "config";
const std::string ROOT_KEY_PASS_CONFIG = "passConfig";
const std::string ROOT_KEY_NAME = "name";
const std::string ROOT_KEY_TYPE = "type";
const std::string ROOT_KEY_DATA = "data";
const std::string ROOT_KEY_RESOURCE = "underlyingResource";
const std::string ROOT_KEY_NUM_DESCRIPTOR = "numDescriptors";
const std::string ROOT_KEY_RANGES = "ranges";
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
        {"UAV", VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
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

VkDescriptorType getDescriptorType(const nlohmann::json &config) {
  const std::string type =
      getValueIfInJson(config, ROOT_KEY_TYPE, ROOT_DEFAULT_STRING);
  assert(!type.empty());
  const std::string resource =
      getValueIfInJson(config, ROOT_KEY_RESOURCE, ROOT_DEFAULT_STRING);
  if (type == "SRV") {
    assert(!resource.empty());
  }

  const std::string actualType = type + "-" + resource;
  // sampler the map
  const auto found = STRING_TO_DESCRIPTOR_TYPE.find(actualType);
  if (found != STRING_TO_DESCRIPTOR_TYPE.end()) {
    return found->second;
  }

  return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

VkShaderStageFlags getVisibilityFlags(const nlohmann::json &jobj) {
  const auto found = jobj.find(ROOT_KEY_VISIBILITY);
  if (found == jobj.end()) {
    return 0;
  }

  const auto &visibility = found.value();
  const auto flagFound =
      STRING_TO_SHADER_FLAGS.find(visibility.get<std::string>());
  if (flagFound != STRING_TO_SHADER_FLAGS.end()) {
    return flagFound->second;
  }

  assert(0 && "could not find requested shader flags");
  return 0;
}

void VkPipelineLayoutManager::cleanup() {
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

void VkPipelineLayoutManager::loadSignaturesInFolder(const char *directory) {
  assert(0);
}

void VkPipelineLayoutManager::loadSignatureBinaryFile(const char *file) {
  assert(0);
}

void processConfig(const nlohmann::json &jobj,
                   VkDescriptorSetLayoutBinding *binding) {
  const auto &ranges = jobj[ROOT_KEY_DATA][ROOT_KEY_RANGES];
  assert(
      ranges.size() == 1 &&
      "Currently ranges bigger than one are not supported in the VK backend");
  const auto &range = ranges[0];

  int bindingIdx = getValueIfInJson(range, ROOT_KEY_BASE_REGISTER, -1);
  int descriptorCount = getValueIfInJson(range, ROOT_KEY_NUM_DESCRIPTOR, -1);

  assert(bindingIdx != -1);
  assert(descriptorCount != -1);
  VkDescriptorType descriptorType = getDescriptorType(range);
  assert(descriptorType != VK_DESCRIPTOR_TYPE_MAX_ENUM);

  binding->binding = bindingIdx;
  binding->descriptorType = descriptorType;
  binding->descriptorCount = descriptorCount;
  binding->stageFlags = getVisibilityFlags(jobj);
}

RSHandle VkPipelineLayoutManager::loadSignatureFile(
    const char *file, const VkDescriptorSetLayout perFrameLayout,
    const VkDescriptorSetLayout samplersLayout) {
  const std::string name = getFileName(file);

  RSHandle cachedHandle;
  if (m_rootRegister.get(name.c_str(), cachedHandle)) {
    return cachedHandle;
  }

  auto jobj = getJsonObj(file);

  // From spec:  The pipeline layout represents a sequence of descriptor sets
  // with each having a specific layout.
  // this is the same as root signature in DX12
  VkPipelineLayoutCreateInfo layoutInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

  // check the number of bindings
  bool hasConfig = jobj.find(ROOT_KEY_CONFIG) != jobj.end();
  VkDescriptorSetLayout descriptorLayout = nullptr;
  bool useConfigEmpty = true;
  if (hasConfig) {
    auto config = jobj[ROOT_KEY_CONFIG];
    const uint32_t configLen = config.size();
    if (configLen > 0) {
      useConfigEmpty = false;
      int allocSize = sizeof(VkDescriptorSetLayoutBinding) * configLen;
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

      for (uint32_t i = 0; i < configLen; ++i) {
        const auto &currentConfigJ = config[i];
        processConfig(currentConfigJ, &bindings[i]);
      }
      // passing in the "root signature"
      VkDescriptorSetLayoutCreateInfo descriptorInfo{
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};

      descriptorInfo.bindingCount = configLen;
      descriptorInfo.pBindings = bindings;

      // can be freed
      vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, &descriptorInfo, nullptr,
                                  &descriptorLayout);
      SET_DEBUG_NAME(descriptorLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                     frameConcatenation(name.c_str(), "DescriptorLayout"));
    }
  } else {
    const char *passname =
        frameConcatenation(name.c_str(), "PerObjectDescriptorLayoutEmpty");
    descriptorLayout = getEmptyLayout(passname);
  }

  bool passFound = jobj.find(ROOT_KEY_PASS_CONFIG) != jobj.end();
  VkDescriptorSetLayout passDescriptorLayout = nullptr;
  bool usePassEmpty = true;
  if (passFound) {
    auto passConfig = jobj[ROOT_KEY_PASS_CONFIG];
    const uint32_t passConfigLen = passConfig.size();

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
      for (int i = 0; i < passConfigLen; ++i) {
        const auto &currentConfigJ = passConfig[i];
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
                     frameConcatenation(name.c_str(), "PassDescriptorLayout"));
    }
  }

  if (usePassEmpty) {
    const char *passname =
        frameConcatenation(name.c_str(), "PassDescriptorLayoutEmpty");
    passDescriptorLayout = getEmptyLayout(passname);
  }

  // we are going to use multiple layouts if static samplers are requested
  // (which is always the case)
  bool useStaticSamplers =
      getValueIfInJson(jobj, ROOT_KEY_STATIC_SAMPLERS, false);
  // always allocating two layouts, and dynamically changing how many layouts
  // we use depending on user config
  assert(
      useStaticSamplers &&
      "if not using static sampler we need an empty layout for the samplers");
  VkDescriptorSetLayout layouts[4] = {
      perFrameLayout,
      samplersLayout,
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
                 frameConcatenation(name.c_str(), "PipelineLayout"));

  const RSHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
  rsdata.magicNumber = MAGIC_NUMBER_COUNTER;
  rsdata.descriptorSetLayout = descriptorLayout;
  rsdata.usesStaticSamplers = useStaticSamplers;
  rsdata.passSetLayout = passDescriptorLayout;
  m_rootRegister.insert(name.c_str(), handle);
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

  // we are going to use multiple layouts if static samplers are requested
  // (which is always the case)
  bool useStaticSamplers = true;
  // always allocating two layouts, and dynamically changing how many layouts
  // we use depending on user config passing in the "root signature"
  VkDescriptorSetLayout emptyLayout =
      getEmptyLayout("engineDescriptorSetLayout");

  VkDescriptorSetLayout layouts[3] = {perFrameLayout, samplersLayout,
                                      emptyLayout};
  layoutInfo.setLayoutCount = 3;
  layoutInfo.pSetLayouts = layouts;

  VkPipelineLayout layout;
  vkCreatePipelineLayout(vk::LOGICAL_DEVICE, &layoutInfo, nullptr, &layout);
  // destroying the descriptor set layout immediately since wont be needed
  vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE, emptyLayout, nullptr);
  return layout;
}
}  // namespace SirEngine::vk
