#pragma once
#include "SirEngine/memory/cpu/linearBufferManager.h"

namespace SirEngine::debug {

void renderMemoryPoolTracker(const char *headerName,
                             const size_t poolRangeInBytes,
                             const ResizableVector<BufferRangeTracker> *allocs);
}; // namespace SirEngine::debug
