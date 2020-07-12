#pragma once

#include <cassert>
#include <nlohmann/json_fwd.hpp>
#include <string>

#include "SirEngine/PSOManager.h"
#include "SirEngine/handle.h"
#include "SirEngine/memory/resizableVector.h"
#include "SirEngine/memory/sparseMemoryPool.h"
#include "SirEngine/memory/stringHashMap.h"
#include "platform/windows/graphics/vk/volk.h"
#include "vkRootSignatureManager.h"

namespace SirEngine::vk {
// TODO move this to graphics define and use the same for both dx12 and vk
enum class PSO_TYPE { DXR = 0, RASTER, COMPUTE, INVALID };

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
};

#define STATIC_SAMPLER_COUNT 7
extern VkSampler STATIC_SAMPLERS[STATIC_SAMPLER_COUNT];
extern VkDescriptorImageInfo STATIC_SAMPLERS_INFO[STATIC_SAMPLER_COUNT];
extern VkDescriptorSetLayout STATIC_SAMPLERS_LAYOUT;
extern VkDescriptorSet
    STATIC_SAMPLERS_DESCRIPTOR_SET;  // used in case you want to manually update
                                     // the samplers and not bound them as
                                     // static
extern VkDescriptorSetLayout PER_FRAME_LAYOUT;
extern DescriptorHandle PER_FRAME_DATA_HANDLE;
extern DescriptorHandle STATIC_SAMPLERS_HANDLE;

void initStaticSamplers();
void destroyStaticSamplers();

// TODO make it not copyable assignable
class VkPSOManager final : public PSOManager {
  struct PSOData {
    VkPipeline pso;
    VkRenderPass renderPass;
    VkPipelineLayout layout;
    RSHandle rootSignature;
    uint32_t magicNumber;
    TOPOLOGY_TYPE topology;
  };

 public:
  VkPSOManager()
      : PSOManager(),
        m_psoRegister(RESERVE_SIZE),
        m_psoRegisterHandle(RESERVE_SIZE),
        m_shaderToPSOFile(RESERVE_SIZE),
        m_psoPool(RESERVE_SIZE){};
  virtual ~VkPSOManager() = default;

  VkPSOManager(const VkPSOManager &) = delete;
  VkPSOManager &operator=(const VkPSOManager &) = delete;
  VkPSOManager(VkPSOManager &&) = delete;
  VkPSOManager &operator=(VkPSOManager &&) = delete;
  void initialize() override;
  void cleanup() override;
  void loadRawPSOInFolder(const char *directory) override;
  void loadCachedPSOInFolder(const char *directory) override;
  // TODO temporary function to be removed once the load rawPSO In folder works
  PSOHandle loadRawPSO(const char *file);
  VkPSOCompileResult compileRawPSO(const char *file);

  void recompilePSOFromShader(const char *shaderName,
                              const char *getOffsetPath) override;
  inline void bindPSO(const PSOHandle handle,
                      VkCommandBuffer commandList) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    vkCmdBindPipeline(commandList, VK_PIPELINE_BIND_POINT_GRAPHICS, data.pso);
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
  VkPipelineLayout getPipelineLayoutFromPSOHandle(const PSOHandle handle) const {
    assertMagicNumber(handle);
    const uint32_t index = getIndexFromHandle(handle);
    const PSOData &data = m_psoPool.getConstRef(index);
    return data.layout;
  }

  PSOHandle getHandleFromName(const char *name) const override;

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
  PSOHandle insertInPSOCache(const VkPSOCompileResult &result);

  inline void assertMagicNumber(const PSOHandle handle) const {
    const uint32_t magic = getMagicFromHandle(handle);
    const uint32_t idx = getIndexFromHandle(handle);
    assert(static_cast<uint32_t>(m_psoPool.getConstRef(idx).magicNumber) ==
               magic &&
           "invalid magic handle for constant buffer");
  }

  VkPSOCompileResult processRasterPSO(
      const char *filePath, const nlohmann::json &jobj,
      VkPipelineVertexInputStateCreateInfo *vertexInfo) const;

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
