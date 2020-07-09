#pragma once
#if BUILD_DX12
// NOTE DX12.h needs to appear before pix3.h!!!
#include "platform/windows/graphics/dx12/DX12.h"
// keep this gap

#include "WinPixEventRuntime/WinPixEventRuntime/pix3.h"
#endif

namespace SirEngine {

inline void annotateGraphicsBegin(const char *name) {
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
#if BUILD_DX12
    PIXBeginEvent(dx12::CURRENT_FRAME_RESOURCE->fc.commandList, 0, name);
#endif
  }
}
inline void annotateGraphicsEnd() {
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
#if BUILD_DX12
    PIXEndEvent(dx12::CURRENT_FRAME_RESOURCE->fc.commandList);
#endif
  }
}

}  // namespace SirEngine
