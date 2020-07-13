#include "vkRootSignatureManager.h"

#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"

namespace SirEngine::vk {

const std::string ROOT_KEY_CONFIG = "config";
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
    }
  }
}

void VkPipelineLayoutManager::loadSignaturesInFolder(const char *directory) {
  assert(0);
}

void VkPipelineLayoutManager::loadSignatureBinaryFile(const char *file) {
  assert(0);
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
  assert(jobj.find(ROOT_KEY_CONFIG) != jobj.end());
  auto config = jobj[ROOT_KEY_CONFIG];
  const uint32_t configLen = config.size();

  int allocSize = sizeof(VkDescriptorSetLayoutBinding) * configLen;
  // allocating enough memory of the set layout binding
  auto *bindings = reinterpret_cast<VkDescriptorSetLayoutBinding *>(
      globals::FRAME_ALLOCATOR->allocate(allocSize));
  // zeroing out
  memset(bindings, 0, allocSize);

  // we support at most 3 descriptor set
  // 0 = per frame data
  // 1 = static samplers
  // 2 = per object data
  // everything in the bindings will be for set 1, the object
  // the per  frame data is defined by the engine and will be a different
  // descriptor set

  for (int i = 0; i < configLen; ++i) {
    const auto &currentConfigJ = config[i];
    const auto &ranges = currentConfigJ[ROOT_KEY_DATA][ROOT_KEY_RANGES];
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

    bindings[i].binding = bindingIdx;
    bindings[i].descriptorType = descriptorType;
    bindings[i].descriptorCount = descriptorCount;
    bindings[i].stageFlags = getVisibilityFlags(currentConfigJ);
  }

  // passing in the "root signature"
  VkDescriptorSetLayoutCreateInfo descriptorInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};

  descriptorInfo.bindingCount = configLen;
  descriptorInfo.pBindings = bindings;

  // can be freed
  VkDescriptorSetLayout descriptorLayout;
  vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, &descriptorInfo, nullptr,
                              &descriptorLayout);
  SET_DEBUG_NAME(descriptorLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                 frameConcatenation(name.c_str(), "DescriptorLayout"));

  // we are going to use multiple layouts if static samplers are requested
  // (which is always the case)
  bool useStaticSamplers =
      getValueIfInJson(jobj, ROOT_KEY_STATIC_SAMPLERS, false);
  // always allocating two layouts, and dynamically changing how many layouts we
  // use depending on user config
  VkDescriptorSetLayout layouts[3] = {
      perFrameLayout,
      samplersLayout,
      descriptorLayout,
  };
  layoutInfo.setLayoutCount = 2 + (useStaticSamplers ? 1 : 0);
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
  // always allocating two layouts, and dynamically changing how many layouts we
  // use depending on user config
  // passing in the "root signature"
  VkDescriptorSetLayoutCreateInfo descriptorInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  descriptorInfo.bindingCount = 0;
  descriptorInfo.pBindings = nullptr;
  VkDescriptorSetLayout emptyLayout;
  vkCreateDescriptorSetLayout(vk::LOGICAL_DEVICE, &descriptorInfo, nullptr,
                              &emptyLayout);

  VkDescriptorSetLayout layouts[3] = {perFrameLayout,samplersLayout, emptyLayout};
  layoutInfo.setLayoutCount = 3;
  layoutInfo.pSetLayouts = layouts;

  VkPipelineLayout layout;
  vkCreatePipelineLayout(vk::LOGICAL_DEVICE, &layoutInfo, nullptr, &layout);
  return layout;
}
}  // namespace SirEngine::vk
