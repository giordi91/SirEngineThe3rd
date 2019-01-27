#pragma once
#include <cstdint>

namespace SirEngine {
namespace dx12
{
	uint32_t getTotalGpuMemoryMB();
	uint32_t getUsedGpuMemoryMB();

	void renderImGuiMemoryWidget();

	

}
} // namespace SirEngine
