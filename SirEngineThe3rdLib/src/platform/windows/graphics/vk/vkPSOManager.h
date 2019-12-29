#pragma once

#include "SirEngine/PSOManager.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/resizableVector.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/memory/stringHashMap.h"
#include "platform/windows/graphics/vk/volk.h"

#include <nlohmann/json_fwd.hpp>

#include <cassert>
#include <string>
#include "vkRootSignatureManager.h"

namespace SirEngine::vk {

#define STATIC_SAMPLER_COUNT 7
extern VkSampler STATIC_SAMPLERS[STATIC_SAMPLER_COUNT];
extern VkDescriptorImageInfo STATIC_SAMPLERS_INFO[STATIC_SAMPLER_COUNT];
extern VkDescriptorSetLayout STATIC_SAMPLER_LAYOUT;
extern VkDescriptorSet
    STATIC_SAMPLER_DESCRIPTOR_SET; // used in case you want to manually update
                                   // the samplers and not bound them as static
extern VkDescriptorSetLayout PER_FRAME_LAYOUT;
extern VkDescriptorSet* PER_FRAME_DESCRIPTOR_SET;

// VkPipeline
// createGraphicsPipeline(const char *psoPath, VkDevice logicalDevice,
//                       VkRenderPass &renderPass,
//                       VkPipelineVertexInputStateCreateInfo *vertexInfo);
void initStaticSamplers();
void createStaticSamplerDescriptorSet(VkDescriptorPool &pool,
                                      VkDescriptorSet &outSet,
                                      VkDescriptorSetLayout &layout);
void destroyStaticSamplers();

// TODO make it not copyable assignable
class VkPSOManager final : public PSOManager {

  struct PSOData {
    VkPipeline pso;
    VkRenderPass renderPass;
    RSHandle rootSignature;
    uint32_t magicNumber;
  };

public:
  VkPSOManager()
      : PSOManager(), m_psoRegister(RESERVE_SIZE),
        m_psoRegisterHandle(RESERVE_SIZE), m_shaderToPSOFile(RESERVE_SIZE),
        m_psoPool(RESERVE_SIZE){};
  virtual ~VkPSOManager() = default;

  VkPSOManager(const VkPSOManager &) = delete;
  VkPSOManager &operator=(const VkPSOManager &) = delete;
  void initialize() override;
  void cleanup() override;
  void loadRawPSOInFolder(const char *directory) override;
  void loadCachedPSOInFolder(const char *directory) override;
  PSOHandle loadRawPSO(const char *file);

  void recompilePSOFromShader(const char *shaderName,
                              const char *getOffsetPath) override;
  inline void bindPSO(const PSOHandle handle,
                      VkCommandBuffer commandList) const {

    assert(0);
    // assertMagicNumber(handle);
    // const uint32_t index = getIndexFromHandle(handle);
    // const PSOData &data = m_psoPool.getConstRef(index);
    // commandList->SetPipelineState(data.pso);
  }
  inline VkPipeline getPipelineFromHandle(const PSOHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return data.pso;
  }
  VkRenderPass getRenderPassFromHandle(const PSOHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return data.renderPass;
  }
  RSHandle getRootSignatureHandleFromPSOHandle(const PSOHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return data.rootSignature;
  }

  inline PSOHandle getHandleFromName(const char *name) const {

    assert(m_psoRegisterHandle.containsKey(name));
    PSOHandle value{};
    m_psoRegisterHandle.get(name, value);
    return value;
  }

private:
  // PSOCompileResult processComputePSO(nlohmann::json &jobj,
  //                                   const std::string &path);
  // PSOCompileResult processRasterPSO(nlohmann::json &jobj,
  //                                  const std::string &path);

  // void processGlobalRootSignature(nlohmann::json &jobj,
  //                                CD3DX12_STATE_OBJECT_DESC &pipe) const;
  // void processPipelineConfig(nlohmann::json &jobj,
  //                           CD3DX12_STATE_OBJECT_DESC &pipe) const;
  // PSOCompileResult loadCachedPSO(const char *path);

private:
  // void updatePSOCache(const char *name, ID3D12PipelineState *pso);
  // void insertInPSOCache(const PSOCompileResult &result);

  inline void assertMagicNumber(const PSOHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_psoPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for constant buffer");
  }

  PSOHandle processRasterPSO(const char *filePath, const nlohmann::json &jobj,
                             VkPipelineVertexInputStateCreateInfo *vertexInfo);

private:
  HashMap<const char *, VkPipeline, hashString32> m_psoRegister;
  HashMap<const char *, PSOHandle, hashString32> m_psoRegisterHandle;

  HashMap<const char *, ResizableVector<const char *> *, hashString32>
      m_shaderToPSOFile;

  // this is only used for the hot recompilation
  // TODO deal with this, remove from header
  std::string compileLog;

  // handles
  SparseMemoryPool<PSOData> m_psoPool;
  uint32_t MAGIC_NUMBER_COUNTER = 1;
  static const uint32_t RESERVE_SIZE = 400;
  static const uint32_t INDEX_MASK = (1 << 16) - 1;
  static const uint32_t MAGIC_NUMBER_MASK = ~INDEX_MASK;
}; // namespace SirEngine::vk
} // namespace SirEngine::vk
