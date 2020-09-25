#pragma once

#include <assert.h>
#include <nlohmann/json_fwd.hpp>
#include <string>

#include "SirEngine/PSOManager.h"
#include "SirEngine/graphics/materialMetadata.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/cpu/resizableVector.h"
#include "SirEngine/memory/cpu/sparseMemoryPool.h"
#include "SirEngine/memory/cpu/stringHashMap.h"
#include "platform/windows/graphics/vk/vk.h"
#include "vkRootSignatureManager.h"

namespace SirEngine::vk {

struct SIR_ENGINE_API VkPSOCompileResult {
  VkPipeline pso = nullptr;
  PSO_TYPE psoType = PSO_TYPE::INVALID;
  const char *VSName = nullptr;
  const char *PSName = nullptr;
  const char *CSName = nullptr;
  const char *PSOFullPathFile = nullptr;
  const char *inputLayout = nullptr;
  const char *rootSignature = nullptr;
  TOPOLOGY_TYPE topologyType;
  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  graphics::MaterialMetadata metadata;
};

// TODO make it not copyable assignable
class VkPSOManager final : public PSOManager {
  struct PSOData {
    VkPipeline pso;
    VkRenderPass renderPass;
    VkPipelineLayout layout;
    RSHandle rootSignature;
    uint32_t magicNumber;
    TOPOLOGY_TYPE topology;
    graphics::MaterialMetadata metadata;
  };

 public:
  VkPSOManager()
      : PSOManager(),
        m_psoRegister(RESERVE_SIZE),
        m_psoRegisterHandle(RESERVE_SIZE),
        m_shaderToPSOFile(RESERVE_SIZE),
        m_psoPool(RESERVE_SIZE){};
  ~VkPSOManager() override = default;

  VkPSOManager(const VkPSOManager &) = delete;
  VkPSOManager &operator=(const VkPSOManager &) = delete;
  VkPSOManager(VkPSOManager &&) = delete;
  VkPSOManager &operator=(VkPSOManager &&) = delete;
  void initialize() override;
  void cleanup() override;
  void loadRawPSOInFolder(const char *directory) override;
  void loadCachedPSOInFolder(const char *directory) override;
  VkPSOCompileResult processComputePSO(const char *file,
                                       const nlohmann::json &jobj) const;
  VkPSOCompileResult compileRawPSO(const char *file) const;

  void recompilePSOFromShader(const char *shaderName,
                              const char *getOffsetPath) override;
  inline void bindPSO(const PSOHandle handle,
                      const VkCommandBuffer commandList) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    VkPipelineBindPoint bindPoint = data.topology != TOPOLOGY_TYPE::UNDEFINED
                                        ? VK_PIPELINE_BIND_POINT_GRAPHICS
                                        : VK_PIPELINE_BIND_POINT_COMPUTE;

    vkCmdBindPipeline(commandList, bindPoint, data.pso);
  }
  inline VkPipeline getPipelineFromHandle(const PSOHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return data.pso;
  }
  const graphics::MaterialMetadata *getMetadata(
      const PSOHandle &handle) override {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return &data.metadata;
  };

  [[nodiscard]] VkRenderPass getRenderPassFromHandle(
      const PSOHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return data.renderPass;
  }
  inline RSHandle getRootSignatureHandleFromPSOHandle(
      const PSOHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return data.rootSignature;
  }

  inline VkPipelineLayout getPipelineLayoutFromPSOHandle(
      const PSOHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return data.layout;
  }

  PSOHandle getHandleFromName(const char *name) const override;
  void bindPSO(const PSOHandle handle) const override {
    VkCommandBuffer buffer = vk::CURRENT_FRAME_COMMAND->m_commandBuffer;
    bindPSO(handle, buffer);
  }
  RSHandle getRS(const PSOHandle handle) const override {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return data.rootSignature;
  };

 private:
  PSOHandle insertInPSOCache(const VkPSOCompileResult &result);

  inline void assertMagicNumber(const PSOHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_psoPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for constant buffer");
  }

  VkPSOCompileResult processRasterPSO(const char *filePath,
                                      const nlohmann::json &jobj) const;

  void updatePSOCache(const char *name, const VkPSOCompileResult &result);

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
};
}  // namespace SirEngine::vk
