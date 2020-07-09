#pragma once
#if BUILD_DX12
// NOTE DX12.h needs to appear before pix3.h!!!
#include "platform/windows/graphics/dx12/DX12.h"
// keep this gap

#include "WinPixEventRuntime/WinPixEventRuntime/pix3.h"
#endif

#if BUILD_VK
#include "platform/windows/graphics/vk/vk.h"
#endif

namespace SirEngine {

inline void annotateGraphicsBegin(const char *name) {
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
#if BUILD_DX12
    PIXBeginEvent(dx12::CURRENT_FRAME_RESOURCE->fc.commandList, 0, name);
#endif
  }
  if ((globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::VULKAN) &
      vk::DEBUG_MARKERS_ENABLED) {
#if BUILD_VK
    VkDebugMarkerMarkerInfoEXT markerInfo = {};
    markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
    // Color to display this region with (if supported by debugger)
    float color[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
    // Name of the region displayed by the debugging application
    markerInfo.pMarkerName = name;
    vkCmdDebugMarkerBeginEXT(vk::CURRENT_FRAME_COMMAND->m_commandBuffer,
                             &markerInfo);
#endif
  }
}
inline void annotateGraphicsEnd() {
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
#if BUILD_DX12
    PIXEndEvent(dx12::CURRENT_FRAME_RESOURCE->fc.commandList);
#endif
  }
  if ((globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::VULKAN) &
      vk::DEBUG_MARKERS_ENABLED) {
#if BUILD_VK
    vkCmdDebugMarkerEndEXT(vk::CURRENT_FRAME_COMMAND->m_commandBuffer);
#endif
  }
}

}  // namespace SirEngine
