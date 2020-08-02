#pragma once
#include "SirEngine/handle.h"
#include "graphicsDefines.h"

namespace SirEngine::graphics {

struct BindingDescription {
  uint32_t m_bindingIndex;
  GRAPHIC_RESOURCE_TYPE m_resourceType;
  GRAPHIC_RESOURCE_VISIBILITY m_visibility;
};

enum BINDING_TABLE_FLAGS_BITS {
  BINDING_TABLE_NONE = 0,
  BINDING_TABLE_BUFFERED = 1
};
typedef uint32_t BINDING_TABLE_FLAGS;

class BindingTableManager {
 public:
  virtual ~BindingTableManager() = default;
  BindingTableManager() = default;
  BindingTableManager(const BindingTableManager &) = delete;
  BindingTableManager &operator=(const BindingTableManager &) = delete;
  BindingTableManager(BindingTableManager &&o) noexcept =
      delete;  // move constructor
  BindingTableManager &operator=(BindingTableManager &&other) =
      delete;  // move assignment

  virtual void initialize() = 0;
  virtual void cleanup() = 0;
  virtual void free(const BindingTableHandle &bindingTable) = 0;
  virtual BindingTableHandle allocateBindingTable(
      const graphics::BindingDescription *descriptions, const uint32_t count,
      graphics::BINDING_TABLE_FLAGS flags, const char *name = nullptr) = 0;
  virtual void bindTexture(const BindingTableHandle bindHandle,
                           const TextureHandle texture,
                           const uint32_t descriptorIndex,
                           const uint32_t bindingIndex, const bool isCube) = 0;
  virtual void bindBuffer(const BindingTableHandle bindHandle,
                           const BufferHandle buffer,
                           const uint32_t descriptorIndex,
                           const uint32_t bindingIndex) = 0;
  virtual void bindConstantBuffer(const BindingTableHandle &bindingTable,
                          const ConstantBufferHandle &constantBufferHandle,
                          const uint32_t descriptorIndex,
                          const uint32_t bindingIndex) =0;

  virtual void bindTable(uint32_t bindSpace,
                         const BindingTableHandle bindHandle,
                         const RSHandle rsHandle) = 0;
};
}  // namespace SirEngine::graphics
