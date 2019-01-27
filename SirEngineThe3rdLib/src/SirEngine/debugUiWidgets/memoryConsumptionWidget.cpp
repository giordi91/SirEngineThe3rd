#include "SirEngine/debugUiWidgets/memoryConsumptionWidget.h"

#if GRAPHIC_API == DX12
#include "platform/windows/graphics/dx12/memoryDebug.h"
#endif

namespace SirEngine {
namespace debug {
	MemoryConsumptionWidget::MemoryConsumptionWidget()
	{
	}

	void MemoryConsumptionWidget::render()
	{
#if GRAPHIC_API == DX12
		dx12::renderImGuiMemoryWidget();
#endif

	}
} // namespace debug
} // namespace SirEngine
