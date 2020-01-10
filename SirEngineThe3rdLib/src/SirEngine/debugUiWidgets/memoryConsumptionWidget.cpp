#include "SirEngine/debugUiWidgets/memoryConsumptionWidget.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/globals.h"

#if BUILD_DX12
#include "platform/windows/graphics/dx12/memoryDebug.h"
#endif 
#if BUILD_VK
#include "platform/windows/graphics/vk/vkMemoryDebug.h"
#endif

namespace SirEngine::debug {
MemoryConsumptionWidget::MemoryConsumptionWidget() {}

void MemoryConsumptionWidget::render() {
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
#if BUILD_DX12
    dx12::renderImGuiMemoryWidget();
#endif
  } else {
#if BUILD_VK
    vk::renderImGuiMemoryWidget();
#endif
  }
}
} // namespace SirEngine::debug
