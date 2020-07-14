#include "platform/windows/graphics/dx12/dx12BindingTableManager.h"

#include "SirEngine/globals.h"
#include "SirEngine/memory/threeSizesPool.h"
#include "dx12TextureManager.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/dx12PSOManager.h"

namespace SirEngine::dx12 {
void Dx12BindingTableManager::initialize() {}

void Dx12BindingTableManager::cleanup() {}

inline bool isFlagSet(const uint32_t flags,
                      const graphics::BINDING_TABLE_FLAGS toCheck) {
  return (flags & toCheck) > 0;
}

BindingTableHandle Dx12BindingTableManager::allocateBindingTable(
    const graphics::BindingDescription *descriptions, const uint32_t count,
    graphics::BINDING_TABLE_FLAGS flags, const char *name) {
  // if the table is buffered we need to allocate N times where N is the number
  // of buffers in flight
  uint32_t descriptorCount =
      count *
      (isFlagSet(flags,
                 graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED)
           ? FRAME_BUFFERS_COUNT
           : 1);
  // lets allocate enough descriptors to use for the descriptor table
  auto *descriptors = static_cast<DescriptorPair *>(
      m_allocator.allocate(sizeof(DescriptorPair) * descriptorCount));
  // now we have enough descriptors that we can use to bind everything
  uint32_t baseDescriptorIdx =
      dx12::GLOBAL_CBV_SRV_UAV_HEAP->reserveDescriptors(descriptors,
                                                        descriptorCount);

  uint32_t descriptrionSize = sizeof(graphics::BindingDescription) * count;
  auto *descriptrionMemory = reinterpret_cast<graphics::BindingDescription *>(
      m_allocator.allocate(descriptrionSize));
  memcpy(descriptrionMemory, descriptions, descriptrionSize);
  // store data in the pool
  uint32_t poolIdx;
  BindingTableData &data = m_bindingTablePool.getFreeMemoryData(poolIdx);
  data.descriptors = descriptors;
  data.magicNumber = MAGIC_NUMBER_COUNTER++;
  data.descriptions = descriptrionMemory;
  data.flags = flags;
  data.descriptionsCount = count;

  return {data.magicNumber << 16 | poolIdx};
}

void Dx12BindingTableManager::bindTexture(const BindingTableHandle bindHandle,
                                          const TextureHandle texture,
                                          const uint32_t descriptorIndex,
                                          const uint32_t bindingIndex,
                                          const bool isCube) {
  assertMagicNumber(bindHandle);
  uint32_t poolIndex = getIndexFromHandle(bindHandle);
  const auto &data = m_bindingTablePool.getConstRef(poolIndex);

  bool isBuffered = isFlagSet(
      data.flags, graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED);
  uint32_t multiplier = isBuffered ? globals::CURRENT_FRAME : 0;
  uint32_t index = descriptorIndex + (multiplier * data.descriptionsCount);
  DescriptorPair &pair = data.descriptors[index];
  dx12::TEXTURE_MANAGER->createSRV(texture, pair, isCube);
}

void Dx12BindingTableManager::bindTable(const BindingTableHandle bindHandle,
                                        const PSOHandle psoHandle) {
  assertMagicNumber(bindHandle);
  uint32_t poolIndex = getIndexFromHandle(bindHandle);
  const auto &data = m_bindingTablePool.getConstRef(poolIndex);

  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;
  dx12::PSO_MANAGER->bindPSO(psoHandle, commandList, true);

  // TODO move this to the bind pso
  TOPOLOGY_TYPE topology = dx12::PSO_MANAGER->getTopology(psoHandle);
  topology = dx12::PSO_MANAGER->getTopology(psoHandle);
  switch (topology) {
    case (TOPOLOGY_TYPE::TRIANGLE): {
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      break;
    }
    case TOPOLOGY_TYPE::UNDEFINED: {
      assert(0 && "trying to bind undefined topology");
      return;
    }
    case TOPOLOGY_TYPE::LINE: {
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
      break;
    }
    case TOPOLOGY_TYPE::LINE_STRIP: {
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
      break;
    }
    case TOPOLOGY_TYPE::TRIANGLE_STRIP: {
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      break;
    }
    default:;
  }

  bool isBuffered = isFlagSet(
      data.flags, graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED);
  uint32_t multiplier = isBuffered ? globals::CURRENT_FRAME : 0;
  uint32_t index = 0 + (multiplier * data.descriptionsCount);
  DescriptorPair &pair = data.descriptors[index];

  commandList->SetGraphicsRootDescriptorTable(1, pair.gpuHandle);
}
}  // namespace SirEngine::dx12
