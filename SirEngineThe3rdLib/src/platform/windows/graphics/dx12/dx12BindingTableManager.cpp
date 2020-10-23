#include "platform/windows/graphics/dx12/dx12BindingTableManager.h"

#include <catch/catch.hpp>

#include "SirEngine/globals.h"
#include "SirEngine/memory/cpu/threeSizesPool.h"
#include "dx12BufferManager.h"
#include "dx12MeshManager.h"
#include "platform/windows/graphics/dx12/descriptorHeap.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"

namespace SirEngine::dx12 {
void Dx12BindingTableManager::initialize() {}

void Dx12BindingTableManager::cleanup() {}

inline bool isFlagSet(const uint32_t flags,
                      const graphics::BINDING_TABLE_FLAGS toCheck) {
  return (flags & toCheck) > 0;
}

BindingTableHandle Dx12BindingTableManager::allocateBindingTable(
    const graphics::BindingDescription *descriptions, const uint32_t count,
    const graphics::BINDING_TABLE_FLAGS flags, const char *) {
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
  dx12::GLOBAL_CBV_SRV_UAV_HEAP->reserveDescriptors(descriptors,
                                                    descriptorCount);

  uint32_t descriptrionSize = sizeof(graphics::BindingDescription) * count;
  auto *descriptrionMemory = static_cast<graphics::BindingDescription *>(
      m_allocator.allocate(descriptrionSize));
  memcpy(descriptrionMemory, descriptions, descriptrionSize);
  // store data in the pool
  uint32_t poolIdx;
  BindingTableData &data = m_bindingTablePool.getFreeMemoryData(poolIdx);
  data.descriptors = descriptors;
  data.descriptorCount = descriptorCount;
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

void Dx12BindingTableManager::bindTable(const uint32_t bindingSpace,
                                        const BindingTableHandle bindHandle,
                                        const RSHandle rsHandle,
                                        const bool isCompute) {
  assertMagicNumber(bindHandle);
  uint32_t poolIndex = getIndexFromHandle(bindHandle);
  const auto &data = m_bindingTablePool.getConstRef(poolIndex);

  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;
  if (!isCompute) {
    dx12::ROOT_SIGNATURE_MANAGER->bindGraphicsRS(rsHandle);
  } else {
    dx12::ROOT_SIGNATURE_MANAGER->bindComputeRS(rsHandle);
  }

  bool isBuffered = isFlagSet(
      data.flags, graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED);
  uint32_t multiplier = isBuffered ? globals::CURRENT_FRAME : 0;
  uint32_t index = 0 + (multiplier * data.descriptionsCount);
  DescriptorPair &pair = data.descriptors[index];

  int bindSlot =
      dx12::ROOT_SIGNATURE_MANAGER->getBindingSlot(rsHandle, bindingSpace);
  assert(bindSlot != -1);

  if (!isCompute) {
    commandList->SetGraphicsRootDescriptorTable(bindSlot, pair.gpuHandle);
  } else {
    commandList->SetComputeRootDescriptorTable(bindSlot, pair.gpuHandle);
  }
  // let us bind the samplers
  int samplersBindSlot =
      dx12::ROOT_SIGNATURE_MANAGER->getBindingSlot(rsHandle, 1);
  if (samplersBindSlot != -1) {
    auto samplersHandle = dx12::GLOBAL_SAMPLER_HEAP->getGpuStart();
    if (!isCompute) {
      commandList->SetGraphicsRootDescriptorTable(samplersBindSlot,
                                                  samplersHandle);
    } else {
      commandList->SetComputeRootDescriptorTable(samplersBindSlot,
                                                 samplersHandle);
    }
  }
}

void Dx12BindingTableManager::bindConstantBuffer(
    const BindingTableHandle &bindingTable,
    const ConstantBufferHandle &constantBufferHandle,
    const uint32_t descriptorIndex, const uint32_t) {
  assertMagicNumber(bindingTable);
  uint32_t poolIndex = getIndexFromHandle(bindingTable);
  const auto &data = m_bindingTablePool.getConstRef(poolIndex);

  bool isBuffered = isFlagSet(
      data.flags, graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED);
  uint32_t multiplier = isBuffered ? globals::CURRENT_FRAME : 0;
  uint32_t index = descriptorIndex + (multiplier * data.descriptionsCount);

  dx12::CONSTANT_BUFFER_MANAGER->createSrv(constantBufferHandle,
                                           data.descriptors[index]);
}

void Dx12BindingTableManager::bindBuffer(const BindingTableHandle bindHandle,
                                         const BufferHandle buffer,
                                         const uint32_t descriptorIndex,
                                         const uint32_t bindingIndex) {
  assertMagicNumber(bindHandle);
  uint32_t poolIndex = getIndexFromHandle(bindHandle);
  const auto &data = m_bindingTablePool.getConstRef(poolIndex);

  bool isBuffered = isFlagSet(
      data.flags, graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED);
  uint32_t multiplier = isBuffered ? globals::CURRENT_FRAME : 0;
  uint32_t index = descriptorIndex + (multiplier * data.descriptionsCount);

  bool isUAV = false;
  for (uint32_t i = 0; i < data.descriptionsCount; ++i) {
    bool isRW = data.descriptions[i].m_resourceType ==
                GRAPHIC_RESOURCE_TYPE::READWRITE_BUFFER;
    bool isCorrectIndex = data.descriptions[i].m_bindingIndex == bindingIndex;
    bool finalCondition = isRW & isCorrectIndex;
    isUAV |= finalCondition;
  }

  if (isUAV) {
    dx12::BUFFER_MANAGER->createUav(buffer, data.descriptors[index], 0, true);
  } else {
    dx12::BUFFER_MANAGER->createSrv(buffer, data.descriptors[index], 0, true);
  }
}

void Dx12BindingTableManager::bindMesh(const BindingTableHandle bindHandle,
                                       const MeshHandle mesh,
                                       const uint32_t startIndex,
                                       const MESH_ATTRIBUTE_FLAGS meshFlags) {
  assertMagicNumber(bindHandle);
  uint32_t poolIndex = getIndexFromHandle(bindHandle);
  const auto &data = m_bindingTablePool.getConstRef(poolIndex);

  bool isBuffered = isFlagSet(
      data.flags, graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED);
  assert(isBuffered == 0);
  uint32_t multiplier = isBuffered ? globals::CURRENT_FRAME : 0;
  uint32_t index = (multiplier * data.descriptionsCount);

  dx12::MESH_MANAGER->bindFlatMesh(mesh, &data.descriptors[index], meshFlags,
                                   startIndex);
}

void Dx12BindingTableManager::free(const BindingTableHandle &bindingTable) {
  // TODO freeing descriptor has never been done properly and is currently not
  // supported at runtime. This method mostly exists for clean up in the vulkan
  // back end and make the validation layer happy
  assertMagicNumber(bindingTable);
  uint32_t poolIndex = getIndexFromHandle(bindingTable);
  const BindingTableData &data = m_bindingTablePool.getConstRef(poolIndex);
  for (uint32_t i = 0; i < data.descriptorCount; ++i) {
    dx12::GLOBAL_CBV_SRV_UAV_HEAP->freeDescriptor(data.descriptors[i]);
  }
  // TODO free descriptions
}
}  // namespace SirEngine::dx12
