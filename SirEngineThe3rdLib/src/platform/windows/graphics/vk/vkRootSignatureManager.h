#pragma once
#include "SirEngine/handle.h"
#include "SirEngine/hashing.h"
#include "SirEngine/memory/SparseMemoryPool.h"
#include "SirEngine/memory/stringHashMap.h"
#include "volk.h"
#include <cassert>
#include "SirEngine/fileUtils.h"
#include "vk.h"

namespace SirEngine::vk {

const std::string ROOT_KEY_CONFIG = "config";
const std::string ROOT_KEY_NAME = "name";
const std::string ROOT_KEY_TYPE = "type";
const std::string ROOT_KEY_DATA = "data";
const std::string ROOT_KEY_RESOURCE = "underlyingResource";
const std::string ROOT_KEY_NUM_DESCRIPTOR = "numDescriptors";
const std::string ROOT_KEY_RANGES = "ranges";
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
    };

const std::unordered_map<std::string, VkShaderStageFlags>
    STRING_TO_SHADER_FLAGS{
        {"VS", VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT},
        {"PS", VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT},
        {"CS", VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT},
        {"ALL", VkShaderStageFlagBits::VK_SHADER_STAGE_ALL},
    };

enum class ROOT_FILE_TYPE { RASTER = 0, COMPUTE = 1, DXR = 2, NULL_TYPE };

class VkPipelineLayoutManager final {

public:
  VkPipelineLayoutManager()
      : m_rootRegister(RESERVE_SIZE), m_rsPool(RESERVE_SIZE){};
  VkPipelineLayoutManager(const VkPipelineLayoutManager &) = delete;
  VkPipelineLayoutManager &operator=(const VkPipelineLayoutManager &) = delete;
  ~VkPipelineLayoutManager() = default;
  void cleanup();
  void loadSignaturesInFolder(const char *directory);
  void loadSignatureBinaryFile(const char *file);
  RSHandle loadSignatureFile(const char *file, VkDescriptorSetLayout samplersLayout);

  inline VkPipelineLayout getLayoutFromName(const char *name) const {

    const RSHandle handle = getHandleFromName(name);
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const LayoutData &data = m_rsPool.getConstRef(index);
    return data.layout;
  }
  inline VkPipelineLayout getLayoutFromHandle(const RSHandle handle) const {

    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const LayoutData &data = m_rsPool.getConstRef(index);
    return data.layout;
  }

  inline void bindGraphicsRS(const RSHandle handle,
                             VkCommandBuffer *commandList) const {

    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const LayoutData &data = m_rsPool.getConstRef(index);
    assert(0);
    // commandList->SetGraphicsLayout(data.rs);
  }

  inline RSHandle getHandleFromName(const char *name) const {
    assert(m_rootRegister.containsKey(name));
    RSHandle value;
    m_rootRegister.get(name, value);
    return value;
  }

  //mostly to keep API uniform
  void init(){};

private:
  inline uint32_t getIndexFromHandle(const RSHandle h) const {
    return h.handle & INDEX_MASK;
  }
  inline uint32_t getMagicFromHandle(const RSHandle h) const {
    return (h.handle & MAGIC_NUMBER_MASK) >> 16;
  }
  inline void assertMagicNumber(const RSHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_rsPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for pipeline layout");
  }

private:
  struct LayoutData {
    VkPipelineLayout layout;
    uint32_t magicNumber;
  };

  HashMap<const char *, RSHandle, hashString32> m_rootRegister;
  // handles
  SparseMemoryPool<LayoutData> m_rsPool;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
};


} // namespace SirEngine::vk
