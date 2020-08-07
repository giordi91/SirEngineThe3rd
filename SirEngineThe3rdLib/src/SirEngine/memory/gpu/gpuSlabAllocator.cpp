
#include "SirEngine/memory/gpu/gpuSlabAllocator.h"

#include "SirEngine/bufferManager.h"
#include "SirEngine/log.h"
#include "platform/windows/graphics/vk/vk.h"

namespace SirEngine {
void GPUSlabAllocator::initialize(
    const GPUSlabAllocatorInitializeConfig config) {
  m_config = config;
  uint32_t allocCount = m_config.initialSlabs == 0 ? 1 : m_config.initialSlabs;
  for (uint32_t i = 0; i < allocCount; ++i) {
    allocateSlab();
  }
}

GPUSlabAllocationHandle GPUSlabAllocator::allocate(const uint32_t sizeInBytes,
                                                   void* initialData) {
  // search the slabs, slabs per frame are exactly identical, searching one will
  // yield the same result of searching all of them
  const int freeSlab = getFreeSlabIndex(sizeInBytes);

  BufferRangeHandle handle{};
  BufferRange range{};
  handle = m_slabs[freeSlab]->m_slabTracker.allocate(
      sizeInBytes, m_config.allocationsRequestAligment);
  range = m_slabs[freeSlab]->m_slabTracker.getBufferRange(handle);
  // copying the data if we have a valid pointer
  if (initialData != nullptr) {
    // offsetting the pointer by the amount the tracker tells us
    assert(sizeInBytes <= range.m_size);
    char* ptr = static_cast<char*>(
        globals::BUFFER_MANAGER->getMappedData(m_slabs[freeSlab]->handle));

    // TODO probably we want to do some checks on whether or not is doable
    memcpy(ptr + range.m_offset, initialData, sizeInBytes);
  }

  // if we are here we allocated successfully!
  // the handle is composed of the slab index and buffer range handle
  // finding a free block in the pool
  uint32_t index;
  SlabAllocData& buffData = m_allocInfoStorage.getFreeMemoryData(index);
  buffData.m_version = VERSION_COUNTER++;
  buffData.m_rangeHandle = handle;
  buffData.m_range = range;
  buffData.m_slabIdx = freeSlab;

  const GPUSlabAllocationHandle outHandle{(buffData.m_version << 16) | index};
  return outHandle;
}

void GPUSlabAllocator::clear() {
  // assert(0);
  for (int i = 0; i < m_slabs.size(); ++i) {
    m_slabs[i]->m_slabTracker.clear();
  }
  m_allocInfoStorage.clear();
}

void* GPUSlabAllocator::getMappedPtr(GPUSlabAllocationHandle allocHandle) {
  return nullptr;
}

void GPUSlabAllocator::cleanup() {
  int count = m_slabs.size();
  for (int i = 0; i < count; ++i) {
    globals::BUFFER_MANAGER->free(m_slabs[i]->handle);
  }
}

uint32_t GPUSlabAllocator::allocateSlab() {
  Slab* slab = new Slab(m_config.slabSizeInBytes);
  m_slabs.pushBack(slab);

  char printbuff[64];
  sprintf(printbuff, "gpuSlab%i", m_slabs.size() - 1);
  BufferHandle handle;
  int numberOfElments = m_config.slabSizeInBytes / (sizeof(float) * 4);
  int elementSize = sizeof(float) * 4;

  slab->handle = globals::BUFFER_MANAGER->allocate(
      m_config.slabSizeInBytes, nullptr, printbuff, numberOfElments,
      elementSize, BufferManager::BUFFER_FLAGS_BITS::STORAGE_BUFFER);
  return m_slabs.size() - 1;
}

int GPUSlabAllocator::getFreeSlabIndex(const uint32_t allocSize) {
  int freeSlab = -1;
  for (uint32_t i = 0; i < m_slabs.size(); ++i) {
    bool canAllocate = m_slabs[i]->m_slabTracker.canAllocate(allocSize);
    if (!canAllocate) {
      continue;
    }
    freeSlab = i;
    break;
  }
  if (freeSlab != -1) {
    // we found a slab, good to go
    return freeSlab;
  }
  // no slab found but can we allocate?
  return allocateSlab();
}

}  // namespace SirEngine
