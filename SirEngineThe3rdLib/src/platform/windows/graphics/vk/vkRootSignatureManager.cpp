#include "vkRootSignatureManager.h"
#include "SirEngine/binary/binaryFile.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"

namespace SirEngine::vk {
VkDescriptorType getDescriptorType(const nlohmann::json &config) {
  const std::string type =
      getValueIfInJson(config, ROOT_KEY_TYPE, ROOT_DEFAULT_STRING);
  assert(!type.empty());
  const std::string resource =
      getValueIfInJson(config, ROOT_KEY_RESOURCE, ROOT_DEFAULT_STRING);
  assert(!resource.empty());

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

RSHandle VkPipelineLayoutManager::loadSignatureFile(
    const char *file, VkDescriptorSetLayout samplersLayout) {
  auto jobj = getJsonObj(file);

  const std::string name = getFileName(file);
  // From spec:  The pipeline layout represents a sequence of descriptor sets
  // with each having a specific layout.
  // this is the same as root signature in DX12
  VkPipelineLayoutCreateInfo layoutInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

  // check the number of bindings
  assert(jobj.find(ROOT_KEY_CONFIG) != jobj.end());
  auto config = jobj[ROOT_KEY_CONFIG];
  const int configLen = config.size();

  int allocSize = sizeof(VkDescriptorSetLayoutBinding) * configLen;
  // allocating enough memory of the set layout binding
  auto *bindings = reinterpret_cast<VkDescriptorSetLayoutBinding *>(
      globals::FRAME_ALLOCATOR->allocate(allocSize));
  // zeroing out
  memset(bindings, 0, allocSize);

  for (int i = 0; i < configLen; ++i) {
    const auto &currentConfigJ = config[i];
    const auto &ranges = currentConfigJ[ROOT_KEY_DATA][ROOT_KEY_RANGES];
    assert(ranges.size() == 1 &&
           "Currently ranges bigger than one are not supported in VK");
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

  // we are going to use multiple layouts if static samplers are requested
  // (which is always the case)
  bool useStaticSamplers =
      getValueIfInJson(jobj, ROOT_KEY_STATIC_SAMPLERS, false);
  // always allocating two layouts, and dynamically changing how many layouts we
  // use depending on user config
  VkDescriptorSetLayout layouts[2] = {descriptorLayout, samplersLayout};
  layoutInfo.setLayoutCount = 1 + (useStaticSamplers ? 1 : 0);
  layoutInfo.pSetLayouts = layouts;

  // generate the handle
  uint32_t index;
  LayoutData &rsdata = m_rsPool.getFreeMemoryData(index);
  // TODO need a manager
  vkCreatePipelineLayout(vk::LOGICAL_DEVICE, &layoutInfo, nullptr,
                         &rsdata.layout);

  const RSHandle handle{(MAGIC_NUMBER_COUNTER << 16) | index};
  rsdata.magicNumber = MAGIC_NUMBER_COUNTER;
  m_rootRegister.insert(name.c_str(), handle);
  ++MAGIC_NUMBER_COUNTER;

  // cleanup
  //descriptor layout can be deleted immediately after the creation of the pipeline
  //layout, the pipeline layout will still be valid
  vkDestroyDescriptorSetLayout(vk::LOGICAL_DEVICE, descriptorLayout, nullptr);

  return handle;
}

} // namespace SirEngine::vk