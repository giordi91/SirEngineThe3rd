#include "SirEngine/debugUiWidgets/memoryConsumptionWidget.h"
#include "SirEngine/engineConfig.h"
#include "SirEngine/globals.h"

#include "platform/windows/graphics/dx12/memoryDebug.h"
#include "platform/windows/graphics/vk/vkMemoryDebug.h"

namespace SirEngine::debug {
MemoryConsumptionWidget::MemoryConsumptionWidget() {}

void MemoryConsumptionWidget::render() {
  if (globals::ENGINE_CONFIG->m_graphicsAPI == GRAPHIC_API::DX12) {
    dx12::renderImGuiMemoryWidget();
  } else {
    vk::renderImGuiMemoryWidget();
  }
}
} // namespace SirEngine::debug
