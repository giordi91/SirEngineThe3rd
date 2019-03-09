#pragma once
#if GRAPHICS_API == DX12
#include "WinPixEventRuntime/WinPixEventRuntime/pix3.h"
#include "platform/windows/graphics/dx12/DX12.h"
#endif

namespace SirEngine {

inline void annotateGraphicsBegin(const char *name) {
  PIXBeginEvent(dx12::CURRENT_FRAME_RESOURCE->fc.commandList, 0, name);
}
inline void annotateGraphicsEnd() {
  PIXEndEvent(dx12::CURRENT_FRAME_RESOURCE->fc.commandList);
}

} // namespace SirEngine
