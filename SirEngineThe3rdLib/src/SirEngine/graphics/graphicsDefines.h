#pragma once

namespace SirEngine {
enum class ADAPTER_VENDOR { NVIDIA = 0, AMD, INTEL, WARP, ANY };
enum class ADAPTER_SELECTION_RULE { LARGEST_FRAME_BUFFER, FIRST_VALID };
inline const char *ADAPTER_VENDOR_NAMES[]{"NVIDIA", "AMD", "INTEL", "WARP",
                                          "ANY"};
enum class GRAPHIC_API { DX12 = 0, VULKAN = 1, UNKNOWN };
} // namespace SirEngine