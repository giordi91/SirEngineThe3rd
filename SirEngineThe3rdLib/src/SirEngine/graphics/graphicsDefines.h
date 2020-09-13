#pragma once
#include <cstdint>

#include "SirEngine/core.h"

namespace SirEngine {
// vendors
enum class ADAPTER_VENDOR { NVIDIA = 0, AMD, INTEL, WARP, ANY };
enum class ADAPTER_SELECTION_RULE { LARGEST_FRAME_BUFFER, FIRST_VALID };
inline const char *ADAPTER_VENDOR_NAMES[]{"NVIDIA", "AMD", "INTEL", "WARP",
                                          "ANY"};
inline const uint32_t VENDOR_ID[] = {
    0x10DE,  // NVIDIA
    0x1002,  // AMD
    0x8086,  // INTEL
    0x1414,  // MICROSOFT warp adapter, DX only
    0xFFFF   // NONE
};
enum class GRAPHIC_API { DX12 = 0, VULKAN = 1, UNKNOWN };
enum class PSO_TYPE { DXR = 0, RASTER, COMPUTE, INVALID };

struct SIR_ENGINE_API AdapterRequestConfig {
  ADAPTER_VENDOR m_vendor;
  ADAPTER_SELECTION_RULE m_genericRule;
  bool m_vendorTolerant;
};

enum class SHADER_TYPE { VERTEX = 0, FRAGMENT, COMPUTE, INVALID };
enum SHADER_FLAGS { SHADER_DEBUG = 1 };

// angles
static constexpr float SE_PI = 3.14159265358979323846f;
static constexpr double SE_PI_D = 3.141592653589793238462643383279502884;
static constexpr float TO_RAD = static_cast<float>(SE_PI_D / 180.0);
static constexpr double TO_RAD_D = SE_PI_D / 180.0;
static constexpr float TO_DEG = static_cast<float>(180.0 / SE_PI_D);
static constexpr double TO_DEG_D = 180.0 / SE_PI_D;

// rendering
enum class SHADER_QUEUE_FLAGS {
  FORWARD = 1 << 0,
  DEFERRED = 1 << 1,
  SHADOW = 1 << 2,
  QUEUE_DEBUG = 1 << 3,
  CUSTOM = 1 << 4,
};

enum MESH_ATTRIBUTE_FLAGS {
  MESH_ATTRIBUTE_NONE = 0,
  POSITIONS = 1,
  NORMALS = 2,
  UV = 4,
  TANGENTS = 8,
  ALL = 15
};

enum class TOPOLOGY_TYPE {
  UNDEFINED = 0,
  LINE,
  LINE_STRIP,
  TRIANGLE,
  TRIANGLE_STRIP
};

enum class GRAPHIC_RESOURCE_TYPE {
  CONSTANT_BUFFER,
  READ_BUFFER,
  READWRITE_BUFFER,
  TEXTURE,
};

enum GRAPHIC_RESOURCE_VISIBILITY_BITS {
  GRAPHICS_RESOURCE_VISIBILITY_VERTEX = 1,
  GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT = 2,
  GRAPHICS_RESOURCE_VISIBILITY_COMPUTE = 4,
};

typedef uint32_t GRAPHIC_RESOURCE_VISIBILITY;

// memory
static constexpr uint64_t MB_TO_BYTE = 1024 * 1024;
static constexpr double BYTE_TO_MB_D = 1.0 / MB_TO_BYTE;
static constexpr float BYTE_TO_MB = BYTE_TO_MB_D;

struct MemoryRange {
  uint32_t m_offset;
  uint32_t m_size;  // can allocate max 4 gigs beware
};

}  // namespace SirEngine