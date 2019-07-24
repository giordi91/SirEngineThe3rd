#pragma once
#include <d3d12.h>
#include "SirEngine/graphics/cpuGraphicsStructures.h"
#include "SirEngine/graphics/postProcess/postProcessStack.h"
#include "SirEngine/handle.h"

namespace SirEngine {
class SSSSSEffect final : public PostProcessEffect {
 public:
  explicit SSSSSEffect(const char *name);
  virtual ~SSSSSEffect() = default;
  void initialize() override;
  void render(const TextureHandle input, const TextureHandle output,
              const PostProcessResources &resources) override;
  void clear() override;
  inline SSSSSConfig &getConfig() { return m_config; };

  inline void setConfigDirty() {
    m_updateConfig = true;
    copyConfigs();
  }

 private:
  void copyConfigs() {
    m_configW.correction = m_config.correction;
    m_configH.correction = m_config.correction;

    m_configW.maxdd = m_config.maxdd;
    m_configH.maxdd = m_config.maxdd;

    m_configW.sssLevel = m_config.sssLevel;
    m_configH.sssLevel = m_config.sssLevel;

    m_configW.width = m_config.width;
    m_configH.width = m_config.width;
  }

  ID3D12RootSignature *rs = nullptr;
  PSOHandle pso = {};
  ID3D12RootSignature *copyRs = nullptr;
  PSOHandle copyPso = {};

  // shader configuration handles
  SSSSSConfig m_config{};
  SSSSSConfig m_configW{};
  SSSSSConfig m_configH{};
  bool m_updateConfig;
  ConstantBufferHandle m_constantBufferHandleW;
  ConstantBufferHandle m_constantBufferHandleH;
};

}  // namespace SirEngine
