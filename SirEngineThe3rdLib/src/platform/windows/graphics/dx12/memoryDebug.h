#pragma once
#include <cstdint>

namespace SirEngine {
namespace dx12
{
	uint32_t getTotalGpuMemory();
	uint32_t getUsedGpuMemory();

	void renderImGuiMemoryWidget();

	

}
} // namespace SirEngine
