#pragma once
#include <cstdint>

namespace SirEngine {
enum class ADAPTER_VENDOR { NVIDIA = 0, AMD, INTEL, WARP, ANY };
enum class ADAPTER_SELECTION_RULE { LARGEST_FRAME_BUFFER, FIRST_VALID };
inline const char *ADAPTER_VENDOR_NAMES[]{"NVIDIA", "AMD", "INTEL", "WARP",
                                          "ANY"};
inline const uint32_t VENDOR_ID[] = {
    0x10DE, // NVIDIA
    0x1002, // AMD
    0x8086, // INTEL
    0x1414, // MICROSOFT warp adapter, DX only
    0xFFFF  // NONE
};
enum class GRAPHIC_API { DX12 = 0, VULKAN = 1, UNKNOWN };

struct AdapterRequestConfig {
  ADAPTER_VENDOR m_vendor;
  ADAPTER_SELECTION_RULE m_genericRule;
  bool m_vendorTolerant;
};
} // namespace SirEngine