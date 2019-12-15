#pragma once
#include "SirEngine/core.h"
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

struct SIR_ENGINE_API AdapterRequestConfig {
  ADAPTER_VENDOR m_vendor;
  ADAPTER_SELECTION_RULE m_genericRule;
  bool m_vendorTolerant;
};

enum class SHADER_TYPE { VERTEX = 0, FRAGMENT, COMPUTE, INVALID };
enum SHADER_FLAGS { DEBUG = 1 };

// angles
static constexpr float SE_PI = 3.14159265358979323846f;
static constexpr double SE_PI_D = 3.141592653589793238462643383279502884;
static constexpr float TO_RAD = static_cast<float>(SE_PI_D / 180.0);
static constexpr double TO_RAD_D = SE_PI_D / 180.0;
static constexpr float TO_DEG = static_cast<float>(180.0 / SE_PI_D);
static constexpr double TO_DEG_D = 180.0 / SE_PI_D;

} // namespace SirEngine