#include "platform/windows/graphics/dx12/dx12BindingTableManager.h"

#include "SirEngine/globals.h"
#include "SirEngine/memory/cpu/threeSizesPool.h"
#include "platform/windows/graphics/dx12/dx12ConstantBufferManager.h"
#include "platform/windows/graphics/dx12/dx12RootSignatureManager.h"
#include "platform/windows/graphics/dx12/dx12TextureManager.h"
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

void Dx12BindingTableManager::bindTable(uint32_t bindingSpace,
                                        const BindingTableHandle bindHandle,
                                        const RSHandle rsHandle) {
  assertMagicNumber(bindHandle);
  uint32_t poolIndex = getIndexFromHandle(bindHandle);
  const auto &data = m_bindingTablePool.getConstRef(poolIndex);

  auto *commandList = dx12::CURRENT_FRAME_RESOURCE->fc.commandList;
  //dx12::ROOT_SIGNATURE_MANAGER->bindGraphicsRS(rsHandle, commandList);

  bool isBuffered = isFlagSet(
      data.flags, graphics::BINDING_TABLE_FLAGS_BITS::BINDING_TABLE_BUFFERED);
  uint32_t multiplier = isBuffered ? globals::CURRENT_FRAME : 0;
  uint32_t index = 0 + (multiplier * data.descriptionsCount);
  DescriptorPair &pair = data.descriptors[index];

  uint32_t bindSlot =
      dx12::ROOT_SIGNATURE_MANAGER->getBindingSlot(rsHandle, bindingSpace);
  assert(bindSlot != -1);
  commandList->SetGraphicsRootDescriptorTable(bindSlot, pair.gpuHandle);
}

void Dx12BindingTableManager::bindConstantBuffer(
    const BindingTableHandle &bindingTable,
    const ConstantBufferHandle &constantBufferHandle,
    const uint32_t descriptorIndex, const uint32_t ) {
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

void Dx12BindingTableManager::bindBuffer(const BindingTableHandle bindHandle, const BufferHandle buffer,
	const uint32_t descriptorIndex, const uint32_t bindingIndex)
{
    assert(0);
}

void Dx12BindingTableManager::free(const BindingTableHandle &bindingTable) {
  // TODO freeing descriptor has never been done properly and is currently not
  // supported at runtime. This method mostly exists for clean up in the vulkan
  // back end and make the validation layer happy
}
}  // namespace SirEngine::dx12
